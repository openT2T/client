
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

// This is the "main.js" script for JXCore. It doesn't do much; most execution should be driven by
// defining additional named script files and directly evaluating script code strings.
const char* mainScriptCode =
    // Override console methods to redirect to the console callback.
    // Note the constants here must correspond to the LogSeverity enum values.
    "global.console = {"
        "error: function (msg) { process.natives.jxlog(1, msg); }"
        "warn: function (msg) { process.natives.jxlog(2, msg); }"
        "info: function (msg) { process.natives.jxlog(3, msg); }"
        "log: function (msg) { process.natives.jxlog(4, msg); }"
    "};";

// Evaluates script code and returns the result (or error) via a callback.
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

void JXLogCallback(JXValue* result, int argc)
{
    if (argc != 2)
    {
        LogWarning("Invalid log callback.");
        return;
    }

    LogSeverity severity = static_cast<LogSeverity>(JX_GetInt32(result));
    const char* message = JX_GetString(result + 1);
    Log(severity, message);
}

void JXCallCallback(JXValue* result, int argc)
{
    if (argc != 2)
    {
        LogWarning("Invalid call callback.");
        return;
    }

    const char* callIdHex = JX_GetString(result);
    unsigned long long callId = std::strtoull(callIdHex, nullptr, 16);
    if (callId == 0)
    {
        LogWarning("Invalid result callback ID.");
        return;
    }

    const char* argsJson = JX_GetString(result + 1);

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

void JXResultCallback(JXValue* result, int argc)
{
    if (argc != 2)
    {
        LogWarning("Invalid result callback.");
        return;
    }

    const char* callIdHex = JX_GetString(result);
    unsigned long long callId = std::strtoull(callIdHex, nullptr, 16);
    if (callId == 0)
    {
        LogWarning("Invalid result callback ID.");
        return;
    }

    const char* resultJson = JX_GetString(result + 1);

    std::function<void(std::string, std::exception_ptr)>* callbackPtr =
        reinterpret_cast<std::function<void(std::string, std::exception_ptr)>*>(callId);
    try
    {
        (*callbackPtr)(resultJson ? std::string(resultJson) : std::string(), nullptr);
    }
    catch (...)
    {
        LogWarning("Script result callback function threw an exception.");
    }

    delete callbackPtr;
}

void JXErrorCallback(JXValue* result, int argc)
{
    if (argc != 2)
    {
        LogWarning("Invalid error callback.");
        return;
    }

    const char* callIdHex = JX_GetString(result);
    uintptr_t callId = std::strtoul(callIdHex, nullptr, 16);
    if (callId == 0)
    {
        LogWarning("Invalid result callback ID.");
        return;
    }

    // Get the message property from the JavaScript error object (2nd argument), if available.
    JXValue errorMessageValue;
    JX_New(&errorMessageValue);
    JX_GetNamedProperty(result + 1, "message", &errorMessageValue);
    const char* errorMessage = JX_GetString(&errorMessageValue);

    std::function<void(std::string, std::exception_ptr) > * callbackPtr =
        reinterpret_cast<std::function<void(std::string, std::exception_ptr)>*>(callId);
    try
    {
        throw (errorMessage ? std::runtime_error(errorMessage) : std::runtime_error("Unknown script error."));
    }
    catch (...)
    {
        try
        {
            (*callbackPtr)(nullptr, std::current_exception());
        }
        catch (...)
        {
            LogWarning("Script error callback function threw an exception.");
        }
    }

    JX_Free(&errorMessageValue);
    delete callbackPtr;
}


std::once_flag JXCoreEngine::_initOnce;

inline void LogErrorAndThrow(const char* message)
{
    LogError(message);
    throw std::logic_error(message);
}

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
        callback(std::current_exception());
        return;
    }

    _dispatcher.Dispatch([=]()
    {
        try
        {
            if (_started)
            {
                LogErrorAndThrow("JXCore engine is already started.");
            }

            JX_InitializeNewEngine();
            JX_DefineMainFile(mainScriptCode);

            for (const std::pair<std::string, std::string>& scriptEntry : _initialScriptMap)
            {
                JX_DefineFile(scriptEntry.first.c_str(), scriptEntry.second.c_str());
            }

            JX_DefineExtension("jxlog", JXLogCallback);
            JX_DefineExtension("jxcall", JXCallCallback);
            JX_DefineExtension("jxresult", JXResultCallback);
            JX_DefineExtension("jxerror", JXErrorCallback);

            JX_StartEngine();

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

    _dispatcher.Dispatch([=]()
    {
        if (!_started)
        {
            LogErrorAndThrow("JXCore engine is not started.");
        }

        try
        {
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
        try
        {
            if (!_started)
            {
                LogErrorAndThrow("JXCore engine is not started.");
            }

            std::function<void(std::string, std::exception_ptr)>* callbackPtr =
                new std::function<void(std::string, std::exception_ptr)>(callback);
            unsigned long long callId = reinterpret_cast<unsigned long long>(callbackPtr);
            char callIdBuf[20];
            snprintf(callIdBuf, sizeof(callIdBuf), "%llx", callId);

            JXValue args[2];
            JX_New(&args[0]);
            JX_New(&args[1]);
            JX_SetString(&args[0], callIdBuf);
            JX_SetString(&args[1], scriptCode.c_str(), static_cast<int>(scriptCode.size()));

            JXValue unusedResult;
            if (JX_CallFunction(reinterpret_cast<JXValue*>(_callScriptFunction), args, 2, &unusedResult))
            {
                JX_Free(&unusedResult);
                LogVerbose("Successfully evaluated script code.");
            }
            else
            {
                LogErrorAndThrow("Failed to evaluate script code.");
            }

            JX_Free(&args[0]);
            JX_Free(&args[1]);
            JX_Loop();
        }
        catch (...)
        {
            callback(nullptr, std::current_exception());
        }
    });
}

void JXCoreEngine::RegisterCallFromScript(
    std::string scriptFunctionName,
    std::function<void(std::string argsJson)> callback)
{
    LogTrace("JXCoreEngine::RegisterCallFromScript(\"%s\")", scriptFunctionName.c_str());

    std::string scriptFunctionNameString(scriptFunctionName);

    _dispatcher.Dispatch([this, scriptFunctionNameString, callback]()
    {
        try
        {
            // TODO: Save callbacks registered before startup, and register them at startup.
            if (!_started)
            {
                LogErrorAndThrow("JXCore engine is not started.");
            }

            std::function<void(std::string)>* callbackPtr = new std::function<void(std::string)>(callback);
            unsigned long long callId = reinterpret_cast<unsigned long long>(callbackPtr);

            const char scriptWrapperFormat[] =
                "function %s() {"
                    "process.natives.jxcall('%llx', arguments);"
                "}";
            size_t scriptBufSize = scriptFunctionNameString.size() + sizeof(scriptWrapperFormat) + 20;
            std::vector<char> scriptBuf(scriptBufSize);
            snprintf(scriptBuf.data(), scriptBufSize, scriptWrapperFormat, scriptFunctionNameString.c_str(), callId);

            JXValue result;
            if (!JX_Evaluate(scriptBuf.data(), nullptr, &result))
            {
                LogErrorAndThrow("Failed to evaluate script callback code.");
            }

            JX_Loop();
        }
        catch (...)
        {
            LogError("Failed to register call from script.");
        }
    });
}
