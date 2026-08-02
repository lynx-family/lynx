// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <utility>

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

// Other UI suites may build paragraphs before TextSelectionTest runs. Load ICU
// before any test constructs or lays out a paragraph.
class TextSelectionEnvironment : public ::testing::Environment {
 public:
  void SetUp() override { fml::icu::InitializeICU("icudtl.dat"); }
};

::testing::Environment* const kTextSelectionEnvironment =
    ::testing::AddGlobalTestEnvironment(new TextSelectionEnvironment());

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

bool IsNumber(const clay::Value& value) {
  return value.IsFloat() || value.IsDouble() || value.IsInt() ||
         value.IsUint() || value.IsLong();
}

std::unique_ptr<txt::Paragraph> CreateParagraph(
    const std::u16string& text,
    std::optional<TextDirection> direction = std::nullopt) {
  TextStyle style;
  style.font_size = 50.f;
  style.text_direction = direction;
  auto builder = std::make_unique<TextParagraphBuilder>(true, style);
  builder->PushStyle(style);
  builder->AddText(text);
  builder->Pop();
  auto paragraph = Build(std::move(builder));
  paragraph->Layout(1000);
  return paragraph;
}

void ExpectPositiveRect(const clay::Value::Map& rect) {
  for (const char* key :
       {"left", "right", "top", "bottom", "width", "height"}) {
    const auto it = rect.find(key);
    ASSERT_NE(it, rect.end()) << key;
    ASSERT_TRUE(IsNumber(it->second)) << key;
  }
  EXPECT_LT(GetNumber(rect.at("left")), GetNumber(rect.at("right")));
  EXPECT_LT(GetNumber(rect.at("top")), GetNumber(rect.at("bottom")));
  EXPECT_GT(GetNumber(rect.at("width")), 0.f);
  EXPECT_GT(GetNumber(rect.at("height")), 0.f);
}

void ExpectRectContains(const clay::Value::Map& outer,
                        const clay::Value::Map& inner) {
  for (const char* key : {"left", "right", "top", "bottom"}) {
    ASSERT_NE(outer.find(key), outer.end()) << "outer." << key;
    ASSERT_NE(inner.find(key), inner.end()) << "inner." << key;
  }
  EXPECT_LE(GetNumber(outer.at("left")), GetNumber(inner.at("left")));
  EXPECT_LE(GetNumber(outer.at("top")), GetNumber(inner.at("top")));
  EXPECT_GE(GetNumber(outer.at("right")), GetNumber(inner.at("right")));
  EXPECT_GE(GetNumber(outer.at("bottom")), GetNumber(inner.at("bottom")));
}

