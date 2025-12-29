// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxDebugInfoRecorderProtocol.h>
#import <Lynx/LynxPageReloadHelper.h>
#import <Lynx/LynxResourceProvider.h>
#if TARGET_OS_IOS
#import <Lynx/LynxUIOwner.h>
#endif
#import <Lynx/LynxError.h>
#import <Lynx/LynxView.h>

NS_ASSUME_NONNULL_BEGIN

typedef void (^CDPResultCallback)(NSString *result);

@protocol CDPEventListener <NSObject>

@required
- (void)onEvent:(nonnull NSString *)event;

@end

@protocol LynxBaseInspectorOwner <NSObject>

@required

- (nonnull instancetype)initWithLynxView:(nullable LynxView *)view;

- (void)setReloadHelper:(nullable LynxPageReloadHelper *)reloadHelper;
- (void)setDebugInfoInterceptor:(nonnull id<LynxDebugInfoRecorderProtocol>)debugInfoRecorder;

#if TARGET_OS_IOS
- (void)onBackgroundRuntimeCreated:(LynxBackgroundRuntime *)runtime
                   groupThreadName:(NSString *)groupThreadName;
#endif

- (void)onTemplateAssemblerCreated:(intptr_t)ptr;

- (void)handleLongPress;

- (void)continueCasting;

- (void)pauseCasting;

- (void)onLoadFinished;

- (void)reloadLynxView:(BOOL)ignoreCache;

- (void)attach:(nonnull LynxView *)lynxView;

- (void)reloadLynxView:(BOOL)ignoreCache
          withTemplate:(nullable NSString *)templateBin
         fromFragments:(BOOL)fromFragments
              withSize:(int32_t)size;

- (void)reloadLynxView:(BOOL)ignoreCache
          withTemplate:(nullable NSString *)templateBin
         fromFragments:(BOOL)fromFragments
              withSize:(int32_t)size
             reloadUrl:(nullable NSString *)reloadUrl;
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

/**
 * Adds a listener identified by a given name for view-level CDP events.
 *
 * This method registers a view-level CDP event listener associated with the specified name. The
 * name acts as an identifier for the listener, which can be used later to remove the listener via
 * `removeCDPEventListener:` method.
 *
 * @param name The unique name identifying the event listener. This parameter must not be nil.
 * @param listener An object conforming to the `CDPEventListener` protocol that will receive
 * notifications for the view-level CDP events. This parameter must not be nil.
 *
 * @discussion Multiple listeners can be registered under different names. When a view-level CDP
 * event occurs, all registered listener interfaces will receive callbacks. All event callbacks are
 * dispatched and executed on the dedicated `cdp_event_listener` thread.
 *
 * The lifecycle of the listener object is managed by the caller. This API holds only a weak
 * reference to the listener. This means the listener will only be called as long as it is still
 * alive.
 *
 *
 * @note Example usage:
 *
 * ```
 * [inspectorOwner addCDPEventListener:@"test_cdp_listener" withListener:eventListener];
 * ```
 */
- (void)addCDPEventListener:(nonnull NSString *)name
               withListener:(nonnull id<CDPEventListener>)listener;

/**
 * Removes the CDP event listener identified by the given name.
 *
 * This method unregisters and removes the event listener associated with the specified name.
 * After this call, the listener will no longer receive event notifications.
 *
 * @param name The unique name identifying the event listener to remove. This parameter must not be
 * nil.
 *
 * @discussion If no listener is found with the given name, this method performs no operation.
 *
 *
 * @note Example usage:
 *
 * ```
 * [inspectorOwner removeCDPEventListener:@"test_cdp_listener"];
 * ```
 */
- (void)removeCDPEventListener:(nonnull NSString *)name;

- (void)attachDebugBridge:(NSString *)url;

- (void)endTestbench:(NSString *_Nonnull)filePath;

- (void)onPageUpdate;

#if TARGET_OS_IOS
- (void)attachLynxUIOwnerToAgent:(nullable LynxUIOwner *)uiOwner;
#endif

- (void)setLynxInspectorConsoleDelegate:(id _Nonnull)delegate;

- (void)getConsoleObject:(NSString *_Nonnull)objectId
           needStringify:(BOOL)stringify
           resultHandler:(void (^_Nonnull)(NSString *_Nonnull detail))handler;

- (void)onPerfMetricsEvent:(NSString *_Nonnull)eventName
                  withData:(NSDictionary<NSString *, NSObject *> *_Nonnull)data;

- (void)onReceiveMessageEvent:(NSDictionary *)event;

- (void)setDispatchMessageEventBlock:(void (^)(NSDictionary *))block;

- (NSString *)debugInfoUrl:(NSString *_Nonnull)filename;

/**
 * Called when LynxTemplateRender triggers an update using UpdateData or UpdateMetaData
 */
- (void)onTemplateDataUpdated:(LynxTemplateData *)data;
/**
 * Called when data is reset through LynxTemplateRender resetData method
 */
- (void)onResetDataWithTemplateData:(LynxTemplateData *)data;

- (void)onGlobalPropsUpdated:(LynxTemplateData *)props;

- (void)showErrorMessageOnConsole:(LynxError *)error;
- (void)showMessageOnConsole:(NSString *)message withLevel:(int32_t)level;

- (void)setDebugTag:(NSString *)debugTag;

@end

NS_ASSUME_NONNULL_END
