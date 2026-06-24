// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "clay/fml/icu_util.h"
#include "clay/ui/component/text/text_paragraph_builder.h"
#include "clay/ui/component/text/text_view.h"
#include "clay/ui/lynx_module/lynx_ui_method_types.h"
#include "clay/ui/testing/test_utils.h"
#include "clay/ui/testing/ui_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace {

constexpr float kCustomSelectionHandleSize = 40.f;

float GetNumber(const clay::Value& value) {
  if (value.IsFloat()) {
    return value.GetFloat();
  }
  if (value.IsDouble()) {
    return static_cast<float>(value.GetDouble());
  }
  if (value.IsInt()) {
    return static_cast<float>(value.GetInt());
  }
  if (value.IsUint()) {
    return static_cast<float>(value.GetUint());
  }
  if (value.IsLong()) {
    return static_cast<float>(value.GetLong());
  }
  return 0.f;
}

std::unique_ptr<txt::Paragraph> CreateParagraph(const std::u16string& text) {
  TextStyle style;
  style.font_size = 50.f;
  auto builder = std::make_unique<TextParagraphBuilder>(true, style);
  builder->PushStyle(style);
  builder->AddText(text);
  builder->Pop();
  auto paragraph = Build(std::move(builder));
  paragraph->Layout(1000);
  return paragraph;
}

class TextSelectionTest : public UITest {
 protected:
  void UISetUp() override {
    fml::icu::InitializeICU("icudtl.dat");
    text_view_ = std::make_unique<TextView>(1, page_.get());
  }

  void UITearDown() override { text_view_.reset(); }

  std::unique_ptr<TextView> text_view_;
};

}  // namespace

TEST_F_UI(TextSelectionTest, SetTextSelectionReturnsHandlesForSelectedRange) {
  const std::u16string text = u"hello world\nhello world";
  text_view_->SetParagraph(CreateParagraph(text), text);
  text_view_->SetSelectionHandleSize(kCustomSelectionHandleSize);

  const auto line_rects =
      text_view_->GetRenderText()->GetTextLineRects(0, text.length());
  ASSERT_GE(line_rects.size(), 2u);

  auto args = CreateLynxModuleValues(
      {"startX", "startY", "endX", "endY", "showStartHandle", "showEndHandle"},
      {clay::Value(static_cast<int>(line_rects.front().left() + 1)),
       clay::Value(static_cast<int>(line_rects.front().Center().y())),
       clay::Value(static_cast<int>(line_rects.back().right() - 1)),
       clay::Value(static_cast<int>(line_rects.back().Center().y())),
       clay::Value(false), clay::Value(false)});

  LynxUIMethodResult callback_code = LynxUIMethodResult::kUnknown;
  clay::Value callback_data;
  text_view_->setTextSelection(
      args, [&callback_code, &callback_data](LynxUIMethodResult code,
                                             clay::Value data) {
        callback_code = code;
        callback_data = std::move(data);
      });

  ASSERT_EQ(callback_code, LynxUIMethodResult::kSuccess);
  ASSERT_TRUE(callback_data.IsMap());
  const auto& result_map = callback_data.GetMap();
  const auto& boxes = result_map.at("boxes").GetArray();
  ASSERT_GE(boxes.size(), 2u);
  const auto& handles = result_map.at("handles").GetArray();
  ASSERT_EQ(handles.size(), 2u);

  const auto& start_handle = handles[0].GetMap();
  const auto& end_handle = handles[1].GetMap();
  const auto& first_box = boxes.front().GetMap();
  const auto& last_box = boxes.back().GetMap();
  constexpr float kExpectedRadius = kCustomSelectionHandleSize / 2;
  EXPECT_FLOAT_EQ(GetNumber(start_handle.at("radius")), kExpectedRadius);
  EXPECT_FLOAT_EQ(GetNumber(end_handle.at("radius")), kExpectedRadius);
  EXPECT_FLOAT_EQ(GetNumber(start_handle.at("y")),
                  GetNumber(first_box.at("top")) - kExpectedRadius);
  EXPECT_FLOAT_EQ(GetNumber(end_handle.at("y")),
                  GetNumber(last_box.at("bottom")) + kExpectedRadius);
}

