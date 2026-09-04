// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/imperative_animation_metadata.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

TEST(ImperativeAnimationMetadataTest, StartUsesSourceSpecificIdentity) {
  ImperativeAnimationMetadata metadata;
  metadata.RecordStart(ImperativeAnimationSource::kAnimate, "first-js",
                       "first-animation");
  metadata.RecordStart(ImperativeAnimationSource::kAnimate, "second-js",
                       "second-animation");
  EXPECT_FALSE(metadata.HasAnimationName("first-animation"));
  EXPECT_TRUE(metadata.HasAnimationName("second-animation"));

  metadata.RecordStart(ImperativeAnimationSource::kAnimateV2, "first-js",
                       "first-animation");
  metadata.RecordStart(ImperativeAnimationSource::kAnimateV2, "second-js",
                       "second-animation");
  metadata.RecordStart(ImperativeAnimationSource::kAnimateV2, "first-js",
                       "first-animation-updated");

  EXPECT_FALSE(metadata.HasAnimationName("first-animation"));
  EXPECT_TRUE(metadata.HasAnimationName("first-animation-updated"));
  EXPECT_TRUE(metadata.HasAnimationName("second-animation"));
}

TEST(ImperativeAnimationMetadataTest, EndAndClearRemoveRecords) {
  ImperativeAnimationMetadata metadata;
  metadata.RecordStart(ImperativeAnimationSource::kAnimateV2, "cancel-js",
                       "cancel-animation");
  metadata.RecordStart(ImperativeAnimationSource::kAnimateV2, "finish-js",
                       "finish-animation");

  metadata.Cancel(ImperativeAnimationSource::kAnimateV2, "cancel-js");
  metadata.Finish(ImperativeAnimationSource::kAnimateV2, "finish-animation");

  EXPECT_FALSE(metadata.HasAnimationName("cancel-animation"));
  EXPECT_FALSE(metadata.HasAnimationName("finish-animation"));
  metadata.RecordStart(ImperativeAnimationSource::kAnimate, "animate-js",
                       "animate-animation");
  metadata.RecordStart(ImperativeAnimationSource::kAnimateV2, "animate-v2-js",
                       "animate-v2-animation");

  metadata.Clear();

  EXPECT_FALSE(metadata.HasAnimationName("animate-animation"));
  EXPECT_FALSE(metadata.HasAnimationName("animate-v2-animation"));
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
