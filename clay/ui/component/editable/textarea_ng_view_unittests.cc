// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/public/style_types.h"
#include "clay/ui/common/input_client_manager.h"
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

TEST_F_UI(TextAreaNGViewTest, platformTextInputSelectorBridge) {
  constexpr int kClientId = 101;
  TextAreaNGView *text_area = new TextAreaNGView(kClientId, page_.get());
  page_->AddChild(text_area);
  text_area->SetBound(0, 0, 200, 200);
  text_area->editable_view_->UpdateEditingState(
      "alpha beta", TextSelection(5, 5, Affinity::kDownstream), TextRange(0),
      Affinity::kDownstream);
  Layout();
  text_area->focus({}, [](LynxUIMethodResult, clay::Value) {});

  InputClientManager::TextInputCallback callback;
  callback.on_perform_selector =
      [editable = text_area->editable_view_](const std::string &selector) {
        return editable->PerformSelector(selector);
      };
  page_->GetInputClientManager()->AddClientCallback(kClientId, callback);

  EXPECT_TRUE(
      page_->OnPlatformPerformTextInputSelector(kClientId, "selectAll:"));
  EXPECT_EQ(text_area->editable_view_->GetTextEditingValue().selection(),
            TextRange(0, 10));
  EXPECT_FALSE(page_->OnPlatformPerformTextInputSelector(
      kClientId, "unsupportedSelector:"));
}

TEST_F_UI(TextAreaNGViewTest, moveByWordBoundarySkipsPunctuationAndSpaces) {
  constexpr int kClientId = 102;
  TextAreaNGView *text_area = new TextAreaNGView(kClientId, page_.get());
  page_->AddChild(text_area);
  text_area->SetBound(0, 0, 200, 200);
  text_area->editable_view_->UpdateEditingState(
      "one,  two\nthree", TextSelection(3, 3, Affinity::kDownstream),
      TextRange(0), Affinity::kDownstream);
  Layout();
  text_area->focus({}, [](LynxUIMethodResult, clay::Value) {});

  EXPECT_TRUE(text_area->editable_view_->PerformSelector("moveWordRight:"));
  EXPECT_EQ(text_area->editable_view_->GetTextEditingValue().selection(),
            TextRange(9));
  EXPECT_TRUE(text_area->editable_view_->PerformSelector("moveWordLeft:"));
  EXPECT_EQ(text_area->editable_view_->GetTextEditingValue().selection(),
            TextRange(6));
  EXPECT_TRUE(text_area->editable_view_->PerformSelector("moveWordLeft:"));
  EXPECT_EQ(text_area->editable_view_->GetTextEditingValue().selection(),
            TextRange(0));

  text_area->editable_view_->UpdateEditingState(
      "one,  two\nthree", TextSelection(3, 3, Affinity::kDownstream),
      TextRange(0), Affinity::kDownstream);
  EXPECT_TRUE(text_area->editable_view_->PerformSelector(
      "moveWordRightAndModifySelection:"));
  EXPECT_EQ(text_area->editable_view_->GetTextEditingValue().selection(),
            TextRange(3, 9));
}

};  // namespace clay
