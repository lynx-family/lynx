// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <string>

#include "clay/public/layout_delegate.h"
#include "clay/ui/common/measure_constraint.h"
#include "clay/ui/component/text/raw_text_view.h"
#include "clay/ui/shadow/inline_text_shadow_node.h"
#include "clay/ui/shadow/inline_truncation_shadow_node.h"
#include "clay/ui/shadow/inline_view_shadow_node.h"
#include "clay/ui/shadow/raw_text_shadow_node.h"
#include "clay/ui/shadow/shadow_node_owner.h"
#include "clay/ui/shadow/text_shadow_node.h"
#include "clay/ui/testing/ui_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
class TestLayoutDelegate : public LayoutDelegate {
 public:
  void OnTriggerLayout() override {}
  void OnMarkDirty(int32_t) override {}
  void OnAlignNativeNode(int32_t id, float top, float left) override {
    align_count++;
    last_aligned_id = id;
    last_top = top;
    last_left = left;
  }
  ClayMeasureOutput OnMeasureNativeNode(int32_t, float, int, float,
                                        int) override {
    return {measure_width, measure_height, 0.f};
  }
  ClayLayoutStyles OnGetLayoutStyles(int32_t) override {
    return ClayLayoutStyles();
  }

  void ResetAlignState() {
    align_count = 0;
    last_aligned_id = 0;
    last_top = -1.f;
    last_left = -1.f;
  }

  float measure_width = 0.f;
  float measure_height = 0.f;
  int align_count = 0;
  int32_t last_aligned_id = 0;
  float last_top = -1.f;
  float last_left = -1.f;
};

class TextTest : public UITest {
 protected:
  void UISetUp() override {
    owner_ =
        new ShadowNodeOwner(fml::MessageLoop::GetCurrent().GetTaskRunner());
    text_shadow_node_ =
        std::make_unique<TextShadowNode>(owner_, std::string("text"), -1);
    raw_text_shadow_node_ = std::make_unique<RawTextShadowNode>(
        owner_, std::string("raw_text"), -1);
    inline_text_shadow_node_ = std::make_unique<InlineTextShadowNode>(
        owner_, std::string("inline-text"), -1);
    text_shadow_node_->AddChild(raw_text_shadow_node_.get());
    text_shadow_node_->AddChild(inline_text_shadow_node_.get());
    text_shadow_node_->SetFontSize(42);
  }
  void UITearDown() override {
    text_shadow_node_.reset();
    raw_text_shadow_node_.reset();
    inline_text_shadow_node_.reset();
    delete owner_;
  }
  ShadowNodeOwner* owner_;
  std::unique_ptr<TextShadowNode> text_shadow_node_;
  std::unique_ptr<InlineTextShadowNode> inline_text_shadow_node_;
  std::unique_ptr<RawTextShadowNode> raw_text_shadow_node_;
};

TEST_F_UI(TextTest, AutoFontSizeStepGranularity) {
  MeasureConstraint constraint{1000, MeasureMode::kDefinite, 100,
                               MeasureMode::kDefinite};
  text_shadow_node_->enable_auto_font_size_ = true;
  text_shadow_node_->auto_font_size_max_size_ = 50;
  text_shadow_node_->auto_font_size_min_size_ = 30;
  text_shadow_node_->auto_font_size_step_granularity_ = 3;
  std::string text = std::string(
      "Hello, Compiler NG Hello, Compiler NG Hello, Compiler NG Hello, "
      "Compiler NG Hello, Compiler NG Hello, Compiler NG ");
  raw_text_shadow_node_->SetText(text);
  text_shadow_node_->Measure(constraint);
  auto font_size = text_shadow_node_->text_style_->font_size;
  EXPECT_NE(font_size, 42);
  constraint.width = 4000;
  constraint.height = 1000;
  text_shadow_node_->Measure(constraint);
  EXPECT_NE(text_shadow_node_->text_style_->font_size, font_size);
}

