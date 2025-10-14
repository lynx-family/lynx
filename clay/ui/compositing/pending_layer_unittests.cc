// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/compositing/pending_layer.h"
#include "clay/ui/compositing/testing/mock_pending_layer.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

TEST(LayerTest, Simple) {
  auto layer = std::make_unique<MockPendingLayer>();

  EXPECT_FALSE(layer->PreviousSibling());
  EXPECT_FALSE(layer->NextSibling());
}

}  // namespace testing
}  // namespace clay
