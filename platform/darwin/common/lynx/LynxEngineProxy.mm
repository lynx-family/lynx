// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxEvent.h>
#import <Lynx/LynxTemplateData+Converter.h>
#import <Lynx/LynxThreadManager.h>
#import <Lynx/LynxTouchEvent.h>
#import "LynxEngineProxy+Native.h"

#include "core/renderer/dom/ios/lepus_value_converter.h"
#include "core/shell/ios/lynx_engine_proxy_darwin.h"
#include "core/value_wrapper/value_impl_lepus.h"

#include <utility>

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

- (const std::shared_ptr<lynx::shell::LynxEngineProxyDarwin> &)nativeProxy {
  return native_engine_proxy_;
}

- (void)reportExternalMemoryWithTotalSize:(int64_t)totalSize garbageSize:(int64_t)garbageSize {
  if (native_engine_proxy_) {
    native_engine_proxy_->ReportExternalMemory({totalSize, garbageSize});
  }
}

- (void)dispatchTaskToLynxEngine:(dispatch_block_t)task {
  if (native_engine_proxy_ && task) {
    native_engine_proxy_->DispatchTaskToLynxEngine([task]() { task(); });
  }
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

- (void)startEventGenerate:(LynxTouchEvent *)event {
  if (native_engine_proxy_) {
    native_engine_proxy_->StartEventGenerate(
        PubLepusValue(LynxConvertToLepusValue([event getEventParams])));
  }
}

- (void)startEventCapture:(int64_t)eventID {
  if (native_engine_proxy_) {
    native_engine_proxy_->StartEventCapture(eventID);
  }
}

- (void)startEventBubble:(int64_t)eventID {
  if (native_engine_proxy_) {
    native_engine_proxy_->StartEventBubble(eventID);
  }
}

- (void)startEventFire:(BOOL)isStop withEventID:(int64_t)eventID {
  if (native_engine_proxy_) {
    native_engine_proxy_->StartEventFire(isStop, eventID);
  }
}

- (void)queryLynxElementRoot:(LynxEngineProxyQueryCallback)callback {
  [self queryLynxElement:0 queryType:0 argument:nil callback:callback];
}

- (void)queryLynxElement:(int32_t)sign
               queryType:(int32_t)queryType
                argument:(NSString *)argument
                callback:(LynxEngineProxyQueryCallback)callback {
  if (!callback) {
    return;
  }
  if (!native_engine_proxy_) {
    [LynxThreadManager runBlockInMainQueueImmediately:^{
      callback(nil);
    }];
    return;
  }
  std::string native_argument = argument ? std::string([argument UTF8String]) : std::string();
  native_engine_proxy_->QueryLynxElement(
      sign, queryType, std::move(native_argument), [callback](lynx::lepus::Value value) {
        id result = value.IsNil() ? nil : lynx::tasm::convertLepusValueToNSObject(value);
        [LynxThreadManager runBlockInMainQueueImmediately:^{
          callback(result);
        }];
      });
}

@end
