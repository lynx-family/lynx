// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/platform/native_view_tags.h"

namespace clay {

namespace {

constexpr NativeViewTagSet kEmptyNativeViewTags(nullptr, 0);

}  // namespace

const NativeViewTagSet& InternalPlatformViewTags() {
  return kEmptyNativeViewTags;
}

const NativeViewTagSet& InternalPlatformViewShadowNodeTags() {
  return kEmptyNativeViewTags;
}

const NativeViewTagSet& InternalPlatformViewWithoutShadowNodeTags() {
  return kEmptyNativeViewTags;
}

bool ShouldCreateFallbackNativeViewDirectly() { return false; }

}  // namespace clay
