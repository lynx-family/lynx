// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxFrameRender.h>

#import <Lynx/LynxTemplateRenderContext+Internal.h>
#import <Lynx/LynxTemplateRenderHelper.h>
#import <Lynx/LynxUIRenderer.h>
#import <Lynx/TemplateRenderCallbackProtocol.h>

#pragma mark - LynxFrameRender

@implementation LynxFrameRender {
  LynxTemplateRenderContext *_renderContext;
}

- (instancetype)init {
  if (self = [super init]) {
    // TODO(zhoupeng.z): get context from root view
    _renderContext = [[LynxTemplateRenderContext alloc] init];
    _renderContext->_lynxUIRenderer = [[LynxUIRenderer alloc] init];
    [LynxTemplateRenderHelper setUpFrameRender:self screenSize:[UIScreen mainScreen].bounds.size];
  }
  return self;
}

- (void)dealloc {
  [_renderContext->_lynxUIRenderer reset];
  _renderContext->timing_collector_platform_impl_.reset();
  // ios block cannot capture std::unique_ptr, tricky...
  auto *shell = _renderContext->shell_.release();
  // LynxView maybe release in main flow of TemplateAssembler,
  // so need just release LynxShell delay, avoid crash
  dispatch_async(dispatch_get_main_queue(), ^{
    delete shell;
  });
}

#pragma mark - LynxTemplateRenderContextProtocol

- (LynxTemplateRenderContext *)getRenderContext {
  return _renderContext;
}

- (void)attachRenderContext:(LynxTemplateRenderContext *)context {
  _renderContext = context;
}

#pragma mark - LynxErrorReceiverProtocol

- (void)onErrorOccurred:(LynxError *)error {
}

#pragma mark - TemplateRenderCallbackProtocol

// TODO(zhoupeng.z): implement following methods, some of them are useless for LynxFrameView.
// Optimize it later

- (void)onDataUpdated {
}

- (void)onPageChanged:(BOOL)isFirstScreen {
}

- (void)onTasmFinishByNative {
}

- (void)onTemplateLoaded:(NSString *)url {
}

- (void)onRuntimeReady {
}

- (void)onErrorOccurred:(NSInteger)code message:(NSString *)errMessage {
}

- (void)didInvokeMethod:(NSString *)method inModule:(NSString *)module errorCode:(int32_t)code {
}

- (void)onTimingSetup:(NSDictionary *)timingInfo {
}

- (void)onTimingUpdate:(NSDictionary *)timingInfo updateTiming:(NSDictionary *)updateTiming {
}

- (void)onPerformanceEvent:(NSDictionary *)originDict {
}

- (void)onFirstLoadPerf:(NSDictionary *)perf {
}

- (void)onUpdatePerfReady:(NSDictionary *)perf {
}

- (void)onDynamicComponentPerf:(NSDictionary *)perf {
}

- (void)setPageConfig:(const std::shared_ptr<lynx::tasm::PageConfig> &)pageConfig {
}

- (void)setTiming:(uint64_t)timestamp
              key:(NSString *)key
       pipelineID:(nullable NSString *)pipelineID {
}

- (NSString *)translatedResourceWithId:(NSString *)resId themeKey:(NSString *)key {
  return nil;
}

- (void)getI18nResourceForChannel:(NSString *)channel withFallbackUrl:(NSString *)url {
}

- (void)invokeLepusFunc:(NSDictionary *)data callbackID:(int32_t)callbackID {
}

- (void)onCallJSBFinished:(NSDictionary *)info {
}

- (void)onJSBInvoked:(NSDictionary *)info {
}

- (void)onReceiveMessageEvent:(NSDictionary *)event {
}

- (long)initStartTiming {
  return 0L;
}
- (long)initEndTiming {
  return 0L;
}

- (void)invokeUIMethod:(NSString *_Nonnull)method_string
                params:(NSDictionary *_Nonnull)params
              callback:(int)callback
                toNode:(int)node {
}

- (void)onSSRHydrateFinished:(NSString *)url {
}

- (void)onTemplateBundleReady:(LynxTemplateBundle *)bundle {
}

- (void)setLocalTheme:(LynxTheme *)theme {
}

- (LynxContext *)getLynxContext {
  return nil;
}

- (NSMutableDictionary<NSString *, id> *)getLepusModulesClasses {
  return nil;
}

- (void)onReloadTemplate:(const std::vector<uint8_t> &)data
                 withURL:(const std::string &)url
                initData:(const std::shared_ptr<lynx::tasm::TemplateData> &)init_data {
}

- (void)onEventCapture:(NSInteger)targetID
        withEventCatch:(BOOL)isCatch
            andEventID:(int64_t)eventID {
}

- (void)onEventBubble:(NSInteger)targetID withEventCatch:(BOOL)isCatch andEventID:(int64_t)eventID {
}

- (void)onEventFire:(NSInteger)targetID withEventStop:(BOOL)isStop andEventID:(int64_t)eventID {
}

- (void)notifyIntersectionObservers {
}

- (BOOL)onLynxEvent:(LynxEvent *)event {
  return NO;
}

@end
