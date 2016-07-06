
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

std::once_flag JXCoreEngine::_initOnce;

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
    return "main.js";
}

void JXCoreEngine::DefineScriptFile(const char* scriptFileName, const char* scriptCode)
{
    LogTrace("JXCoreEngine::DefineScriptFile(\"%s\", \"...\")", scriptFileName);

    _initialScriptMap.emplace(scriptFileName, scriptCode);
}

void JXCoreEngine::Start(const char* workingDirectory, std::function<void(std::exception_ptr ex)> callback)
{
    LogTrace("JXCoreEngine::Start(\"%s\")", workingDirectory);

    _dispatcher.Dispatch([=]()
    {
        std::call_once(_initOnce, [=]()
        {
            JX_InitializeOnce(workingDirectory);
        });

        JX_InitializeNewEngine();

        for (const std::pair<std::string, std::string>& scriptEntry : _initialScriptMap)
        {
            if (scriptEntry.first == this->GetMainScriptFileName())
            {
                JX_DefineMainFile(scriptEntry.second.c_str());
            }
            else
            {
                JX_DefineFile(scriptEntry.first.c_str(), scriptEntry.second.c_str());
            }
        }

        JX_StartEngine();

        LogVerbose("Started JXCore engine.");
        callback(nullptr);
    });
}

void JXCoreEngine::Stop(std::function<void(std::exception_ptr ex)> callback)
{
    LogTrace("JXCoreEngine::Stop()");

    _dispatcher.Dispatch([=]()
    {
        JX_StopEngine();

        LogVerbose("Stopped JXCore engine.");
        callback(nullptr);
    });
}

void JXCoreEngine::CallScript(
    const char* scriptCode,
    std::function<void(const char* resultJson, std::exception_ptr ex)> callback)
{
    LogTrace("JXCoreEngine::CallScript(\"%s\")", scriptCode);
    callback("null", nullptr);
}

void JXCoreEngine::RegisterCallFromScript(
    const char* scriptFunctionName,
    std::function<void(const char* argsJson)> callback)
{
    LogTrace("JXCoreEngine::RegisterCallFromScript(\"%s\")", scriptFunctionName);
}
