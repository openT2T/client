
#include <cstdlib>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>

#include "Log.h"
#include "AsyncQueue.h"
#include "WorkItemDispatcher.h"
#include "INodeEngine.h"
#include "JXCoreEngine.h"

#import "OT2TNodeEngine.h"
#import "ObjCppUtils.h"

using namespace OpenT2T;

@implementation OT2TNodeCallEvent

- (OT2TNodeCallEvent*) init
{
    self = [super init];
    return self;
}

- (OT2TNodeCallEvent*) initWithFunctionName: (NSString*) function
                                   argsJson: (NSString*) args
{
    self = [super init];
    if (self)
    {
        self.functionName = function;
        self.argsJson = args;
    }
    return self;
}

@synthesize functionName;
@synthesize argsJson;

@end


@implementation OT2TNodeEngine
{
    OpenT2T::INodeEngine* _node;
    NSHashTable<OT2TNodeCallListener>* _callFromScriptListeners;
}

+ (void) initialize
{
#if DEBUG
    SetLogLevel(LogSeverity::Trace);
#else
    SetLogLevel(LogSeverity::Info);
#endif

    SetLogHandler([](LogSeverity severity, const char* message)
    {
        NSLog(@"%s", message);
    });
}

- (OT2TNodeEngine*) init
{
    self = [super init];
    if (self)
    {
        _node = new OpenT2T::JXCoreEngine();
        _callFromScriptListeners = [[NSHashTable<OT2TNodeCallListener> alloc] init];
    }
    return self;
}

- (void) defineScriptFile: (NSString*) scriptFileName
             withContents: (NSString*) scriptCode
                    error: (NSError**) outError
{
    LogTrace("defineScriptFile(\"%s\")", [scriptFileName UTF8String]);
    try
    {
        _node->DefineScriptFile(
            std::string([scriptFileName UTF8String]),
            std::string([scriptCode UTF8String]));
        LogTrace("defineScriptFile succeeded");
    }
    catch (...)
    {
        LogTrace("defineScriptFile failed");
        ExceptionToNSError(std::current_exception(), outError);
    }
}

- (void) startAsync: (NSString*) workingDirectory
               then: (void(^)()) success
              catch: (void(^)(NSError*)) failure
{
    LogTrace("start(\"%s\")", [workingDirectory UTF8String]);
    try
    {
        _node->Start(std::string([workingDirectory UTF8String]), [=](std::exception_ptr ex)
        {
            if (ex == nullptr)
            {
                LogTrace("start succeeded");
                success();
            }
            else
            {
                LogTrace("start failed");
                NSError* error;
                ExceptionToNSError(std::current_exception(), &error);
                failure(error);
            }
        });
    }
    catch (...)
    {
        LogTrace("start failed");
        NSError* error;
        ExceptionToNSError(std::current_exception(), &error);
        failure(error);
    }
}

- (void) stopAsyncThen: (void(^)()) success
                 catch: (void(^)(NSError*)) failure
{
    LogTrace("stop()");
    try
    {
        _node->Stop([=](std::exception_ptr ex)
        {
            if (ex == nullptr)
            {
                LogTrace("stop succeeded");
                success();
            }
            else
            {
                LogTrace("stop failed");
                NSError* error;
                ExceptionToNSError(std::current_exception(), &error);
                failure(error);
            }
        });
    }
    catch (...)
    {
        LogTrace("stop failed");
        NSError* error;
        ExceptionToNSError(std::current_exception(), &error);
        failure(error);
    }
}

- (void) callScriptAsync: (NSString*) scriptCode
                  result: (void(^)(NSString*)) success
                   catch: (void(^)(NSError*)) failure
{
    LogTrace("callScript(\"%s\")", [scriptCode UTF8String]);
    try
    {
        _node->CallScript(std::string([scriptCode UTF8String]),
            [=](std::string resultJson, std::exception_ptr ex)
        {
            if (ex == nullptr)
            {
                LogTrace("callScript succeeded");
                NSString* resultJsonString = [NSString stringWithUTF8String: resultJson.c_str()];
                success(resultJsonString);
            }
            else
            {
                LogTrace("callScript failed");
                NSError* error;
                ExceptionToNSError(std::current_exception(), &error);
                failure(error);
            }
        });
    }
    catch (...)
    {
        LogTrace("callScript failed");
        NSError* error;
        ExceptionToNSError(std::current_exception(), &error);
        failure(error);
    }
}

- (void) registerCallFromScript: (NSString*) scriptFunctionName
                          error: (NSError**) outError
{
    LogTrace("registerCallFromScript(\"%s\")", [scriptFunctionName UTF8String]);
    try
    {
        _node->RegisterCallFromScript([scriptFunctionName UTF8String], [=](std::string argsJson)
        {
            LogTrace("callFromScript(\"%s\")", [scriptFunctionName UTF8String]);
            [self raiseCallFromScript: scriptFunctionName
                             argsJson: [NSString stringWithUTF8String: argsJson.c_str()]];
        });
    }
    catch (...)
    {
        LogTrace("registerCallFromScript failed");
        ExceptionToNSError(std::current_exception(), outError);
    }
}

- (void) addCallFromScriptListener: (OT2TNodeCallListener) listener
{
    @synchronized (self)
    {
        [_callFromScriptListeners addObject:listener];
    }
}

- (void) removeCallFromScriptListener: (OT2TNodeCallListener) listener
{
    @synchronized (self)
    {
        if ([_callFromScriptListeners containsObject: listener])
        {
            [_callFromScriptListeners removeObject: listener];
        }
    }
}

- (void) raiseCallFromScript: (NSString*) functionName argsJson: (NSString*) argsJson
{
    OT2TNodeCallEvent* e = [[OT2TNodeCallEvent alloc] initWithFunctionName: functionName argsJson: argsJson];
    @synchronized (self)
    {
        for (OT2TNodeCallListener listener in _callFromScriptListeners)
        {
            listener(self, e);
        }
    }
}

@end
