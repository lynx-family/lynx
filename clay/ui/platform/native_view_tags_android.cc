// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/platform/native_view_tags.h"

namespace clay {

const std::unordered_set<std::string>& InternalPlatformViewTags() {
  static const std::unordered_set<std::string> tags = {
      "map-view",      "input-view",     "textarea-view", "input",
      "x-input",       "x-input-ng",     "textarea",      "x-textarea",
      "x-textarea-ng", "x-video-engine",
  };
  return tags;
}

const std::unordered_set<std::string>& InternalPlatformViewShadowNodeTags() {
  static const std::unordered_set<std::string> tags = {
      "map-view",      "input-view",     "textarea-view", "input",
      "x-input",       "x-input-ng",     "textarea",      "x-textarea",
      "x-textarea-ng", "x-video-engine",
  };
  return tags;
}

bool ShouldCreateFallbackNativeViewDirectly() { return true; }

}  // namespace clay
