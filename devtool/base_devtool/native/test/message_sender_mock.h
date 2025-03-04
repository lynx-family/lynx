// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_TEST_MESSAGE_SENDER_MOCK_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_TEST_MESSAGE_SENDER_MOCK_H_

#include <string>
#include <utility>

#include "devtool/base_devtool/native/public/message_sender.h"
#include "devtool/base_devtool/native/test/mock_receiver.h"

namespace lynx {
namespace devtool {

class MessageSenderMock : public MessageSender {
 public:
  virtual ~MessageSenderMock() = default;
  void SendMessage(const std::string& type, const Json::Value& msg) override {
    MockReceiver::GetInstance().OnMessage(type, msg.toStyledString());
  }
  void SendMessage(const std::string& type, const std::string& msg) override {
    MockReceiver::GetInstance().OnMessage(type, msg);
  }
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_TEST_MESSAGE_SENDER_MOCK_H_
