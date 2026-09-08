// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBackgroundRuntime+Internal.h>
#import <LynxDevtool/LynxDevToolNGDarwinDelegate.h>
#include <cstddef>

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "devtool/base_devtool/native/public/message_sender.h"
#include "devtool/lynx_devtool/agent/network_request_observer.h"
#include "devtool/lynx_devtool/js_debug/lepus/manager/rts_inspector_manager_factory.h"
#include "devtool/lynx_devtool/lynx_devtool_ng.h"

#pragma mark - LynxDevToolNGDarwinDelegate

namespace lynx {
namespace devtool {
class InvokeCDPFromSDKSenderIos : public MessageSender {
 public:
  InvokeCDPFromSDKSenderIos(CDPResultCallback callback) { _callback = callback; }

  void SendMessage(const std::string& type, const Json::Value& msg) override {
    std::string msg_str = msg.toStyledString();
    _callback([NSString stringWithUTF8String:msg_str.c_str()]);
  }

  void SendMessage(const std::string& type, const std::string& msg) override {
    _callback([NSString stringWithUTF8String:msg.c_str()]);
  }

 private:
  __strong CDPResultCallback _callback;
};

class CDPEventListenerSender : public MessageSender {
 public:
  CDPEventListenerSender(id<CDPEventListener> listener) { _listener = listener; }
  void SendMessage(const std::string& type, const Json::Value& msg) override {
    std::string msg_str = msg.toStyledString();
    __strong id<CDPEventListener> listener = _listener;
    [listener onEvent:[NSString stringWithUTF8String:msg_str.c_str()]];
  }

  void SendMessage(const std::string& type, const std::string& msg) override {
    __strong id<CDPEventListener> listener = _listener;
    [listener onEvent:[NSString stringWithUTF8String:msg.c_str()]];
  }

 private:
  __weak id<CDPEventListener> _listener;
};

class DevToolMessageHandlerIos : public DevToolMessageHandler {
 public:
  DevToolMessageHandlerIos(id<MessageHandler> handler) { _handler = handler; }
  void handle(const std::shared_ptr<MessageSender>& sender, const std::string& type,
              const Json::Value& message) override {
    __strong typeof(_handler) handler = _handler;
    std::string message_str = message.toStyledString();
    [handler onMessage:[NSString stringWithUTF8String:message_str.c_str()]];
  }

 private:
  __weak id<MessageHandler> _handler;
};

}  // namespace devtool
}  // namespace lynx

namespace {

std::string ConvertNSString(NSString* value) {
  if (value == nil) {
    return {};
  }
  const char* utf8_value = value.UTF8String;
  return utf8_value != nullptr ? std::string(utf8_value) : std::string();
}

std::map<std::string, std::string> ConvertHeaders(NSDictionary* headers) {
  std::map<std::string, std::string> result;
  for (id key in headers) {
    id value = headers[key];
    NSString* key_string = [key isKindOfClass:NSString.class] ? key : [key description];
    NSString* value_string = [value isKindOfClass:NSString.class] ? value : [value description];
    result.emplace(ConvertNSString(key_string), ConvertNSString(value_string));
  }
  return result;
}

std::vector<uint8_t> ConvertNSData(NSData* data) {
  if (data.length == 0) {
    return {};
  }
  const auto* bytes = static_cast<const uint8_t*>(data.bytes);
  return std::vector<uint8_t>(bytes, bytes + data.length);
}

std::shared_ptr<lynx::devtool::NetworkRequestObserver> GetNetworkObserver(
    const std::shared_ptr<lynx::devtool::LynxDevToolNG>& devtool) {
  return devtool != nullptr ? devtool->GetNetworkRequestObserver().lock() : nullptr;
}

}  // namespace

@implementation LynxDevToolNGDarwinDelegate {
  int session_id_;
  std::shared_ptr<lynx::devtool::LynxDevToolNG> devtool_ng_;
}

- (instancetype)initWithDebuggable:(BOOL)debuggable {
  self = [super init];
  if (self) {
    session_id_ = 0;
    // Anchor the RTS inspector factory in iOS static frameworks so its pure
    // C++ registration object is not dropped by the linker.
    auto* anchor = &LynxRegisterRTSInspectorManagerFactoryImpl;
    (void)anchor;
    devtool_ng_ = std::make_shared<lynx::devtool::LynxDevToolNG>(static_cast<bool>(debuggable));
  }
  return self;
}

- (int)getSessionId {
  return session_id_;
}

- (bool)isAttachToDebugRouter {
  return session_id_ != 0;
}

- (BOOL)isEnabled {
  auto observer = GetNetworkObserver(devtool_ng_);
  return observer != nullptr && observer->IsEnabled();
}

- (NSString*)requestWillBeSent:(nullable NSString*)url
                        method:(nullable NSString*)method
                       headers:(nullable NSDictionary*)headers
                          body:(nullable NSData*)body {
  auto observer = GetNetworkObserver(devtool_ng_);
  if (observer == nullptr || !observer->IsEnabled()) {
    return @"";
  }
  lynx::devtool::NetworkRequestInfo request;
  request.url = ConvertNSString(url);
  request.method = ConvertNSString(method);
  request.headers = ConvertHeaders(headers);
  request.body = ConvertNSData(body);
  const std::string request_id = observer->RequestWillBeSent(std::move(request));
  return [NSString stringWithUTF8String:request_id.c_str()];
}

