
#include <cstdlib>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

#include "Log.h"
#include "INodeEngine.h"
#include "AsyncQueue.h"
#include "WorkItemDispatcher.h"
#include "NodeCoreEngine.h"

#include "node_wrapper.h"

using namespace OpenT2T;

const char* mainScriptFileName = "main.js";

/// JavaScript contents of the "main.js" script for Node. It doesn't do much; most execution should be
/// driven by defining additional named script files and directly evaluating script code strings.
const char* mainScriptCode =
    // Override console methods to redirect to the logging callback.
    // Note the constants here must correspond to the LogSeverity enum values.
    "console.error = function (msg) { process.natives.jxlog(1, msg); };"
    "console.warn = function (msg) { process.natives.jxlog(2, msg); };"
    "console.info = function (msg) { process.natives.jxlog(3, msg); };"
    "console.log = function (msg) { process.natives.jxlog(4, msg); };"

    // Save the main module object and require function in globals so they are available to evaluated scripts.
    "global.module = module;"
    "global.require = require;"

    "console.log('Node: Loaded main.js.');"
    ;

/// JavaScript code for a function that evaluates the caller's script code and returns the result (or error)
/// via a callback.
const char* callScriptFunctionCode =
    "(function (callId, scriptCode) {"
    "var resultJson;"
        "try {"
            "var result = eval(scriptCode);"
            "resultJson = JSON.stringify(result);"
        "} catch (e) {"
            "process.natives.jxerror(callId, e);"
            "return;"
        "}"
        "process.natives.jxresult(callId, resultJson);"
    "})";

/// Callback invoked by JavaScript calls to console.log (overridden by main.js).
void JXLogCallback(JS_Value* argv, int argc)
{
    if (argc != 2)
    {
        LogWarning("Invalid log callback.");
        return;
    }

    LogSeverity severity = static_cast<LogSeverity>(JS_GetInt32(argv));
    const char* message = JS_GetString(argv + 1);
    Log(severity, message);
}

/// Callback invoked with the result of evaluation of caller's JavaScript code.
void JXResultCallback(JS_Value* argv, int argc)
{
    if (argc != 2)
    {
        LogWarning("Invalid result callback.");
        return;
    }

    const char* callIdHex = JS_GetString(argv);
    unsigned long long callId = std::strtoull(callIdHex, nullptr, 16);
    if (callId == 0)
    {
        LogWarning("Invalid result callback ID.");
        return;
    }

    const char* resultJson = JS_GetString(argv + 1);

    LogTrace("JXResultCallback(\"%s\", \"%s\")", callIdHex, resultJson);

    std::function<void(std::string, std::exception_ptr)>* callbackPtr =
        reinterpret_cast<std::function<void(std::string, std::exception_ptr)>*>(callId);
    try
    {
        // Since this was a successful evaluation, the first parameter passed to the callback is the
        // JSON result of the evaluation, and the second parameter (exception) is null.
        (*callbackPtr)(resultJson ? std::string(resultJson) : std::string(), nullptr);
    }
    catch (...)
    {
        LogWarning("Script result callback function threw an exception.");
    }

    delete callbackPtr;
}

/// Callback invoked when evaluation of caller's JavaScript code threw an error.
void JXErrorCallback(JS_Value* argv, int argc)
{
    if (argc != 2)
    {
        LogWarning("Invalid error callback.");
        return;
    }

    const char* callIdHex = JS_GetString(argv);
    unsigned long long callId = std::strtoull(callIdHex, nullptr, 16);
    if (callId == 0)
    {
        LogWarning("Invalid result callback ID.");
        return;
    }

    // Get the message property from the JavaScript Error object (2nd argument), if available.
    JS_Value errorMessageValue;
    JS_New(&errorMessageValue);
    JS_GetNamedProperty(argv + 1, "message", &errorMessageValue);
    const char* errorMessage = JS_GetString(&errorMessageValue);

    LogTrace("JXErrorCallback(\"%s\", \"%s\")", callIdHex, (errorMessage != nullptr ? errorMessage : ""));

    std::function<void(std::string, std::exception_ptr)>* callbackPtr =
        reinterpret_cast<std::function<void(std::string, std::exception_ptr)>*>(callId);

    // Convert the JavaScript Error to a std::runtime_error with the same message.
    std::exception_ptr ex = std::make_exception_ptr(
        errorMessage ? std::runtime_error(errorMessage) : std::runtime_error("Unknown script error."));
    try
    {
        // Since this was a failed evaluation, the first parameter passed to the callback (the result)
        // is empty, and the second is the exception pointer.
        (*callbackPtr)(std::string(), ex);
    }
    catch (...)
    {
        LogWarning("Script error callback function threw an exception.");
    }

    JS_Free(&errorMessageValue);
    delete callbackPtr;
}

