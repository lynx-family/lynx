// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxPageReloadHelper.h"
#import "LynxResourceProvider.h"
#if TARGET_OS_IOS
#import "LynxUIOwner.h"
#endif
#import "LynxError.h"
#import "LynxView.h"

NS_ASSUME_NONNULL_BEGIN

typedef void (^CDPResultCallback)(NSString *result);

@protocol LynxBaseInspectorOwner <NSObject>

@required

- (nonnull instancetype)initWithLynxView:(nullable LynxView *)view;

- (void)setReloadHelper:(nullable LynxPageReloadHelper *)reloadHelper;

#if TARGET_OS_IOS
- (void)onBackgroundRuntimeCreated:(LynxBackgroundRuntime *)runtime
                   groupThreadName:(NSString *)groupThreadName;
#endif

- (void)onTemplateAssemblerCreated:(intptr_t)ptr;

- (void)handleLongPress;

- (void)stopCasting;

- (void)continueCasting;

- (void)pauseCasting;

- (void)setPostUrl:(nullable NSString *)postUrl;

- (void)onLoadFinished;

- (void)reloadLynxView:(BOOL)ignoreCache;

- (void)navigateLynxView:(nonnull NSString *)url;

- (void)emulateTouch:(nonnull NSString *)type
         coordinateX:(int)x
         coordinateY:(int)y
              button:(nonnull NSString *)button
              deltaX:(CGFloat)dx
              deltaY:(CGFloat)dy
           modifiers:(int)modifiers
          clickCount:(int)clickCount;

- (void)call:(NSString *_Nonnull)function withParam:(NSString *_Nullable)params;

- (void)attach:(nonnull LynxView *)lynxView;

- (nonnull NSString *)groupID __attribute__((deprecated("Deprecated after Lynx2.18")));

- (void)reloadLynxView:(BOOL)ignoreCache
          withTemplate:(nullable NSString *)templateBin
         fromFragments:(BOOL)fromFragments
              withSize:(int32_t)size;
/**
 * Invokes a CDP method from the SDK.
 *
 * This method replaces the previous `invokeCDPFromSDK:` method. Unlike the old method,
 * the new method does not limit the use of the main thread. Therefore, it can be called
 * from any thread.
 *
 * @discussion This method accepts a CDP command message and a callback block to handle the result.
 * The result of the CDP command will be returned asynchronously through the callback block.
 *
 * <b>Note:</b> This is a breaking change introduced in version 3.0
 *
 * @param msg The CDP command method to be sent. This parameter must not be nil.
 * @param callback A block to be called when the CDP command result is available.
 * The final execution thread of this block depends on the last thread that processes
 * the CDP protocol, which could be a TASM thread, UI thread, devtool thread, etc.
 *
 * @since 3.0
 *
 * @note Example usage:
 *
 * ```
 * [inspectorOwner invokeCDPFromSDK:jsonString
 *                        withCallback:^(NSString* result){
 *                         }];
 * ```
 */
- (void)invokeCDPFromSDK:(NSString *)msg withCallback:(CDPResultCallback)callback;

- (void)onReceiveTemplateFragment:(nullable NSString *)data withEof:(BOOL)eof;

- (void)attachDebugBridge:(NSString *)url;

- (void)endTestbench:(NSString *_Nonnull)filePath;

- (void)onPageUpdate;

#if TARGET_OS_IOS
- (void)attachLynxUIOwnerToAgent:(nullable LynxUIOwner *)uiOwner;
#endif

- (void)downloadResource:(NSString *_Nonnull)url callback:(LynxResourceLoadBlock _Nonnull)callback;

- (void)setLynxInspectorConsoleDelegate:(id _Nonnull)delegate;

- (void)getConsoleObject:(NSString *_Nonnull)objectId
           needStringify:(BOOL)stringify
           resultHandler:(void (^_Nonnull)(NSString *_Nonnull detail))handler;

- (void)onPerfMetricsEvent:(NSString *_Nonnull)eventName
                  withData:(NSDictionary<NSString *, NSObject *> *_Nonnull)data;

- (void)onReceiveMessageEvent:(NSDictionary *)event;

- (void)setDispatchMessageEventBlock:(void (^)(NSDictionary *))block;

- (NSString *)debugInfoUrl;

- (void)onGlobalPropsUpdated:(LynxTemplateData *)props;

- (void)showErrorMessageOnConsole:(LynxError *)error;
- (void)showMessageOnConsole:(NSString *)message withLevel:(int32_t)level;

@end

@protocol LynxViewStateListener <NSObject>

@required
- (void)onLoadFinished;
- (void)onMovedToWindow;
- (void)onEnterForeground;
- (void)onEnterBackground;
- (void)onDestroy;

@end

NS_ASSUME_NONNULL_END
