// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_COMPONENT_AGENT_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_COMPONENT_AGENT_H_

#include <map>
#include <memory>

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/lynx_devtool/agent/agent_defines.h"

namespace lynx {
namespace devtool {

class LynxGlobalDevToolMediator;

class InspectorComponentAgent : public CDPDomainAgentBase {
 public:
  InspectorComponentAgent();
  explicit InspectorComponentAgent(
      const LynxGlobalDevToolMediator& global_devtool_mediator);
  ~InspectorComponentAgent() override = default;
  void CallMethod(const std::shared_ptr<CDPResponder>& responder,
                  const Json::Value& message) override;

 private:
  using ComponentAgentMethod = void (InspectorComponentAgent::*)(
      const std::shared_ptr<CDPResponder>& responder,
      const Json::Value& params);

  DECLARE_DEVTOOL_CDP_METHOD(UselessUpdate);

  std::map<std::string, ComponentAgentMethod> functions_map_;
  const LynxGlobalDevToolMediator& global_devtool_mediator_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_COMPONENT_AGENT_H_
