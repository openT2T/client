
#include <string>
#include <functional>

#include "INodeEngine.h"
#include "JXCoreEngine.h"

JXCoreEngine::JXCoreEngine()
{
}

const char* JXCoreEngine::GetMainScriptFileName()
{
    return "main.js";
}

void JXCoreEngine::DefineScriptFile(const char* scriptFileName, const char* scriptCode)
{
}

void JXCoreEngine::Start(const char* workingDirectory, std::function<void(std::exception_ptr ex)> callback)
{
    callback(nullptr);
}

void JXCoreEngine::Stop(std::function<void(std::exception_ptr ex)> callback)
{
    callback(nullptr);
}

void JXCoreEngine::CallScript(const char* scriptCode, std::function<void(std::exception_ptr ex)> callback)
{

}

void JXCoreEngine::RegisterCallFromScript(
    const char* scriptFunctionName,
    std::function<void(const char* argsJson)> callback)
{
}
