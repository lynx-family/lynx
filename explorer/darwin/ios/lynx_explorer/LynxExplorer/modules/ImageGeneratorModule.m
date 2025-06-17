// explorer/darwin/ios/lynx_explorer/LynxExplorer/modules/ImageGeneratorModule.m
#import <Foundation/Foundation.h>
#import <Lynx/LynxModule.h> // Assuming LynxModule.h is the correct import
#import <Lynx/LynxDefines.h> // For LYNX_EXPORT_METHOD etc.
#import <Lynx/LynxCallback.h>
#import <Lynx/LynxPromise.h>

@interface ImageGeneratorModule : LynxModule
@end

@implementation ImageGeneratorModule

LYNX_MODULE_NAME(@"ImageGeneratorModule") // Macro to define module name for JS

// Expose methods to JavaScript
// The format is: LYNX_EXPORT_METHOD(jsName, objcSelector)
// Ensure the selector matches the actual Objective-C method signature.

LYNX_EXPORT_METHOD(generateImage, generateImageWithPrompt:(NSString *)prompt promise:(LynxPromise *)promise)
LYNX_EXPORT_METHOD(logPrompt, logPromptWithMessage:(NSString *)prompt callback:(LynxCallback *)callback)

- (void)generateImageWithPrompt:(NSString *)prompt promise:(LynxPromise *)promise {
    NSLog(@"Native iOS: generateImage called with prompt: %@", prompt);

    // Simulate some work
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        if (prompt == nil || [prompt length] == 0) {
            NSString *errorMsg = @"Prompt cannot be empty.";
            if (promise) {
                [promise reject:@"Error" message:errorMsg error:nil];
            }
            return;
        }

        NSString *encodedPrompt = [prompt stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLQueryAllowedCharacterSet]];
        if (encodedPrompt == nil) {
            encodedPrompt = @"empty_prompt";
        }
        NSString *simulatedImageUrl = [NSString stringWithFormat:@"https://dummyjson.com/image/400x300/007bff/fff?text=Native+iOS+%@&v=%@", encodedPrompt, [[NSUUID UUID] UUIDString]];

        if (promise) {
            [promise resolve:simulatedImageUrl];
        }
    });
}

- (void)logPromptWithMessage:(NSString *)prompt callback:(LynxCallback *)callback {
    NSString *message = [NSString stringWithFormat:@"Native iOS: logPrompt called with: %@", prompt];
    NSLog(@"%@", message);
    if (callback) {
        [callback invoke:@[[NSString stringWithFormat:@"Prompt logged successfully by iOS Native Module: %@", prompt]]];
    }
}

@end
