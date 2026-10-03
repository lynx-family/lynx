// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// Priority coverage for the Clay video-engine element tag resolution. The
// decision is extracted into the pure ResolveVideoEngineNodeTag() so the
// platform priority can be asserted from a host build without a live
// PropBundle or a platform-specific compile.

#include "clay/lynx_adaptor/video_engine_tag_priority.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace {

// iOS (Clay C++ video enabled): the video-engine element defaults to the Clay
// built-in clay-video-engine view. This is the primary invariant the product
// requires: iOS video uses Clay, not the platform view.
TEST(ClayVideoEngineTagPriorityTest, IOSDefaultsToClayBuiltinVideo) {
  EXPECT_EQ(ResolveVideoEngineNodeTag("x-video-engine",
                                      /*clay_cpp_video_enabled=*/true,
                                      /*use_platform_video=*/false),
            "clay-video-engine");
}

// iOS opt-out: an element that explicitly requests platform video keeps the
// platform x-video-engine view.
TEST(ClayVideoEngineTagPriorityTest, IOSKeepsPlatformVideoWhenOptedIn) {
  EXPECT_EQ(ResolveVideoEngineNodeTag("x-video-engine",
                                      /*clay_cpp_video_enabled=*/true,
                                      /*use_platform_video=*/true),
            "x-video-engine");
}

// Where Clay C++ video is not compiled in (e.g. Android, which reuses the
// XElement platform video), the video-engine tag is never rewritten.
TEST(ClayVideoEngineTagPriorityTest, NonClayCppVideoKeepsPlatformTag) {
  EXPECT_EQ(ResolveVideoEngineNodeTag("x-video-engine",
                                      /*clay_cpp_video_enabled=*/false,
                                      /*use_platform_video=*/false),
            "x-video-engine");
  EXPECT_EQ(ResolveVideoEngineNodeTag("x-video-engine",
                                      /*clay_cpp_video_enabled=*/false,
                                      /*use_platform_video=*/true),
            "x-video-engine");
}

// Any non video-engine tag is returned unchanged regardless of the flags, so
// the priority never disturbs other platform tags such as x-webview or input.
TEST(ClayVideoEngineTagPriorityTest, UnrelatedTagsPassThrough) {
  for (bool clay_cpp : {false, true}) {
    for (bool platform : {false, true}) {
      EXPECT_EQ(
          ResolveVideoEngineNodeTag("clay-video-engine", clay_cpp, platform),
          "clay-video-engine");
      EXPECT_EQ(ResolveVideoEngineNodeTag("x-webview", clay_cpp, platform),
                "x-webview");
      EXPECT_EQ(ResolveVideoEngineNodeTag("x-input-ng", clay_cpp, platform),
                "x-input-ng");
    }
  }
}

}  // namespace
}  // namespace lynx
