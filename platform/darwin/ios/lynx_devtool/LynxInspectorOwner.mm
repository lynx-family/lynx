// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <DebugRouter/DebugRouter.h>
#import <DebugRouter/DebugRouterSlot.h>
#import <Lynx/LynxBackgroundRuntime.h>
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxEnvKey.h>
#import <Lynx/LynxLazyLoad.h>
#import <Lynx/LynxPageReloadHelper+Internal.h>
#import <Lynx/LynxRootUI.h>
#import <Lynx/LynxTemplateRender+Internal.h>
#import <Lynx/LynxUIRendererProtocol.h>
#import <LynxDevtool/DevToolMonitorView.h>
#import <LynxDevtool/DevToolPlatformDarwinDelegate.h>
#import <LynxDevtool/LynxDebugBridge.h>
#import <LynxDevtool/LynxDevMenu.h>
#import <LynxDevtool/LynxDevToolDownloader.h>
#import <LynxDevtool/LynxDevToolErrorUtils.h>
#import <LynxDevtool/LynxDevToolNGDarwinDelegate.h>
#import <LynxDevtool/LynxDevToolToast.h>
#import <LynxDevtool/LynxDevtoolEnv.h>
#import <LynxDevtool/LynxInspectorOwner+Internal.h>
#import <LynxDevtool/LynxUITreeHelper.h>
#import <LynxDevtool/TestbenchDumpFileHelper.h>
#import <objc/runtime.h>

#include "core/renderer/dom/ios/lepus_value_converter.h"
#include "core/renderer/template_assembler.h"
#include "core/services/recorder/recorder_controller.h"

#include <memory>
#include <mutex>
#include <queue>
#include "base/include/closure.h"
#include "base/screen_metadata.h"
#include "core/services/replay/replay_controller.h"
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
#import <LynxDevtool/LynxFrameViewTrace.h>
#endif

@interface LynxInspectorOwner () <DebugRouterSlotDelegate>

@end

#pragma mark - LynxInspectorOwner
@implementation LynxInspectorOwner {
  int connection_id_;
  __weak LynxView* _lynxView;
  BOOL _isDebugging;
  int64_t record_id;
  std::mutex mutex_;
  id<GlobalPropsUpdatedObserver> _observer;

  // DevMenu
  LynxDevMenu* _devMenu;

  // PageReload
  LynxPageReloadHelper* _reloadHelper;

  // DevToolPlatformDarwinDelegate for new architecture
  DevToolPlatformDarwinDelegate* _platform;

  // LynxDevToolNGDarwinDelegate for new architecture
  LynxDevToolNGDarwinDelegate* _devtoolNG;

  void (^_dispatch_message_block)(NSDictionary*);

  LynxTemplateData* _globalProps;
}

- (instancetype)init {
  self = [super init];
  // LynxDevToolNGDarwinDelegate
  _devtoolNG = [[LynxDevToolNGDarwinDelegate alloc] init];
  return self;
}

- (nonnull instancetype)initWithLynxView:(nullable LynxView*)view {
  _lynxView = view;
  _isDebugging = NO;

  // DevMenu
  _devMenu = [[LynxDevMenu alloc] initWithInspectorOwner:self];

  // PageReload
  _reloadHelper = nil;

  // DevToolPlatformDarwinDelegate
  _platform = nil;

  // LynxDevToolNGDarwinDelegate
  _devtoolNG = [[LynxDevToolNGDarwinDelegate alloc] init];

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
  [[LynxFrameViewTrace shareInstance] attachView:view];
#endif

  return self;
}

- (void)setReloadHelper:(nullable LynxPageReloadHelper*)reloadHelper {
  _reloadHelper = reloadHelper;
  if (_platform) {
    [_platform setReloadHelper:_reloadHelper];
  }
}

- (void)onBackgroundRuntimeCreated:(LynxBackgroundRuntime*)runtime
                   groupThreadName:(NSString*)groupThreadName {
  if (_devtoolNG != nil) {
    [_devtoolNG onBackgroundRuntimeCreated:runtime groupThreadName:groupThreadName];
  }
}

- (void)onTemplateAssemblerCreated:(intptr_t)ptr {
  if (_devtoolNG != nil) {
    [_devtoolNG onTemplateAssemblerCreated:ptr];
    if (_platform != nil) {
      [_devtoolNG setDevToolPlatformAbility:[_platform getNativePtr]];
      [_platform setDevToolCallback:_dispatch_message_block];
    }
  }

  self->record_id = ptr;
}

