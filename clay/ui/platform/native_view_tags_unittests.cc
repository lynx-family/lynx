// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/platform/native_view_tags.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

TEST(NativeViewTagsTest, AndroidHeadlessDisablesInternalPlatformViews) {
  EXPECT_TRUE(InternalPlatformViewTags().empty());
  EXPECT_TRUE(InternalPlatformViewShadowNodeTags().empty());
  EXPECT_TRUE(InternalPlatformViewWithoutShadowNodeTags().empty());
  EXPECT_FALSE(ShouldCreateFallbackNativeViewDirectly());
}

}  // namespace clay
