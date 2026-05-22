// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/compositing/pending_container_layer.h"
#include "clay/ui/compositing/testing/mock_pending_layer.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

TEST(PendingContainerLayerTest, Simple) {
  // Empty layer tree.
  auto root_layer = std::make_unique<PendingContainerLayer>();

  EXPECT_FALSE(root_layer->HasChildren());

  // Add a child.
  auto child_layer = new MockPendingLayer();
  EXPECT_EQ(child_layer->Parent(), nullptr);

  root_layer->AppendChild(child_layer);
  EXPECT_TRUE(root_layer->HasChildren());
  EXPECT_EQ(root_layer->FirstChild(), root_layer->LastChild());
  EXPECT_EQ(child_layer->Parent(), root_layer.get());

  // Remove a child.
  root_layer->RemoveChild(child_layer);
  EXPECT_FALSE(root_layer->HasChildren());
}

}  // namespace testing
}  // namespace clay
