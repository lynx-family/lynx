// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/testing/mock_shared_image_backing.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

TEST(MockSharedImageTests, QuerySize) {
  auto shared_image =
      fml::MakeRefCounted<MockSharedImageBacking>(skity::Vec2(100, 100));

  EXPECT_EQ(shared_image->GetSize(), skity::Vec2(100, 100));
}

TEST(MockSharedImageTests, QueryTransformation) {
  auto shared_image =
      fml::MakeRefCounted<MockSharedImageBacking>(skity::Vec2(100, 100));

  EXPECT_EQ(shared_image->GetTransformation(), skity::Matrix());

  skity::Matrix y_flip_mat = skity::Matrix(1, 0, 0, 0, -1, 1, 0, 0, 1);
  shared_image->SetTransformation(y_flip_mat);
  EXPECT_EQ(shared_image->GetTransformation(), y_flip_mat);
}

}  // namespace testing
}  // namespace clay
