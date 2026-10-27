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
    auto task_runner = fml::MessageLoop::GetCurrent().GetTaskRunner();
    FontCollection::Instance()->PreLoadFontOnMem(task_runner, task_runner,
                                                 nullptr, nullptr, family_name,
                                                 {"data:font/ttf;base64,AA=="});
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

TEST_F_UI(EditableViewTest, InputDeletionRespectsLengthAndSingleDeleteAction) {
  auto* input = new InputView(1, page_.get());
  page_->AddChild(input);
  input->SetContent("abcd");
  int callbacks = 0;
  auto expect_success = [&callbacks](LynxUIMethodResult code,
                                     const clay::Value&) {
    EXPECT_EQ(code, LynxUIMethodResult::kSuccess);
    ++callbacks;
  };
  InvokeUIMethod(
      input, "setSelectionRange",
      {{"selectionStart", clay::Value(4)}, {"selectionEnd", clay::Value(4)}},
      expect_success);
  InvokeUIMethod(input, "sendDelEvent",
                 {{"action", clay::Value(0)}, {"length", clay::Value(2)}},
                 expect_success);
  EXPECT_EQ(input->GetTextEditingValue().GetText(), "ab");
  InvokeUIMethod(input, "sendDelEvent",
                 {{"action", clay::Value(1)}, {"length", clay::Value(99)}},
                 expect_success);
  EXPECT_EQ(input->GetTextEditingValue().GetText(), "a");
  EXPECT_EQ(callbacks, 3);
}

TEST_F_UI(EditableViewTest, InputSelectionUsesUtf16Offsets) {
  auto* input = new InputView(1, page_.get());
  page_->AddChild(input);
  input->SetContent(
      "A\xF0\x9F\x98\x80"
      "B");
  int callbacks = 0;
  InvokeUIMethod(
      input, "setSelectionRange",
      {{"selectionStart", clay::Value(1)}, {"selectionEnd", clay::Value(3)}},
      [&callbacks](LynxUIMethodResult code, const clay::Value&) {
        EXPECT_EQ(code, LynxUIMethodResult::kSuccess);
        ++callbacks;
      });
  EXPECT_EQ(callbacks, 1);
  EXPECT_EQ(input->GetTextEditingValue().selection().base(), 1u);
  EXPECT_EQ(input->GetTextEditingValue().selection().extent(), 3u);
  EXPECT_EQ(input->GetTextEditingValue().GetText(),
            "A\xF0\x9F\x98\x80"
            "B");
}

TEST_F_UI(EditableViewTest, InputKeyboardFocusAndBlurActions) {
  auto* input = new InputView(1, page_.get());
  page_->AddChild(input);
  int callbacks = 0;
  auto expect_success = [&callbacks](LynxUIMethodResult code,
                                     const clay::Value&) {
    EXPECT_EQ(code, LynxUIMethodResult::kSuccess);
    ++callbacks;
  };
  InvokeUIMethod(input, "controlKeyBoard", {{"action", clay::Value(2)}},
                 expect_success);
  EXPECT_TRUE(input->IsFocused());
  InvokeUIMethod(input, "controlKeyBoard", {{"action", clay::Value(3)}},
                 expect_success);
  EXPECT_FALSE(input->IsFocused());
  EXPECT_EQ(callbacks, 2);
}

TEST_F_UI(EditableViewTest, InputConfirmDeliversExactValueOnce) {
  auto* input = new InputView(1, page_.get());
  page_->AddChild(input);
  input->SetContent("confirmed value");
  input->SetKeyboardAction(KeyboardAction::kDone);
  int events = 0;
  custom_event_callback_ = [&events](int view_id, const char* event_name,
                                     clay::Value::Map params) {
    EXPECT_EQ(view_id, 1);
    EXPECT_STREQ(event_name, event_attr::kEventConfirm);
    ASSERT_EQ(params.size(), 1u);
    ASSERT_NE(params.find("value"), params.end());
    EXPECT_EQ(params.at("value").GetString(), "confirmed value");
    ++events;
  };
  input->PerformAction();
  EXPECT_EQ(events, 1);
}

}  // namespace clay
