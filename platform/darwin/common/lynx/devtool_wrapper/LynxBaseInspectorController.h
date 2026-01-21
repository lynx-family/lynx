// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

#import <Lynx/LynxDebugInfoRecorderProtocol.h>
#import <Lynx/LynxPageReloadHelper.h>

#if TARGET_OS_IOS
#import <Lynx/LynxUIOwner.h>
#endif
#import <Lynx/LynxError.h>
#import <Lynx/LynxView.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LynxBaseInspectorController <NSObject>

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

- (void)reloadLynxView:(BOOL)ignoreCache
          withTemplate:(nullable NSString *)templateBin
         fromFragments:(BOOL)fromFragments
              withSize:(int32_t)size
             reloadUrl:(nullable NSString *)reloadUrl;

- (void)attach:(nonnull LynxView *)lynxView;

- (void)attachDebugBridge:(NSString *)url;

- (void)endTestbench:(NSString *_Nonnull)filePath;

- (void)onPageUpdate;

#if TARGET_OS_IOS
- (void)attachLynxUIOwnerToAgent:(nullable LynxUIOwner *)uiOwner;
#endif

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

@end

NS_ASSUME_NONNULL_END