- (void)setPostUrl:(nullable NSString*)postUrl {
  // deprecated
}

- (void)onLoadFinished {
  // Attach debug bridge if necessary
  if ([[LynxDebugBridge singleton] isEnabled]) {
    if ([_lynxView isKindOfClass:[DevToolMonitorView class]]) {
      [[LynxDebugBridge singleton] sendDebugStateEvent];
    }
  }
}

- (void)reloadLynxView:(BOOL)ignoreCache {
  [self reloadLynxView:ignoreCache withTemplate:nil fromFragments:NO withSize:0];
}

- (void)reloadLynxView:(BOOL)ignoreCache
          withTemplate:(NSString*)templateBin
         fromFragments:(BOOL)fromFragments
              withSize:(int32_t)size {
  [LynxDevToolToast showToast:@"Start to download & reload..."];
  [_reloadHelper reloadLynxView:ignoreCache
                   withTemplate:templateBin
                  fromFragments:fromFragments
                       withSize:size];
}

- (void)onReceiveTemplateFragment:(NSString*)data withEof:(BOOL)eof {
  [_reloadHelper onReceiveTemplateFragment:data withEof:eof];
}

- (void)navigateLynxView:(nonnull NSString*)url {
  [_reloadHelper navigateLynxView:url];
}

- (void)stopCasting {
  [_platform stopCasting];
}

- (void)continueCasting {
  if (_devtoolNG != nil && [_devtoolNG isAttachToDebugRouter]) {
    [_platform continueCasting];
  }
}

- (void)pauseCasting {
  if (_devtoolNG != nil && [_devtoolNG isAttachToDebugRouter]) {
    [_platform pauseCasting];
  }
}

- (LynxView*)getLynxView {
  return _lynxView;
}

- (void)setConnectionID:(int)connectionID {
  connection_id_ = connectionID;
}

- (void)dealloc {
  [self stopCasting];
  if (_devtoolNG) {
    [_devtoolNG detachToDebug];
  }
}

- (void)handleLongPress {
  if (LynxEnv.sharedInstance.devtoolEnabled && LynxDevtoolEnv.sharedInstance.longPressMenuEnabled) {
    [self showDevMenu];
  }
}

- (void)showDevMenu {
  if (_devMenu != nil) {
    [_devMenu show];
  }
}

- (NSInteger)getSessionId {
  return _devtoolNG != nil ? [_devtoolNG getSessionId] : 0;
}

- (NSString*)getTemplateUrl {
  return _reloadHelper ? [_reloadHelper getURL] : @"___UNKNOWN___";
}

- (UIView*)getTemplateView {
  return _lynxView;
}

- (LynxTemplateData*)getTemplateData {
  return _reloadHelper ? [_reloadHelper getTemplateData] : nullptr;
}

- (NSString*)getTemplateJsInfo:(uint32_t)offset withSize:(uint32_t)size {
  if (_reloadHelper != nil) {
    return [_reloadHelper getTemplateJsInfo:offset withSize:size];
  }
  return nil;
}

- (void)sendResponse:(std::string)response {
  LOGV("LynxInspectorOwner" << response);
  if (_devtoolNG != nil) {
    [_devtoolNG sendMessageToDebugPlatform:[NSString stringWithUTF8String:response.c_str()]
                                  withType:@"CDP"];
  }
}

- (BOOL)isDebugging {
  return _isDebugging;
}

- (void)sendConsoleMessage:(NSString*)message
                 withLevel:(int32_t)level
             withTimeStamp:(int64_t)timeStamp {
  if (_platform != nil) {
    [_platform sendConsoleEvent:message withLevel:level withTimeStamp:timeStamp];
  }
}

- (void)attach:(nonnull LynxView*)lynxView {
  _lynxView = lynxView;
  [_reloadHelper attachLynxView:lynxView];
  [_platform attachLynxView:lynxView];
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
  [[LynxFrameViewTrace shareInstance] attachView:lynxView];
#endif
}

