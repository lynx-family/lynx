// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PLATFORM_NATIVE_VIEW_TAGS_H_
#define CLAY_UI_PLATFORM_NATIVE_VIEW_TAGS_H_

#include <string>
#include <unordered_set>

namespace clay {

const std::unordered_set<std::string>& InternalPlatformViewTags();
const std::unordered_set<std::string>& InternalPlatformViewShadowNodeTags();
bool ShouldCreateFallbackNativeViewDirectly();

}  // namespace clay

#endif  // CLAY_UI_PLATFORM_NATIVE_VIEW_TAGS_H_
