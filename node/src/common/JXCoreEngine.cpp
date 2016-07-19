
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
#include "JXCoreEngine.h"

#include "jxcore/jx.h"

using namespace OpenT2T;

const char* mainScriptFileName = "main.js";

/// JavaScript contents of the "main.js" script for JXCore. It doesn't do much; most execution should be
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

    "console.log('JXCore: Loaded main.js.');"
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
void JXLogCallback(JXValue* argv, int argc)
{
    if (argc != 2)
    {
        LogWarning("Invalid log callback.");
        return;
    }

    LogSeverity severity = static_cast<LogSeverity>(JX_GetInt32(argv));
    const char* message = JX_GetString(argv + 1);
    Log(severity, message);
}

/// Callback invoked with the result of evaluation of caller's JavaScript code.
void JXResultCallback(JXValue* argv, int argc)
{
    if (argc != 2)
    {
        LogWarning("Invalid result callback.");
        return;
    }

    const char* callIdHex = JX_GetString(argv);
    unsigned long long callId = std::strtoull(callIdHex, nullptr, 16);
    if (callId == 0)
    {
        LogWarning("Invalid result callback ID.");
        return;
    }

    const char* resultJson = JX_GetString(argv + 1);

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
void JXErrorCallback(JXValue* argv, int argc)
{
    if (argc != 2)
    {
        LogWarning("Invalid error callback.");
        return;
    }

    const char* callIdHex = JX_GetString(argv);
    unsigned long long callId = std::strtoull(callIdHex, nullptr, 16);
    if (callId == 0)
    {
        LogWarning("Invalid result callback ID.");
        return;
    }

    // Get the message property from the JavaScript Error object (2nd argument), if available.
    JXValue errorMessageValue;
    JX_New(&errorMessageValue);
    JX_GetNamedProperty(argv + 1, "message", &errorMessageValue);
    const char* errorMessage = JX_GetString(&errorMessageValue);

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

    JX_Free(&errorMessageValue);
    delete callbackPtr;
}

/// Callback invoked when JavaScript code calls a function that was registered as a call from script.
void JXCallCallback(JXValue* argv, int argc)
{
    if (argc != 2)
    {
        LogWarning("Invalid call callback.");
        return;
    }

    const char* callIdHex = JX_GetString(argv);
    unsigned long long callId = std::strtoull(callIdHex, nullptr, 16);
    if (callId == 0)
    {
        LogWarning("Invalid result callback ID.");
        return;
    }

    const char* argsJson = JX_GetString(argv + 1);

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
std::once_flag JXCoreEngine::_initOnce;
std::string JXCoreEngine::_workingDirectory;

JXCoreEngine::JXCoreEngine() :
    _started(false),
    _callScriptFunction(nullptr)
{
    _dispatcher.Initialize();
}

JXCoreEngine::~JXCoreEngine()
{
    _dispatcher.Shutdown();
}

void JXCoreEngine::DefineScriptFile(std::string scriptFileName, std::string scriptCode)
{
    LogTrace("JXCoreEngine::DefineScriptFile(\"%s\", \"...\")", scriptFileName.c_str());

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
            JX_DefineFile(scriptFileName.c_str(), scriptCode.c_str());
        }
    });
}

void JXCoreEngine::Start(std::string workingDirectory, std::function<void(std::exception_ptr ex)> callback)
{
    LogTrace("JXCoreEngine::Start(\"%s\")", workingDirectory.c_str());

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
        // This limitation of JXCore is not represented in the INodeEngine interface (e.g. as a static
        // initialization method taking the working directory), because other node engines might not
        // have the same limitation.
        LogErrorAndThrow("Cannot start multiple JXCore instances with different working directories.");
    }

    try
    {
        std::call_once(_initOnce, [=]()
        {
            JX_InitializeOnce(workingDirectory.c_str());
        });
    }
    catch (...)
    {
        LogError("Failed to initialize JXCore engine.");
        throw;
    }

    _dispatcher.Dispatch([this, callback]()
    {
        try
        {
            if (_started)
            {
                LogErrorAndThrow("JXCore engine is already started.");
            }

            JX_InitializeNewEngine();
            JX_DefineMainFile(mainScriptCode);

            JX_DefineExtension("jxlog", JXLogCallback);
            JX_DefineExtension("jxcall", JXCallCallback);
            JX_DefineExtension("jxresult", JXResultCallback);
            JX_DefineExtension("jxerror", JXErrorCallback);

            for (const std::pair<std::string, std::string>& scriptEntry : _initialScriptMap)
            {
                JX_DefineFile(scriptEntry.first.c_str(), scriptEntry.second.c_str());
            }

            JX_StartEngine();

            for (const std::pair<std::string, std::function<void(std::string)>> callFromScriptEntry : _initialCallFromScriptMap)
            {
                this->RegisterCallFromScriptInternal(callFromScriptEntry.first, callFromScriptEntry.second);
            }

            _callScriptFunction = new JXValue();
            JX_New(reinterpret_cast<JXValue*>(_callScriptFunction));
            JX_Evaluate(callScriptFunctionCode, nullptr, reinterpret_cast<JXValue*>(_callScriptFunction));

            _started = true;
        }
        catch (...)
        {
            LogError("Failed to start JXCore engine.");
            callback(std::current_exception());
            return;
        }

        LogVerbose("Started JXCore engine.");
        callback(nullptr);
    });
}

