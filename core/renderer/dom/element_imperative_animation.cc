// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <utility>

#include "base/include/value/base_value.h"
#include "core/public/prop_bundle.h"
#include "core/renderer/css/css_keyframes_token.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

bool Element::ShouldTrackImperativeAnimationsForNewPipeline() const {
  return element_manager_ != nullptr &&
         element_manager_->EnableNewStylingPipeline();
}

void Element::ApplyImperativeAnimationMutation(
    const ImperativeAnimationState::Mutation& mutation) {
  if (!ShouldTrackImperativeAnimationsForNewPipeline()) {
    return;
  }
  for (const auto id : mutation.cleanup_properties) {
    ClearPersistedAnimationFillStyle(id);
  }
  for (const auto& animation_name : mutation.keyframes_to_remove) {
    RemoveOwnedImperativeAnimationKeyframe(animation_name);
  }
}

void Element::RemoveOwnedImperativeAnimationKeyframe(
    const base::String& animation_name) {
  if (animation_name.empty()) {
    return;
  }
  if (keyframes_map_.has_value()) {
    keyframes_map_->erase(animation_name);
  }
  if (!enable_new_animator_) {
    auto remove_name = lepus::Value(animation_name);
    auto bundle = element_manager()->GetPropBundleCreator()->CreatePropBundle();
    bundle->SetProps("removeKeyframe", pub::ValueImplLepus(remove_name));
    element_container()->SetKeyframes(std::move(bundle));
  }
  if (will_removed_keyframe_name_ == animation_name) {
    will_removed_keyframe_name_ = base::String();
  }
}

void Element::RecordImperativeAnimationStart(
    ImperativeAnimationState::Source source, const base::String& js_name,
    const base::String& animation_name, bool owns_generated_keyframe,
    const StyleMap& timing_styles) {
  if (!ShouldTrackImperativeAnimationsForNewPipeline()) {
    return;
  }
  CSSKeyframesToken* keyframes_token = nullptr;
  if (keyframes_map_.has_value()) {
    auto keyframes_iter = keyframes_map_->find(animation_name);
    if (keyframes_iter != keyframes_map_->end()) {
      keyframes_token = keyframes_iter->second.get();
    }
  }
  ApplyImperativeAnimationMutation(imperative_animation_state_.RecordStart(
      source, js_name, animation_name, owns_generated_keyframe, timing_styles,
      keyframes_token));
}

void Element::UpdateImperativeAnimationPlayState(
    ImperativeAnimationState::Source source, const base::String& name,
    const StyleMap& timing_styles, bool paused) {
  if (!ShouldTrackImperativeAnimationsForNewPipeline()) {
    return;
  }
  imperative_animation_state_.UpdatePlayState(source, name, timing_styles,
                                              paused);
}

void Element::CancelImperativeAnimation(ImperativeAnimationState::Source source,
                                        const base::String& name) {
  if (!ShouldTrackImperativeAnimationsForNewPipeline()) {
    return;
  }
  ApplyImperativeAnimationMutation(
      imperative_animation_state_.Cancel(source, name));
}

void Element::FinishImperativeAnimation(ImperativeAnimationState::Source source,
                                        const base::String& name) {
  if (!ShouldTrackImperativeAnimationsForNewPipeline()) {
    return;
  }
  ApplyImperativeAnimationMutation(
      imperative_animation_state_.Finish(source, name));
}

void Element::ClearImperativeAnimationsForStyleAnimationUpdate() {
  if (!ShouldTrackImperativeAnimationsForNewPipeline()) {
    return;
  }
  ApplyImperativeAnimationMutation(
      imperative_animation_state_.ClearForStyleAnimationUpdate());
}

void Element::ReplayImperativeAnimationsToStyle(
    starlight::ComputedCSSStyle& computed_style) const {
  if (!ShouldTrackImperativeAnimationsForNewPipeline()) {
    return;
  }
  imperative_animation_state_.ReplayToStyle(computed_style);
}

CSSIDBitset Element::TakePendingImperativeAnimationCleanupProperties() {
  if (!ShouldTrackImperativeAnimationsForNewPipeline()) {
    return CSSIDBitset();
  }
  return imperative_animation_state_.TakePendingCleanupProperties();
}

bool Element::HasImperativeAnimations() const {
  if (!ShouldTrackImperativeAnimationsForNewPipeline()) {
    return false;
  }
  return imperative_animation_state_.HasRecords();
}

void Element::ClearImperativeAnimationState() {
  if (!ShouldTrackImperativeAnimationsForNewPipeline()) {
    return;
  }
  ApplyImperativeAnimationMutation(imperative_animation_state_.Clear());
  will_removed_keyframe_name_ = base::String();
}

}  // namespace tasm
}  // namespace lynx
