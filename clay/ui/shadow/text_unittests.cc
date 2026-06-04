// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <array>
#include <memory>
#include <string>

#include "clay/ui/common/measure_constraint.h"
#include "clay/ui/component/text/raw_text_view.h"
#include "clay/ui/shadow/inline_image_shadow_node.h"
#include "clay/ui/shadow/inline_text_shadow_node.h"
#include "clay/ui/shadow/inline_truncation_shadow_node.h"
#include "clay/ui/shadow/inline_view_shadow_node.h"
#include "clay/ui/shadow/raw_text_shadow_node.h"
#include "clay/ui/shadow/shadow_node_owner.h"
#include "clay/ui/shadow/text_shadow_node.h"
#include "clay/ui/testing/ui_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
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

TEST_F_UI(TextTest, BuildEditableInlineStreamIncludesInlineTextAndAtomicItems) {
  auto text_node =
      std::make_unique<TextShadowNode>(owner_, std::string("text"), -1);
  auto raw_text =
      std::make_unique<RawTextShadowNode>(owner_, std::string("raw-text"), -1);
  auto inline_text = std::make_unique<InlineTextShadowNode>(
      owner_, std::string("inline-text"), -1);
  auto inline_raw_text =
      std::make_unique<RawTextShadowNode>(owner_, std::string("raw-text"), -1);
  auto inline_image = std::make_unique<InlineImageShadowNode>(
      owner_, std::string("inline-image"), -1);
  auto trailing_raw_text =
      std::make_unique<RawTextShadowNode>(owner_, std::string("raw-text"), -1);
  auto inline_view = std::make_unique<InlineViewShadowNode>(
      owner_, std::string("inline-view"), -1);

  raw_text->SetText(std::u16string(u"A\U0001F600"));
  raw_text->SetEndIndex(1);
  inline_raw_text->SetText("BC");
  trailing_raw_text->SetText("D");

  inline_text->AddChild(inline_raw_text.get());
  text_node->AddChild(raw_text.get());
  text_node->AddChild(inline_text.get());
  text_node->AddChild(inline_image.get());
  text_node->AddChild(trailing_raw_text.get());
  text_node->AddChild(inline_view.get());

  auto stream = text_node->BuildEditableInlineStream();
  ASSERT_EQ(stream.size(), 5u);

  EXPECT_EQ(stream[0].type, EditableInlineStreamItemType::kText);
  EXPECT_EQ(stream[0].start, 0u);
  EXPECT_EQ(stream[0].length, 3u);
  EXPECT_EQ(stream[0].text, std::u16string(u"A\U0001F600"));
  EXPECT_EQ(stream[0].node, raw_text.get());

  EXPECT_EQ(stream[1].type, EditableInlineStreamItemType::kText);
  EXPECT_EQ(stream[1].start, 3u);
  EXPECT_EQ(stream[1].length, 2u);
  EXPECT_EQ(stream[1].text, std::u16string(u"BC"));
  EXPECT_EQ(stream[1].node, inline_raw_text.get());

  EXPECT_EQ(stream[2].type,
            EditableInlineStreamItemType::kAtomicInlineImage);
  EXPECT_EQ(stream[2].start, 5u);
  EXPECT_EQ(stream[2].length, 1u);
  EXPECT_EQ(stream[2].node, inline_image.get());

  EXPECT_EQ(stream[3].type, EditableInlineStreamItemType::kText);
  EXPECT_EQ(stream[3].start, 6u);
  EXPECT_EQ(stream[3].length, 1u);
  EXPECT_EQ(stream[3].text, std::u16string(u"D"));
  EXPECT_EQ(stream[3].node, trailing_raw_text.get());

  EXPECT_EQ(stream[4].type, EditableInlineStreamItemType::kAtomicInlineView);
  EXPECT_EQ(stream[4].start, 7u);
  EXPECT_EQ(stream[4].length, 1u);
  EXPECT_EQ(stream[4].node, inline_view.get());

  EXPECT_EQ(text_node->GetRawText(), std::u16string(u"ABCD"));
}

TEST_F_UI(TextTest, ApplyEditableInlineMutationReplacesUtf16RawText) {
  auto text_node =
      std::make_unique<TextShadowNode>(owner_, std::string("text"), -1);
  auto raw_text =
      std::make_unique<RawTextShadowNode>(owner_, std::string("raw-text"), -1);
  auto inline_text = std::make_unique<InlineTextShadowNode>(
      owner_, std::string("inline-text"), -1);
  auto inline_raw_text =
      std::make_unique<RawTextShadowNode>(owner_, std::string("raw-text"), -1);

  raw_text->SetText(std::u16string(u"A\U0001F600B"));
  inline_raw_text->SetText("CD");
  inline_text->AddChild(inline_raw_text.get());
  text_node->AddChild(raw_text.get());
  text_node->AddChild(inline_text.get());

  std::array<float, 4> zero = {0, 0, 0, 0};
  text_node->OnLayout(0, TextMeasureMode::kDefinite, 0,
                      TextMeasureMode::kDefinite, zero, zero);
  EXPECT_FALSE(text_node->IsDirty());

  auto result = text_node->ApplyEditableInlineStreamMutation(
      1, 3, std::u16string(u"x"));

  EXPECT_EQ(result.status, EditableInlineMutationStatus::kApplied);
  EXPECT_TRUE(result.text_changed);
  EXPECT_EQ(result.selection_offset, 2u);
  EXPECT_TRUE(result.atomic_deletes.empty());
  EXPECT_EQ(raw_text->EditableText(), std::u16string(u"AxB"));
  EXPECT_EQ(inline_raw_text->EditableText(), std::u16string(u"CD"));
  EXPECT_TRUE(text_node->IsDirty());
}

