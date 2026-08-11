// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstdint>
#include <memory>

#include "clay/ui/component/scrollbar/scrollbar_view.h"
#include "clay/ui/testing/ui_test.h"

namespace clay {

class ScrollbarViewTest : public UITest {};

TEST_F_UI(ScrollbarViewTest, FirstMouseEnterAppliesThumbHoverColor) {
  auto scrollbar = std::make_unique<ScrollbarView>(page_.get());
  ASSERT_EQ(scrollbar->child_count(), 1u);

  constexpr uint32_t kHoverColor = 0xff123456;
  scrollbar->SetAttribute("scroll-bar-thumb-hover-color", Value("#123456"));

  BaseView* thumb = scrollbar->GetChildren().front();
  thumb->OnMouseEnter(PointerEvent(PointerEvent::EventType::kHoverEvent));

  EXPECT_EQ(thumb->render_object()->Background().background_color,
            Color(kHoverColor));
}

}  // namespace clay
