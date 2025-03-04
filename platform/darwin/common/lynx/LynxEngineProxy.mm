// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxEngineProxy+Native.h"
#import "LynxEvent.h"
#import "LynxTemplateData+Converter.h"
#import "LynxTouchEvent.h"

#include "core/shell/ios/lynx_engine_proxy_darwin.h"
#include "core/value_wrapper/value_impl_lepus.h"

@interface LynxEngineProxy () {
  std::shared_ptr<lynx::shell::LynxEngineProxyDarwin> native_engine_proxy_;
}

@end

@implementation LynxEngineProxy

- (instancetype)init {
  if (self = [super init]) {
    native_engine_proxy_ = nullptr;
  }
  return self;
}

- (void)setNativeEngineProxy:(std::shared_ptr<lynx::shell::LynxEngineProxyDarwin>)proxy {
  native_engine_proxy_ = proxy;
}

- (void)invokeLepusFunc:(NSDictionary *)data callbackID:(int32_t)callbackID {
  if (!native_engine_proxy_ || !data[@"entry_name"]) {
    return;
  }
  lynx::lepus::Value value = LynxConvertToLepusValue(data);
  native_engine_proxy_->InvokeLepusApiCallback(
      callbackID, std::string([data[@"entry_name"] UTF8String]), value);
}

- (void)sendSyncTouchEvent:(LynxTouchEvent *)event {
  if (native_engine_proxy_ && event) {
    native_engine_proxy_->SendTouchEvent([event.eventName UTF8String], (int)event.targetSign,
                                         event.viewPoint.x, event.viewPoint.y, event.clientPoint.x,
                                         event.clientPoint.y, event.pagePoint.x, event.pagePoint.y,
                                         (int64_t)(event.timestamp * 1000));
  }
}

- (void)sendSyncMultiTouchEvent:(LynxTouchEvent *)event {
  if (native_engine_proxy_ && event) {
    native_engine_proxy_->SendTouchEvent([event.eventName UTF8String],
                                         PubLepusValue(LynxConvertToLepusValue(event.uiTouchMap)),
                                         (int64_t)(event.timestamp * 1000));
  }
}

- (void)sendCustomEvent:(LynxCustomEvent *)event {
  if (native_engine_proxy_ && event) {
    native_engine_proxy_->SendCustomEvent([event.eventName UTF8String], (int)event.targetSign,
                                          PubLepusValue(LynxConvertToLepusValue(event.params)),
                                          [[event paramsName] UTF8String]);
  }
}

- (void)sendGestureEvent:(int)gestureId event:(LynxCustomEvent *)event {
  if (native_engine_proxy_ && event) {
    native_engine_proxy_->SendGestureEvent((int)event.targetSign, gestureId,
                                           [event.eventName UTF8String],
                                           PubLepusValue(LynxConvertToLepusValue(event.params)));
  }
}

- (void)onPseudoStatusChanged:(int32_t)tag
                fromPreStatus:(int32_t)preStatus
              toCurrentStatus:(int32_t)currentStatus {
  if (native_engine_proxy_ != nullptr) {
    native_engine_proxy_->OnPseudoStatusChanged(tag, preStatus, currentStatus);
  }
}

@end
