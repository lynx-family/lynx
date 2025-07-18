// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_TESTING_MOCK_CDP_EVENT_LISTENER_SENDER_MOCK_H_
#define DEVTOOL_TESTING_MOCK_CDP_EVENT_LISTENER_SENDER_MOCK_H_

#include <string>
#include <utility>

#include "devtool/base_devtool/native/public/message_sender.h"
#include "devtool/base_devtool/native/test/mock_receiver.h"

namespace lynx {
namespace devtool {

class CDPEventListenerSenderMock : public MessageSender {
 public:
  virtual ~CDPEventListenerSenderMock() = default;
  void SendMessage(const std::string& type, const Json::Value& msg) override {
    received_msg_ = std::make_pair(type, msg.toStyledString());
  }
  void SendMessage(const std::string& type, const std::string& msg) override {
    received_msg_ = std::make_pair(type, msg);
  }

  std::pair<std::string, std::string> received_msg_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_TESTING_MOCK_CDP_EVENT_LISTENER_SENDER_MOCK_H_
