// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_LOG_AGENT_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_LOG_AGENT_H_

#include <map>
#include <memory>

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/lynx_devtool/agent/agent_defines.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {

class InspectorLogAgent : public CDPDomainAgentBase {
 public:
  explicit InspectorLogAgent(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator);
  virtual ~InspectorLogAgent();
  void CallMethod(const std::shared_ptr<CDPResponder>& responder,
                  const Json::Value& message) override;

 private:
  using LogAgentMethod = void (InspectorLogAgent::*)(
      const std::shared_ptr<CDPResponder>& responder,
      const Json::Value& params);

  DECLARE_DEVTOOL_CDP_METHOD(Enable);
  DECLARE_DEVTOOL_CDP_METHOD(Disable);
  DECLARE_DEVTOOL_CDP_METHOD(Clear);

  std::map<std::string, LogAgentMethod> functions_map_;
  const std::shared_ptr<LynxDevToolMediator> devtool_mediator_;
};
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_LOG_AGENT_H_
