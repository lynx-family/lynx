// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_INPUT_AGENT_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_INPUT_AGENT_H_

#include <memory>
#include <unordered_map>

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {

class InspectorManager;

class InspectorInputAgent : public CDPDomainAgentBase {
 public:
  InspectorInputAgent(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator);
  virtual ~InspectorInputAgent();
  void CallMethod(const std::shared_ptr<MessageSender>& sender,
                  const Json::Value& message) override;

 private:
  typedef void (InspectorInputAgent::*InputAgentMethod)(
      const std::shared_ptr<MessageSender>& sender, const Json::Value& params);

  void EmulateTouchFromMouseEvent(const std::shared_ptr<MessageSender>& sender,
                                  const Json::Value& message);

  std::map<std::string, InputAgentMethod> functions_map_;
  const std::shared_ptr<LynxDevToolMediator> devtool_mediator_;
};
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_INPUT_AGENT_H_
