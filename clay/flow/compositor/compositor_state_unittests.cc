// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/compositor/compositor_state.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

namespace testing {

using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::Key;

TEST(CompositorStateTest, AddCompositorOrder) {
  CompositorState compositor_state({64, 64});
  compositor_state.PrerollCompositeEmbeddedView(
      0, std::make_unique<EmbeddedViewParams>());
  compositor_state.PrerollCompositeEmbeddedView(
      1, std::make_unique<EmbeddedViewParams>());
  EXPECT_THAT(compositor_state.GetCompositionOrder(), ElementsAre(0, 1));
  EXPECT_THAT(compositor_state.GetViewParams(), Contains(Key(0)));
  EXPECT_THAT(compositor_state.GetViewParams(), Contains(Key(1)));
}

TEST(CompositorStateTest, GetSlicingCanvas) {
  CompositorState compositor_state({64, 64});
  compositor_state.PrerollCompositeEmbeddedView(
      0, std::make_unique<EmbeddedViewParams>());
  EXPECT_NE(compositor_state.CompositeEmbeddedView(0), nullptr);
}

}  // namespace testing

}  // namespace clay
