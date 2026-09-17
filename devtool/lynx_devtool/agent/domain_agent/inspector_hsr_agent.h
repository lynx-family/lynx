// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_HSR_AGENT_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_HSR_AGENT_H_

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/lynx_devtool/agent/agent_defines.h"
#include "devtool/lynx_devtool/agent/global_devtool_platform_facade.h"

namespace lynx {
namespace devtool {

class InspectorHSRAgent : public CDPDomainAgentBase {
 public:
  InspectorHSRAgent();
  // Injectable platform boundary for protocol tests; must outlive the agent.
  explicit InspectorHSRAgent(GlobalDevToolPlatformFacade& facade);

  void CallMethod(const std::shared_ptr<CDPResponder>& responder,
                  const Json::Value& message) override;

 private:
  void Execute(const std::shared_ptr<CDPResponder>& responder,
               HSRScriptRequest&& request);

  GlobalDevToolPlatformFacade& facade_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_HSR_AGENT_H_
