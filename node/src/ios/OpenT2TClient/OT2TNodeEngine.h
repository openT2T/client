
#import <Foundation/Foundation.h>

@interface OT2TNodeCallEvent : NSObject

- (OT2TNodeCallEvent*) init;

- (OT2TNodeCallEvent*) initWithFunctionName: (NSString*) functionName
                                   argsJson: (NSString*) argsJson;

@property NSString* functionName;
@property NSString* argsJson;

@end

typedef void (^OT2TNodeCallListener)(NSObject* sender, OT2TNodeCallEvent* e);

@interface OT2TNodeEngine : NSObject

+ (void) initialize;

- (OT2TNodeEngine*) init;

@property (readonly) NSString* mainScriptFileName;

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
