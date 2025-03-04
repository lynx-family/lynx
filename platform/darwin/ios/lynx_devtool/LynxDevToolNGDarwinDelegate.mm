// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxDevToolNGDarwinDelegate.h"
#import <Lynx/LynxBackgroundRuntime+Internal.h>
#include <cstddef>

#include <memory>

#include "devtool/base_devtool/native/public/message_sender.h"
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

@implementation LynxDevToolNGDarwinDelegate {
  int session_id_;
  std::shared_ptr<lynx::devtool::LynxDevToolNG> devtool_ng_;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    session_id_ = 0;
    devtool_ng_ = std::make_shared<lynx::devtool::LynxDevToolNG>();
  }
  return self;
}

- (int)getSessionId {
  return session_id_;
}

- (bool)isAttachToDebugRouter {
  return session_id_ != 0;
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

@end
