// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_TEMPLATE_AGENT_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_TEMPLATE_AGENT_H_

#include <map>
#include <memory>
#include <string>

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/lynx_devtool/agent/agent_defines.h"

namespace lynx {
namespace devtool {

class LynxDevToolMediator;

class InspectorTemplateAgent : public CDPDomainAgentBase {
 public:
  explicit InspectorTemplateAgent(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator);
  ~InspectorTemplateAgent() override = default;
  void CallMethod(const std::shared_ptr<CDPResponder>& responder,
                  const Json::Value& message) override;

 private:
  using TemplateAgentMethod = void (InspectorTemplateAgent::*)(
      const std::shared_ptr<CDPResponder>& responder,
      const Json::Value& params);

  DECLARE_DEVTOOL_CDP_METHOD(GetTemplateData);
  DECLARE_DEVTOOL_CDP_METHOD(GetTemplateConfigInfo);
  DECLARE_DEVTOOL_CDP_METHOD(GetTemplateApiInfo);
  DECLARE_DEVTOOL_CDP_METHOD(GetTemplateJsInfo);

  std::map<std::string, TemplateAgentMethod> functions_map_;
  const std::shared_ptr<LynxDevToolMediator> devtool_mediator_;
};

}  // namespace devtool
}  // namespace lynx
#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_TEMPLATE_AGENT_H_
