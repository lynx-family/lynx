// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/layout_property.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/utils/prop_bundle_style_writer.h"

namespace lynx {
namespace tasm {

bool Element::UpdateAuthorAnimationDataForNewPipeline(
    const starlight::ComputedCSSStyle& new_base_style) {
  const auto* new_animation_data = new_base_style.animation_data_or_null();
  const bool has_new_animation_data =
      new_animation_data != nullptr && !new_animation_data->empty();
  const bool has_old_animation_data =
      author_animation_data_for_new_pipeline_.has_value();
  const bool changed =
      has_new_animation_data != has_old_animation_data ||
      (has_new_animation_data &&
       *new_animation_data != *author_animation_data_for_new_pipeline_);
  if (!changed) {
    return false;
  }
  if (has_new_animation_data) {
    author_animation_data_for_new_pipeline_.emplace(*new_animation_data);
  } else {
    author_animation_data_for_new_pipeline_.reset();
  }
  return true;
}

void Element::FlushImperativeAnimationCleanupForNewPipeline(
    starlight::ComputedCSSStyle& cleanup_style, bool& need_update,
    CSSIDBitset* replayed_ids, const CSSIDBitset* source_style_ids) {
  auto cleanup_properties = TakePendingImperativeAnimationCleanupProperties();
  if (!cleanup_properties.HasAny()) {
    return;
  }

  const auto& resolved_values = cleanup_style.GetResolvedValues();
  auto replay_mode = [this, source_style_ids](CSSPropertyID id) {
    return source_style_ids != nullptr &&
                   ShouldPreserveLayoutOnlyForInheritedPlatformStyle(
                       id, *source_style_ids)
               ? StyleSideEffectReplayMode::kPreserveLayoutOnly
               : StyleSideEffectReplayMode::kNormal;
  };
  for (const auto id : cleanup_properties) {
    if (replayed_ids != nullptr && replayed_ids->Has(id)) {
      continue;
    }
    const auto resolved_iter = resolved_values.find(id);
    if (resolved_iter != resolved_values.end()) {
      ReplayChangedStyleSideEffect(id, resolved_iter->second, replay_mode(id));
      if (!LayoutProperty::IsLayoutOnly(id)) {
        PreparePropBundleIfNeed();
        PropBundleStyleWriter::PushStyleToBundle(prop_bundle_.get(), id,
                                                 &cleanup_style);
      }
    } else {
      ReplayResetStyleSideEffect(id, replay_mode(id));
      if (!LayoutProperty::IsLayoutOnly(id)) {
        PreparePropBundleIfNeed();
        prop_bundle_->SetNullPropsByID(id);
      }
    }
    if (replayed_ids != nullptr) {
      replayed_ids->Set(id);
    }
    need_update = true;
  }
}

}  // namespace tasm
}  // namespace lynx
