
#include <functional>
#include <exception>
#include <stdexcept>

#include "Log.h"
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

- (NSString*) mainScriptFileName
{
    const char* mainScriptFileNameChars = _node->GetMainScriptFileName();
    return [NSString stringWithUTF8String: mainScriptFileNameChars];
}

- (void) defineScriptFile: (NSString*) scriptFileName
             withContents: (NSString*) scriptCode
                    error: (NSError**) outError
{
    try
    {
        _node->DefineScriptFile([scriptFileName UTF8String], [scriptCode UTF8String]);
    }
    catch (...)
    {
        ExceptionToNSError(std::current_exception(), outError);
    }
}

- (void) startAsync: (NSString*) workingDirectory
               then: (void(^)()) success
              catch: (void(^)(NSError*)) failure
{
    try
    {
        _node->Start([workingDirectory UTF8String], [=](std::exception_ptr ex)
        {
            if (ex == nullptr)
            {
                success();
            }
            else
            {
                NSError* error;
                ExceptionToNSError(std::current_exception(), &error);
                failure(error);
            }
        });
    }
    catch (...)
    {
        NSError* error;
        ExceptionToNSError(std::current_exception(), &error);
        failure(error);
    }
}

- (void) stopAsyncThen: (void(^)()) success
                 catch: (void(^)(NSError*)) failure
{
    try
    {
        _node->Stop([=](std::exception_ptr ex)
        {
            if (ex == nullptr)
            {
                success();
            }
            else
            {
                NSError* error;
                ExceptionToNSError(std::current_exception(), &error);
                failure(error);
            }
        });
    }
    catch (...)
    {
        NSError* error;
        ExceptionToNSError(std::current_exception(), &error);
        failure(error);
    }
}

- (void) callScriptAsync: (NSString*) scriptCode
                  result: (void(^)(NSString*)) success
                   catch: (void(^)(NSError*)) failure
{
    try
    {
        _node->Start([scriptCode UTF8String], [=](const char* resultJson, std::exception_ptr ex)
        {
            if (ex == nullptr)
            {
                NSString* resultJsonString = [NSString stringWithUTF8String: resultJson];
                success(resultJsonString);
            }
            else
            {
                NSError* error;
                ExceptionToNSError(std::current_exception(), &error);
                failure(error);
            }
        });
    }
    catch (...)
    {
        NSError* error;
        ExceptionToNSError(std::current_exception(), &error);
        failure(error);
    }
}

- (void) registerCallFromScript: (NSString*) scriptFunctionName
                          error: (NSError**) outError
{
    try
    {
        _node->RegisterCallFromScript([scriptFunctionName UTF8String], [=](const char* argsJson)
        {
            [self raiseCallFromScript: scriptFunctionName argsJson: [NSString stringWithUTF8String: argsJson]];
        });
    }
    catch (...)
    {
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
