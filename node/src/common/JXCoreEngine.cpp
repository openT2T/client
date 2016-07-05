
#include <string>
#include <functional>

#include "Log.h"
#include "INodeEngine.h"
#include "JXCoreEngine.h"

using namespace OpenT2T;

JXCoreEngine::JXCoreEngine()
{
}

const char* JXCoreEngine::GetMainScriptFileName()
{
    LogTrace("JXCoreEngine::GetMainScriptFileName()");
    return "main.js";
}

void JXCoreEngine::DefineScriptFile(const char* scriptFileName, const char* scriptCode)
{
    LogTrace("JXCoreEngine::DefineScriptFile(\"%s\", \"...\")", scriptFileName);
}

void JXCoreEngine::Start(const char* workingDirectory, std::function<void(std::exception_ptr ex)> callback)
{
    LogTrace("JXCoreEngine::Start(\"%s\")", workingDirectory);
    callback(nullptr);
}

void JXCoreEngine::Stop(std::function<void(std::exception_ptr ex)> callback)
{
    LogTrace("JXCoreEngine::Stop()");
    callback(nullptr);
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
