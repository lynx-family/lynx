// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/editable/textarea_ng_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/lynx_module/lynx_ui_method_types.h"
#include "clay/ui/lynx_module/types.h"
#include "clay/ui/testing/ui_test.h"
#include "gtest/gtest.h"

namespace clay {

class TextAreaNGViewTest : public UITest {
  void UISetUp() override {}
};

TEST_F_UI(TextAreaNGViewTest, scroll) {
  TextAreaNGView *text_area = new TextAreaNGView(-1, page_.get());
  page_->AddChild(text_area);
  const char *a_long_cstr =
      "hello world hello world hello world hello world"
      "hello world hello world hello world hello world"
      "hello world hello world hello world hello world"
      "hello world hello world hello world hello world";
  LynxModuleValues params;
  params.names.emplace_back("value");
  params.values.emplace_back(a_long_cstr);
  text_area->setValue(params, [](LynxUIMethodResult code, clay::Value data) {});
  text_area->SetBound(0, 0, 30, 200);
  Layout();
  DispatchDragEvent({20, 100}, {22, 0});
  EXPECT_GT(text_area->editable_scroll_->TotalScrollOffset().y(), 0);
  // because of the existence of touch_slop
  EXPECT_LT(text_area->editable_scroll_->TotalScrollOffset().y(), 100);
}

};  // namespace clay