/// Callback invoked when JavaScript code calls a function that was registered as a call from script.
void JXCallCallback(JS_Value* argv, int argc)
{
    if (argc != 2)
    {
        LogWarning("Invalid call callback.");
        return;
    }

    const char* callIdHex = JS_GetString(argv);
    unsigned long long callId = std::strtoull(callIdHex, nullptr, 16);
    if (callId == 0)
    {
        LogWarning("Invalid result callback ID.");
        return;
    }

    const char* argsJson = JS_GetString(argv + 1);

    std::function<void(std::string)>* callbackPtr = reinterpret_cast<std::function<void(std::string)>*>(callId);
    try
    {
        (*callbackPtr)(argsJson ? std::string(argsJson) : std::string());
    }
    catch (...)
    {
        LogWarning("Script call callback function threw an exception.");
    }

    // Don't delete this callback function; it may be invoked multiple times.
}

inline void LogErrorAndThrow(const char* message)
{
    LogError(message);
    throw std::logic_error(message);
}

// Static member initialization
std::once_flag NodeCoreEngine::_initOnce;
std::string NodeCoreEngine::_workingDirectory;

NodeCoreEngine::NodeCoreEngine() :
    _started(false),
    _callScriptFunction(nullptr)
{
    _dispatcher.Initialize();
}

NodeCoreEngine::~NodeCoreEngine()
{
    _dispatcher.Shutdown();
}

void NodeCoreEngine::DefineScriptFile(std::string scriptFileName, std::string scriptCode)
{
    LogTrace("NodeCoreEngine::DefineScriptFile(\"%s\", \"...\")", scriptFileName.c_str());

    if (scriptFileName == mainScriptFileName)
    {
        throw new std::invalid_argument("Invalid script file name: 'main.js' is a reserved name.");
    }

    _dispatcher.Dispatch([this, scriptFileName, scriptCode]()
    {
        if (!_started)
        {
            _initialScriptMap.emplace(scriptFileName, scriptCode);
        }
        else
        {
            JS_DefineFile(scriptFileName.c_str(), scriptCode.c_str());
        }
    });
}

void NodeCoreEngine::Start(std::string workingDirectory, std::function<void(std::exception_ptr ex)> callback)
{
    LogTrace("NodeCoreEngine::Start(\"%s\")", workingDirectory.c_str());

    if (workingDirectory.length() == 0)
    {
        throw std::invalid_argument("A working directory is required.");
    }

    if (_workingDirectory.length() == 0)
    {
        _workingDirectory = workingDirectory;
    }
    else if (workingDirectory != _workingDirectory)
    {
        // This limitation of Node is not represented in the INodeEngine interface (e.g. as a static
        // initialization method taking the working directory), because other node engines might not
        // have the same limitation.
        LogErrorAndThrow("Cannot start multiple Node instances with different working directories.");
    }

    try
    {
        std::call_once(_initOnce, [=]()
        {
            JS_InitializeOnce(workingDirectory.c_str());
        });
    }
    catch (...)
    {
        LogError("Failed to initialize Node engine.");
        throw;
    }

    _dispatcher.Dispatch([this, callback]()
    {
        try
        {
            if (_started)
            {
                LogErrorAndThrow("Node engine is already started.");
            }

            JS_DefineMainFile(mainScriptCode);

            JS_SetProcessNative("jxlog", JXLogCallback);
            JS_SetProcessNative("jxcall", JXCallCallback);
            JS_SetProcessNative("jxresult", JXResultCallback);
            JS_SetProcessNative("jxerror", JXErrorCallback);

            for (const std::pair<std::string, std::string>& scriptEntry : _initialScriptMap)
            {
                JS_DefineFile(scriptEntry.first.c_str(), scriptEntry.second.c_str());
            }

            JS_StartEngine("/");

            for (const std::pair<std::string, std::function<void(std::string)>> callFromScriptEntry : _initialCallFromScriptMap)
            {
                this->RegisterCallFromScriptInternal(callFromScriptEntry.first, callFromScriptEntry.second);
            }

            _callScriptFunction = new JS_Value();
            JS_New(reinterpret_cast<JS_Value*>(_callScriptFunction));
            JS_Evaluate(callScriptFunctionCode, nullptr, reinterpret_cast<JS_Value*>(_callScriptFunction));

            _started = true;
        }
        catch (...)
        {
            LogError("Failed to start Node engine.");
            callback(std::current_exception());
            return;
        }

        LogVerbose("Started Node engine.");
        callback(nullptr);
    });
}

