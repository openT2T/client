
#import <Foundation/Foundation.h>

// Event raised when a registered function is called by script.
@interface OT2TNodeCallEvent : NSObject

- (OT2TNodeCallEvent*) init;

- (OT2TNodeCallEvent*) initWithFunctionName: (NSString*) functionName
                                   argsJson: (NSString*) argsJson;

// Name of the function that was called by script.
@property (retain) NSString* functionName;

// JSON-serialized array of arguments passed by the script.
@property (retain) NSString* argsJson;

@end

// Listens for calls to functions registered in the node scripting environment.
typedef void (^OT2TNodeCallListener)(NSObject* sender, OT2TNodeCallEvent* e);

// APIs for interacting with a node engine hosted in the application.
// Wrapper around the cross-platform C++ INodeEngine interface.
// (See INodeEngine.h for interface documentation.)
@interface OT2TNodeEngine : NSObject

+ (void) initialize;

- (OT2TNodeEngine*) init;

- (void) defineScriptFile: (NSString*) scriptFileName
             withContents: (NSString*) scriptCode
                    error: (NSError**) outError;

- (void) startAsync: (NSString*) workingDirectory
               then: (void(^)()) success
              catch: (void(^)(NSError*)) failure;

- (void) stopAsyncThen: (void(^)()) success
                 catch: (void(^)(NSError*)) failure;

- (void) callScriptAsync: (NSString*) scriptCode
                  result: (void(^)(NSString*)) success
                   catch: (void(^)(NSError*)) failure;

- (void) registerCallFromScript: (NSString*) scriptFunctionName
                          error: (NSError**) outError;

- (void) addCallFromScriptListener: (OT2TNodeCallListener) listener;

- (void) removeCallFromScriptListener: (OT2TNodeCallListener) listener;

@end
