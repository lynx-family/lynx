// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <string>

#include "base/include/fml/message_loop.h"
#include "clay/ui/component/editable/input_view.h"
#include "clay/ui/resource/font_collection.h"
#include "clay/ui/testing/ui_test.h"
#include "gtest/gtest.h"

namespace clay {

class EditableViewTest : public UITest {
 protected:
  void UISetUp() override {}

  void LoadDataUriFont(const std::string& family_name) {
    FontCollection::Instance()->PreLoadFontOnMem(
        fml::MessageLoop::GetCurrent().GetTaskRunner(), nullptr, nullptr,
        family_name, {"data:font/ttf;base64,AA=="});
  }
};

TEST_F_UI(EditableViewTest, AsyncFontLoadRequestsRelayout) {
  const std::string family_name = "editable_font_loaded_after_layout";
  auto* input = new InputView(-1, page_.get());
  page_->AddChild(input);
  input->SetContent("existing content");
  input->SetFontFamily(family_name);
  input->SetBound(0, 0, 200, 80);

  Layout();
  ASSERT_FALSE(input->NeedsLayout());

  LoadDataUriFont(family_name);

  EXPECT_TRUE(input->NeedsLayout());
}

}  // namespace clay
