// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/imperative_animation_metadata.h"

namespace lynx {
namespace tasm {

void ImperativeAnimationMetadata::RecordStart(
    ImperativeAnimationSource source, const base::String& js_name,
    const base::String& animation_name) {
  for (auto iter = records_.begin(); iter != records_.end();) {
    const bool same_identity =
        iter->source == source &&
        ((js_name.empty() && animation_name.empty()) ||
         (!js_name.empty() &&
          (iter->js_name == js_name || iter->animation_name == js_name)) ||
         (!animation_name.empty() && (iter->js_name == animation_name ||
                                      iter->animation_name == animation_name)));
    if (iter->source == source &&
        (source == ImperativeAnimationSource::kAnimate || same_identity)) {
      iter = records_.erase(iter);
    } else {
      ++iter;
    }
  }
  records_.emplace_back(Record{source, js_name, animation_name});
}

void ImperativeAnimationMetadata::Cancel(ImperativeAnimationSource source,
                                         const base::String& name) {
  for (auto iter = records_.begin(); iter != records_.end();) {
    if (iter->source == source && (name.empty() || iter->js_name == name ||
                                   iter->animation_name == name)) {
      iter = records_.erase(iter);
    } else {
      ++iter;
    }
  }
}

void ImperativeAnimationMetadata::Finish(ImperativeAnimationSource source,
                                         const base::String& name) {
  Cancel(source, name);
}

void ImperativeAnimationMetadata::Clear() { records_.clear(); }

bool ImperativeAnimationMetadata::HasAnimationName(
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
