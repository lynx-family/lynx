// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_TEST_MOCK_BASE_AGENT_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_TEST_MOCK_BASE_AGENT_H_

#include <json/value.h>

#include <memory>

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/base_devtool/native/test/mock_receiver.h"

namespace lynx {
namespace devtool {

class MockBaseAgent : public lynx::devtool::CDPDomainAgentBase {
 public:
  MockBaseAgent() = default;
  void CallMethod(const std::shared_ptr<lynx::devtool::MessageSender>& sender,
                  const Json::Value& msg) override {
    MockReceiver::GetInstance().OnMethodCalled(msg.toStyledString());
  }
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_TEST_MOCK_BASE_AGENT_H_