- (void)responseReceived:(NSString*)requestId
                     url:(nullable NSString*)url
                  status:(NSInteger)status
              statusText:(nullable NSString*)statusText
                 headers:(nullable NSDictionary*)headers {
  auto observer = GetNetworkObserver(devtool_ng_);
  if (observer == nullptr || !observer->IsEnabled()) {
    return;
  }
  lynx::devtool::NetworkResponseInfo response;
  response.url = ConvertNSString(url);
  response.status = static_cast<int>(status);
  response.status_text = ConvertNSString(statusText);
  response.headers = ConvertHeaders(headers);
  observer->ResponseReceived(ConvertNSString(requestId), std::move(response));
}

- (void)dataReceived:(NSString*)requestId data:(nullable NSData*)data {
  auto observer = GetNetworkObserver(devtool_ng_);
  if (observer == nullptr || !observer->IsEnabled()) {
    return;
  }
  observer->DataReceived(ConvertNSString(requestId), ConvertNSData(data));
}

- (void)loadingFinished:(NSString*)requestId {
  auto observer = GetNetworkObserver(devtool_ng_);
  if (observer == nullptr || !observer->IsEnabled()) {
    return;
  }
  observer->LoadingFinished(ConvertNSString(requestId));
}

- (void)loadingFailed:(NSString*)requestId
            errorText:(nullable NSString*)errorText
             canceled:(BOOL)canceled {
  auto observer = GetNetworkObserver(devtool_ng_);
  if (observer == nullptr || !observer->IsEnabled()) {
    return;
  }
  observer->LoadingFailed(ConvertNSString(requestId), ConvertNSString(errorText),
                          static_cast<bool>(canceled));
}

- (void)onBackgroundRuntimeCreated:(LynxBackgroundRuntime*)runtime
                   groupThreadName:(NSString*)groupThreadName {
  if (devtool_ng_ != nullptr && groupThreadName != nil) {
    [runtime
        setRuntimeObserver:devtool_ng_->OnBackgroundRuntimeCreated([groupThreadName UTF8String])];
  }
}

- (void)onTemplateAssemblerCreated:(intptr_t)ptr {
  if (devtool_ng_ != nullptr) {
    devtool_ng_->OnTasmCreated(ptr);
  }
}

- (void)onMTSRuntimeCreated:(intptr_t)devtool_pool_ptr {
  if (devtool_ng_ != nullptr) {
    devtool_ng_->OnMTSRuntimeCreated(devtool_pool_ptr);
  }
}

- (int)attachToDebug:(NSString*)url {
  if (devtool_ng_ != nullptr && url != nil) {
    session_id_ = devtool_ng_->Attach([url UTF8String]);
    return session_id_;
  }
  return 0;
}

- (void)detachToDebug {
  if (devtool_ng_ != nullptr) {
    devtool_ng_->Detach();
    session_id_ = 0;
  }
}

- (void)setDevToolPlatformAbility:
    (std::shared_ptr<lynx::devtool::DevToolPlatformFacade>)devtool_platform_facade {
  if (devtool_ng_ != nullptr) {
    devtool_ng_->SetDevToolPlatformFacade(devtool_platform_facade);
  }
}

- (void)sendMessageToDebugPlatform:(NSString*)msg withType:(NSString*)type {
  if (devtool_ng_ != nullptr && msg != nil && type != nil) {
    devtool_ng_->SendMessageToDebugPlatform([type UTF8String], [msg UTF8String]);
  }
}

- (void)invokeCDPFromSDK:(NSString*)msg withCallback:(CDPResultCallback)callback {
  if (devtool_ng_ != nullptr && msg != nil) {
    devtool_ng_->DispatchMessage(
        std::make_shared<lynx::devtool::InvokeCDPFromSDKSenderIos>(callback), "CDP",
        [msg UTF8String]);
  } else {
    LOGE("LynxDevToolNGDarwinDelegate "
         << "invokeCDPFromSDK failed with msg:" << msg);
  }
}

- (void)addCDPEventListener:(nonnull NSString*)name
               withListener:(nonnull id<CDPEventListener>)listener {
  if (devtool_ng_ != nullptr) {
    devtool_ng_->AddCDPEventListener(
        [name UTF8String], std::make_shared<lynx::devtool::CDPEventListenerSender>(listener));
  }
}

- (void)removeCDPEventListener:(nonnull NSString*)name {
  if (devtool_ng_ != nullptr) {
    devtool_ng_->RemoveCDPEventListener([name UTF8String]);
  }
}

- (void)subscribeMessage:(NSString*)type withHandler:(id<MessageHandler>)handler {
  if (devtool_ng_ != nullptr && type != nil) {
    devtool_ng_->SubscribeMessage(
        [type UTF8String], std::make_unique<lynx::devtool::DevToolMessageHandlerIos>(handler));
  } else {
    LOGE("LynxDevToolNGDarwinDelegate "
         << "subscribeMessage failed with type:" << type);
  }
}

- (void)unsubscribeMessage:(NSString*)type {
  if (devtool_ng_ != nullptr && type != nil) {
    devtool_ng_->UnSubscribeMessage([type UTF8String]);
  } else {
    LOGE("LynxDevToolNGDarwinDelegate "
         << "unsubscribeMessage failed with type:" << type);
  }
}

- (void)setTag:(NSString*)tag {
  if (devtool_ng_ != nullptr && tag != nil) {
    devtool_ng_->SetTag([tag UTF8String]);
  }
}

@end