class TextSelectionTest : public UITest {
 protected:
  void UISetUp() override {
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

TEST_F_UI(TextSelectionTest, GetTextBoundingRectReturnsMultilinePublicSchema) {
  const std::u16string text = u"hello world\nhello world";
  text_view_->SetParagraph(CreateParagraph(text), text);

  bool callback_invoked = false;
  InvokeUIMethod(
      text_view_.get(), "getTextBoundingRect",
      {{"start", clay::Value(0)},
       {"end", clay::Value(static_cast<int>(text.length()))}},
      [&callback_invoked](LynxUIMethodResult code, const clay::Value& data) {
        callback_invoked = true;
        ASSERT_EQ(code, LynxUIMethodResult::kSuccess);
        ASSERT_TRUE(data.IsMap());
        const auto& result = data.GetMap();
        ASSERT_NE(result.find("boundingRect"), result.end());
        ASSERT_NE(result.find("boxes"), result.end());
        ASSERT_TRUE(result.at("boundingRect").IsMap());
        ASSERT_TRUE(result.at("boxes").IsArray());
        const auto& bounding_rect = result.at("boundingRect").GetMap();
        const auto& boxes = result.at("boxes").GetArray();
        ASSERT_EQ(boxes.size(), 2u);
        ExpectPositiveRect(bounding_rect);
        for (const auto& box : boxes) {
          ASSERT_TRUE(box.IsMap());
          ExpectPositiveRect(box.GetMap());
          ExpectRectContains(bounding_rect, box.GetMap());
        }
      });

  EXPECT_TRUE(callback_invoked);
}

TEST_F_UI(TextSelectionTest, GetTextBoundingRectRejectsInvalidRanges) {
  const std::u16string text = u"hello";
  text_view_->SetParagraph(CreateParagraph(text), text);
  const std::array<std::pair<int, int>, 5> invalid_ranges = {
      std::pair{-1, 2}, std::pair{2, 2}, std::pair{4, 2}, std::pair{0, 6},
      std::pair{-1, -1}};

  LynxUIMethodResult missing_args_code = LynxUIMethodResult::kUnknown;
  bool missing_args_callback_invoked = false;
  InvokeUIMethod(text_view_.get(), "getTextBoundingRect", {},
                 [&missing_args_code, &missing_args_callback_invoked](
                     LynxUIMethodResult code, const clay::Value& data) {
                   missing_args_code = code;
                   missing_args_callback_invoked = true;
                 });
  EXPECT_TRUE(missing_args_callback_invoked);
  EXPECT_EQ(missing_args_code, LynxUIMethodResult::kParamInvalid);

  for (const auto& [start, end] : invalid_ranges) {
    LynxUIMethodResult callback_code = LynxUIMethodResult::kUnknown;
    bool callback_invoked = false;
    InvokeUIMethod(text_view_.get(), "getTextBoundingRect",
                   {{"start", clay::Value(start)}, {"end", clay::Value(end)}},
                   [&callback_code, &callback_invoked](
                       LynxUIMethodResult code, const clay::Value& data) {
                     callback_code = code;
                     callback_invoked = true;
                   });

    EXPECT_TRUE(callback_invoked) << "range [" << start << ", " << end << ")";
    EXPECT_EQ(callback_code, LynxUIMethodResult::kParamInvalid)
        << "range [" << start << ", " << end << ")";
  }
}

#if defined(CLAY_ENABLE_SKSHAPER)
TEST_F_UI(TextSelectionTest,
          SkParagraphGetTextBoundingRectHandlesRtlAndEmojiUtf16Range) {
  const std::u16string text = u"\u05d0\u05d1\U0001F600\u05d2\u05d3";
  text_view_->SetParagraph(CreateParagraph(text, TextDirection::kRtl), text);

  bool callback_invoked = false;
  InvokeUIMethod(
      text_view_.get(), "getTextBoundingRect",
      {{"start", clay::Value(2)}, {"end", clay::Value(4)}},
      [&callback_invoked](LynxUIMethodResult code, const clay::Value& data) {
        callback_invoked = true;
        ASSERT_EQ(code, LynxUIMethodResult::kSuccess);
        ASSERT_TRUE(data.IsMap());
        const auto& result = data.GetMap();
        ASSERT_NE(result.find("boundingRect"), result.end());
        ASSERT_NE(result.find("boxes"), result.end());
        ASSERT_TRUE(result.at("boundingRect").IsMap());
        ASSERT_TRUE(result.at("boxes").IsArray());
        const auto& bounding_rect = result.at("boundingRect").GetMap();
        const auto& boxes = result.at("boxes").GetArray();
        ASSERT_FALSE(boxes.empty());
        ExpectPositiveRect(bounding_rect);
        for (const auto& box : boxes) {
          ASSERT_TRUE(box.IsMap());
          ExpectPositiveRect(box.GetMap());
          ExpectRectContains(bounding_rect, box.GetMap());
        }
      });

  EXPECT_TRUE(callback_invoked);

  text_view_->GetRenderText()->SetSelection(TextRange(2, 4));
  EXPECT_TRUE(text_view_->GetRenderText()->GetSelectionString() ==
              u"\U0001F600");
}
#endif

TEST_F_UI(TextSelectionTest, SelectionChangeEventReportsForwardUtf16Range) {
  const std::u16string text = u"A\U0001F600\u4e2dB";
  text_view_->SetParagraph(CreateParagraph(text), text);
  int event_count = 0;
  int last_view_id = -1;
  std::string last_event_name;
  clay::Value::Map last_payload;
  custom_event_callback_ = [&](int view_id, const char* event_name,
                               clay::Value::Map payload) {
    ++event_count;
    last_view_id = view_id;
    last_event_name = event_name;
    last_payload = std::move(payload);
  };

  text_view_->SetAttribute("text-selection", clay::Value(true));
  text_view_->GetRenderText()->SetSelection(TextRange(1, 4));

  ASSERT_EQ(event_count, 1);
  EXPECT_EQ(last_view_id, 1);
  EXPECT_EQ(last_event_name, "selectionchange");
  EXPECT_EQ(GetNumber(last_payload.at("start")), 1.f);
  EXPECT_EQ(GetNumber(last_payload.at("end")), 4.f);
  EXPECT_EQ(last_payload.at("direction").GetString(), "forward");
}

TEST_F_UI(TextSelectionTest,
          CustomTextSelectionBeforeEnableSuppressesBuiltInGesture) {
  const std::u16string text = u"custom selection";
  text_view_->SetParagraph(CreateParagraph(text), text);
  int event_count = 0;
  custom_event_callback_ = [&](int, const char*, clay::Value::Map) {
    ++event_count;
  };

  text_view_->SetAttribute("custom-text-selection", clay::Value(true));
  text_view_->SetAttribute("text-selection", clay::Value(true));

  EXPECT_FALSE(
      text_view_->HasDragGestureRecognizer(ScrollDirection::kHorizontal));
  EXPECT_FALSE(
      text_view_->HasDragGestureRecognizer(ScrollDirection::kVertical));
  EXPECT_FALSE(text_view_->HasLongPressGestureRecognizer());
  text_view_->GetRenderText()->SetSelection(TextRange(0, text.length()));
  EXPECT_EQ(event_count, 1);
}

#if !defined(OS_ANDROID) && !defined(OS_IOS)
TEST_F_UI(TextSelectionTest, TextSelectionEnableDisableTogglesBuiltInGesture) {
  EXPECT_FALSE(
      text_view_->HasDragGestureRecognizer(ScrollDirection::kHorizontal));
  EXPECT_FALSE(
      text_view_->HasDragGestureRecognizer(ScrollDirection::kVertical));

  text_view_->SetAttribute("text-selection", clay::Value(true));

  EXPECT_TRUE(
      text_view_->HasDragGestureRecognizer(ScrollDirection::kHorizontal));
  EXPECT_TRUE(text_view_->HasDragGestureRecognizer(ScrollDirection::kVertical));

  text_view_->SetAttribute("text-selection", clay::Value(false));

  EXPECT_FALSE(
      text_view_->HasDragGestureRecognizer(ScrollDirection::kHorizontal));
  EXPECT_FALSE(
      text_view_->HasDragGestureRecognizer(ScrollDirection::kVertical));
}
#endif

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
