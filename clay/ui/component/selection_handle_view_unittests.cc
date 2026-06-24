// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "base/include/fml/thread.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/selection_handle_view.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

TEST(SelectionHandleViewTest, SetSelectionHandleSizeUpdatesRadiusOnly) {
  auto thread = std::make_unique<fml::Thread>("ui");
  auto page_view =
      std::make_unique<PageView>(0, nullptr, thread->GetTaskRunner());
  auto handle = std::make_unique<SelectionHandleView>(
      page_view.get(), TextSelectionHandleType::kLeft);

  handle->SetSelectionHandleSize(20.f);
  EXPECT_EQ(handle->Width(), 0.f);
  EXPECT_EQ(handle->Height(), 0.f);

  constexpr float kLineHeight = 40.f;
  const FloatPoint offset(100.f, 200.f);
  handle->BuildSelectionHandle(kLineHeight, offset);

  const float old_radius = handle->GetSelectionHandleRadius();
  const float old_width = handle->Width();
  const float old_height = handle->Height();
  const float old_left = handle->Left();
  const float old_top = handle->Top();
  EXPECT_FLOAT_EQ(handle->Width(), 2 * old_radius);
  EXPECT_FLOAT_EQ(handle->Height(), kLineHeight + 2 * old_radius);

  handle->SetSelectionHandleSize(40.f);

  const float new_radius = handle->GetSelectionHandleRadius();
  EXPECT_GT(new_radius, old_radius);
  EXPECT_FLOAT_EQ(handle->Width(), old_width);
  EXPECT_FLOAT_EQ(handle->Height(), old_height);
  EXPECT_FLOAT_EQ(handle->Left(), old_left);
  EXPECT_FLOAT_EQ(handle->Top(), old_top);
}

TEST(SelectionHandleViewTest, SetSelectionHandleColorUpdatesBackground) {
  auto thread = std::make_unique<fml::Thread>("ui");
  auto page_view =
      std::make_unique<PageView>(0, nullptr, thread->GetTaskRunner());
  auto handle = std::make_unique<SelectionHandleView>(
      page_view.get(), TextSelectionHandleType::kLeft);

  constexpr Color kHandleColor(0xFFFF0000);
  handle->SetSelectionHandleColor(kHandleColor);

  ASSERT_TRUE(handle->render_object()->HasBackground());
  EXPECT_EQ(handle->render_object()->Background().background_color,
            kHandleColor);

  constexpr Color kTransparentRed(0x00FF0000);
  handle->SetSelectionHandleColor(kTransparentRed);

  ASSERT_TRUE(handle->render_object()->HasBackground());
  EXPECT_EQ(handle->render_object()->Background().background_color,
            Color::kBlue());
}

}  // namespace clay