- (void)attachDebugBridge:(NSString*)url {
  LOGI("LynxInspectorOwner attachToDebugBridge:" << url);
  if (_devtoolNG != nil && (![_devtoolNG isAttachToDebugRouter])) {
    int sessionId = [_devtoolNG attachToDebug:url];
    LynxView* lynxView = [self getLynxView];
    if (sessionId > 0 && lynxView != nil) {
      [[DebugRouter instance] setSessionId:sessionId ofView:lynxView];
    }
    [self initRecord];
  }
}

- (void)initRecord {
  CGSize size = [UIScreen mainScreen].bounds.size;
  NSString* filePath =
      [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject];
  if (filePath) {
    lynx::tasm::recorder::RecorderController::InitConfig([filePath UTF8String], [self getSessionId],
                                                         size.width, size.height, self->record_id);
  }
}

- (void)dispatchMessageEvent:(NSDictionary*)event {
  if (_dispatch_message_block == nil) {
    return;
  }
  _dispatch_message_block(event);
}

- (void)sendMessage:(CustomizedMessage*)message {
  if (_devtoolNG != nil && message != nil) {
    [_devtoolNG sendMessageToDebugPlatform:[message data] withType:[message type]];
  }
}

- (void)subscribeMessage:(NSString*)type withHandler:(id<MessageHandler>)handler {
  if (_devtoolNG != nil && type != nil) {
    [_devtoolNG subscribeMessage:type withHandler:handler];
  }
}

- (void)unsubscribeMessage:(NSString*)type {
  if (_devtoolNG != nil && type != nil) {
    [_devtoolNG unsubscribeMessage:type];
  }
}

- (void)invokeCDPFromSDK:(NSString*)msg withCallback:(CDPResultCallback)callback {
  if (_devtoolNG != nil) {
    [_devtoolNG invokeCDPFromSDK:msg withCallback:callback];
  }
}

- (void)endTestbench:(NSString*)filePath {
  if (filePath == nil) {
    return;
  }
  NSLog(@"end testbench replay test");
  LynxTemplateRender* render = [self getLynxView].templateRender;
  NSString* ui_tree = [TestbenchDumpFileHelper getUITree:[render.lynxUIRenderer rootUI]];
  if (ui_tree) {
    std::string ui_tree_str([ui_tree UTF8String]);
    lynx::tasm::replay::ReplayController::SendFileByAgent("UITree", ui_tree_str);
  }

  lynx::tasm::replay::ReplayController::EndTest(std::string([filePath UTF8String]));
}

- (int64_t)getRecordID {
  return self->record_id;
}

- (void)enableRecording:(bool)enable {
  LynxEnv.sharedInstance.recordEnable = enable;
}

- (void)enableTraceMode:(bool)enable {
  [LynxDevtoolEnv.sharedInstance setSwitchMask:!enable forKey:SP_KEY_ENABLE_DOM_TREE];
  [LynxDevtoolEnv.sharedInstance setSwitchMask:!enable forKey:SP_KEY_ENABLE_PREVIEW_SCREEN_SHOT];
  [LynxDevtoolEnv.sharedInstance setSwitchMask:!enable forKey:SP_KEY_ENABLE_HIGHLIGHT_TOUCH];
}

- (void)onPageUpdate {
  if (_platform != nil) {
    [_platform sendLayerTreeDidChangeEvent];
  }
}

- (void)downloadResource:(NSString* _Nonnull)url callback:(LynxResourceLoadBlock _Nonnull)callback {
  [LynxDevToolDownloader
          download:url
      withCallback:^(NSData* _Nullable data, NSError* _Nullable error) {
        if (!error) {
          callback([[LynxResourceResponse alloc] initWithData:data]);
        } else {
          callback([[LynxResourceResponse alloc] initWithError:error
                                                          code:LynxResourceResponseCodeFailed]);
        }
      }];
}

- (void)setLynxInspectorConsoleDelegate:(id)delegate {
  [_platform setLynxInspectorConsoleDelegate:delegate];
}

- (void)getConsoleObject:(NSString*)objectId
           needStringify:(BOOL)stringify
           resultHandler:(void (^)(NSString* _Nonnull detail))handler {
  [_platform getConsoleObject:objectId needStringify:stringify resultHandler:handler];
}