TEST_F_UI(TextSelectionTest, GetTextLineRectsReturnsForwardSelectionRects) {
  const std::u16string text = u"hello world\nhello world";
  text_view_->SetParagraph(CreateParagraph(text), text);

  const auto line_rects =
      text_view_->GetRenderText()->GetTextLineRects(0, text.length());

  ASSERT_EQ(line_rects.size(), 2u);
  EXPECT_LT(line_rects.front().left(), line_rects.front().right());
  EXPECT_LT(line_rects.back().left(), line_rects.back().right());
}

TEST_F_UI(TextSelectionTest, SetSelectionHandleSizeRebuildsVisibleHandles) {
  const std::u16string text = u"hello world";
  text_view_->SetParagraph(CreateParagraph(text), text);
  text_view_->GetRenderText()->SetSelection(TextRange(text.length(), 0));
  text_view_->ShowSelectionHandle();

  ASSERT_NE(text_view_->start_selection_handle_, nullptr);
  ASSERT_NE(text_view_->end_selection_handle_, nullptr);
  const float old_start_width = text_view_->start_selection_handle_->Width();
  const float old_start_height = text_view_->start_selection_handle_->Height();
  const float old_end_width = text_view_->end_selection_handle_->Width();
  const float old_end_height = text_view_->end_selection_handle_->Height();

  text_view_->SetSelectionHandleSize(kCustomSelectionHandleSize);

  const float expected_radius = kCustomSelectionHandleSize / 2;
  EXPECT_FLOAT_EQ(
      text_view_->start_selection_handle_->GetSelectionHandleRadius(),
      expected_radius);
  EXPECT_FLOAT_EQ(text_view_->end_selection_handle_->GetSelectionHandleRadius(),
                  expected_radius);
  EXPECT_FLOAT_EQ(text_view_->start_selection_handle_->Width(),
                  2 * expected_radius);
  EXPECT_GT(text_view_->start_selection_handle_->Width(), old_start_width);
  EXPECT_GT(text_view_->start_selection_handle_->Height(), old_start_height);
  EXPECT_FLOAT_EQ(text_view_->end_selection_handle_->Width(),
                  2 * expected_radius);
  EXPECT_GT(text_view_->end_selection_handle_->Width(), old_end_width);
  EXPECT_GT(text_view_->end_selection_handle_->Height(), old_end_height);
  EXPECT_EQ(text_view_->start_selection_handle_->GetHandleType(), kLeft);
  EXPECT_EQ(text_view_->end_selection_handle_->GetHandleType(), kRight);
  EXPECT_LT(text_view_->start_selection_handle_->Left(),
            text_view_->end_selection_handle_->Left());
}

TEST_F_UI(TextSelectionTest,
          SetTextSelectionReturnsDefaultRadiusForNonPositiveHandleSizes) {
  const std::u16string text = u"hello world";
  text_view_->SetParagraph(CreateParagraph(text), text);
  const auto line_rects =
      text_view_->GetRenderText()->GetTextLineRects(0, text.length());
  ASSERT_EQ(line_rects.size(), 1u);

  auto verify_default_radius = [this, &line_rects] {
    auto args = CreateLynxModuleValues(
        {"startX", "startY", "endX", "endY", "showStartHandle",
         "showEndHandle"},
        {clay::Value(static_cast<int>(line_rects.front().left() + 1)),
         clay::Value(static_cast<int>(line_rects.front().Center().y())),
         clay::Value(static_cast<int>(line_rects.front().right() - 1)),
         clay::Value(static_cast<int>(line_rects.front().Center().y())),
         clay::Value(false), clay::Value(false)});

    clay::Value callback_data;
    text_view_->setTextSelection(
        args, [&callback_data](LynxUIMethodResult, clay::Value data) {
          callback_data = std::move(data);
        });

    const auto& handles = callback_data.GetMap().at("handles").GetArray();
    ASSERT_EQ(handles.size(), 2u);
    EXPECT_FLOAT_EQ(GetNumber(handles[0].GetMap().at("radius")),
                    kSelectionHandleRadius);
    EXPECT_FLOAT_EQ(GetNumber(handles[1].GetMap().at("radius")),
                    kSelectionHandleRadius);
  };

  verify_default_radius();
  text_view_->SetSelectionHandleSize(0);
  verify_default_radius();
  text_view_->SetSelectionHandleSize(-1);
  verify_default_radius();
}

