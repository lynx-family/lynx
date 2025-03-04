// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_css_agent_ng.h"

#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

#if !defined(OS_WIN)
#include <unistd.h>
#endif

namespace lynx {
namespace devtool {

InspectorCSSAgentNG::InspectorCSSAgentNG(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {
  functions_map_["CSS.enable"] = &InspectorCSSAgentNG::Enable;
  functions_map_["CSS.disable"] = &InspectorCSSAgentNG::Disable;

  functions_map_["CSS.getMatchedStylesForNode"] =
      &InspectorCSSAgentNG::GetMatchedStylesForNode;
  functions_map_["CSS.getComputedStyleForNode"] =
      &InspectorCSSAgentNG::GetComputedStyleForNode;
  functions_map_["CSS.getInlineStylesForNode"] =
      &InspectorCSSAgentNG::GetInlineStylesForNode;
  functions_map_["CSS.setStyleTexts"] = &InspectorCSSAgentNG::SetStyleTexts;
  functions_map_["CSS.getBackgroundColors"] =
      &InspectorCSSAgentNG::GetBackgroundColors;
  functions_map_["CSS.getStyleSheetText"] =
      &InspectorCSSAgentNG::GetStyleSheetText;
  functions_map_["CSS.setStyleSheetText"] =
      &InspectorCSSAgentNG::SetStyleSheetText;
  functions_map_["CSS.createStyleSheet"] =
      &InspectorCSSAgentNG::CreateStyleSheet;
  functions_map_["CSS.addRule"] = &InspectorCSSAgentNG::AddRule;
  functions_map_["CSS.startRuleUsageTracking"] =
      &InspectorCSSAgentNG::StartRuleUsageTracking;
  functions_map_["CSS.updateRuleUsageTracking"] =
      &InspectorCSSAgentNG::UpdateRuleUsageTracking;
  functions_map_["CSS.stopRuleUsageTracking"] =
      &InspectorCSSAgentNG::StopRuleUsageTracking;
}

InspectorCSSAgentNG::~InspectorCSSAgentNG() = default;

void InspectorCSSAgentNG::CreateStyleSheet(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->CreateStyleSheet(sender, message);
}

void InspectorCSSAgentNG::AddRule(const std::shared_ptr<MessageSender>& sender,
                                  const Json::Value& message) {
  devtool_mediator_->AddRule(sender, message);
}

void InspectorCSSAgentNG::SetStyleSheetText(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->SetStyleSheetText(sender, message);
}

void InspectorCSSAgentNG::Enable(const std::shared_ptr<MessageSender>& sender,
                                 const Json::Value& message) {
  devtool_mediator_->CSS_Enable(sender, message);
}

void InspectorCSSAgentNG::Disable(const std::shared_ptr<MessageSender>& sender,
                                  const Json::Value& message) {
  devtool_mediator_->CSS_Disable(sender, message);
}

void InspectorCSSAgentNG::GetMatchedStylesForNode(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->GetMatchedStylesForNode(sender, message);
}

void InspectorCSSAgentNG::GetComputedStyleForNode(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->GetComputedStyleForNode(sender, message);
}

void InspectorCSSAgentNG::GetInlineStylesForNode(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->GetInlineStylesForNode(sender, message);
}

void InspectorCSSAgentNG::SetStyleTexts(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->SetStyleTexts(sender, message);
}

void InspectorCSSAgentNG::GetStyleSheetText(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->GetStyleSheetText(sender, message);
}

void InspectorCSSAgentNG::GetBackgroundColors(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->GetBackgroundColors(sender, message);
}

void InspectorCSSAgentNG::CallMethod(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  std::string method = message["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter == functions_map_.end()) {
    Json::Value res;
    res["error"] = Json::ValueType::objectValue;
    res["error"]["code"] = kInspectorErrorCode;
    res["error"]["message"] = "Not implemented: " + method;
    res["id"] = message["id"].asInt64();
    sender->SendMessage("CDP", res);
  } else {
    (this->*(iter->second))(sender, message);
  }
}

void InspectorCSSAgentNG::StartRuleUsageTracking(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->StartRuleUsageTracking(sender, message);
}

void InspectorCSSAgentNG::UpdateRuleUsageTracking(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->UpdateRuleUsageTracking(sender, message);
}

void InspectorCSSAgentNG::StopRuleUsageTracking(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->StopRuleUsageTracking(sender, message);
}

}  // namespace devtool
}  // namespace lynx
