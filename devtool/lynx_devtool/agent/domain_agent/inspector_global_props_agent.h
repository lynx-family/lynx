// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_GLOBAL_PROPS_AGENT_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_GLOBAL_PROPS_AGENT_H_

#include <memory>
#include <unordered_map>

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {

class InspectorGlobalPropsAgent : public CDPDomainAgentBase {
 public:
  explicit InspectorGlobalPropsAgent(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator);
  ~InspectorGlobalPropsAgent() override = default;

  void CallMethod(const std::shared_ptr<MessageSender>& sender,
                  const Json::Value& message) override;

 private:
  using GlobalPropsAgentMethod = void (InspectorGlobalPropsAgent::*)(
      const std::shared_ptr<MessageSender>& sender, const Json::Value& message);

  void Enable(const std::shared_ptr<MessageSender>& sender,
              const Json::Value& message);
  void Disable(const std::shared_ptr<MessageSender>& sender,
               const Json::Value& message);
  void Get(const std::shared_ptr<MessageSender>& sender,
           const Json::Value& message);
  void Replace(const std::shared_ptr<MessageSender>& sender,
               const Json::Value& message);

  std::unordered_map<std::string, GlobalPropsAgentMethod> functions_map_;
  std::shared_ptr<LynxDevToolMediator> devtool_mediator_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_GLOBAL_PROPS_AGENT_H_
