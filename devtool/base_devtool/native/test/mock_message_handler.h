// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_TEST_MOCK_MESSAGE_HANDLER_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_TEST_MOCK_MESSAGE_HANDLER_H_

#include <memory>
#include <string>

#include "devtool/base_devtool/native/public/devtool_message_handler.h"
#include "devtool/base_devtool/native/test/mock_receiver.h"
#include "third_party/jsoncpp/include/json/value.h"

namespace lynx {
namespace devtool {

class MockMessageHandler : public DevToolMessageHandler {
 public:
  MockMessageHandler() = default;
  ~MockMessageHandler() override = default;

  void handle(const std::shared_ptr<MessageSender>& sender,
              const std::string& type, const Json::Value& message) override {
    Json::Value response;
    response["type"] = type;
    response["handler"] = "MockMessageHandler";
    response["message"] = message;
    sender->SendMessage(type, response);
  }
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_TEST_MOCK_MESSAGE_HANDLER_H_
