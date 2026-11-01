// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_CSS_AGENT_NG_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_CSS_AGENT_NG_H_

#include <memory>
#include <unordered_map>

#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/lynx_devtool/agent/agent_defines.h"

namespace lynx {
namespace devtool {

class LynxDevToolMediator;

class InspectorCSSAgentNG : public CDPDomainAgentBase {
 public:
  explicit InspectorCSSAgentNG(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator);
  ~InspectorCSSAgentNG() override;
  void CallMethod(const std::shared_ptr<CDPResponder>& responder,
                  const Json::Value& message) override;

 private:
  using CSSAgentMethod = void (InspectorCSSAgentNG::*)(
      const std::shared_ptr<CDPResponder>& responder,
      const Json::Value& params);

  DECLARE_DEVTOOL_CDP_METHOD(Enable);
  DECLARE_DEVTOOL_CDP_METHOD(Disable);
  DECLARE_DEVTOOL_CDP_METHOD(GetMatchedStylesForNode);
  DECLARE_DEVTOOL_CDP_METHOD(GetLayersForNode);
  DECLARE_DEVTOOL_CDP_METHOD(GetComputedStyleForNode);
  DECLARE_DEVTOOL_CDP_METHOD(GetInlineStylesForNode);
  DECLARE_DEVTOOL_CDP_METHOD(SetStyleTexts);
  DECLARE_DEVTOOL_CDP_METHOD(GetStyleSheetText);
  DECLARE_DEVTOOL_CDP_METHOD(GetBackgroundColors);
  DECLARE_DEVTOOL_CDP_METHOD(SetStyleSheetText);
  DECLARE_DEVTOOL_CDP_METHOD(CreateStyleSheet);
  DECLARE_DEVTOOL_CDP_METHOD(AddRule);
  DECLARE_DEVTOOL_CDP_METHOD(StartRuleUsageTracking);
  DECLARE_DEVTOOL_CDP_METHOD(UpdateRuleUsageTracking);
  DECLARE_DEVTOOL_CDP_METHOD(StopRuleUsageTracking);
  DECLARE_DEVTOOL_CDP_METHOD(GetMediaQueries);
  DECLARE_DEVTOOL_CDP_METHOD(SetMediaText);
  DECLARE_DEVTOOL_CDP_METHOD(SetSupportsText);

  std::unordered_map<std::string, CSSAgentMethod> functions_map_;
  const std::shared_ptr<LynxDevToolMediator> devtool_mediator_;
};
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_CSS_AGENT_NG_H_
