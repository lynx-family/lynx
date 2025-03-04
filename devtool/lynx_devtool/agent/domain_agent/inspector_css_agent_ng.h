// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_CSS_AGENT_NG_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_CSS_AGENT_NG_H_

#include <memory>
#include <set>
#include <unordered_map>

#include "core/renderer/dom/element.h"
#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {

class DevToolAgentNG;

class InspectorCSSAgentNG : public CDPDomainAgentBase {
 public:
  explicit InspectorCSSAgentNG(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator);
  virtual ~InspectorCSSAgentNG();
  virtual void CallMethod(const std::shared_ptr<MessageSender>& sender,
                          const Json::Value& message) override;

 private:
  typedef void (InspectorCSSAgentNG::*CSSAgentMethod)(
      const std::shared_ptr<MessageSender>& sender, const Json::Value& message);

  void Enable(const std::shared_ptr<MessageSender>& sender,
              const Json::Value& message);
  void Disable(const std::shared_ptr<MessageSender>& sender,
               const Json::Value& message);
  void GetMatchedStylesForNode(const std::shared_ptr<MessageSender>& sender,
                               const Json::Value& message);
  void GetComputedStyleForNode(const std::shared_ptr<MessageSender>& sender,
                               const Json::Value& message);
  void GetInlineStylesForNode(const std::shared_ptr<MessageSender>& sender,
                              const Json::Value& message);
  void SetStyleTexts(const std::shared_ptr<MessageSender>& sender,
                     const Json::Value& message);
  void GetStyleSheetText(const std::shared_ptr<MessageSender>& sender,
                         const Json::Value& message);
  void GetBackgroundColors(const std::shared_ptr<MessageSender>& sender,
                           const Json::Value& message);
  void DispatchMessage(const std::shared_ptr<MessageSender>& sender,
                       lynx::tasm::Element* ptr, const std::string& sheet_id);
  void SetStyleSheetText(const std::shared_ptr<MessageSender>& sender,
                         const Json::Value& message);
  void CreateStyleSheet(const std::shared_ptr<MessageSender>& sender,
                        const Json::Value& message);
  void AddRule(const std::shared_ptr<MessageSender>& sender,
               const Json::Value& message);
  void StartRuleUsageTracking(const std::shared_ptr<MessageSender>& sender,
                              const Json::Value& message);
  void UpdateRuleUsageTracking(const std::shared_ptr<MessageSender>& sender,
                               const Json::Value& message);
  void StopRuleUsageTracking(const std::shared_ptr<MessageSender>& sender,
                             const Json::Value& message);
  void CollectDomTreeCssUsage(const std::shared_ptr<MessageSender>& sender,
                              Json::Value& rule_usage_array,
                              const std::string& stylesheet_id,
                              const std::string& content);
  Json::Value GetUsageItem(const std::string& stylesheet_id,
                           const std::string& content,
                           const std::string& selector);

  std::map<std::string, CSSAgentMethod> functions_map_;

  std::set<std::string> css_used_selector_;
  bool rule_usage_tracking_;
  const std::shared_ptr<LynxDevToolMediator> devtool_mediator_;
};
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DOMAIN_AGENT_INSPECTOR_CSS_AGENT_NG_H_
