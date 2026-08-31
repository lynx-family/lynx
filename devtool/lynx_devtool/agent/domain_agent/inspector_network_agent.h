// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_NETWORK_AGENT_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_NETWORK_AGENT_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {

class InspectorNetworkAgent : public CDPDomainAgentBase {
 public:
  explicit InspectorNetworkAgent(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator);
  ~InspectorNetworkAgent() override = default;

  void CallMethod(const std::shared_ptr<MessageSender>& sender,
                  const Json::Value& message) override;

 private:
  using NetworkAgentMethod = void (InspectorNetworkAgent::*)(
      const std::shared_ptr<MessageSender>& sender, const Json::Value& message);

  void Enable(const std::shared_ptr<MessageSender>& sender,
              const Json::Value& message);
  void Disable(const std::shared_ptr<MessageSender>& sender,
               const Json::Value& message);
  void GetResponseBody(const std::shared_ptr<MessageSender>& sender,
                       const Json::Value& message);
  void GetRequestPostData(const std::shared_ptr<MessageSender>& sender,
                          const Json::Value& message);

  std::unordered_map<std::string, NetworkAgentMethod> functions_map_;
  const std::shared_ptr<LynxDevToolMediator> devtool_mediator_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_NETWORK_AGENT_H_
