// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/inspector_animation_controller.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/animation/animation.h"
#include "core/animation/animation_curve.h"
#include "core/animation/keyframe_effect.h"
#include "core/animation/keyframe_model.h"
#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "devtool/lynx_devtool/element/element_inspector.h"
#include "gfx/animation/animation_keyframe.h"
#include "gfx/animation/timing_function.h"

namespace lynx {
namespace devtool {
namespace {

bool TryParseAnimationId(const std::string& value, int64_t& out) {
  if (value.empty()) {
    return false;
  }
  errno = 0;
  char* end = nullptr;
  const long long parsed = std::strtoll(value.c_str(), &end, 10);
  if (errno != 0 || end == value.c_str() || *end != '\0') {
    return false;
  }
  out = static_cast<int64_t>(parsed);
  return true;
}

// Formats a normalized keyframe offset (0..1) as the percentage string expected
// by CDP KeyframeStyle.offset ("0%", "45%", "100%", ...).
std::string FormatKeyframeOffset(double normalized_offset) {
  const double percentage = std::clamp(normalized_offset, 0.0, 1.0) * 100.0;
  char buf[32];
  snprintf(buf, sizeof(buf), "%.6f", percentage);
  std::string result(buf);
  const size_t dot = result.find('.');
  if (dot != std::string::npos) {
    const size_t last = result.find_last_not_of('0');
    result.erase(result[last] == '.' ? last : last + 1);
  }
  return result + "%";
}

// Converts a gfx::TimingFunction (per-keyframe easing) to its CSS string form.
std::string GfxTimingFunctionToString(const gfx::TimingFunction* tf) {
  if (tf == nullptr || tf->GetType() == gfx::TimingFunction::Type::LINEAR) {
    return "linear";
  }
  if (tf->GetType() == gfx::TimingFunction::Type::CUBIC_BEZIER) {
    const auto* cb = static_cast<const gfx::CubicBezierTimingFunction*>(tf);
    switch (cb->ease_type()) {
      case gfx::CubicBezierTimingFunction::EaseType::EASE:
        return "ease";
      case gfx::CubicBezierTimingFunction::EaseType::EASE_IN:
        return "ease-in";
      case gfx::CubicBezierTimingFunction::EaseType::EASE_OUT:
        return "ease-out";
      case gfx::CubicBezierTimingFunction::EaseType::EASE_IN_OUT:
        return "ease-in-out";
      case gfx::CubicBezierTimingFunction::EaseType::CUSTOM: {
        const auto& b = cb->bezier();
        char buf[96];
        snprintf(buf, sizeof(buf), "cubic-bezier(%g, %g, %g, %g", b.GetX1(),
                 b.GetY1(), b.GetX2(), b.GetY2());
        return std::string(buf) + ")";
      }
    }
  }
  if (tf->GetType() == gfx::TimingFunction::Type::STEPS) {
    const auto* st = static_cast<const gfx::StepsTimingFunction*>(tf);
    const char* pos = "end";
    switch (st->step_position()) {
      case gfx::StepsType::kStart:
        pos = "start";
        break;
      case gfx::StepsType::kJumpBoth:
        pos = "jump-both";
        break;
      case gfx::StepsType::kJumpNone:
        pos = "jump-none";
        break;
      case gfx::StepsType::kEnd:
      default:
        pos = "end";
        break;
    }
    char buf[48];
    snprintf(buf, sizeof(buf), "steps(%d, %s)", st->steps(), pos);
    return std::string(buf);
  }
  return "linear";
}

// Converts the overall animation timing function (from AnimationData) to its
// CSS string form.
std::string StarlightTimingFunctionToString(
    const starlight::TimingFunctionData& tf) {
  switch (tf.timing_func) {
    case starlight::TimingFunctionType::kLinear:
      return "linear";
    case starlight::TimingFunctionType::kEaseIn:
      return "ease-in";
    case starlight::TimingFunctionType::kEaseOut:
      return "ease-out";
    case starlight::TimingFunctionType::kEaseInEaseOut:
      return "ease-in-out";
    case starlight::TimingFunctionType::kCubicBezier: {
      char buf[96];
      snprintf(buf, sizeof(buf), "cubic-bezier(%g, %g, %g, %g",
               static_cast<double>(tf.x1), static_cast<double>(tf.y1),
               static_cast<double>(tf.x2), static_cast<double>(tf.y2));
      return std::string(buf) + ")";
    }
    // TODO(animations): Starlight carries no step count for kSteps, and
    // kSquareBezier has no direct CSS representation; both fall back to linear
    // in the initial Animation-domain implementation.
    case starlight::TimingFunctionType::kSteps:
    case starlight::TimingFunctionType::kSquareBezier:
    default:
      return "linear";
  }
}

const char* PlayStateToString(animation::Animation::State state) {
  switch (state) {
    case animation::Animation::State::kIdle:
      return "idle";
    case animation::Animation::State::kPlay:
      return "running";
    case animation::Animation::State::kPause:
      return "paused";
    case animation::Animation::State::kStop:
      return "finished";
  }
  return "idle";
}

const char* DirectionToString(starlight::AnimationDirectionType dir) {
  switch (dir) {
    case starlight::AnimationDirectionType::kReverse:
      return "reverse";
    case starlight::AnimationDirectionType::kAlternate:
      return "alternate";
    case starlight::AnimationDirectionType::kAlternateReverse:
      return "alternate-reverse";
    case starlight::AnimationDirectionType::kNormal:
    default:
      return "normal";
  }
}

const char* FillModeToString(starlight::AnimationFillModeType fill) {
  switch (fill) {
    case starlight::AnimationFillModeType::kForwards:
      return "forwards";
    case starlight::AnimationFillModeType::kBackwards:
      return "backwards";
    case starlight::AnimationFillModeType::kBoth:
      return "both";
    case starlight::AnimationFillModeType::kNone:
    default:
      return "none";
  }
}

}  // namespace

InspectorAnimationController::InspectorAnimationController(
    std::weak_ptr<LynxDevToolMediator> devtool_mediator)
    : devtool_mediator_wp_(std::move(devtool_mediator)) {}

void InspectorAnimationController::SendEvent(const std::string& method,
                                             const Json::Value& params) const {
  if (!enabled_) {
    return;
  }
  auto devtool_mediator = devtool_mediator_wp_.lock();
  if (devtool_mediator == nullptr) {
    return;
  }
  Json::Value message(Json::objectValue);
  message["method"] = method;
  message["params"] = params;
  devtool_mediator->RunOnDevToolThread(
      [devtool_mediator, message]() mutable {
        devtool_mediator->SendCDPEvent(message);
      },
      true);
}

Json::Value InspectorAnimationController::BuildAnimationSnapshot(
    animation::Animation* animation) {
  Json::Value anim(Json::ValueType::objectValue);
  if (animation == nullptr) {
    return anim;
  }

  anim["id"] = std::to_string(animation->id());
  anim["name"] = animation->name().str();

  switch (animation->GetOrigin()) {
    case animation::Animation::Origin::kCSSTransition:
      anim["type"] = "CSSTransition";
      break;
    case animation::Animation::Origin::kWebAnimation:
      anim["type"] = "WebAnimation";
      break;
    case animation::Animation::Origin::kCSSAnimation:
    default:
      anim["type"] = "CSSAnimation";
      break;
  }

  anim["pausedState"] =
      animation->GetState() == animation::Animation::State::kPause;
  anim["playState"] = PlayStateToString(animation->GetState());
  // Playback-rate control is not implemented; report the fixed effective rate.
  anim["playbackRate"] = 1.0;

  const fml::TimePoint& start_time = animation->start_time();
  if (start_time == fml::TimePoint::Min() ||
      start_time == animation::Animation::GetAnimationDummyStartTime()) {
    anim["startTime"] = 0;
  } else {
    anim["startTime"] =
        static_cast<Json::Int64>(start_time.ToEpochDelta().ToMilliseconds());
  }
  anim["currentTime"] =
      static_cast<Json::Int64>(animation->GetCurrentTime().ToMilliseconds());

  // cssId links a CSS @keyframes animation to its rule; only meaningful for
  // CSS animations.
  if (animation->GetOrigin() == animation::Animation::Origin::kCSSAnimation) {
    anim["cssId"] = animation->name().str();
  }

  // source: AnimationEffect
  Json::Value source(Json::ValueType::objectValue);
  const starlight::AnimationData& data = animation->get_animation_data();
  source["delay"] = static_cast<Json::Int64>(data.delay);
  source["endDelay"] = 0;
  source["iterationStart"] = 0;
  // Lynx represents CSS `infinite` with this parser sentinel. CDP requires
  // `iterations` to be omitted, rather than reported as a very large finite
  // value, for an infinite animation.
  constexpr int kInfiniteAnimationIterations = 1000000000;
  if (data.iteration_count != kInfiniteAnimationIterations) {
    source["iterations"] = static_cast<Json::Int64>(data.iteration_count);
  }
  // Iteration duration in ms; the panel sizes the animation bar from this.
  source["duration"] = static_cast<Json::Int64>(data.duration);
  source["direction"] = DirectionToString(data.direction);
  source["fill"] = FillModeToString(data.fill_mode);
  source["easing"] = StarlightTimingFunctionToString(data.timing_func);

  tasm::Element* element = animation->GetElement();
  source["backendNodeId"] =
      element != nullptr ? ElementInspector::NodeId(element) : 0;

  // keyframesRule
  Json::Value keyframes_rule(Json::ValueType::objectValue);
  keyframes_rule["name"] = data.name.str();
  Json::Value keyframes(Json::ValueType::arrayValue);
  if (data.duration > 0) {
    animation::KeyframeEffect* effect = animation->keyframe_effect();
    if (effect != nullptr) {
      // Collect the union of (offset, easing) across all property curves. CSS
      // @keyframes and transitions share offsets across properties, so the
      // union is robust to any divergence while staying deduplicated.
      std::map<double, std::string> offset_to_easing;
      for (auto& model_ptr : effect->keyframe_models()) {
        if (model_ptr == nullptr) {
          continue;
        }
        animation::AnimationCurve* curve = model_ptr->animation_curve();
        if (curve == nullptr) {
          continue;
        }
        size_t count = curve->get_keyframes_size();
        for (size_t i = 0; i < count; ++i) {
          const gfx::Keyframe* kf = curve->KeyframeAt(i);
          if (kf == nullptr) {
            continue;
          }
          const double normalized_offset = kf->Time().ToSecondsF();
          // Read the timing used by sampling, not the current author styles.
          offset_to_easing.emplace(
              normalized_offset,
              GfxTimingFunctionToString(kf->timing_function()));
        }
      }
      for (const auto& kv : offset_to_easing) {
        Json::Value kf_style(Json::ValueType::objectValue);
        kf_style["offset"] = FormatKeyframeOffset(kv.first);
        kf_style["easing"] = kv.second;
        keyframes.append(kf_style);
      }
    }
  }
  keyframes_rule["keyframes"] = keyframes;
  source["keyframesRule"] = keyframes_rule;

  anim["source"] = source;
  return anim;
}

void InspectorAnimationController::Clear() {
  // Navigation or teardown invalidates every live Animation pointer.
  animation_registry_.clear();
  released_animation_ids_.clear();
}

void InspectorAnimationController::OnAnimationCreated(
    animation::Animation* animation) {
  if (animation == nullptr) {
    return;
  }
  animation_registry_[animation->id()] = animation;
  Json::Value params(Json::objectValue);
  params["id"] = std::to_string(animation->id());
  SendEvent("Animation.animationCreated", params);
}

void InspectorAnimationController::OnAnimationStarted(
    animation::Animation* animation) {
  if (animation == nullptr) {
    return;
  }
  // Preserve the late-observer fallback unless the frontend has released this
  // stable ID. Animation IDs are never reused, so release must remain
  // effective across a later started callback.
  if (released_animation_ids_.find(animation->id()) ==
      released_animation_ids_.end()) {
    animation_registry_[animation->id()] = animation;
  }
  Json::Value params(Json::objectValue);
  params["animation"] = BuildAnimationSnapshot(animation);
  SendEvent("Animation.animationStarted", params);
}

void InspectorAnimationController::OnAnimationUpdated(
    animation::Animation* animation) {
  if (animation == nullptr) {
    return;
  }
  Json::Value params(Json::objectValue);
  params["animation"] = BuildAnimationSnapshot(animation);
  SendEvent("Animation.animationUpdated", params);
}

void InspectorAnimationController::OnAnimationCanceled(
    animation::Animation* animation) {
  if (animation == nullptr) {
    return;
  }
  Json::Value params(Json::objectValue);
  params["id"] = std::to_string(animation->id());
  SendEvent("Animation.animationCanceled", params);
  animation_registry_.erase(animation->id());
  released_animation_ids_.erase(animation->id());
}

void InspectorAnimationController::Enable(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value&) {
  enabled_ = true;
  // Backfill both lifecycle events so the panel can show animations that
  // started before the Animation domain was enabled.
  for (const auto& entry : animation_registry_) {
    Json::Value created_params(Json::objectValue);
    created_params["id"] = std::to_string(entry.first);
    SendEvent("Animation.animationCreated", created_params);
  }
  for (const auto& entry : animation_registry_) {
    Json::Value started_params(Json::objectValue);
    started_params["animation"] = BuildAnimationSnapshot(entry.second);
    SendEvent("Animation.animationStarted", started_params);
  }
  responder->SendSuccess();
}

void InspectorAnimationController::Disable(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value&) {
  enabled_ = false;
  responder->SendSuccess();
}

void InspectorAnimationController::GetCurrentTime(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  if (!params.isObject() || !params["id"].isString()) {
    responder->SendError(CDPErrorCode::InvalidParams,
                         "Missing or invalid 'id' parameter.");
    return;
  }
  int64_t id = 0;
  if (!TryParseAnimationId(params["id"].asString(), id)) {
    responder->SendError(CDPErrorCode::InvalidParams, "Invalid animation id.");
    return;
  }
  const auto it = animation_registry_.find(id);
  if (it == animation_registry_.end() || it->second == nullptr) {
    responder->SendError(CDPErrorCode::InvalidParams, "Animation not found.");
    return;
  }
  Json::Value result(Json::objectValue);
  result["currentTime"] =
      static_cast<Json::Int64>(it->second->GetCurrentTime().ToMilliseconds());
  responder->SendSuccess(result);
}

void InspectorAnimationController::SeekAnimations(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  if (!params.isObject() || !params["animations"].isArray() ||
      !params["currentTime"].isNumeric()) {
    responder->SendError(CDPErrorCode::InvalidParams,
                         "Missing or invalid Animation parameters.");
    return;
  }
  const double current_time_ms = params["currentTime"].asDouble();
  constexpr double kMaxMilliseconds =
      static_cast<double>(std::numeric_limits<int64_t>::max()) / 1000000.0;
  if (!std::isfinite(current_time_ms) || current_time_ms < 0 ||
      current_time_ms > kMaxMilliseconds) {
    responder->SendError(CDPErrorCode::InvalidParams,
                         "'currentTime' must be a finite, non-negative time.");
    return;
  }
  std::vector<animation::Animation*> animations;
  std::unordered_set<int64_t> unique_ids;
  for (const auto& value : params["animations"]) {
    int64_t id = 0;
    if (!value.isString() || !TryParseAnimationId(value.asString(), id)) {
      responder->SendError(CDPErrorCode::InvalidParams,
                           "Animation ids must be strings.");
      return;
    }
    const auto it = animation_registry_.find(id);
    if (it == animation_registry_.end() || it->second == nullptr) {
      responder->SendError(CDPErrorCode::InvalidParams, "Animation not found.");
      return;
    }
    if (unique_ids.insert(id).second) {
      animations.push_back(it->second);
    }
  }
  // Validate the whole batch before mutating it, then use one reference time
  // so all animations land on the requested timeline position together.
  const auto current_time = fml::TimeDelta::FromMillisecondsF(current_time_ms);
  const auto reference_time = fml::TimePoint::Now();
  for (auto* animation : animations) {
    animation->SeekTo(current_time, reference_time);
  }
  responder->SendSuccess();
}

void InspectorAnimationController::SetPaused(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  if (!params.isObject() || !params["animations"].isArray() ||
      !params["paused"].isBool()) {
    responder->SendError(CDPErrorCode::InvalidParams,
                         "Missing or invalid Animation parameters.");
    return;
  }
  std::vector<animation::Animation*> animations;
  std::unordered_set<int64_t> unique_ids;
  for (const auto& value : params["animations"]) {
    int64_t id = 0;
    if (!value.isString() || !TryParseAnimationId(value.asString(), id)) {
      responder->SendError(CDPErrorCode::InvalidParams,
                           "Animation ids must be strings.");
      return;
    }
    const auto it = animation_registry_.find(id);
    if (it == animation_registry_.end() || it->second == nullptr) {
      responder->SendError(CDPErrorCode::InvalidParams, "Animation not found.");
      return;
    }
    if (unique_ids.insert(id).second) {
      animations.push_back(it->second);
    }
  }
  // Validate the whole batch before changing any state. A shared reference
  // time keeps every animation in the group paused or resumed in lockstep.
  const bool paused = params["paused"].asBool();
  const auto reference_time = fml::TimePoint::Now();
  for (auto* animation : animations) {
    animation->SetPaused(paused, reference_time);
  }
  responder->SendSuccess();
}

void InspectorAnimationController::ReleaseAnimations(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  if (!params.isObject() || !params["animations"].isArray()) {
    responder->SendError(CDPErrorCode::InvalidParams,
                         "Missing or invalid 'animations' parameter.");
    return;
  }
  std::vector<int64_t> ids;
  for (const auto& value : params["animations"]) {
    int64_t id = 0;
    if (!value.isString() || !TryParseAnimationId(value.asString(), id)) {
      responder->SendError(CDPErrorCode::InvalidParams,
                           "Animation ids must be valid strings.");
      return;
    }
    ids.push_back(id);
  }
  // Validation above makes release atomic for malformed input. Unknown IDs are
  // accepted, so repeated releaseAnimations calls remain idempotent.
  for (int64_t id : ids) {
    animation_registry_.erase(id);
    released_animation_ids_.insert(id);
  }
  responder->SendSuccess();
}

}  // namespace devtool
}  // namespace lynx
