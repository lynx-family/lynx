// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_animation_agent.h"

#include "devtool/base_devtool/native/public/message_sender.h"
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
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->AnimationEnable(sender, message);
}

void InspectorAnimationAgent::Disable(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->AnimationDisable(sender, message);
}

void InspectorAnimationAgent::GetCurrentTime(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->AnimationGetCurrentTime(sender, message);
}

void InspectorAnimationAgent::SeekAnimations(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->AnimationSeekAnimations(sender, message);
}

void InspectorAnimationAgent::SetPaused(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->AnimationSetPaused(sender, message);
}

void InspectorAnimationAgent::ReleaseAnimations(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->AnimationReleaseAnimations(sender, message);
}

void InspectorAnimationAgent::CallMethod(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  std::string method = message["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter == functions_map_.end()) {
    // Remaining mutation/control methods (setPlaybackRate, setTiming,
    // resolveAnimation, getPlaybackRate, ...) are not implemented and report
    // "Method not found" (-32601).
    SendNotImplementedResponse(sender, message["id"].asInt64(), method);
  } else {
    (this->*(iter->second))(sender, message);
  }
}

}  // namespace devtool
}  // namespace lynx