void NodeCoreEngine::Stop(std::function<void(std::exception_ptr ex)> callback)
{
    LogTrace("NodeCoreEngine::Stop()");

    _dispatcher.Dispatch([this, callback]()
    {
        try
        {
            if (!_started)
            {
                LogErrorAndThrow("Node engine is not started.");
            }

            JS_Free(reinterpret_cast<JS_Value*>(_callScriptFunction));
            delete reinterpret_cast<JS_Value*>(_callScriptFunction);
            _callScriptFunction = nullptr;

            JS_StopEngine();
            _started = false;
        }
        catch (...)
        {
            LogError("Failed to stop Node engine.");
            callback(std::current_exception());
            return;
        }

        LogVerbose("Stopped Node engine.");
        callback(nullptr);
    });
}

void NodeCoreEngine::CallScript(
    std::string scriptCode,
    std::function<void(std::string resultJson, std::exception_ptr ex)> callback)
{
    LogTrace("NodeCoreEngine::CallScript(\"%s\")", scriptCode.c_str());

    _dispatcher.Dispatch([this, scriptCode, callback]()
    {
        this->CallScriptInternal(scriptCode, callback);
    });
}

void NodeCoreEngine::RegisterCallFromScript(
    std::string scriptFunctionName,
    std::function<void(std::string argsJson)> callback)
{
    LogTrace("NodeCoreEngine::RegisterCallFromScript(\"%s\")", scriptFunctionName.c_str());

    _dispatcher.Dispatch([this, scriptFunctionName, callback]()
    {
        if (!_started)
        {
            _initialCallFromScriptMap.emplace(scriptFunctionName, callback);
        }
        else
        {
            this->RegisterCallFromScriptInternal(scriptFunctionName, callback);
        }
    });
}

void NodeCoreEngine::CallScriptInternal(
    std::string scriptCode,
    std::function<void(std::string resultJson, std::exception_ptr ex)> callback)
{
    try
    {
        if (!_started)
        {
            LogErrorAndThrow("Node engine is not started.");
        }

        std::function<void(std::string, std::exception_ptr)>* callbackPtr =
        new std::function<void(std::string, std::exception_ptr)>(callback);

        // The callback function pointer is passed through JavaScript as a hex-formatted number.
        unsigned long long callId = reinterpret_cast<unsigned long long>(callbackPtr);
        char callIdBuf[20];
        snprintf(callIdBuf, sizeof(callIdBuf), "%llx", callId);

        // Create JS_Value arguments to the call-script function: callback pointer and script code string.
        JS_Value args[2];
        JS_New(&args[0]);
        JS_New(&args[1]);
        JS_SetString(&args[0], callIdBuf);
        JS_SetString(&args[1], scriptCode.c_str(), static_cast<int>(scriptCode.size()));

        JS_Value unusedResult;
        JS_New(&unusedResult);

        // Invoke the script function that will evaluate the provided script code then callback
        // via the result or error callback.
        bool evaluated = JS_CallFunction(reinterpret_cast<JS_Value*>(_callScriptFunction), args, 2, &unusedResult);

        JS_Free(&unusedResult);
        JS_Free(&args[0]);
        JS_Free(&args[1]);

        if (evaluated)
        {
            LogVerbose("Successfully evaluated script code.");
            JS_LoopOnce();
        }
        else
        {
            LogErrorAndThrow("Failed to evaluate script code.");
        }
    }
    catch (...)
    {
        callback(nullptr, std::current_exception());
    }
}

void NodeCoreEngine::RegisterCallFromScriptInternal(
    std::string scriptFunctionName,
    std::function<void(std::string argsJson)> callback)
{
    try
    {
        std::function<void(std::string)>* callbackPtr = new std::function<void(std::string)>(callback);

        // The callback function pointer is passed through JavaScript as a hex-formatted number.
        unsigned long long callId = reinterpret_cast<unsigned long long>(callbackPtr);

        // Note the Array.prototype.slice is necessary for proper array JSON-serialization
        // because arguments is only an array-like object, not actually an array.
        const char scriptFunctionFormat[] =
        "function %s() {"
            "process.natives.jxcall('%llx', JSON.stringify(Array.prototype.slice.call(arguments)));"
        "}";

        size_t scriptBufSize = scriptFunctionName.size() + sizeof(scriptFunctionFormat) + 20;
        std::vector<char> scriptBuf(scriptBufSize);
        snprintf(scriptBuf.data(), scriptBufSize, scriptFunctionFormat, scriptFunctionName.c_str(), callId);

        JS_Value unusedResult;
        JS_New(&unusedResult);

        // Evaluate the script, which defines the named function as invoking the script call callback.
        bool evaluated = JS_Evaluate(scriptBuf.data(), nullptr, &unusedResult);

        JS_Free(&unusedResult);

        if (evaluated)
        {
            JS_LoopOnce();
        }
        else
        {
            LogErrorAndThrow("Failed to evaluate script callback code.");
        }
    }
    catch (...)
    {
        LogError("Failed to register call from script.");
    }
}
