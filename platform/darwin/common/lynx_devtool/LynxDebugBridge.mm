// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <memory>
#include <unordered_map>

#import <Lynx/LynxEnv.h>
#import <Lynx/LynxEventReporter.h>
#import <Lynx/LynxLog.h>
#import "DevToolMonitorView.h"
#import "LynxDebugBridge.h"
#import "LynxDevToolToast.h"
#import "LynxDeviceInfoHelper.h"
#import "LynxDevtoolEnv.h"
#import "LynxInspectorOwner.h"
#if OS_IOS
#import <DebugRouter/DebugRouter.h>
#import <DebugRouter/DebugRouterGlobalHandler.h>
#endif
#include "core/services/recorder/recorder_controller.h"
#include "devtool/lynx_devtool/config/devtool_config.h"

typedef LynxInspectorOwner DevToolAgentDispatcher;

@interface LynxEventReporter (LynxDebugBridge)

+ (instancetype)sharedInstance;

@end

@interface LynxDebugBridge () <DebugRouterGlobalHandler,
                               DebugRouterStateListener,
                               LynxEventReportObserverProtocol>

@end

@implementation LynxDebugBridge {
  DevToolMonitorView *monitor_view_;
  DevToolAgentDispatcher *agent_dispatcher_;
  BOOL has_set_open_card_callback_;
  NSMutableArray<LynxDebugBridgeOpenCardCallback> *open_card_callbacks;
}

+ (instancetype)singleton {
  static LynxDebugBridge *_instance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    _instance = [[LynxDebugBridge alloc] init];
  });

  return _instance;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    agent_dispatcher_ = [[DevToolAgentDispatcher alloc] init];
    open_card_callbacks = [[NSMutableArray alloc] init];
    [[DebugRouter instance] addGlobalHandler:self];
    [[DebugRouter instance] addStateListener:self];
  }
  if (LynxEnv.sharedInstance.launchRecordEnabled) {
    lynx::tasm::recorder::RecorderController::StartRecord();
  }
  [LynxEventReporter addEventReportObserver:self];
  return self;
}

- (BOOL)enable:(NSURL *)url withOptions:(NSDictionary *)options {
  if (!LynxEnv.sharedInstance.devtoolEnabled &&
      !LynxEnv.sharedInstance.devtoolEnabledForDebuggableView) {
    [LynxDevToolToast showToast:@"DevTool not enabled, turn on the switch!"];
    return NO;
  }

  if ([[DebugRouter instance] isValidSchema:url.absoluteString]) {
    signal(SIGPIPE, SIG_IGN);
    _hostOptions = options;
    [self setAppInfo:options];
    return [[DebugRouter instance] handleSchema:url.absoluteString];
  }
  return NO;
}

// only used by automatic test
- (void)enableDebugging:(NSString *)params {
  [[DebugRouter instance] handleSchema:params];
}

- (BOOL)isEnabled {
  return [DebugRouter instance].connection_state == CONNECTED;
}

using ClientInfo = std::unordered_map<std::string, std::string>;
- (ClientInfo)getClientInfo {
  ClientInfo map;
  for (NSString *key in _hostOptions) {
    if (key && [_hostOptions objectForKey:key]) {
      map.insert(std::make_pair([key UTF8String], [[_hostOptions objectForKey:key] UTF8String]));
    }
  }
  map.insert(std::make_pair("network", [[LynxDeviceInfoHelper getNetworkType] UTF8String]));
  map.insert(std::make_pair("deviceModel", [[LynxDeviceInfoHelper getDeviceModel] UTF8String]));
  map.insert(std::make_pair("osVersion", [[LynxDeviceInfoHelper getSystemVersion] UTF8String]));
  map.insert(std::make_pair("sdkVersion", [[LynxDeviceInfoHelper getLynxVersion] UTF8String]));

  return map;
}

- (void)sendDebugStateEvent {
  if (self.debugState) {
    NSMutableArray *args = [[NSMutableArray alloc] init];
    [args addObject:[NSString stringWithFormat:@"%@", self.debugState]];
    [monitor_view_ sendGlobalEvent:@"debugState" withParams:args];
  }
}

- (BOOL)hasSetOpenCardCallback {
  return has_set_open_card_callback_;
}

- (void)setOpenCardCallback:(LynxDebugBridgeOpenCardCallback)callback {
  [self addOpenCardCallback:callback];
}

- (void)addOpenCardCallback:(LynxDebugBridgeOpenCardCallback)callback {
  has_set_open_card_callback_ = YES;
  if (![open_card_callbacks containsObject:callback]) {
    [open_card_callbacks addObject:callback];
  }
}

