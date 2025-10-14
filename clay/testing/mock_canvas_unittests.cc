// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/testing/canvas_test.h"
#include "clay/testing/mock_canvas.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

using MockCanvasTest = CanvasTest;

TEST_F(MockCanvasTest, DrawCalls) {
  const SkRect rect = SkRect::MakeWH(5.0f, 5.0f);
  const SkPaint paint = SkPaint(SkColors::kGreen);
  const auto expected_draw_calls = std::vector{
      MockCanvas::DrawCall{0, MockCanvas::DrawRectData{rect, paint}}};

  mock_canvas().drawRect(rect, paint);
  EXPECT_EQ(mock_canvas().draw_calls(), expected_draw_calls);
}

}  // namespace testing
}  // namespace clay