TEST_F_UI(TextSelectionTest, SetAttributeUpdatesVisibleSelectionHandleColors) {
  const std::u16string text = u"hello world";
  text_view_->SetParagraph(CreateParagraph(text), text);
  text_view_->GetRenderText()->SetSelection(TextRange(0, text.length()));
  text_view_->ShowSelectionHandle();

  ASSERT_NE(text_view_->start_selection_handle_, nullptr);
  ASSERT_NE(text_view_->end_selection_handle_, nullptr);

  constexpr Color kHandleColor(0xFFFF0000);
  text_view_->SetAttribute("selection-handle-color",
                           clay::Value(static_cast<uint32_t>(kHandleColor)));
  ASSERT_TRUE(
      text_view_->start_selection_handle_->render_object()->HasBackground());
  ASSERT_TRUE(
      text_view_->end_selection_handle_->render_object()->HasBackground());
  EXPECT_EQ(text_view_->start_selection_handle_->render_object()
                ->Background()
                .background_color,
            kHandleColor);
  EXPECT_EQ(text_view_->end_selection_handle_->render_object()
                ->Background()
                .background_color,
            kHandleColor);

  text_view_->SetAttribute("selection-handle-color",
                           clay::Value(static_cast<uint32_t>(0x00FF0000)));
  EXPECT_EQ(text_view_->start_selection_handle_->render_object()
                ->Background()
                .background_color,
            Color::kBlue());
  EXPECT_EQ(text_view_->end_selection_handle_->render_object()
                ->Background()
                .background_color,
            Color::kBlue());
}

TEST_F_UI(TextSelectionTest,
          SetTextSelectionKeepsHandlesAtVisualSelectionEnds) {
  const std::u16string text = u"hello world";
  text_view_->SetParagraph(CreateParagraph(text), text);
  const auto line_rects =
      text_view_->GetRenderText()->GetTextLineRects(0, text.length());
  ASSERT_EQ(line_rects.size(), 1u);

  auto args = CreateLynxModuleValues(
      {"startX", "startY", "endX", "endY", "showStartHandle", "showEndHandle"},
      {clay::Value(static_cast<int>(line_rects.front().right() - 1)),
       clay::Value(static_cast<int>(line_rects.front().Center().y())),
       clay::Value(static_cast<int>(line_rects.front().left() + 1)),
       clay::Value(static_cast<int>(line_rects.front().Center().y())),
       clay::Value(true), clay::Value(true)});

  text_view_->setTextSelection(args, [](LynxUIMethodResult, clay::Value) {});

  ASSERT_NE(text_view_->start_selection_handle_, nullptr);
  ASSERT_NE(text_view_->end_selection_handle_, nullptr);
  EXPECT_EQ(text_view_->start_selection_handle_->GetHandleType(), kLeft);
  EXPECT_EQ(text_view_->end_selection_handle_->GetHandleType(), kRight);
  EXPECT_LT(text_view_->start_selection_handle_->Left(),
            text_view_->end_selection_handle_->Left());
}

TEST_F_UI(TextSelectionTest, SetTextSelectionHidesVisualStartHandle) {
  const std::u16string text = u"hello world";
  text_view_->SetParagraph(CreateParagraph(text), text);
  const auto line_rects =
      text_view_->GetRenderText()->GetTextLineRects(0, text.length());
  ASSERT_EQ(line_rects.size(), 1u);

  auto args = CreateLynxModuleValues(
      {"startX", "startY", "endX", "endY", "showStartHandle", "showEndHandle"},
      {clay::Value(static_cast<int>(line_rects.front().right() - 1)),
       clay::Value(static_cast<int>(line_rects.front().Center().y())),
       clay::Value(static_cast<int>(line_rects.front().left() + 1)),
       clay::Value(static_cast<int>(line_rects.front().Center().y())),
       clay::Value(false), clay::Value(true)});

  text_view_->setTextSelection(args, [](LynxUIMethodResult, clay::Value) {});

  EXPECT_EQ(text_view_->start_selection_handle_, nullptr);
  ASSERT_NE(text_view_->end_selection_handle_, nullptr);
  EXPECT_EQ(text_view_->end_selection_handle_->GetHandleType(), kRight);
}

}  // namespace clay
