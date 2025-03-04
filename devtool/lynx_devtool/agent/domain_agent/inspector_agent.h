// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_AGENT_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_AGENT_H_

#include <memory>
#include <unordered_map>

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {
class InspectorAgent : public CDPDomainAgentBase {
 public:
  explicit InspectorAgent(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator);
  virtual ~InspectorAgent();
  void CallMethod(const std::shared_ptr<MessageSender>& sender,
                  const Json::Value& message) override;

 private:
  typedef void (InspectorAgent::*InspectorAgentMethod)(
      const std::shared_ptr<MessageSender>& sender, const Json::Value& message);

  void Enable(const std::shared_ptr<MessageSender>& sender,
              const Json::Value& message);
  void Detached(const std::shared_ptr<MessageSender>& sender,
                const Json::Value& message);

  std::map<std::string, InspectorAgentMethod> functions_map_;
  std::shared_ptr<LynxDevToolMediator> devtool_mediator_;
};
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_AGENT_H_
