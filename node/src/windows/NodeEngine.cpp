#include "pch.h"
#include "WinrtUtils.h"
#include "Log.h"
#include "INodeEngine.h"
#include "NodeEngine.h"
#include "AsyncQueue.h"
#include "WorkItemDispatcher.h"
#include "JXCoreEngine.h"

using namespace Platform;
using namespace Windows::Foundation;
using namespace OpenT2T;

static struct _init
{
    _init()
    {
#if DEBUG
        OpenT2T::logLevel = LogSeverity::Trace;
#else
        OpenT2T::logLevel = LogSeverity::Info;
#endif

        OpenT2T::logHandler = [](LogSeverity severity, const char* message)
        {
            ::OutputDebugStringA(message);
        };
    }
} _staticInit;


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

void NodeEngine::DefineScriptFile(Platform::String^ scriptFileName, Platform::String^ scriptCode)
{
    ExceptionsToPlatformExceptions<void>([=]()
    {
        this->node->DefineScriptFile(
            PlatformStringToString(scriptFileName),
            PlatformStringToString(scriptCode));
    });
}

IAsyncAction^ NodeEngine::StartAsync(String^ workingDirectory)
{
    return ExceptionsToPlatformExceptions<IAsyncAction^>([=]()
    {
        concurrency::task_completion_event<void> tce;
        concurrency::task<void> task(tce);

        this->node->Start(PlatformStringToString(workingDirectory), [tce](std::exception_ptr ex)
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

IAsyncOperation<String^>^ NodeEngine::CallScriptAsync(Platform::String^ scriptCode)
{
    return ExceptionsToPlatformExceptions<IAsyncOperation<String^>^>([=]()
    {
        concurrency::task_completion_event<String^> tce;
        concurrency::task<String^> task(tce);

        this->node->CallScript(PlatformStringToString(scriptCode),
            [tce](std::string resultJson, std::exception_ptr ex)
        {
            if (ex == nullptr)
            {
                String^ resultJsonString = StringToPlatformString(resultJson);
                tce.set(resultJsonString);
            }
            else
            {
                tce.set_exception(ex);
            }
        });

        return TaskToAsyncOperation(task);
    });
}

void NodeEngine::RegisterCallFromScript(String^ scriptFunctionName)
{
    return ExceptionsToPlatformExceptions<void>([=]()
    {
        this->node->RegisterCallFromScript(
            PlatformStringToString(scriptFunctionName), [this, scriptFunctionName](std::string argsJson)
        {
            NodeCallEvent^ callEvent = ref new NodeCallEvent(scriptFunctionName, StringToPlatformString(argsJson));
            try
            {
                this->CallFromScript(this, callEvent);
            }
            catch (Exception^ ex)
            {
                LogWarning("Caught exception from CallFromScript handler: %ws", ex->Message);
            }
        });
    });
}