- (void)openCard:(NSString *)url {
  LLogInfo(@"openCard: %@", url);
  for (LynxDebugBridgeOpenCardCallback callback in open_card_callbacks) {
    callback(url);
  }
}

- (void)onMessage:(NSString *)message withType:(NSString *)type {
  if ([type isEqualToString:@"D2RStopAtEntry"]) {
    bool stop = [message isEqualToString:@"true"];
    lynx::devtool::DevToolConfig::SetStopAtEntry(stop);
    [[DebugRouter instance] sendDataAsync:message WithType:@"R2DStopAtEntry" ForSession:-1];
  } else if ([type isEqualToString:@"D2RStopLepusAtEntry"]) {
#if OS_IOS
    bool stop = [message isEqualToString:@"true"];
    lynx::devtool::DevToolConfig::SetStopAtEntry(stop, true);
    [[DebugRouter instance] sendDataAsync:message WithType:@"R2DStopLepusAtEntry" ForSession:-1];
#endif
  } else if ([type isEqualToString:@"SetGlobalSwitch"]) {
    NSData *messageObj = [message dataUsingEncoding:NSUTF8StringEncoding];
    NSDictionary *messageDict =
        [NSJSONSerialization JSONObjectWithData:messageObj
                                        options:NSJSONReadingMutableContainers
                                          error:0];
    BOOL globalValue = [[messageDict objectForKey:@"global_value"] boolValue];
    [LynxDevtoolEnv.sharedInstance set:globalValue forKey:messageDict[@"global_key"]];
    [[DebugRouter instance] sendDataAsync:message WithType:@"SetGlobalSwitch" ForSession:-1];
  } else if ([type isEqualToString:@"GetGlobalSwitch"]) {
    NSData *messageObj = [message dataUsingEncoding:NSUTF8StringEncoding];
    NSDictionary *messageDict =
        [NSJSONSerialization JSONObjectWithData:messageObj
                                        options:NSJSONReadingMutableContainers
                                          error:0];
    NSString *key = messageDict[@"global_key"];
    BOOL result =
        [LynxDevtoolEnv.sharedInstance get:key
                          withDefaultValue:[LynxDevtoolEnv.sharedInstance getDefaultValue:key]];
    [[DebugRouter instance] sendDataAsync:((result) ? @"true" : @"false")
                                 WithType:@"GetGlobalSwitch"
                               ForSession:-1];
  }
}

- (void)setAppInfo:(NSDictionary *)hostOptions {
  for (NSString *key in hostOptions) {
    [[DebugRouter instance] setAppInfo:key withValue:hostOptions[key]];
  }
  [[DebugRouter instance] setAppInfo:@"network" withValue:[LynxDeviceInfoHelper getNetworkType]];
  [[DebugRouter instance] setAppInfo:@"deviceModel"
                           withValue:[LynxDeviceInfoHelper getDeviceModel]];
  [[DebugRouter instance] setAppInfo:@"osVersion"
                           withValue:[LynxDeviceInfoHelper getSystemVersion]];
  [[DebugRouter instance] setAppInfo:@"sdkVersion" withValue:[LynxDeviceInfoHelper getLynxVersion]];
}

- (void)onClose:(int32_t)code withReason:(NSString *)reason {
  if (agent_dispatcher_) {
    [agent_dispatcher_ enableTraceMode:false];
  }
}

- (void)onError:(NSString *)error_msg {
  if (agent_dispatcher_) {
    [agent_dispatcher_ enableTraceMode:false];
  }
}

- (void)onMessage:(NSString *)message {
  // TODO(tanxuelian.rovic): to be removed
}

- (void)onOpen:(ConnectionType)type {
  [[LynxDevtoolEnv sharedInstance] set:YES forKey:SP_KEY_DEVTOOL_CONNECTED];
}

- (void)onPerfMetricsEvent:(NSString *)eventName
                  withData:(NSDictionary<NSString *, NSObject *> *)data
                instanceId:(int32_t)instanceId {
  if (LynxDevtoolEnv.sharedInstance.perfMetricsEnabled && agent_dispatcher_) {
    NSMutableDictionary *dataM = [data mutableCopy];
    [dataM setValue:[NSNumber numberWithInt:instanceId] forKey:@"instanceId"];
    [agent_dispatcher_ onPerfMetricsEvent:eventName withData:[dataM copy]];
  }
}

- (void)onReportEvent:(nonnull NSString *)eventName
           instanceId:(NSInteger)instanceId
                props:(NSDictionary *_Nullable)props
            extraData:(NSDictionary *_Nullable)extraData {
  [self onPerfMetricsEvent:eventName withData:props instanceId:(int32_t)instanceId];
}

@end