void JXCoreEngine::Stop(std::function<void(std::exception_ptr ex)> callback)
{
    LogTrace("JXCoreEngine::Stop()");

    _dispatcher.Dispatch([this, callback]()
    {
        try
        {
            if (!_started)
            {
                LogErrorAndThrow("JXCore engine is not started.");
            }

            JX_Free(reinterpret_cast<JXValue*>(_callScriptFunction));
            delete reinterpret_cast<JXValue*>(_callScriptFunction);
            _callScriptFunction = nullptr;

            JX_StopEngine();
            _started = false;
        }
        catch (...)
        {
            LogError("Failed to stop JXCore engine.");
            callback(std::current_exception());
            return;
        }

        LogVerbose("Stopped JXCore engine.");
        callback(nullptr);
    });
}

void JXCoreEngine::CallScript(
    std::string scriptCode,
    std::function<void(std::string resultJson, std::exception_ptr ex)> callback)
{
    LogTrace("JXCoreEngine::CallScript(\"%s\")", scriptCode.c_str());

    _dispatcher.Dispatch([this, scriptCode, callback]()
    {
        this->CallScriptInternal(scriptCode, callback);
    });
}

void JXCoreEngine::RegisterCallFromScript(
    std::string scriptFunctionName,
    std::function<void(std::string argsJson)> callback)
{
    LogTrace("JXCoreEngine::RegisterCallFromScript(\"%s\")", scriptFunctionName.c_str());

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

void JXCoreEngine::CallScriptInternal(
    std::string scriptCode,
    std::function<void(std::string resultJson, std::exception_ptr ex)> callback)
{
    try
    {
        if (!_started)
        {
            LogErrorAndThrow("JXCore engine is not started.");
        }

        std::function<void(std::string, std::exception_ptr)>* callbackPtr =
        new std::function<void(std::string, std::exception_ptr)>(callback);

        // The callback function pointer is passed through JavaScript as a hex-formatted number.
        unsigned long long callId = reinterpret_cast<unsigned long long>(callbackPtr);
        char callIdBuf[20];
        snprintf(callIdBuf, sizeof(callIdBuf), "%llx", callId);

        // Create JXValue arguments to the call-script function: callback pointer and script code string.
        JXValue args[2];
        JX_New(&args[0]);
        JX_New(&args[1]);
        JX_SetString(&args[0], callIdBuf);
        JX_SetString(&args[1], scriptCode.c_str(), static_cast<int>(scriptCode.size()));

        JXValue unusedResult;
        JX_New(&unusedResult);

        // Invoke the script function that will evaluate the provided script code then callback
        // via the result or error callback.
        bool evaluated = JX_CallFunction(reinterpret_cast<JXValue*>(_callScriptFunction), args, 2, &unusedResult);

        JX_Free(&unusedResult);
        JX_Free(&args[0]);
        JX_Free(&args[1]);

        if (evaluated)
        {
            LogVerbose("Successfully evaluated script code.");
            JX_LoopOnce();
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

void JXCoreEngine::RegisterCallFromScriptInternal(
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

        JXValue unusedResult;
        JX_New(&unusedResult);

        // Evaluate the script, which defines the named function as invoking the script call callback.
        bool evaluated = JX_Evaluate(scriptBuf.data(), nullptr, &unusedResult);

        JX_Free(&unusedResult);

        if (evaluated)
        {
            JX_LoopOnce();
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
