// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/common/graphics/drawable_image.h"
#include "clay/flow/testing/mock_drawable_image.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

TEST(DrawableImageRegistryTest, UnregisterDrawableImageCallbackTriggered) {
  DrawableImageRegistry registry;
  auto mock_image1 = std::make_shared<MockDrawableImage>();
  int64_t id1 = mock_image1->Id();
  auto mock_image2 = std::make_shared<MockDrawableImage>();
  int64_t id2 = mock_image2->Id();

  registry.RegisterDrawableImage(mock_image1);
  registry.RegisterDrawableImage(mock_image2);
  ASSERT_EQ(registry.GetDrawableImage(id1), mock_image1);
  ASSERT_EQ(registry.GetDrawableImage(id2), mock_image2);
  ASSERT_FALSE(mock_image1->unregistered());
  ASSERT_FALSE(mock_image2->unregistered());

  registry.UnregisterDrawableImage(id1);
  ASSERT_EQ(registry.GetDrawableImage(id1), nullptr);
  ASSERT_TRUE(mock_image1->unregistered());
  ASSERT_FALSE(mock_image2->unregistered());

  registry.UnregisterDrawableImage(id2);
  ASSERT_EQ(registry.GetDrawableImage(id2), nullptr);
  ASSERT_TRUE(mock_image1->unregistered());
  ASSERT_TRUE(mock_image2->unregistered());
}

TEST(DrawableImageRegistryTest, GrContextCallbackTriggered) {
  DrawableImageRegistry registry;
  auto mock_image1 = std::make_shared<MockDrawableImage>();
  int64_t id1 = mock_image1->Id();
  auto mock_image2 = std::make_shared<MockDrawableImage>();

  registry.RegisterDrawableImage(mock_image1);
  registry.RegisterDrawableImage(mock_image2);
  ASSERT_FALSE(mock_image1->gr_context_created());
  ASSERT_FALSE(mock_image2->gr_context_created());
  ASSERT_FALSE(mock_image1->gr_context_destroyed());
  ASSERT_FALSE(mock_image2->gr_context_destroyed());

  registry.OnGrContextCreated();
  ASSERT_TRUE(mock_image1->gr_context_created());
  ASSERT_TRUE(mock_image2->gr_context_created());

  registry.UnregisterDrawableImage(id1);
  registry.OnGrContextDestroyed();
  ASSERT_FALSE(mock_image1->gr_context_destroyed());
  ASSERT_TRUE(mock_image2->gr_context_created());
}

}  // namespace testing
}  // namespace clay