TEST_F_UI(TextTest, AutoFontSizeIgnoresInvalidStepGranularity) {
  MeasureConstraint constraint{4000, MeasureMode::kDefinite, 1000,
                               MeasureMode::kDefinite};
  text_shadow_node_->enable_auto_font_size_ = true;
  text_shadow_node_->auto_font_size_max_size_ = 50;
  text_shadow_node_->auto_font_size_min_size_ = 30;
  text_shadow_node_->auto_font_size_step_granularity_ = 0;
  raw_text_shadow_node_->SetText("Hello");

  text_shadow_node_->Measure(constraint);

  EXPECT_EQ(text_shadow_node_->text_style_->font_size, 42);
}

TEST_F_UI(TextTest, AutoFontSizeAllowsUnsetMaxSize) {
  MeasureConstraint constraint{1000, MeasureMode::kDefinite, 100,
                               MeasureMode::kDefinite};
  text_shadow_node_->enable_auto_font_size_ = true;
  text_shadow_node_->auto_font_size_min_size_ = 10;
  text_shadow_node_->auto_font_size_max_size_ = 0;
  text_shadow_node_->auto_font_size_step_granularity_ = 1;
  raw_text_shadow_node_->SetText(
      "Hello, Compiler NG Hello, Compiler NG Hello, Compiler NG Hello, "
      "Compiler NG Hello, Compiler NG Hello, Compiler NG ");

  text_shadow_node_->Measure(constraint);

  EXPECT_LT(text_shadow_node_->text_style_->font_size, 42);
}

TEST_F_UI(TextTest, AutoFontSizeIgnoresUnsetMinSize) {
  MeasureConstraint constraint{100, MeasureMode::kDefinite, 1,
                               MeasureMode::kDefinite};
  text_shadow_node_->enable_auto_font_size_ = true;
  text_shadow_node_->auto_font_size_min_size_ = 0;
  text_shadow_node_->auto_font_size_max_size_ = 0;
  text_shadow_node_->auto_font_size_step_granularity_ = 1;
  raw_text_shadow_node_->SetText(
      "Hello, Compiler NG Hello, Compiler NG Hello, Compiler NG Hello, "
      "Compiler NG Hello, Compiler NG Hello, Compiler NG ");

  text_shadow_node_->Measure(constraint);

  EXPECT_EQ(text_shadow_node_->text_style_->font_size, 42);
}

TEST_F_UI(TextTest, AutoFontSizeDoesNotShrinkBelowMinSize) {
  MeasureConstraint constraint{100, MeasureMode::kDefinite, 1,
                               MeasureMode::kDefinite};
  text_shadow_node_->enable_auto_font_size_ = true;
  text_shadow_node_->auto_font_size_min_size_ = 10;
  text_shadow_node_->auto_font_size_max_size_ = 0;
  text_shadow_node_->auto_font_size_step_granularity_ = 3;
  raw_text_shadow_node_->SetText(
      "Hello, Compiler NG Hello, Compiler NG Hello, Compiler NG Hello, "
      "Compiler NG Hello, Compiler NG Hello, Compiler NG ");

  text_shadow_node_->Measure(constraint);

  EXPECT_GE(text_shadow_node_->text_style_->font_size, 10);
}

