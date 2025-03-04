// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_TESTBENCH_RECORDER_AGENT_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_TESTBENCH_RECORDER_AGENT_H_

#include <memory>
#include <unordered_map>

#include "base/include/log/logging.h"
#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"

namespace lynx {
namespace devtool {

class InspectorTestBenchRecorderAgent : public CDPDomainAgentBase {
 public:
  InspectorTestBenchRecorderAgent();
  ~InspectorTestBenchRecorderAgent() override = default;
  void CallMethod(const std::shared_ptr<MessageSender>& sender,
                  const Json::Value& message) override;

 private:
  using InspectorTestBenchRecorderAgentMethod = void (
      InspectorTestBenchRecorderAgent::*)(
      const std::shared_ptr<MessageSender>& sender, const Json::Value& message);
  void Start(const std::shared_ptr<MessageSender>& sender,
             const Json::Value& message);
  void End(const std::shared_ptr<MessageSender>& sender,
           const Json::Value& message);
  std::unordered_map<std::string, InspectorTestBenchRecorderAgentMethod>
      functions_map_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_TESTBENCH_RECORDER_AGENT_H_
