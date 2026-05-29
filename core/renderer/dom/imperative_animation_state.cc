// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/imperative_animation_state.h"

#include <utility>

#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/css_keyframes_token.h"

namespace lynx {
namespace tasm {
namespace {

bool IsForwardFilling(starlight::AnimationFillModeType fill_mode) {
  return fill_mode == starlight::AnimationFillModeType::kForwards ||
         fill_mode == starlight::AnimationFillModeType::kBoth;
}

starlight::AnimationFillModeType GetAnimationFillMode(
    const StyleMap& timing_styles) {
  auto iter = timing_styles.find(kPropertyIDAnimationFillMode);
  if (iter == timing_styles.end()) {
    return starlight::AnimationFillModeType::kNone;
  }
  return iter->second.GetEnum<starlight::AnimationFillModeType>();
}

CSSIDBitset CollectAffectedPropertiesFromKeyframes(
    CSSKeyframesToken* keyframes_token) {
  CSSIDBitset affected_properties;
  if (keyframes_token == nullptr) {
    return affected_properties;
  }
  for (const auto& keyframe : keyframes_token->GetKeyframesContent()) {
    const auto& keyframe_styles = keyframe.second;
    if (keyframe_styles == nullptr) {
      continue;
    }
    for (const auto& style : *keyframe_styles) {
      const auto id = style.first;
      if (CSSProperty::IsTransitionProps(id) ||
          CSSProperty::IsKeyframeProps(id)) {
        continue;
      }
      affected_properties.Set(id);
    }
  }
  return affected_properties;
}

}  // namespace

void ImperativeAnimationState::QueueCleanup(const CSSIDBitset& properties,
                                            Mutation& mutation) {
  pending_cleanup_properties_.Or(properties);
  mutation.cleanup_properties.Or(properties);
}

void ImperativeAnimationState::QueueOwnedKeyframeRemoval(const Record& record,
                                                         Mutation& mutation) {
  if (record.owns_generated_keyframe && !record.animation_name.empty()) {
    mutation.keyframes_to_remove.emplace_back(record.animation_name);
  }
}

bool ImperativeAnimationState::MatchesName(const Record& record,
                                           const base::String& name) {
  return !name.empty() &&
         (record.animation_name == name || record.js_name == name);
}

bool ImperativeAnimationState::MatchesIdentity(
    const Record& record, Source source, const base::String& js_name,
    const base::String& animation_name) {
  if (record.source != source) {
    return false;
  }
  if (js_name.empty() && animation_name.empty()) {
    return true;
  }
  return MatchesName(record, js_name) || MatchesName(record, animation_name);
}

bool ImperativeAnimationState::ShouldReplaceOnStart(
    const Record& record, Source source, const base::String& js_name,
    const base::String& animation_name) {
  if (record.source != source) {
    return false;
  }
  if (source == Source::kAnimate) {
    return true;
  }
  return MatchesIdentity(record, source, js_name, animation_name);
}

ImperativeAnimationState::Mutation ImperativeAnimationState::RecordStart(
    Source source, const base::String& js_name,
    const base::String& animation_name, bool owns_generated_keyframe,
    const StyleMap& timing_styles, CSSKeyframesToken* keyframes_token) {
  Mutation mutation;
  CSSIDBitset new_affected_properties =
      CollectAffectedPropertiesFromKeyframes(keyframes_token);

  for (auto iter = records_.begin(); iter != records_.end();) {
    if (!ShouldReplaceOnStart(*iter, source, js_name, animation_name)) {
      ++iter;
      continue;
    }
    CSSIDBitset cleanup_properties = iter->affected_properties;
    for (const auto id : new_affected_properties) {
      cleanup_properties.Reset(id);
    }
    QueueCleanup(cleanup_properties, mutation);
    if (iter->animation_name != animation_name) {
      QueueOwnedKeyframeRemoval(*iter, mutation);
    }
    iter = records_.erase(iter);
  }

  Record record;
  record.source = source;
  record.js_name = js_name;
  record.animation_name = animation_name;
  record.timing_styles = timing_styles;
  record.affected_properties = new_affected_properties;
  record.fill_mode = GetAnimationFillMode(timing_styles);
  record.owns_generated_keyframe = owns_generated_keyframe;
  records_.emplace_back(std::move(record));
  return mutation;
}

void ImperativeAnimationState::UpdatePlayState(Source source,
                                               const base::String& name,
                                               const StyleMap& timing_styles,
                                               bool paused) {
  (void)paused;
  for (auto& record : records_) {
    if (!MatchesIdentity(record, source, name, name)) {
      continue;
    }
    for (const auto& [id, value] : timing_styles) {
      record.timing_styles.insert_or_assign(id, value);
    }
  }
}

ImperativeAnimationState::Mutation ImperativeAnimationState::Cancel(
    Source source, const base::String& name) {
  Mutation mutation;
  for (auto iter = records_.begin(); iter != records_.end();) {
    if (!MatchesIdentity(*iter, source, name, name)) {
      ++iter;
      continue;
    }
    QueueCleanup(iter->affected_properties, mutation);
    QueueOwnedKeyframeRemoval(*iter, mutation);
    iter = records_.erase(iter);
  }
  return mutation;
}

ImperativeAnimationState::Mutation ImperativeAnimationState::Finish(
    Source source, const base::String& name) {
  Mutation mutation;
  for (auto iter = records_.begin(); iter != records_.end();) {
    if (!MatchesIdentity(*iter, source, name, name)) {
      ++iter;
      continue;
    }
    if (IsForwardFilling(iter->fill_mode)) {
      ++iter;
      continue;
    }
    QueueCleanup(iter->affected_properties, mutation);
    QueueOwnedKeyframeRemoval(*iter, mutation);
    iter = records_.erase(iter);
  }
  return mutation;
}

ImperativeAnimationState::Mutation
ImperativeAnimationState::ClearForStyleAnimationUpdate() {
  Mutation mutation;
  for (const auto& record : records_) {
    QueueCleanup(record.affected_properties, mutation);
    QueueOwnedKeyframeRemoval(record, mutation);
  }
  records_.clear();
  return mutation;
}

ImperativeAnimationState::Mutation ImperativeAnimationState::Clear() {
  Mutation mutation;
  for (const auto& record : records_) {
    QueueCleanup(record.affected_properties, mutation);
    QueueOwnedKeyframeRemoval(record, mutation);
  }
  records_.clear();
  pending_cleanup_properties_.Reset();
  return mutation;
}

void ImperativeAnimationState::ReplayToStyle(
    starlight::ComputedCSSStyle& computed_style) const {
  for (const auto& record : records_) {
    if (record.timing_styles.find(kPropertyIDAnimationName) ==
        record.timing_styles.end()) {
      continue;
    }
    computed_style.AppendAnimatedAnimationValue(record.timing_styles);
  }
}

CSSIDBitset ImperativeAnimationState::TakePendingCleanupProperties() {
  CSSIDBitset cleanup_properties = pending_cleanup_properties_;
  pending_cleanup_properties_.Reset();
  return cleanup_properties;
}

bool ImperativeAnimationState::HasAnimationName(
    const base::String& animation_name) const {
  for (const auto& record : records_) {
    if (record.animation_name == animation_name) {
      return true;
    }
  }
  return false;
}

}  // namespace tasm
}  // namespace lynx