TEST_F_UI(TextTest, ApplyEditableInlineMutationDeletesAcrossInlineTextRuns) {
  auto text_node =
      std::make_unique<TextShadowNode>(owner_, std::string("text"), -1);
  auto raw_text =
      std::make_unique<RawTextShadowNode>(owner_, std::string("raw-text"), -1);
  auto inline_text = std::make_unique<InlineTextShadowNode>(
      owner_, std::string("inline-text"), -1);
  auto inline_raw_text =
      std::make_unique<RawTextShadowNode>(owner_, std::string("raw-text"), -1);
  auto trailing_raw_text =
      std::make_unique<RawTextShadowNode>(owner_, std::string("raw-text"), -1);

  raw_text->SetText(std::u16string(u"A\U0001F600B"));
  inline_raw_text->SetText("CD");
  trailing_raw_text->SetText("EF");
  inline_text->AddChild(inline_raw_text.get());
  text_node->AddChild(raw_text.get());
  text_node->AddChild(inline_text.get());
  text_node->AddChild(trailing_raw_text.get());

  auto result =
      text_node->ApplyEditableInlineStreamMutation(1, 6, std::u16string());

  EXPECT_EQ(result.status, EditableInlineMutationStatus::kApplied);
  EXPECT_TRUE(result.text_changed);
  EXPECT_TRUE(result.atomic_deletes.empty());
  EXPECT_EQ(raw_text->EditableText(), std::u16string(u"A"));
  EXPECT_EQ(inline_raw_text->EditableText(), std::u16string());
  EXPECT_EQ(trailing_raw_text->EditableText(), std::u16string(u"EF"));
}

TEST_F_UI(TextTest, ApplyEditableInlineMutationReturnsAtomicDeleteOperations) {
  auto text_node =
      std::make_unique<TextShadowNode>(owner_, std::string("text"), -1);
  auto leading_raw_text =
      std::make_unique<RawTextShadowNode>(owner_, std::string("raw-text"), -1);
  auto inline_image = std::make_unique<InlineImageShadowNode>(
      owner_, std::string("inline-image"), -1);
  auto middle_raw_text =
      std::make_unique<RawTextShadowNode>(owner_, std::string("raw-text"), -1);
  auto inline_view = std::make_unique<InlineViewShadowNode>(
      owner_, std::string("inline-view"), -1);
  auto trailing_raw_text =
      std::make_unique<RawTextShadowNode>(owner_, std::string("raw-text"), -1);

  leading_raw_text->SetText("A");
  middle_raw_text->SetText("B");
  trailing_raw_text->SetText("C");
  text_node->AddChild(leading_raw_text.get());
  text_node->AddChild(inline_image.get());
  text_node->AddChild(middle_raw_text.get());
  text_node->AddChild(inline_view.get());
  text_node->AddChild(trailing_raw_text.get());

  auto result = text_node->ApplyEditableInlineStreamMutation(
      1, 4, std::u16string(u"Z"));

  EXPECT_EQ(result.status, EditableInlineMutationStatus::kApplied);
  EXPECT_TRUE(result.text_changed);
  ASSERT_EQ(result.atomic_deletes.size(), 2u);
  EXPECT_EQ(result.atomic_deletes[0].type,
            EditableInlineStreamItemType::kAtomicInlineImage);
  EXPECT_EQ(result.atomic_deletes[0].start, 1u);
  EXPECT_EQ(result.atomic_deletes[0].node, inline_image.get());
  EXPECT_EQ(result.atomic_deletes[1].type,
            EditableInlineStreamItemType::kAtomicInlineView);
  EXPECT_EQ(result.atomic_deletes[1].start, 3u);
  EXPECT_EQ(result.atomic_deletes[1].node, inline_view.get());
  EXPECT_EQ(leading_raw_text->EditableText(), std::u16string(u"AZ"));
  EXPECT_EQ(middle_raw_text->EditableText(), std::u16string());
  EXPECT_EQ(trailing_raw_text->EditableText(), std::u16string(u"C"));
}

TEST_F_UI(TextTest, ApplyEditableInlineMutationReturnsPendingAtomicReplacement) {
  auto text_node =
      std::make_unique<TextShadowNode>(owner_, std::string("text"), -1);
  auto inline_image = std::make_unique<InlineImageShadowNode>(
      owner_, std::string("inline-image"), -1);
  text_node->AddChild(inline_image.get());

  auto result = text_node->ApplyEditableInlineStreamMutation(
      0, 1, std::u16string(u"Z"));

  EXPECT_EQ(result.status, EditableInlineMutationStatus::kApplied);
  EXPECT_FALSE(result.text_changed);
  ASSERT_EQ(result.atomic_deletes.size(), 1u);
  EXPECT_EQ(result.atomic_deletes[0].node, inline_image.get());
  EXPECT_TRUE(result.has_pending_text_insertion);
  EXPECT_EQ(result.pending_text_insertion_offset, 0u);
  EXPECT_EQ(result.pending_text_insertion, std::u16string(u"Z"));
}

}  // namespace clay
