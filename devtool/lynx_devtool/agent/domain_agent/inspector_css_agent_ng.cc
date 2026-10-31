// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_css_agent_ng.h"

#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {

InspectorCSSAgentNG::InspectorCSSAgentNG(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {
  functions_map_["CSS.enable"] = &InspectorCSSAgentNG::Enable;
  functions_map_["CSS.disable"] = &InspectorCSSAgentNG::Disable;

  functions_map_["CSS.getMatchedStylesForNode"] =
      &InspectorCSSAgentNG::GetMatchedStylesForNode;
  functions_map_["CSS.getLayersForNode"] =
      &InspectorCSSAgentNG::GetLayersForNode;
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
  functions_map_["CSS.getMediaQueries"] = &InspectorCSSAgentNG::GetMediaQueries;
  functions_map_["CSS.setMediaText"] = &InspectorCSSAgentNG::SetMediaText;
  functions_map_["CSS.setSupportsText"] = &InspectorCSSAgentNG::SetSupportsText;
}

InspectorCSSAgentNG::~InspectorCSSAgentNG() = default;

void InspectorCSSAgentNG::CreateStyleSheet(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->CreateStyleSheet(responder, params);
}

void InspectorCSSAgentNG::AddRule(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->AddRule(responder, params);
}

void InspectorCSSAgentNG::SetStyleSheetText(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->SetStyleSheetText(responder, params);
}

void InspectorCSSAgentNG::Enable(const std::shared_ptr<CDPResponder>& responder,
                                 const Json::Value& params) {
  devtool_mediator_->CSS_Enable(responder, params);
}

void InspectorCSSAgentNG::Disable(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->CSS_Disable(responder, params);
}

void InspectorCSSAgentNG::GetMatchedStylesForNode(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->GetMatchedStylesForNode(responder, params);
}

void InspectorCSSAgentNG::GetLayersForNode(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->GetLayersForNode(responder, params);
}

void InspectorCSSAgentNG::GetComputedStyleForNode(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->GetComputedStyleForNode(responder, params);
}

void InspectorCSSAgentNG::GetInlineStylesForNode(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->GetInlineStylesForNode(responder, params);
}

void InspectorCSSAgentNG::SetStyleTexts(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->SetStyleTexts(responder, params);
}

void InspectorCSSAgentNG::GetStyleSheetText(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->GetStyleSheetText(responder, params);
}

void InspectorCSSAgentNG::GetBackgroundColors(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->GetBackgroundColors(responder, params);
}

void InspectorCSSAgentNG::StartRuleUsageTracking(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->StartRuleUsageTracking(responder, params);
}

void InspectorCSSAgentNG::UpdateRuleUsageTracking(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->UpdateRuleUsageTracking(responder, params);
}

void InspectorCSSAgentNG::StopRuleUsageTracking(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->StopRuleUsageTracking(responder, params);
}

void InspectorCSSAgentNG::GetMediaQueries(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->GetMediaQueries(responder, params);
}

void InspectorCSSAgentNG::SetMediaText(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->SetMediaText(responder, params);
}

void InspectorCSSAgentNG::SetSupportsText(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->SetSupportsText(responder, params);
}

void InspectorCSSAgentNG::CallMethod(
    const std::shared_ptr<CDPResponder>& responder,
    const Json::Value& message) {
  std::string method = message["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter == functions_map_.end()) {
    responder->SendError(CDPErrorCode::MethodNotFound,
                         "'" + method + "' wasn't found");
  } else if (!devtool_mediator_) {
    responder->SendError(CDPErrorCode::ServerError,
                         "CSS target is unavailable");
  } else {
    (this->*(iter->second))(responder, message["params"]);
  }
}

}  // namespace devtool
}  // namespace lynx