TEST_F_UI(TextTest, AutoFontSizeIgnoresInlineTruncation) {
  MeasureConstraint constraint{1000, MeasureMode::kDefinite, 100,
                               MeasureMode::kDefinite};
  text_shadow_node_->enable_auto_font_size_ = true;
  text_shadow_node_->auto_font_size_min_size_ = 10;
  text_shadow_node_->auto_font_size_max_size_ = 0;
  text_shadow_node_->auto_font_size_step_granularity_ = 1;
  text_shadow_node_->SetTextMaxLine(1);
  raw_text_shadow_node_->SetText(
      "Hello, Compiler NG Hello, Compiler NG Hello, Compiler NG Hello, "
      "Compiler NG Hello, Compiler NG Hello, Compiler NG ");
  auto inline_truncation_node = std::make_unique<InlineTruncationShadowNode>(
      owner_, std::string("inline-truncation"), -1);
  auto inline_text_node = std::make_unique<InlineTextShadowNode>(
      owner_, std::string("inline-text"), -1);
  auto inline_raw_text_node =
      std::make_unique<RawTextShadowNode>(owner_, std::string("raw-text"), -1);
  inline_raw_text_node->SetText("...");
  inline_text_node->AddChild(inline_raw_text_node.get());
  inline_truncation_node->AddChild(inline_text_node.get());
  text_shadow_node_->AddChild(inline_truncation_node.get());

  text_shadow_node_->Measure(constraint);

  EXPECT_EQ(text_shadow_node_->text_style_->font_size, 42);
}

TEST_F_UI(TextTest, DISABLED_AutoFontSizePreset) {
  MeasureConstraint constraint{1000, MeasureMode::kDefinite, 100,
                               MeasureMode::kDefinite};
  text_shadow_node_->enable_auto_font_size_ = true;
  text_shadow_node_->auto_font_size_max_size_ = 50;
  text_shadow_node_->auto_font_size_min_size_ = 30;
  text_shadow_node_->auto_font_size_step_granularity_ = 3;
  text_shadow_node_->auto_font_size_preset_sizes_ = std::vector<double>{40, 45};
  std::string text = std::string(
      "Hello, Compiler NG Hello, Compiler NG Hello, Compiler NG Hello, "
      "Compiler NG Hello, Compiler NG Hello, Compiler NG ");
  raw_text_shadow_node_->SetText(text);
  text_shadow_node_->Measure(constraint);
  auto font_size = text_shadow_node_->text_style_->font_size;
  EXPECT_EQ(font_size, 40);
}

TEST_F_UI(TextTest, DISABLED_InlineTruncation) {
  MeasureConstraint constraint{1080, MeasureMode::kDefinite, 100,
                               MeasureMode::kDefinite};
  auto inline_truncation_node = std::make_unique<InlineTruncationShadowNode>(
      owner_, std::string("inline-truncation"), -1);
  auto inline_text_node = std::make_unique<InlineTextShadowNode>(
      owner_, std::string("inline-text"), -1);
  auto inline_raw_text_shadow_node =
      std::make_unique<RawTextShadowNode>(owner_, std::string("raw-text"), -1);
  inline_raw_text_shadow_node->SetText("extend");
  inline_text_node->AddChild(inline_raw_text_shadow_node.get());
  inline_truncation_node->AddChild(inline_text_node.get());
  text_shadow_node_->AddChild(inline_truncation_node.get());
  text_shadow_node_->SetTextMaxLine(1);
  std::string text = std::string(
      "This is a test text. We will use this text to test the function of "
      "inline-truncation. If this text exceeds the limited width, it will be "
      "truncated.");
  raw_text_shadow_node_->SetText(text);
  text_shadow_node_->Measure(constraint);
  int truncation_text_size = text_shadow_node_->GetRawText().size();
  EXPECT_NE(truncation_text_size, 147);
}

TEST_F_UI(TextTest, VerticalAlign) {
  MeasureConstraint constraint{1000, MeasureMode::kDefinite, 100,
                               MeasureMode::kDefinite};
  inline_text_shadow_node_->SetVerticalAlign(
      VerticalAlignType::kVerticalAlignLength, 20);
  std::string text = std::string("test");
  raw_text_shadow_node_->SetText(text);
  text_shadow_node_->Measure(constraint);
  auto baseline_shift = inline_text_shadow_node_->text_style_->baseline_shift;
  EXPECT_EQ(baseline_shift, 20);
}

