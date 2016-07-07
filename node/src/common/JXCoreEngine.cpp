
#include <cstdlib>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

#include "jxcore/jx.h"

#include "Log.h"
#include "INodeEngine.h"
#include "AsyncQueue.h"
#include "WorkItemDispatcher.h"
#include "JXCoreEngine.h"

using namespace OpenT2T;

const char* mainScriptFileName = "main.js";

void JXCallCallback(JXValue *result, int argc)
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

    std::function<void(const char*)>* callbackPtr = reinterpret_cast<std::function<void(const char*)>*>(callId);
    try
    {
        (*callbackPtr)(argsJson);
    }
    catch (...)
    {
        LogWarning("Script call callback function threw an exception.");
    }

    // Don't delete this callback function; it may be invoked multiple times.
}

void JXResultCallback(JXValue *result, int argc)
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

    std::function<void(const char*, std::exception_ptr)>* callbackPtr =
        reinterpret_cast<std::function<void(const char*, std::exception_ptr)>*>(callId);
    try
    {
        (*callbackPtr)(resultJson, nullptr);
    }
    catch (...)
    {
        LogWarning("Script result callback function threw an exception.");
    }

    delete callbackPtr;
}

void JXErrorCallback(JXValue *result, int argc)
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

    JXValue errorMessageValue;
    JX_New(&errorMessageValue);
    JX_GetNamedProperty(result + 1, "message", &errorMessageValue);
    const char* errorMessage = JX_GetString(&errorMessageValue);

    std::function<void(const char*, std::exception_ptr)>* callbackPtr =
        reinterpret_cast<std::function<void(const char*, std::exception_ptr)>*>(callId);
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

    delete callbackPtr;
}


std::once_flag JXCoreEngine::_initOnce;

inline void LogErrorAndThrow(const char* message)
{
    LogError(message);
    throw std::logic_error(message);
}

JXCoreEngine::JXCoreEngine()
{
    _dispatcher.Initialize();
}

JXCoreEngine::~JXCoreEngine()
{
    _dispatcher.Shutdown();
}

const char* JXCoreEngine::GetMainScriptFileName()
{
    LogTrace("JXCoreEngine::GetMainScriptFileName()");

    return mainScriptFileName;
}

void JXCoreEngine::DefineScriptFile(const char* scriptFileName, const char* scriptCode)
{
    LogTrace("JXCoreEngine::DefineScriptFile(\"%s\", \"...\")", scriptFileName);

    std::string scriptFileNameString(scriptFileName);
    std::string scriptCodeString(scriptCode);

    _dispatcher.Dispatch([this, scriptFileNameString, scriptCodeString]()
    {
        if (!_started)
        {
            _initialScriptMap.emplace(scriptFileNameString, scriptCodeString);
        }
        else if (scriptFileNameString == mainScriptFileName)
        {
            LogWarning("Cannot redefine main script file after the engine is started.");
        }
        else
        {
            JX_DefineFile(scriptFileNameString.c_str(), scriptCodeString.c_str());
        }
    });
}

void JXCoreEngine::Start(const char* workingDirectory, std::function<void(std::exception_ptr ex)> callback)
{
    LogTrace("JXCoreEngine::Start(\"%s\")", workingDirectory);

    try
    {
        std::call_once(_initOnce, [=]()
        {
            JX_InitializeOnce(workingDirectory);
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

            bool mainScriptFileDefined = false;
            for (const std::pair<std::string, std::string>& scriptEntry : _initialScriptMap)
            {
                if (scriptEntry.first == mainScriptFileName)
                {
                    JX_DefineMainFile(scriptEntry.second.c_str());
                    mainScriptFileDefined = true;
                }
                else
                {
                    JX_DefineFile(scriptEntry.first.c_str(), scriptEntry.second.c_str());
                }
            }

            if (!mainScriptFileDefined)
            {
                LogErrorAndThrow("Main script file must be defined before starting.");
            }

            JX_DefineExtension("jxcall", JXCallCallback);
            JX_DefineExtension("jxresult", JXResultCallback);
            JX_DefineExtension("jxerror", JXErrorCallback);

            JX_StartEngine();
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
        try
        {
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
    const char* scriptCode,
    std::function<void(const char* resultJson, std::exception_ptr ex)> callback)
{
    LogTrace("JXCoreEngine::CallScript(\"%s\")", scriptCode);

    std::string scriptCodeString(scriptCode);

    _dispatcher.Dispatch([this, scriptCodeString, callback]()
    {
        try
        {
            if (!_started)
            {
                LogErrorAndThrow("JXCore engine is not started.");
            }

            std::function<void(const char*, std::exception_ptr)>* callbackPtr =
                new std::function<void(const char*, std::exception_ptr)>(callback);
            unsigned long long callId = reinterpret_cast<unsigned long long>(callbackPtr);

            const char scriptWrapperFormat[] =
                "(function () {"
                    "var __callId = '%llx';"
                    "try {"
                        "var result = (function () { %s })();"
                        "process.natives.jxresult(__callId, JSON.stringify(result));"
                    "} catch (e) {"
                        "process.natives.jxerror(__callId, e);"
                    "}"
                "})()";
            size_t scriptBufSize = scriptCodeString.size() + sizeof(scriptWrapperFormat) + 20;
            std::vector<char> scriptBuf(scriptBufSize);
            snprintf(scriptBuf.data(), scriptBufSize, scriptWrapperFormat, callId, scriptCodeString.c_str());

            JXValue result;
            if (!JX_Evaluate(scriptBuf.data(), nullptr, &result))
            {
                LogErrorAndThrow("Failed to evaluate script code.");
            }

            JX_Loop();
        }
        catch (...)
        {
            callback(nullptr, std::current_exception());
        }
    });
}

void JXCoreEngine::RegisterCallFromScript(
    const char* scriptFunctionName,
    std::function<void(const char* argsJson)> callback)
{
    LogTrace("JXCoreEngine::RegisterCallFromScript(\"%s\")", scriptFunctionName);

    std::string scriptFunctionNameString(scriptFunctionName);

    _dispatcher.Dispatch([this, scriptFunctionNameString, callback]()
    {
        try
        {
            // TODO: Save initial callbacks and register them at startup.
            if (!_started)
            {
                LogErrorAndThrow("JXCore engine is not started.");
            }

            std::function<void(const char*)>* callbackPtr = new std::function<void(const char*)>(callback);
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
