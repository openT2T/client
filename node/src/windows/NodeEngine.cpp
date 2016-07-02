#include "pch.h"
#include "WinrtUtils.h"
#include "INodeEngine.h"
#include "NodeEngine.h"
#include "JXCoreEngine.h"

using namespace Platform;
using namespace Windows::Foundation;
using namespace OpenT2T;


NodeCallEvent::NodeCallEvent(String^ functionName, String^ argsJson)
{
    this->functionName = functionName;
    this->argsJson = argsJson;
}

String^ NodeCallEvent::FunctionName::get()
{
    return this->functionName;
}

String^ NodeCallEvent::ArgsJson::get()
{
    return this->argsJson;
}


NodeEngine::NodeEngine()
{
    this->node = new JXCoreEngine();
    if (this->node == nullptr)
    {
        throw Platform::Exception::CreateException(E_OUTOFMEMORY);
    }
}

NodeEngine::~NodeEngine()
{
    delete this->node;
}

String^ NodeEngine::MainScriptFileName::get()
{
    return ExceptionsToPlatformExceptions<String^>([=]()
    {
        return StringToPlatformString(this->node->GetMainScriptFileName());
    });
}

void NodeEngine::DefineScriptFile(Platform::String^ scriptFileName, Platform::String^ scriptCode)
{
    ExceptionsToPlatformExceptions<void>([=]()
    {
        this->node->DefineScriptFile(
            PlatformStringToString(scriptFileName).c_str(),
            PlatformStringToString(scriptCode).c_str());
    });
}

IAsyncAction^ NodeEngine::StartAsync(String^ workingDirectory)
{
    return ExceptionsToPlatformExceptions<IAsyncAction^>([=]()
    {
        concurrency::task_completion_event<void> tce;
        concurrency::task<void> task(tce);

        this->node->Start(PlatformStringToString(workingDirectory).c_str(), [tce](std::exception_ptr ex)
        {
            ex ? tce.set_exception(ex) : tce.set();
        });

        return TaskToAsyncAction(task);
    });
}

IAsyncAction^ NodeEngine::StopAsync()
{
    return ExceptionsToPlatformExceptions<IAsyncAction^>([=]()
    {
        concurrency::task_completion_event<void> tce;
        concurrency::task<void> task(tce);

        this->node->Stop([tce](std::exception_ptr ex)
        {
            ex ? tce.set_exception(ex) : tce.set();
        });

        return TaskToAsyncAction(task);
    });
}

IAsyncAction^ NodeEngine::CallScriptAsync(Platform::String^ scriptCode)
{
    return ExceptionsToPlatformExceptions<IAsyncAction^>([=]()
    {
        concurrency::task_completion_event<void> tce;
        concurrency::task<void> task(tce);

        this->node->CallScript(PlatformStringToString(scriptCode).c_str(), [tce](std::exception_ptr ex)
        {
            ex ? tce.set_exception(ex) : tce.set();
        });

        return TaskToAsyncAction(task);
    });
}

void NodeEngine::RegisterCallFromScript(String^ scriptFunctionName)
{
    return ExceptionsToPlatformExceptions<void>([=]()
    {
        this->node->RegisterCallFromScript(
            PlatformStringToString(scriptFunctionName).c_str(), [this, scriptFunctionName](std::string argsJson)
        {
            NodeCallEvent^ callEvent = ref new NodeCallEvent(scriptFunctionName, StringToPlatformString(argsJson));
            try
            {
                this->CallFromScript(this, callEvent);
            }
            catch (Exception^ ex)
            {
                // TODO: log
            }
        });
    });
}