- (void)onPerfMetricsEvent:(NSString*)eventName withData:(NSDictionary<NSString*, NSObject*>*)data {
  if (!LynxDevtoolEnv.sharedInstance.perfMetricsEnabled) {
    return;
  }

  NSMutableDictionary<NSString*, id>* dict = [[NSMutableDictionary alloc] init];
  [dict setValue:eventName forKey:@"method"];
  [dict setValue:data forKey:@"params"];

  CustomizedMessage* msg = [[CustomizedMessage alloc] init];
  msg.type = @"CDP";

  NSData* jsonData = [NSJSONSerialization dataWithJSONObject:dict options:0 error:0];
  if (!jsonData) {
    return;
  }
  NSString* myString = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
  msg.data = myString;
  msg.mark = -1;
  [self sendMessage:msg];
}

- (void)onReceiveMessageEvent:(NSDictionary*)event {
  if (_devtoolNG == nil || ![_devtoolNG isAttachToDebugRouter] || !event) {
    return;
  }
  // 'eventName' corresponds to field 'type' from engine's event
  id eventName = [event objectForKey:@"type"];
  if (![eventName isKindOfClass:[NSString class]]) {
    return;
  }
  id origin = [event objectForKey:@"origin"];
  id data = [event objectForKey:@"data"];
  NSDictionary* params =
      @{@"event" : eventName, @"vmType" : origin ? origin : @"", @"data" : data ? data : @""};
  [self handleCDPEvent:@"Lynx.onVMEvent" withParams:params];
}

- (void)handleCDPEvent:(NSString*)event withParams:(NSDictionary*)params {
  if (_devtoolNG == nil || ![_devtoolNG isAttachToDebugRouter] || !event) {
    return;
  }
  NSMutableDictionary* msg = [[NSMutableDictionary alloc] init];
  msg[@"method"] = event;
  if (params) {
    msg[@"params"] = params;
  }
  if ([NSJSONSerialization isValidJSONObject:msg]) {
    NSData* jsonData = [NSJSONSerialization dataWithJSONObject:msg options:0 error:nil];
    NSString* jsonStr = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
    [_devtoolNG sendMessageToDebugPlatform:jsonStr withType:@"CDP"];
  }
}

- (void)setDispatchMessageEventBlock:(void (^)(NSDictionary*))block {
  _dispatch_message_block = block;
  if (_platform) {
    [_platform setDevToolCallback:block];
  }
}

- (NSString*)debugInfoUrl {
  return [_platform getLepusDebugInfoUrl];
}

- (CGPoint)getViewLocationOnScreen {
  __strong typeof(_lynxView) view = _lynxView;
  if (view && view.window) {
    CGPoint pointInWindow = [view convertPoint:CGPointMake(0, 0) toView:nil];
    CGPoint pointInScreen = [view.window convertPoint:pointInWindow toWindow:nil];
    return pointInScreen;
  }
  return CGPointMake(-1, -1);
}

- (void)setGlobalPropsUpdatedObserver:(id<GlobalPropsUpdatedObserver>)observer {
  _observer = observer;
  if (_globalProps) {
    // if globalProps exists, trigger observer immediately.
    [_observer onGlobalPropsUpdated:[_globalProps dictionary]];
  }
}

- (void)onGlobalPropsUpdated:(LynxTemplateData*)props {
  _globalProps = props;
  if (_observer) {
    [_observer onGlobalPropsUpdated:[_globalProps dictionary]];
  }
}

- (void)showErrorMessageOnConsole:(LynxError*)error {
  // js/lepus errors are already displayed on console
  if (!error || ![error isValid] || [error isJSError] || [error isLepusError]) {
    return;
  }
  NSString* errMsg = [LynxDevToolErrorUtils getKeyMessage:error];
  NSString* consoleLog = [NSString stringWithFormat:@"Native error:\n%@", errMsg];
  int32_t level = [LynxDevToolErrorUtils intValueFromErrorLevelString:error.level];
  [self sendConsoleMessage:consoleLog
                 withLevel:level
             withTimeStamp:[[NSDate date] timeIntervalSince1970] * 1000];
}

- (void)showMessageOnConsole:(NSString*)message withLevel:(int32_t)level {
  [self sendConsoleMessage:message
                 withLevel:level
             withTimeStamp:[[NSDate date] timeIntervalSince1970] * 1000];
}

- (void)attachLynxUIOwnerToAgent:(nullable LynxUIOwner*)uiOwner {
  _platform = [[DevToolPlatformDarwinDelegate alloc] initWithLynxView:_lynxView
                                                          withUIOwner:uiOwner];
}

@end
