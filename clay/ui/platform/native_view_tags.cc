// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/platform/native_view_tags.h"

namespace clay {

namespace {

constexpr std::string_view kInternalPlatformViewTags[] = {
    "input",         "input-view",    "map-view",       "textarea",
    "textarea-view", "x-input",       "x-input-ng",     "x-map-ng",
    "x-textarea",    "x-textarea-ng", "x-video-engine",
};
constexpr NativeViewTagSet kInternalPlatformViewTagSet(
    kInternalPlatformViewTags,
    sizeof(kInternalPlatformViewTags) / sizeof(kInternalPlatformViewTags[0]));

constexpr std::string_view kInternalPlatformViewShadowNodeTags[] = {
    "input",   "input-view", "map-view",   "textarea",      "textarea-view",
    "x-input", "x-input-ng", "x-textarea", "x-textarea-ng", "x-video-engine",
};
constexpr NativeViewTagSet kInternalPlatformViewShadowNodeTagSet(
    kInternalPlatformViewShadowNodeTags,
    sizeof(kInternalPlatformViewShadowNodeTags) /
        sizeof(kInternalPlatformViewShadowNodeTags[0]));

constexpr std::string_view kInternalPlatformViewWithoutShadowNodeTags[] = {
    "x-map-marker-ng",
    "x-map-ng",
};
constexpr NativeViewTagSet kInternalPlatformViewWithoutShadowNodeTagSet(
    kInternalPlatformViewWithoutShadowNodeTags,
    sizeof(kInternalPlatformViewWithoutShadowNodeTags) /
        sizeof(kInternalPlatformViewWithoutShadowNodeTags[0]));

}  // namespace

const NativeViewTagSet& InternalPlatformViewTags() {
  // iOS implementation. Android links native_view_tags_android.cc, where
  // x-video-engine is admitted as an XElement-backed platform view.
  // Clay's c++ video tag is changed to be clay-video-engine.
  return kInternalPlatformViewTagSet;
}

const NativeViewTagSet& InternalPlatformViewShadowNodeTags() {
  return kInternalPlatformViewShadowNodeTagSet;
}

const NativeViewTagSet& InternalPlatformViewWithoutShadowNodeTags() {
  return kInternalPlatformViewWithoutShadowNodeTagSet;
}

bool ShouldCreateFallbackNativeViewDirectly() { return true; }

}  // namespace clay
