// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/public/style_types.h"
#include "clay/ui/component/css_property.h"
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
  DispatchDragEvent({10, 100}, {12, 0});
  EXPECT_GT(text_area->editable_scroll_->TotalScrollOffset().y(), 0);
  // because of the existence of touch_slop
  EXPECT_LT(text_area->editable_scroll_->TotalScrollOffset().y(), 100);

  EXPECT_EQ(text_area->editable_scroll_wrapper_->child_count(), 1u);
  text_area->SetAttribute("enable-scroll-bar", clay::Value(true));
  EXPECT_EQ(text_area->editable_scroll_wrapper_->child_count(), 2u);
  text_area->SetAttribute("enable-scroll-bar", clay::Value(false));
  EXPECT_EQ(text_area->editable_scroll_wrapper_->child_count(), 1u);
}

TEST_F_UI(TextAreaNGViewTest, enableScrollBar) {
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
  text_area->SetAttribute("enable-scroll-bar", clay::Value(true));
  text_area->SetBound(0, 0, 30, 200);
  Layout();

  EXPECT_FALSE(text_area->editable_scroll_wrapper_->IsLayoutRootCandidate());
  EXPECT_EQ(text_area->editable_scroll_wrapper_->GetOverflow(),
            CSSProperty::OVERFLOW_HIDDEN);
  EXPECT_EQ(text_area->editable_scroll_->GetOverflow(),
            CSSProperty::OVERFLOW_HIDDEN);

  text_area->SetAttribute(
      "overflow", clay::Value(static_cast<int>(ClayOverflowType::kVisible)));
  EXPECT_EQ(text_area->GetOverflow(), CSSProperty::OVERFLOW_XY);
  EXPECT_EQ(text_area->editable_scroll_wrapper_->GetOverflow(),
            CSSProperty::OVERFLOW_HIDDEN);
  EXPECT_EQ(text_area->editable_scroll_->GetOverflow(),
            CSSProperty::OVERFLOW_HIDDEN);

  DispatchDragEvent({10, 100}, {12, 0});
  EXPECT_GT(text_area->editable_scroll_->TotalScrollOffset().y(), 0);

  EXPECT_EQ(text_area->editable_scroll_wrapper_->child_count(), 2u);
  text_area->SetAttribute("enable-scroll-bar", clay::Value(false));
  EXPECT_EQ(text_area->editable_scroll_wrapper_->child_count(), 1u);
}

TEST_F_UI(TextAreaNGViewTest, invalidEditingRanges) {
  TextAreaNGView *text_area = new TextAreaNGView(-1, page_.get());
  page_->AddChild(text_area);
  text_area->SetBound(0, 0, 200, 200);

  text_area->editable_view_->UpdateEditingState(
      "a\xf0\x9f\x98\x80g", TextSelection(99, 100, Affinity::kDownstream),
      TextRange(1, 99), Affinity::kDownstream);

  const auto &value = text_area->editable_view_->GetTextEditingValue();
  EXPECT_EQ(value.GetU16Length(), 4u);
  EXPECT_EQ(value.selection(), TextRange(4));
  EXPECT_EQ(value.composing_range(), TextRange(0));
  EXPECT_FALSE(value.composing());
  Layout();
}

TEST_F_UI(TextAreaNGViewTest, platformHistoryActionBridge) {
  constexpr int kClientId = 101;
  TextAreaNGView *text_area = new TextAreaNGView(kClientId, page_.get());
  page_->AddChild(text_area);
  text_area->SetBound(0, 0, 200, 200);
  text_area->editable_view_->UpdateEditingState(
      "alpha", TextSelection(5, 5, Affinity::kDownstream), TextRange(0),
      Affinity::kDownstream);
  Layout();
  text_area->focus({}, [](LynxUIMethodResult, clay::Value) {});

  EXPECT_TRUE(page_->OnPlatformPerformTextInputHistoryAction(
      kClientId, TextInputHistoryAction::kUndo));
  EXPECT_EQ(text_area->editable_view_->GetTextEditingValue().GetText(), "");
  EXPECT_EQ(text_area->editable_view_->GetTextEditingValue().selection(),
            TextRange(0));
  EXPECT_TRUE(page_->OnPlatformPerformTextInputHistoryAction(
      kClientId, TextInputHistoryAction::kRedo));
  EXPECT_EQ(text_area->editable_view_->GetTextEditingValue().GetText(),
            "alpha");
  EXPECT_EQ(text_area->editable_view_->GetTextEditingValue().selection(),
            TextRange(5));
  EXPECT_FALSE(page_->OnPlatformPerformTextInputHistoryAction(
      kClientId + 1, TextInputHistoryAction::kUndo));
}

TEST(UndoStackTest, PreservesTextAndSelectionAcrossUndoRedo) {
  UndoStack stack;
  EXPECT_FALSE(stack.Undo().has_value());
  EXPECT_FALSE(stack.Redo().has_value());

  const TextEditingValue first("alpha", TextRange(5), TextRange(0), false,
                               Affinity::kDownstream);
  const TextEditingValue second("alpha beta", TextRange(6, 10), TextRange(0),
                                false, Affinity::kDownstream);
  stack.Push(first);
  stack.Push(second);

  const auto undone = stack.Undo();
  ASSERT_TRUE(undone.has_value());
  EXPECT_EQ(*undone, first);
  const auto redone = stack.Redo();
  ASSERT_TRUE(redone.has_value());
  EXPECT_EQ(*redone, second);
  const auto redo_at_end = stack.Redo();
  ASSERT_TRUE(redo_at_end.has_value());
  EXPECT_EQ(*redo_at_end, second);
}

};  // namespace clay
