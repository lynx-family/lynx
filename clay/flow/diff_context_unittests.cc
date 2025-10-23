// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/testing/diff_context_test.h"

namespace clay {
namespace testing {

TEST_F(DiffContextTest, ClipAlignment) {
  MockLayerTree t1;
  t1.root()->Add(CreatePictureLayer(
      CreatePicture(skity::Rect::MakeLTRB(30, 30, 50, 50), 1)));
  auto damage =
      DiffLayerTree(t1, MockLayerTree(), skity::Rect::MakeEmpty(), 0, 0);
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(30, 30, 50, 50));
  EXPECT_EQ(damage.buffer_damage, skity::Rect::MakeLTRB(30, 30, 50, 50));

  damage = DiffLayerTree(t1, MockLayerTree(), skity::Rect::MakeEmpty(), 1, 1);
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(30, 30, 50, 50));
  EXPECT_EQ(damage.buffer_damage, skity::Rect::MakeLTRB(30, 30, 50, 50));

  damage = DiffLayerTree(t1, MockLayerTree(), skity::Rect::MakeEmpty(), 8, 1);
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(24, 30, 56, 50));
  EXPECT_EQ(damage.buffer_damage, skity::Rect::MakeLTRB(24, 30, 56, 50));

  damage = DiffLayerTree(t1, MockLayerTree(), skity::Rect::MakeEmpty(), 1, 8);
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(30, 24, 50, 56));
  EXPECT_EQ(damage.buffer_damage, skity::Rect::MakeLTRB(30, 24, 50, 56));

  damage = DiffLayerTree(t1, MockLayerTree(), skity::Rect::MakeEmpty(), 16, 16);
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(16, 16, 64, 64));
  EXPECT_EQ(damage.buffer_damage, skity::Rect::MakeLTRB(16, 16, 64, 64));
}

}  // namespace testing
}  // namespace clay