TEST_F_UI(TextTest, AlignInlineViewsToOriginResetsInlineViewState) {
  TestLayoutDelegate delegate;
  owner_->SetLayoutDelegate(&delegate);
  auto inline_view_node =
      std::make_unique<InlineViewShadowNode>(owner_, std::string("view"), 7);
  inline_view_node->SetEndIndex(0);
  inline_text_shadow_node_->AddChild(inline_view_node.get());

  inline_text_shadow_node_->AlignInlineViewsToOrigin();

  EXPECT_EQ(delegate.align_count, 1);
  EXPECT_EQ(delegate.last_aligned_id, 7);
  EXPECT_EQ(delegate.last_top, 0.f);
  EXPECT_EQ(delegate.last_left, 0.f);
  EXPECT_EQ(inline_view_node->placeholder_index(), -1);
  EXPECT_EQ(inline_view_node->StartGlyph(), 0u);
  EXPECT_EQ(inline_view_node->EndGlyph(), 0u);
  owner_->SetLayoutDelegate(nullptr);
}

TEST_F_UI(TextTest, HiddenTextLayoutAlignsInlineViewToOrigin) {
  TestLayoutDelegate delegate;
  owner_->SetLayoutDelegate(&delegate);
  auto inline_view_node =
      std::make_unique<InlineViewShadowNode>(owner_, std::string("view"), 7);
  inline_text_shadow_node_->AddChild(inline_view_node.get());

  text_shadow_node_->OnLayout(0.f, TextMeasureMode::kDefinite, 0.f,
                              TextMeasureMode::kDefinite, {0.f, 0.f, 0.f, 0.f},
                              {0.f, 0.f, 0.f, 0.f});

  EXPECT_EQ(delegate.align_count, 1);
  EXPECT_EQ(delegate.last_aligned_id, 7);
  EXPECT_EQ(delegate.last_top, 0.f);
  EXPECT_EQ(delegate.last_left, 0.f);
  EXPECT_EQ(inline_view_node->placeholder_index(), -1);
  EXPECT_EQ(inline_view_node->StartGlyph(), 0u);
  EXPECT_EQ(inline_view_node->EndGlyph(), 0u);
  owner_->SetLayoutDelegate(nullptr);
}

TEST_F_UI(TextTest, RebuildsInlineViewLayoutAfterHiddenToggle) {
  TestLayoutDelegate delegate;
  delegate.measure_width = 10.f;
  delegate.measure_height = 8.f;
  owner_->SetLayoutDelegate(&delegate);
  raw_text_shadow_node_->SetText("+25");
  auto inline_view_node =
      std::make_unique<InlineViewShadowNode>(owner_, std::string("view"), 7);
  inline_view_node->EnsureDefaultStyle();
  inline_text_shadow_node_->AddChild(inline_view_node.get());

  MeasureConstraint constraint{100.f, MeasureMode::kDefinite, 100.f,
                               MeasureMode::kDefinite};
  auto result = text_shadow_node_->Measure(constraint);
  text_shadow_node_->OnLayout(result.width, TextMeasureMode::kDefinite,
                              result.height, TextMeasureMode::kDefinite,
                              {0.f, 0.f, 0.f, 0.f}, {0.f, 0.f, 0.f, 0.f});
  text_shadow_node_->OnLayout(0.f, TextMeasureMode::kDefinite, 0.f,
                              TextMeasureMode::kDefinite, {0.f, 0.f, 0.f, 0.f},
                              {0.f, 0.f, 0.f, 0.f});
  EXPECT_EQ(delegate.last_aligned_id, 7);
  EXPECT_EQ(delegate.last_top, 0.f);
  EXPECT_EQ(delegate.last_left, 0.f);

  delegate.ResetAlignState();
  text_shadow_node_->Measure(constraint);
  text_shadow_node_->Align();

  EXPECT_EQ(delegate.align_count, 1);
  EXPECT_EQ(delegate.last_aligned_id, 7);
  EXPECT_GT(delegate.last_left, 0.f);
  owner_->SetLayoutDelegate(nullptr);
}

}  // namespace clay
