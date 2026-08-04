// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <array>
#include <initializer_list>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "clay/fml/icu_util.h"
#include "clay/ui/component/text/inline_text_view.h"
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
    std::optional<TextDirection> direction = std::nullopt,
    float width = 1000.f) {
  TextStyle style;
  style.font_size = 50.f;
  style.text_direction = direction;
  auto builder = std::make_unique<TextParagraphBuilder>(true, style);
  builder->PushStyle(style);
  builder->AddText(text);
  builder->Pop();
  auto paragraph = Build(std::move(builder));
  paragraph->Layout(width);
  return paragraph;
}

std::unique_ptr<txt::Paragraph> CreateParagraph(const std::u16string& text,
                                                float width) {
  return CreateParagraph(text, std::nullopt, width);
}

std::unique_ptr<txt::Paragraph> CreateParagraph(
    std::initializer_list<std::u16string> runs) {
  TextStyle base_style;
  base_style.font_size = 50.f;
  auto builder = std::make_unique<TextParagraphBuilder>(true, base_style);
  constexpr Color kRunColors[] = {
      Color::kBlack(), Color::kRed(),     Color::kGreen(),
      Color::kBlue(),  Color::kMagenta(), Color::kCyan(),
  };
  size_t index = 0;
  for (const auto& run : runs) {
    TextStyle run_style = base_style;
    run_style.text_color = kRunColors[index++ % std::size(kRunColors)];
    builder->PushStyle(run_style);
    builder->AddText(run);
    builder->Pop();
  }
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

#if defined(OS_WIN) || defined(OS_OSX)
TEST_F_UI(TextSelectionTest, SelectWordSupportsMixedChineseEnglishAndEmoji) {
  const std::u16string text =
      u"\u4E2D\u6587 mixed \U0001F642 emoji \u6D4B\u8BD5 \U0001F680 end";
  text_view_->SetParagraph(CreateParagraph(text), text);

  auto first_chinese_boundary = text_view_->SelectWord(1);
  EXPECT_GE(first_chinese_boundary.start(), 0u);
  EXPECT_LE(first_chinese_boundary.start(), 1u);
  EXPECT_EQ(first_chinese_boundary.end(), 2u);
  EXPECT_EQ(text_view_->SelectWord(2, Affinity::kUpstream),
            first_chinese_boundary);
  EXPECT_EQ(text_view_->SelectWord(5), TextRange(3, 8));
  EXPECT_EQ(text_view_->SelectWord(8, Affinity::kUpstream), TextRange(3, 8));
  EXPECT_EQ(text_view_->SelectWord(9), TextRange(9, 11));
  EXPECT_EQ(text_view_->SelectWord(11, Affinity::kUpstream), TextRange(9, 11));
  auto second_chinese_boundary = text_view_->SelectWord(19);
  EXPECT_GE(second_chinese_boundary.start(), 18u);
  EXPECT_LE(second_chinese_boundary.start(), 19u);
  EXPECT_EQ(second_chinese_boundary.end(), 20u);
  EXPECT_EQ(text_view_->SelectWord(20, Affinity::kUpstream),
            second_chinese_boundary);
  EXPECT_EQ(text_view_->SelectWord(21), TextRange(21, 23));
  EXPECT_EQ(text_view_->SelectWord(23, Affinity::kUpstream), TextRange(21, 23));
}

TEST_F_UI(TextSelectionTest, SelectWordCrossesNestedInlineTextBoundaries) {
  // The English and Chinese words are each split into separate styled runs.
  // Their middle/end runs are also represented by nested InlineTextViews,
  // matching the event-routing hierarchy used by TextUpdateBundle.
  const std::u16string text =
      u"\u524D\u7F00 selection\U0001F642 \u4E2D\u6587 \u540E\u7F00";
  auto paragraph =
      CreateParagraph({u"\u524D\u7F00 ", u"sel", u"ect", u"ion", u"\U0001F642 ",
                       u"\u4E2D", u"\u6587", u" \u540E\u7F00"});
  auto* paragraph_ptr = paragraph.get();
  text_view_->SetParagraph(std::move(paragraph), text);

  auto english_boxes = paragraph_ptr->GetRectsForRange(
      7, 8, txt::Paragraph::RectHeightStyle::kTight,
      txt::Paragraph::RectWidthStyle::kTight);
  auto chinese_boxes = paragraph_ptr->GetRectsForRange(
      16, 17, txt::Paragraph::RectHeightStyle::kTight,
      txt::Paragraph::RectWidthStyle::kTight);
  ASSERT_FALSE(english_boxes.empty());
  ASSERT_FALSE(chinese_boxes.empty());

  auto english_outer =
      std::make_unique<InlineTextView>(2, text_view_->page_view());
  auto english_inner =
      std::make_unique<InlineTextView>(3, text_view_->page_view());
  auto chinese_outer =
      std::make_unique<InlineTextView>(4, text_view_->page_view());
  auto chinese_inner =
      std::make_unique<InlineTextView>(5, text_view_->page_view());
  std::list<TextRange> english_outer_ranges{TextRange(3, 12)};
  std::list<TextRange> english_inner_ranges{TextRange(6, 9)};
  std::list<TextRange> chinese_outer_ranges{TextRange(15, 17)};
  std::list<TextRange> chinese_inner_ranges{TextRange(16, 17)};
  english_outer->SetTextRange(english_outer_ranges);
  english_inner->SetTextRange(english_inner_ranges);
  chinese_outer->SetTextRange(chinese_outer_ranges);
  chinese_inner->SetTextRange(chinese_inner_ranges);
  text_view_->AddChild(english_outer.get());
  english_outer->AddChild(english_inner.get());
  text_view_->AddChild(chinese_outer.get());
  chinese_outer->AddChild(chinese_inner.get());

  const auto& english_rect = english_boxes.front().rect;
  FloatPoint english_point((english_rect.Left() + english_rect.Right()) / 2,
                           (english_rect.Top() + english_rect.Bottom()) / 2);
  EXPECT_EQ(text_view_->GetViewAtPosition(english_point, english_point),
            english_inner.get());
  EXPECT_EQ(text_view_->SelectWord(7), TextRange(3, 12));
  EXPECT_EQ(text_view_->GetRenderText()->GetSelectionString(), u"selection");

  const auto& chinese_rect = chinese_boxes.front().rect;
  FloatPoint chinese_point((chinese_rect.Left() + chinese_rect.Right()) / 2,
                           (chinese_rect.Top() + chinese_rect.Bottom()) / 2);
  EXPECT_EQ(text_view_->GetViewAtPosition(chinese_point, chinese_point),
            chinese_inner.get());
  auto chinese_boundary = text_view_->SelectWord(16);
  EXPECT_GE(chinese_boundary.start(), 15u);
  EXPECT_LE(chinese_boundary.start(), 16u);
  EXPECT_EQ(chinese_boundary.end(), 17u);
  auto chinese_selection = text_view_->GetRenderText()->GetSelectionString();
  EXPECT_TRUE(chinese_selection == u"\u4E2D\u6587" ||
              chinese_selection == u"\u6587");

  // The adjacent surrogate-pair emoji must remain a separate boundary.
  EXPECT_EQ(text_view_->SelectWord(12), TextRange(12, 14));
  EXPECT_EQ(text_view_->GetRenderText()->GetSelectionString(), u"\U0001F642");

  english_outer->RemoveChild(english_inner.get());
  chinese_outer->RemoveChild(chinese_inner.get());
  text_view_->RemoveChild(english_outer.get());
  text_view_->RemoveChild(chinese_outer.get());
}

TEST_F_UI(TextSelectionTest, SelectLineSelectsVisualLine) {
  const std::u16string text =
      u"\u4E2D\u6587 mixed \U0001F642 emoji \u6D4B\u8BD5 \U0001F680 end";
  text_view_->SetParagraph(CreateParagraph(text), text);

  EXPECT_EQ(text_view_->SelectLine(5), TextRange(0, 27));
}

TEST_F_UI(TextSelectionTest, SelectLineSelectsWrappedContinuationUnderPointer) {
  const std::u16string text = u"first second third fourth fifth sixth";
  text_view_->SetParagraph(CreateParagraph(text, 260.f), text);

  auto* painter = text_view_->GetRenderText()->GetPainter();
  const auto& lines = painter->GetParagraph()->GetLineMetrics();
  ASSERT_GE(lines.size(), 2u);
  ASSERT_EQ(lines[0].end_index, lines[1].start_index);

  const size_t continuation_start = lines[1].start_index;
  const auto boxes =
      painter->GetRectsForRange(static_cast<int>(continuation_start),
                                static_cast<int>(continuation_start + 1));
  ASSERT_FALSE(boxes.empty());
  const auto& first_glyph = boxes.front().rect;
  const auto glyph_pos = painter->GetGlyphPositionAtCoordinate(
      first_glyph.left() + 0.1f, first_glyph.Center().y());
  ASSERT_EQ(glyph_pos.first, continuation_start);
  ASSERT_EQ(glyph_pos.second, Affinity::kDownstream);

  EXPECT_EQ(text_view_->SelectLine(glyph_pos.first, Affinity::kUpstream),
            TextRange(lines[0].start_index, lines[0].end_index));
  EXPECT_EQ(text_view_->SelectLine(glyph_pos.first, glyph_pos.second),
            TextRange(lines[1].start_index, lines[1].end_index));
}

TEST_F_UI(TextSelectionTest, SelectParagraphIgnoresVisualWrapping) {
  const std::u16string text = u"first second third fourth fifth sixth";
  text_view_->SetParagraph(CreateParagraph(text, 260.f), text);

  const auto& lines = text_view_->GetRenderText()
                          ->GetPainter()
                          ->GetParagraph()
                          ->GetLineMetrics();
  ASSERT_GE(lines.size(), 2u);
  EXPECT_EQ(
      text_view_->SelectParagraph(lines[1].start_index, Affinity::kDownstream),
      TextRange(0, text.size()));
}

TEST_F_UI(TextSelectionTest, SelectParagraphUsesHardBreakBoundaries) {
  const std::u16string text =
      u"first paragraph\nsecond \U0001F642 paragraph\r\nthird";
  text_view_->SetParagraph(CreateParagraph(text), text);

  EXPECT_EQ(text_view_->SelectParagraph(3), TextRange(0, 16));
  EXPECT_EQ(text_view_->GetRenderText()->GetSelectionString(),
            u"first paragraph\n");
  EXPECT_EQ(text_view_->SelectParagraph(23), TextRange(16, 37));
  EXPECT_EQ(text_view_->GetRenderText()->GetSelectionString(),
            u"second \U0001F642 paragraph\r\n");
  EXPECT_EQ(text_view_->SelectParagraph(text.size()),
            TextRange(37, text.size()));
  EXPECT_EQ(text_view_->GetRenderText()->GetSelectionString(), u"third");
}

TEST_F_UI(TextSelectionTest, SelectParagraphRespectsBreakAffinity) {
  const std::u16string text = u"first\nsecond";
  text_view_->SetParagraph(CreateParagraph(text), text);

  EXPECT_EQ(text_view_->SelectParagraph(6, Affinity::kUpstream),
            TextRange(0, 6));
  EXPECT_EQ(text_view_->SelectParagraph(6, Affinity::kDownstream),
            TextRange(6, text.size()));
}

TEST_F_UI(TextSelectionTest, SelectParagraphSupportsEmptyAndUnicodeParagraphs) {
  const std::u16string text = u"first\u2028\u2029last";
  text_view_->SetParagraph(CreateParagraph(text), text);

  EXPECT_EQ(text_view_->SelectParagraph(3), TextRange(0, 6));
  EXPECT_EQ(text_view_->SelectParagraph(6, Affinity::kDownstream),
            TextRange(6, 7));
  EXPECT_EQ(text_view_->SelectParagraph(7, Affinity::kDownstream),
            TextRange(7, text.size()));
}
#endif

}  // namespace clay
