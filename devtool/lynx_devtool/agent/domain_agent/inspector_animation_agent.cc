// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_animation_agent.h"

#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {

InspectorAnimationAgent::InspectorAnimationAgent(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {
  functions_map_["Animation.enable"] = &InspectorAnimationAgent::Enable;
  functions_map_["Animation.disable"] = &InspectorAnimationAgent::Disable;
  functions_map_["Animation.getCurrentTime"] =
      &InspectorAnimationAgent::GetCurrentTime;
  functions_map_["Animation.seekAnimations"] =
      &InspectorAnimationAgent::SeekAnimations;
  functions_map_["Animation.setPaused"] = &InspectorAnimationAgent::SetPaused;
  functions_map_["Animation.releaseAnimations"] =
      &InspectorAnimationAgent::ReleaseAnimations;
}

void InspectorAnimationAgent::Enable(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->AnimationEnable(responder, params);
}

void InspectorAnimationAgent::Disable(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->AnimationDisable(responder, params);
}

void InspectorAnimationAgent::GetCurrentTime(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->AnimationGetCurrentTime(responder, params);
}

void InspectorAnimationAgent::SeekAnimations(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->AnimationSeekAnimations(responder, params);
}

void InspectorAnimationAgent::SetPaused(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->AnimationSetPaused(responder, params);
}

void InspectorAnimationAgent::ReleaseAnimations(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->AnimationReleaseAnimations(responder, params);
}

void InspectorAnimationAgent::CallMethod(
    const std::shared_ptr<CDPResponder>& responder,
    const Json::Value& message) {
  std::string method = message["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter == functions_map_.end()) {
    responder->SendError(CDPErrorCode::MethodNotFound,
                         "'" + method + "' wasn't found");
  } else {
    (this->*(iter->second))(responder, message["params"]);
  }
}

}  // namespace devtool
}  // namespace lynx
