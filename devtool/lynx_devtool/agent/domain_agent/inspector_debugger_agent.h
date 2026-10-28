// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_DEBUGGER_AGENT_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_DEBUGGER_AGENT_H_

#include <unordered_map>

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {

class InspectorDebuggerAgent : public CDPDomainAgentBase {
 public:
  explicit InspectorDebuggerAgent(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator);
  ~InspectorDebuggerAgent() override = default;

  void CallMethod(const std::shared_ptr<CDPResponder>& responder,
                  const Json::Value& message) override;

 private:
  using DebuggerAgentMethod =
      void (InspectorDebuggerAgent::*)(const Json::Value& message);

  void Enable(const Json::Value& message);

  std::shared_ptr<LynxDevToolMediator> devtool_mediator_;
  std::unordered_map<std::string, DebuggerAgentMethod> functions_map_;
};
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_DEBUGGER_AGENT_H_
