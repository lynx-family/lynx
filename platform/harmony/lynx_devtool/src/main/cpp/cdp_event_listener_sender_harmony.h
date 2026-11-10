// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_CDP_EVENT_LISTENER_SENDER_HARMONY_H_
#define PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_CDP_EVENT_LISTENER_SENDER_HARMONY_H_

#include <node_api.h>

#include <memory>
#include <string>

#include "base/include/fml/task_runner.h"
#include "devtool/base_devtool/native/public/message_sender.h"

namespace lynx {
namespace devtool {

class CDPEventListenerSenderHarmony
    : public MessageSender,
      public std::enable_shared_from_this<CDPEventListenerSenderHarmony> {
 public:
  CDPEventListenerSenderHarmony(napi_env env, napi_value listener);
  ~CDPEventListenerSenderHarmony() override;

  void SendMessage(const std::string& type, const Json::Value& msg) override;
  void SendMessage(const std::string& type, const std::string& msg) override;

 private:
  void DispatchEvent(const std::string& event);

  napi_env env_;
  // The listener is held weakly so that the caller manages its lifetime.
  napi_ref listener_ref_;
  fml::RefPtr<fml::TaskRunner> ui_task_runner_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_CDP_EVENT_LISTENER_SENDER_HARMONY_H_
