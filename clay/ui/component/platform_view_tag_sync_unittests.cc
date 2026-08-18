// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// Host-side (mac / windows / headless) coverage for Clay platform-view tag
// routing. The host build links native_view_tags_empty.cc, so these assertions
// pin the "no platform-reserved tags, prefer built-in Clay views" behavior for
// those platforms. iOS / Android specific tag membership is compile-time
// selected in native_view_tags.cc / native_view_tags_android.cc and is
// asserted from the platform (iOS ObjC / Android) test targets instead.

#include "clay/ui/platform/native_view_tags.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace {

// On mac / windows / headless the InternalPlatformViewTags set is empty, so no
// tag is force-routed to a platform (XElement / native) view.
TEST(ClayPlatformViewTagSyncTest, HostHasNoInternalPlatformViewTags) {
  EXPECT_TRUE(InternalPlatformViewTags().empty());
  EXPECT_TRUE(InternalPlatformViewShadowNodeTags().empty());
  EXPECT_TRUE(InternalPlatformViewWithoutShadowNodeTags().empty());

  // Known reserved tags on iOS/Android must NOT be reserved on the host build.
  for (const char* tag : {"x-input-ng", "x-input", "input", "textarea",
                          "x-textarea", "x-map-ng", "x-video-engine"}) {
    EXPECT_EQ(InternalPlatformViewTags().count(tag), 0u)
        << "unexpected host platform-reserved tag: " << tag;
  }
}

// With no platform-reserved tags, the host must not silently fall back to a
// native view for an unknown tag; it prefers the built-in Clay view path.
TEST(ClayPlatformViewTagSyncTest, HostDoesNotForceNativeFallback) {
  EXPECT_FALSE(ShouldCreateFallbackNativeViewDirectly());
}

}  // namespace
}  // namespace clay
