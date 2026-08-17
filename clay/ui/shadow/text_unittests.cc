// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <initializer_list>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "clay/fml/icu_util.h"
#include "clay/public/layout_delegate.h"
#include "clay/public/style_types.h"
#include "clay/public/value.h"
#include "clay/ui/common/measure_constraint.h"
#include "clay/ui/component/text/raw_text_view.h"
#include "clay/ui/resource/font_collection.h"
#include "clay/ui/shadow/inline_image_shadow_node.h"
#include "clay/ui/shadow/inline_text_shadow_node.h"
#include "clay/ui/shadow/inline_truncation_shadow_node.h"
#include "clay/ui/shadow/inline_view_shadow_node.h"
#include "clay/ui/shadow/raw_text_shadow_node.h"
#include "clay/ui/shadow/shadow_node_owner.h"
#include "clay/ui/shadow/text_render.h"
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
  void LoadDataUriFont(const std::string& family_name) {
    FontCollection::Instance()->PreLoadFontOnMem(
        fml::MessageLoop::GetCurrent().GetTaskRunner(), nullptr, nullptr,
        family_name, {"data:font/ttf;base64,AA=="});
  }

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

TEST_F_UI(TextTest, EffectAlignResolvesLogicalAlignmentByDirection) {
  TextRender text_render(text_shadow_node_.get());

  text_shadow_node_->SetTextDirection(TextDirection::kLtr);
  text_shadow_node_->SetTextAlign(TextAlignment::kStart);
  EXPECT_EQ(text_render.EffectAlign(), TextAlignment::kLeft);
  text_shadow_node_->SetTextAlign(TextAlignment::kEnd);
  EXPECT_EQ(text_render.EffectAlign(), TextAlignment::kRight);

  text_shadow_node_->SetTextDirection(TextDirection::kRtl);
  text_shadow_node_->SetTextAlign(TextAlignment::kStart);
  EXPECT_EQ(text_render.EffectAlign(), TextAlignment::kRight);
  text_shadow_node_->SetTextAlign(TextAlignment::kEnd);
  EXPECT_EQ(text_render.EffectAlign(), TextAlignment::kLeft);

  text_shadow_node_->SetTextAlign(TextAlignment::kCenter);
  EXPECT_EQ(text_render.EffectAlign(), TextAlignment::kCenter);
  text_shadow_node_->SetTextAlign(TextAlignment::kJustify);
  EXPECT_EQ(text_render.EffectAlign(), TextAlignment::kJustify);

  text_shadow_node_->SetTextDirection(TextDirection::kNormal);
  text_shadow_node_->SetTextAlign(TextAlignment::kStart);
  EXPECT_EQ(text_shadow_node_->text_style_->text_direction,
            TextDirection::kLtr);
  EXPECT_EQ(text_render.EffectAlign(), TextAlignment::kLeft);
}

TEST_F_UI(TextTest, ExistingTextsResolveFontLoadedAfterStyleUpdate) {
  const std::string family_name = "font_loaded_after_existing_text";
  auto second_text =
      std::make_unique<TextShadowNode>(owner_, std::string("text"), -1);

  text_shadow_node_->SetFontFamily(family_name);
  second_text->SetFontFamily(family_name);
  ASSERT_TRUE(text_shadow_node_->text_style_.has_value());
  EXPECT_NE(text_shadow_node_->text_style_->font_family, family_name);
  ASSERT_TRUE(second_text->text_style_.has_value());
  EXPECT_NE(second_text->text_style_->font_family, family_name);

  LoadDataUriFont(family_name);

  EXPECT_EQ(text_shadow_node_->text_style_->font_family, family_name);
  EXPECT_EQ(second_text->text_style_->font_family, family_name);
}

TEST_F_UI(TextTest, FontCallbacksAreDeduplicatedAndCancelled) {
  const std::string family_name = "deduplicated_font_callback";
  auto font_collection = FontCollection::Instance();

  text_shadow_node_->SetFontFamily(family_name);
  text_shadow_node_->SetFontFamily(family_name);
  EXPECT_EQ(font_collection->font_download_callback_.count(family_name), 1u);

  auto second_text =
      std::make_unique<TextShadowNode>(owner_, std::string("text"), -1);
  second_text->SetFontFamily(family_name);
  EXPECT_EQ(font_collection->font_download_callback_.count(family_name), 2u);
  second_text.reset();
  EXPECT_EQ(font_collection->font_download_callback_.count(family_name), 1u);

  LoadDataUriFont(family_name);
  EXPECT_EQ(font_collection->font_download_callback_.count(family_name), 0u);
  EXPECT_EQ(text_shadow_node_->text_style_->font_family, family_name);
}

TEST_F_UI(TextTest, StaleFontLoadDoesNotOverrideCurrentFontRequest) {
  const std::string old_family = "old_async_font";
  const std::string current_family = "current_async_font";

  text_shadow_node_->SetFontFamily(old_family);
  text_shadow_node_->SetFontFamily(current_family);
  LoadDataUriFont(old_family);

  EXPECT_NE(text_shadow_node_->text_style_->font_family, old_family);

  LoadDataUriFont(current_family);
  EXPECT_EQ(text_shadow_node_->text_style_->font_family, current_family);
}

TEST_F_UI(TextTest, AsyncFallbackCompletionKeepsFontFamilyPriority) {
  const std::string preferred_family = "preferred_async_font";
  const std::string fallback_family = "fallback_async_font";

  text_shadow_node_->SetFontFamily(preferred_family + ", " + fallback_family);
  LoadDataUriFont(fallback_family);
  EXPECT_EQ(text_shadow_node_->text_style_->font_family, fallback_family);

  LoadDataUriFont(preferred_family);
  EXPECT_EQ(text_shadow_node_->text_style_->font_family, preferred_family);
}

#if defined(CLAY_ENABLE_SKSHAPER)
TEST_F_UI(TextTest, SkParagraphMixedBidiRunsExposeDirectionAndGeometry) {
  fml::icu::InitializeICU("icudtl.dat");
  const std::u16string text = u"abc \u05d0\u05d1\u05d2 xyz";
  raw_text_shadow_node_->SetText(text);
  text_shadow_node_->SetTextDirection(TextDirection::kLtr);
  MeasureConstraint constraint{1000.f, MeasureMode::kDefinite, std::nullopt,
                               MeasureMode::kIndefinite};
  TextRender text_render(text_shadow_node_.get());
  text_render.SetUpdateFlag(TextUpdateFlag::kUpdateFlagChildren);
  auto context = text_shadow_node_->CreateLayoutContext(constraint);

  text_render.Measure(constraint, &context);

  ASSERT_NE(text_render.GetCacheParagraph(), nullptr);
  auto* paragraph = text_render.GetCacheParagraph();
  auto left_latin_boxes =
      paragraph->GetRectsForRange(0, 3, txt::Paragraph::RectHeightStyle::kTight,
                                  txt::Paragraph::RectWidthStyle::kTight);
  auto rtl_boxes =
      paragraph->GetRectsForRange(4, 7, txt::Paragraph::RectHeightStyle::kTight,
                                  txt::Paragraph::RectWidthStyle::kTight);
  auto right_latin_boxes = paragraph->GetRectsForRange(
      8, 11, txt::Paragraph::RectHeightStyle::kTight,
      txt::Paragraph::RectWidthStyle::kTight);

  ASSERT_FALSE(left_latin_boxes.empty());
  ASSERT_FALSE(rtl_boxes.empty());
  ASSERT_FALSE(right_latin_boxes.empty());
  EXPECT_EQ(left_latin_boxes.front().direction, txt::TextDirection::ltr);
  EXPECT_EQ(rtl_boxes.front().direction, txt::TextDirection::rtl);
  EXPECT_EQ(right_latin_boxes.front().direction, txt::TextDirection::ltr);
  EXPECT_GT(left_latin_boxes.front().rect.Width(), 0.f);
  EXPECT_GT(rtl_boxes.front().rect.Width(), 0.f);
  EXPECT_GT(right_latin_boxes.front().rect.Width(), 0.f);
}
#endif

TEST_F_UI(TextTest, TextDecorationAttributeMapsLineStyleAndColor) {
  clay::Value::Array value;
  value.emplace_back(static_cast<int>(ClayTextDecorationType::kUnderLine) |
                     static_cast<int>(ClayTextDecorationType::kLineThrough));
  value.emplace_back(static_cast<int>(TextDecorationStyle::kDashed));
  value.emplace_back(static_cast<uint32_t>(0xff123456));

  text_shadow_node_->SetAttribute("text-decoration",
                                  clay::Value(std::move(value)));

  ASSERT_TRUE(text_shadow_node_->text_style_->text_decoration.has_value());
  const auto& decoration =
      text_shadow_node_->text_style_->text_decoration.value();
  EXPECT_EQ(decoration.line,
            static_cast<uint8_t>(kTextDecorationLineUnderline |
                                 kTextDecorationLineLineThrough));
  EXPECT_EQ(decoration.style, TextDecorationStyle::kDashed);
  EXPECT_EQ(decoration.color, Color(0xff123456));
}

TEST_F_UI(TextTest, FontAndShadowSettersPreserveStyleMetadata) {
  text_shadow_node_->SetFontWeight(FontWeight::k700);
  text_shadow_node_->SetFontStyle(FontStyle::kItalic);
  text_shadow_node_->SetLetterSpacing(1.25f);
  text_shadow_node_->SetLineSpacing(48.f);
  text_shadow_node_->SetTextBackgroundColor(Color(0xffabcdef));
  const std::vector<Shadow> expected_shadows = {
      {/* inset */ false, 1.f, 2.f, 3.f, 4.f, Color(0xff010203)},
      {/* inset */ true, -5.f, 6.f, 7.f, 8.f, Color(0xffaabbcc)},
  };
  clay::Value::Array shadow_values;
  for (const auto& shadow : expected_shadows) {
    clay::Value::Array entry;
    entry.emplace_back(shadow.offset_x);
    entry.emplace_back(shadow.offset_y);
    entry.emplace_back(shadow.blur_radius);
    entry.emplace_back(shadow.spread_radius);
    entry.emplace_back(static_cast<int>(
        shadow.inset ? ClayShadowOption::kInset : ClayShadowOption::kNone));
    entry.emplace_back(shadow.color.argb);
    shadow_values.emplace_back(clay::Value(std::move(entry)));
  }
  text_shadow_node_->SetAttribute("text-shadow",
                                  clay::Value(std::move(shadow_values)));

  EXPECT_EQ(text_shadow_node_->text_style_->font_weight, FontWeight::k700);
  EXPECT_EQ(text_shadow_node_->text_style_->font_style, FontStyle::kItalic);
  EXPECT_EQ(text_shadow_node_->text_style_->letter_spacing, 1.25f);
  EXPECT_EQ(text_shadow_node_->text_style_->line_spacing, 48.f);
  EXPECT_EQ(text_shadow_node_->text_style_->background_color,
            Color(0xffabcdef));
  EXPECT_EQ(text_shadow_node_->text_style_->text_shadows, expected_shadows);
}

TEST_F_UI(TextTest, TextGradientAndSolidColorUpdatePaintState) {
  const Gradient gradient;

  text_shadow_node_->SetTextGradient(gradient);
  EXPECT_EQ(text_shadow_node_->text_style_->text_gradient, gradient);
  EXPECT_EQ(text_shadow_node_->text_style_->foreground_id,
            text_shadow_node_->id());

  text_shadow_node_->SetAttribute(
      "color", clay::Value(static_cast<uint32_t>(0xff102030)));

  EXPECT_FALSE(text_shadow_node_->text_style_->text_gradient.has_value());
  EXPECT_EQ(text_shadow_node_->text_style_->text_color, Color(0xff102030));
}

TEST_F_UI(TextTest, TextStrokeSettersMarkStyleDirty) {
  text_shadow_node_->OnLayout(100.f, TextMeasureMode::kDefinite, 100.f,
                              TextMeasureMode::kDefinite, {}, {});
  EXPECT_FALSE(text_shadow_node_->IsDirty());

  text_shadow_node_->SetTextStrokeColor(Color(0xff112233));
  EXPECT_TRUE(text_shadow_node_->IsDirty());
  EXPECT_TRUE(text_shadow_node_->text_style_.has_value());

  text_shadow_node_->OnLayout(100.f, TextMeasureMode::kDefinite, 100.f,
                              TextMeasureMode::kDefinite, {}, {});
  text_shadow_node_->SetTextStrokeColor(Color(0xff112233));
  EXPECT_FALSE(text_shadow_node_->IsDirty());

  text_shadow_node_->SetTextStrokeWidth(2.5);
  EXPECT_TRUE(text_shadow_node_->IsDirty());

  text_shadow_node_->OnLayout(100.f, TextMeasureMode::kDefinite, 100.f,
                              TextMeasureMode::kDefinite, {}, {});
  text_shadow_node_->SetTextStrokeWidth(2.5);
  EXPECT_FALSE(text_shadow_node_->IsDirty());
}

TEST_F_UI(TextTest, GetLineInfoIsEmptyBeforeParagraphLayout) {
  TextRender text_render(text_shadow_node_.get());

  EXPECT_TRUE(text_render.GetLineInfo().empty());
}

TEST_F_UI(TextTest, GetTextInfoHonorsDefaultAndExplicitMaxLine) {
  const std::string text =
      "one two three four five six seven eight nine ten eleven twelve";
  auto get_content = [&](std::optional<double> max_line) {
    clay::Value::Map params;
    params.emplace("pixelRatio", clay::Value(1));
    params.emplace("fontSize", clay::Value("20"));
    params.emplace("maxWidth", clay::Value("80"));
    if (max_line.has_value()) {
      params.emplace("maxLine", clay::Value(max_line.value()));
    }
    auto result =
        TextRender::GetTextInfo(text.c_str(), clay::Value(std::move(params)));
    EXPECT_TRUE(result.IsMap());
    return result.GetMap().at("content").GetArray().size();
  };

  EXPECT_EQ(get_content(std::nullopt), 1u);
  EXPECT_EQ(get_content(3.0), 3u);
}

TEST_F_UI(TextTest, MeasureTextWrapsContentAtWidth) {
  const TextStyle text_style = text_shadow_node_->text_style_.value();
  const std::string text =
      "one two three four five six seven eight nine ten eleven twelve";
  float wide_height = 0.f;
  float narrow_height = 0.f;
  std::vector<std::string> wrapped_content;

  TextRender::MeasureText(text, false, 10000.0, std::nullopt, text_style,
                          nullptr, &wide_height, nullptr);
  TextRender::MeasureText(text, true, 80.0, std::nullopt, text_style, nullptr,
                          &narrow_height, &wrapped_content);

  EXPECT_GT(wide_height, 0.f);
  EXPECT_GT(narrow_height, wide_height);
  EXPECT_GT(wrapped_content.size(), 1u);
}

#if defined(CLAY_ENABLE_SKSHAPER)
TEST_F_UI(TextTest, SkParagraphLineHeightChangesMultilineGeometryAtSameWidth) {
  fml::icu::InitializeICU("icudtl.dat");
  raw_text_shadow_node_->SetText("first line\nsecond line");
  text_shadow_node_->SetFontSize(20.f);
  MeasureConstraint constraint{500.f, MeasureMode::kDefinite, std::nullopt,
                               MeasureMode::kIndefinite};
  TextRender text_render(text_shadow_node_.get());

  auto measure = [&](float line_height) {
    text_shadow_node_->SetLineHeight(line_height);
    text_shadow_node_->UpdateLineHeight();
    text_render.SetUpdateFlag(TextUpdateFlag::kUpdateFlagChildren);
    auto context = text_shadow_node_->CreateLayoutContext(constraint);
    text_render.Measure(constraint, &context);
    EXPECT_NE(text_render.GetCacheParagraph(), nullptr);
    return std::pair{context.measured_height_,
                     text_render.GetCacheParagraph()->GetLineMetrics()};
  };

  auto [short_height, short_metrics] = measure(40.f);
  ASSERT_EQ(short_metrics.size(), 2u);
  const double short_baseline_delta =
      short_metrics[1].baseline - short_metrics[0].baseline;

  auto [tall_height, tall_metrics] = measure(60.f);
  ASSERT_EQ(tall_metrics.size(), 2u);
  const double tall_baseline_delta =
      tall_metrics[1].baseline - tall_metrics[0].baseline;

  EXPECT_GT(tall_height, short_height);
  EXPECT_GT(tall_metrics.back().height, short_metrics.back().height);
  EXPECT_GT(tall_baseline_delta, short_baseline_delta);
}
#endif

TEST_F_UI(TextTest, PercentTextIndentTracksLayoutWidth) {
  raw_text_shadow_node_->SetText("indented text");
  text_shadow_node_->text_indent_use_percent_ = true;
  text_shadow_node_->text_indent_ = 0.1f;

  MeasureConstraint narrow_constraint{200.f, MeasureMode::kDefinite,
                                      std::nullopt, MeasureMode::kIndefinite};
  text_shadow_node_->Measure(narrow_constraint);
  ASSERT_TRUE(text_shadow_node_->text_style_->text_indent.has_value());
  EXPECT_FLOAT_EQ(text_shadow_node_->text_style_->text_indent.value(), 20.f);

  MeasureConstraint wide_constraint{400.f, MeasureMode::kDefinite, std::nullopt,
                                    MeasureMode::kIndefinite};
  text_shadow_node_->Measure(wide_constraint);
  ASSERT_TRUE(text_shadow_node_->text_style_->text_indent.has_value());
  EXPECT_FLOAT_EQ(text_shadow_node_->text_style_->text_indent.value(), 40.f);
}

TEST_F_UI(TextTest, TextIndentAttributeResolvesFixedAndPercentValues) {
  auto make_indent = [](double value, bool use_percent) {
    clay::Value::Array array;
    array.emplace_back(value);
    array.emplace_back(use_percent ? 1 : 0);
    return clay::Value(std::move(array));
  };
  raw_text_shadow_node_->SetText("indented text");
  MeasureConstraint constraint{200.f, MeasureMode::kDefinite, std::nullopt,
                               MeasureMode::kIndefinite};

  text_shadow_node_->SetAttribute("text-indent", make_indent(10.0, false));
  text_shadow_node_->Measure(constraint);
  ASSERT_TRUE(text_shadow_node_->text_style_->text_indent.has_value());
  EXPECT_FLOAT_EQ(text_shadow_node_->text_style_->text_indent.value(), 10.f);

  text_shadow_node_->SetAttribute("text-indent", make_indent(0.1, true));
  text_shadow_node_->Measure(constraint);
  ASSERT_TRUE(text_shadow_node_->text_style_->text_indent.has_value());
  EXPECT_FLOAT_EQ(text_shadow_node_->text_style_->text_indent.value(), 20.f);
}

TEST_F_UI(TextTest, TextMaxLineAttributeHandlesZeroAndPositiveValues) {
  text_shadow_node_->SetAttribute("text-maxline", clay::Value("0"));
  ASSERT_TRUE(text_shadow_node_->text_style_->max_lines.has_value());
  EXPECT_EQ(text_shadow_node_->text_style_->max_lines.value(),
            std::numeric_limits<uint32_t>::max());

  text_shadow_node_->SetAttribute("text-maxline", clay::Value("3"));
  EXPECT_EQ(text_shadow_node_->text_style_->max_lines.value(), 3u);
}

TEST_F_UI(TextTest, TextMaxLengthAttributeLimitsInitialLayout) {
  MeasureConstraint constraint{1000.f, MeasureMode::kAtMost, std::nullopt,
                               MeasureMode::kIndefinite};
  auto measure_width = [&](int max_length) {
    auto raw_text = std::make_unique<RawTextShadowNode>(
        owner_, std::string("raw-text"), -1);
    auto text =
        std::make_unique<TextShadowNode>(owner_, std::string("text"), -1);
    raw_text->SetText("WWWWWWWW");
    text->AddChild(raw_text.get());
    text->SetFontSize(42);
    text->SetAttribute("text-maxlength", clay::Value(max_length));
    return text->Measure(constraint).width;
  };

  EXPECT_GT(measure_width(6), measure_width(3));
}

TEST_F_UI(TextTest, GetLineInfoReportsMaxLineEllipsis) {
  text_shadow_node_->SetTextMaxLine(1);
  text_shadow_node_->SetTextOverflow(TextOverflow::kEllipsis);
  raw_text_shadow_node_->SetText(
      "one two three four five six seven eight nine ten eleven twelve ");
  MeasureConstraint constraint{120.f, MeasureMode::kDefinite, std::nullopt,
                               MeasureMode::kIndefinite};
  TextRender text_render(text_shadow_node_.get());
  text_render.SetUpdateFlag(TextUpdateFlag::kUpdateFlagChildren);
  auto context = text_shadow_node_->CreateLayoutContext(constraint);

  text_render.Measure(constraint, &context);

  ASSERT_NE(text_render.GetCacheParagraph(), nullptr);
  EXPECT_TRUE(text_render.GetCacheParagraph()->DidExceedMaxLines());
  auto lines = text_render.GetLineInfo();
  ASSERT_EQ(lines.size(), 1u);
  EXPECT_EQ(lines.front().start, 0);
  EXPECT_GT(lines.front().end, lines.front().start);
  EXPECT_GT(lines.front().ellipsis_count, 0);
  EXPECT_LT(static_cast<size_t>(lines.front().end),
            raw_text_shadow_node_->Text().size());
}

TEST_F_UI(TextTest, GetLineInfoReportsEllipsisOnlyOnLastVisibleLine) {
  text_shadow_node_->SetTextMaxLine(3);
  text_shadow_node_->SetTextOverflow(TextOverflow::kEllipsis);
  raw_text_shadow_node_->SetText(
      "one two three four five six seven eight nine ten eleven twelve "
      "thirteen fourteen fifteen sixteen seventeen eighteen");
  MeasureConstraint constraint{120.f, MeasureMode::kDefinite, std::nullopt,
                               MeasureMode::kIndefinite};
  TextRender text_render(text_shadow_node_.get());
  text_render.SetUpdateFlag(TextUpdateFlag::kUpdateFlagChildren);
  auto context = text_shadow_node_->CreateLayoutContext(constraint);

  text_render.Measure(constraint, &context);

  ASSERT_NE(text_render.GetCacheParagraph(), nullptr);
  EXPECT_TRUE(text_render.GetCacheParagraph()->DidExceedMaxLines());
  auto lines = text_render.GetLineInfo();
  ASSERT_EQ(lines.size(), 3u);
  EXPECT_EQ(lines[0].ellipsis_count, 0);
  EXPECT_EQ(lines[1].ellipsis_count, 0);
  EXPECT_GT(lines[2].ellipsis_count, 0);
  EXPECT_EQ(lines[0].start, 0);
  EXPECT_LE(lines[0].end, lines[1].start);
  EXPECT_LE(lines[1].end, lines[2].start);
  EXPECT_LT(lines[2].start, lines[2].end);
}

TEST_F_UI(TextTest, NoWrapKeepsOverflowingTextOnOneLine) {
  raw_text_shadow_node_->SetText(
      "one two three four five six seven eight nine ten eleven twelve ");
  text_shadow_node_->SetWhiteSpaceType(WhiteSpace::kNoWrap);
  text_shadow_node_->SetTextOverflow(TextOverflow::kClip);
  MeasureConstraint constraint{80.f, MeasureMode::kDefinite, std::nullopt,
                               MeasureMode::kIndefinite};
  TextRender text_render(text_shadow_node_.get());
  text_render.SetUpdateFlag(TextUpdateFlag::kUpdateFlagStyle);
  auto context = text_shadow_node_->CreateLayoutContext(constraint);

  text_render.Measure(constraint, &context);

  ASSERT_NE(text_render.GetCacheParagraph(), nullptr);
  EXPECT_EQ(text_render.GetCacheParagraph()->GetLineMetrics().size(), 1u);
  EXPECT_GT(text_render.GetCacheParagraph()->GetMinIntrinsicWidth(),
            constraint.width.value());
  auto lines = text_render.GetLineInfo();
  ASSERT_EQ(lines.size(), 1u);
  EXPECT_EQ(lines.front().ellipsis_count, 0);
}

TEST_F_UI(TextTest, NoWrapEllipsisUsesWidthConstraint) {
  raw_text_shadow_node_->SetText(
      "one two three four five six seven eight nine ten eleven twelve ");
  text_shadow_node_->SetWhiteSpaceType(WhiteSpace::kNoWrap);
  text_shadow_node_->SetTextOverflow(TextOverflow::kEllipsis);
  MeasureConstraint constraint{80.f, MeasureMode::kDefinite, std::nullopt,
                               MeasureMode::kIndefinite};
  TextRender text_render(text_shadow_node_.get());
  text_render.SetUpdateFlag(TextUpdateFlag::kUpdateFlagStyle);
  auto context = text_shadow_node_->CreateLayoutContext(constraint);

  text_render.Measure(constraint, &context);

  ASSERT_NE(text_render.GetCacheParagraph(), nullptr);
  EXPECT_TRUE(text_render.GetCacheParagraph()->DidExceedMaxLines());
  EXPECT_EQ(text_render.GetCacheParagraph()->GetLineMetrics().size(), 1u);
  EXPECT_GT(text_render.GetCacheParagraph()->GetMinIntrinsicWidth(),
            constraint.width.value());
  auto lines = text_render.GetLineInfo();
  ASSERT_EQ(lines.size(), 1u);
  EXPECT_GT(lines.front().ellipsis_count, 0);
}

TEST_F_UI(TextTest, InlineTruncationUpdatesBetweenLongAndShortContent) {
  MeasureConstraint constraint{240, MeasureMode::kDefinite, 100,
                               MeasureMode::kDefinite};
  auto inline_truncation_node = std::make_unique<InlineTruncationShadowNode>(
      owner_, std::string("inline-truncation"), -1);
  auto inline_text_node = std::make_unique<InlineTextShadowNode>(
      owner_, std::string("inline-text"), -1);
  auto inline_raw_text_shadow_node =
      std::make_unique<RawTextShadowNode>(owner_, std::string("raw-text"), -1);
  inline_raw_text_shadow_node->SetText("...");
  inline_text_node->AddChild(inline_raw_text_shadow_node.get());
  inline_truncation_node->AddChild(inline_text_node.get());
  text_shadow_node_->AddChild(inline_truncation_node.get());
  text_shadow_node_->SetTextMaxLine(1);
  const std::string long_text =
      "one two three four five six seven eight nine ten eleven twelve "
      "thirteen fourteen fifteen";
  TextRender text_render(text_shadow_node_.get());

  auto measure = [&](const std::string& text) {
    raw_text_shadow_node_->SetText(text);
    text_render.SetUpdateFlag(TextUpdateFlag::kUpdateFlagChildren);
    auto context = text_shadow_node_->CreateLayoutContext(constraint);
    text_render.Measure(constraint, &context);
  };

  measure(long_text);
  EXPECT_TRUE(inline_truncation_node->IfNeedMount());
  EXPECT_LT(raw_text_shadow_node_->GetEndIndex(), long_text.size());

  measure("short");
  EXPECT_FALSE(inline_truncation_node->IfNeedMount());
  EXPECT_EQ(raw_text_shadow_node_->GetEndIndex(), 5u);

  measure(long_text);
  EXPECT_TRUE(inline_truncation_node->IfNeedMount());
  EXPECT_LT(raw_text_shadow_node_->GetEndIndex(), long_text.size());
}

TEST_F_UI(TextTest, ReusesSameWidthLayoutAndLaysOutForNarrowerWidth) {
  raw_text_shadow_node_->SetText(
      "one two three four five six seven eight nine ten eleven twelve ");
  MeasureConstraint wide_constraint{10000.f, MeasureMode::kDefinite,
                                    std::nullopt, MeasureMode::kIndefinite};
  TextRender text_render(text_shadow_node_.get());
  text_render.SetUpdateFlag(TextUpdateFlag::kUpdateFlagChildren);
  auto wide_context = text_shadow_node_->CreateLayoutContext(wide_constraint);
  text_render.Measure(wide_constraint, &wide_context);
  auto* wide_paragraph = text_render.GetCacheParagraph();
  ASSERT_NE(wide_paragraph, nullptr);
  const size_t wide_line_count = wide_paragraph->GetLineMetrics().size();
  ASSERT_EQ(wide_line_count, 1u);

  auto same_width_context =
      text_shadow_node_->CreateLayoutContext(wide_constraint);
  text_render.Measure(wide_constraint, &same_width_context);

  EXPECT_EQ(text_render.GetCacheParagraph(), wide_paragraph);
  EXPECT_EQ(text_render.GetCacheParagraph()->GetLineMetrics().size(),
            wide_line_count);

  MeasureConstraint narrow_constraint{120.f, MeasureMode::kDefinite,
                                      std::nullopt, MeasureMode::kIndefinite};
  auto narrow_context =
      text_shadow_node_->CreateLayoutContext(narrow_constraint);
  text_render.Measure(narrow_constraint, &narrow_context);

  ASSERT_NE(text_render.GetCacheParagraph(), nullptr);
  EXPECT_GT(text_render.GetCacheParagraph()->GetLineMetrics().size(),
            wide_line_count);
}

TEST_F_UI(TextTest, WhiteSpaceStyleUpdateLaysOutAtSameWidth) {
  raw_text_shadow_node_->SetText(
      "one two three four five six seven eight nine ten eleven twelve ");
  text_shadow_node_->SetWhiteSpaceType(WhiteSpace::kNoWrap);
  text_shadow_node_->SetTextOverflow(TextOverflow::kClip);
  MeasureConstraint constraint{120.f, MeasureMode::kDefinite, std::nullopt,
                               MeasureMode::kIndefinite};
  auto nowrap_result = text_shadow_node_->Measure(constraint);
  EXPECT_GT(nowrap_result.height, 0.f);

  text_shadow_node_->SetWhiteSpaceType(WhiteSpace::kNormal);
  auto normal_result = text_shadow_node_->Measure(constraint);

  EXPECT_GT(normal_result.height, nowrap_result.height * 1.5f);
}

TEST_F_UI(TextTest, RawTextMappingsKeepUnicodeCodePointsIntact) {
  raw_text_shadow_node_->SetText(u"A\U0001F600B");

  EXPECT_EQ(raw_text_shadow_node_->GetLayoutTextLength(), 3u);
  EXPECT_EQ(raw_text_shadow_node_->GetLayoutTextUtf16Length(), 4u);
  EXPECT_EQ(raw_text_shadow_node_->GetRawEndIndexForLayoutTextLength(2), 3u);
  EXPECT_EQ(raw_text_shadow_node_->GetRawEndIndexForLayoutTextUtf16Length(2),
            3u);
  EXPECT_EQ(raw_text_shadow_node_->GetRawEndIndexForLayoutTextUtf16Length(3),
            3u);
}

TEST_F_UI(TextTest, InlinePlaceholderGlyphRangesAreContiguous) {
  raw_text_shadow_node_->SetText(u"A\U0001F600");
  auto inline_image =
      std::make_unique<InlineImageShadowNode>(owner_, std::string("image"), 7);
  auto inline_view =
      std::make_unique<InlineViewShadowNode>(owner_, std::string("view"), 8);
  inline_image->SetWidth(16.f);
  inline_image->SetHeight(16.f);
  inline_view->SetWidth(16.f);
  inline_view->SetHeight(16.f);
  inline_text_shadow_node_->AddChild(inline_image.get());
  inline_text_shadow_node_->AddChild(inline_view.get());
  MeasureConstraint constraint{1000.f, MeasureMode::kDefinite, std::nullopt,
                               MeasureMode::kIndefinite};
  TextRender text_render(text_shadow_node_.get());
  text_render.SetUpdateFlag(TextUpdateFlag::kUpdateFlagChildren);
  auto context = text_shadow_node_->CreateLayoutContext(constraint);

  text_render.Measure(constraint, &context);

  ASSERT_GE(inline_image->placeholder_index(), 0);
  ASSERT_GE(inline_view->placeholder_index(), 0);
  EXPECT_GT(inline_image->StartGlyph(), 0u);
  EXPECT_EQ(inline_image->EndGlyph(), inline_image->StartGlyph() + 1);
  EXPECT_EQ(inline_view->StartGlyph(), inline_image->EndGlyph());
  EXPECT_EQ(inline_view->EndGlyph(), inline_view->StartGlyph() + 1);
}

TEST_F_UI(TextTest, CollapsedWhitespaceMappingsPointBackToRawUnicodeText) {
  const std::u16string raw_text = u"A  \U0001F600 \tB";
  const std::u16string expected_layout_text = u"A \U0001F600 B";
  std::vector<size_t> utf16_to_raw_end;
  std::vector<size_t> utf32_to_raw_end;

  auto layout_text = raw_text_shadow_node_->CollapsesWhitespaces(
      raw_text, &utf16_to_raw_end, &utf32_to_raw_end);

  EXPECT_TRUE(layout_text == expected_layout_text);
  EXPECT_EQ(utf16_to_raw_end,
            (std::vector<size_t>{0u, 1u, 3u, 5u, 5u, 7u, 8u}));
  EXPECT_EQ(utf32_to_raw_end, (std::vector<size_t>{0u, 1u, 3u, 5u, 7u, 8u}));
}

TEST_F_UI(TextTest, BreakAllDoesNotSplitSurrogateOrZwjSequences) {
  text_shadow_node_->SetWordBreak(WordBreak::kBreakAll);

  EXPECT_TRUE(raw_text_shadow_node_->ProcessWordBreakIfNeed(u"A\U0001F600B") ==
              u"A\u200B\U0001F600\u200BB");
  EXPECT_TRUE(raw_text_shadow_node_->ProcessWordBreakIfNeed(
                  u"A\u200D\U0001F600B") == u"A\u200D\U0001F600\u200BB");
}

TEST_F_UI(TextTest, KeepAllJoinsOnlyAdjacentCjkCharacters) {
  text_shadow_node_->SetWordBreak(WordBreak::kKeepAll);

  // The first two CJK characters mean "Chinese language"; the final one
  // means "text". ASCII 'A' separates the CJK runs, so U+2060 WORD JOINER
  // is inserted only between the first two adjacent characters.
  EXPECT_TRUE(raw_text_shadow_node_->ProcessWordBreakIfNeed(u"中文A文") ==
              u"中\u2060文A文");
}

TEST_F_UI(TextTest, ProcessTruncationDoesNotSplitSurrogatePair) {
  raw_text_shadow_node_->SetText(u"A\U0001F600B");
  TextRender text_render(text_shadow_node_.get());
  size_t display_glyph_num = 2;

  text_render.ProcessTruncationContent(display_glyph_num,
                                       text_shadow_node_.get());

  EXPECT_EQ(display_glyph_num, 0u);
  EXPECT_EQ(raw_text_shadow_node_->GetEndIndex(), 3u);
  EXPECT_TRUE(raw_text_shadow_node_->Text() == u"A\U0001F600");
}

TEST_F_UI(TextTest, ProcessTruncationTraversesNestedTextAndPlaceholders) {
  raw_text_shadow_node_->SetText("ab");
  auto nested_raw_text =
      std::make_unique<RawTextShadowNode>(owner_, std::string("raw-text"), -1);
  nested_raw_text->SetText("cd");
  auto inline_view =
      std::make_unique<InlineViewShadowNode>(owner_, std::string("view"), 7);
  auto inline_image =
      std::make_unique<InlineImageShadowNode>(owner_, std::string("image"), 8);
  inline_text_shadow_node_->AddChild(nested_raw_text.get());
  inline_text_shadow_node_->AddChild(inline_view.get());
  inline_text_shadow_node_->AddChild(inline_image.get());
  TextRender text_render(text_shadow_node_.get());
  size_t display_glyph_num = 5;

  text_render.ProcessTruncationContent(display_glyph_num,
                                       text_shadow_node_.get());

  EXPECT_EQ(display_glyph_num, 0u);
  EXPECT_EQ(raw_text_shadow_node_->GetEndIndex(), 2u);
  EXPECT_EQ(nested_raw_text->GetEndIndex(), 2u);
  EXPECT_NE(inline_view->GetEndIndex(), 0u);
  EXPECT_EQ(inline_image->GetEndIndex(), 0u);

  text_shadow_node_->ResetEndIndex();
  display_glyph_num = 3;
  text_render.ProcessTruncationContent(display_glyph_num,
                                       text_shadow_node_.get());

  EXPECT_EQ(display_glyph_num, 0u);
  EXPECT_EQ(nested_raw_text->GetEndIndex(), 1u);
  EXPECT_EQ(inline_view->GetEndIndex(), 0u);
  EXPECT_EQ(inline_image->GetEndIndex(), 0u);
}

TEST_F_UI(TextTest, ProcessTruncationUsesCollapsedWhitespaceUnicodeMapping) {
  raw_text_shadow_node_->SetText(u"A  \U0001F600 \tB");
  text_shadow_node_->SetWhiteSpaceType(WhiteSpace::kNormal);
  MeasureConstraint constraint{1000.f, MeasureMode::kDefinite, std::nullopt,
                               MeasureMode::kIndefinite};
  TextRender text_render(text_shadow_node_.get());
  text_render.SetUpdateFlag(TextUpdateFlag::kUpdateFlagChildren);
  auto context = text_shadow_node_->CreateLayoutContext(constraint);
  text_render.Measure(constraint, &context);

  EXPECT_EQ(raw_text_shadow_node_->GetLayoutTextLength(), 5u);
  EXPECT_EQ(raw_text_shadow_node_->GetLayoutTextUtf16Length(), 6u);

  size_t display_glyph_num = 3;
  text_render.ProcessTruncationContent(display_glyph_num,
                                       text_shadow_node_.get());

  EXPECT_EQ(display_glyph_num, 0u);
  EXPECT_EQ(raw_text_shadow_node_->GetEndIndex(), 5u);
  EXPECT_TRUE(raw_text_shadow_node_->Text() == u"A  \U0001F600");
}

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

TEST_F_UI(TextTest, AutoFontSizePresetExpandsToLargestFittingSize) {
  MeasureConstraint constraint{1000, MeasureMode::kDefinite, 1000,
                               MeasureMode::kDefinite};
  text_shadow_node_->SetFontSize(20);
  text_shadow_node_->enable_auto_font_size_ = true;
  text_shadow_node_->auto_font_size_preset_sizes_ =
      std::vector<double>{20, 30, 40};
  raw_text_shadow_node_->SetText("A");

  text_shadow_node_->Measure(constraint);

  EXPECT_EQ(text_shadow_node_->text_style_->font_size, 40);
}

TEST_F_UI(TextTest, AutoFontSizePresetAttributeSortsValues) {
  auto make_preset_sizes = [](std::initializer_list<double> sizes) {
    clay::Value::Array array;
    for (double size : sizes) {
      array.emplace_back(size);
    }
    return clay::Value(std::move(array));
  };

  text_shadow_node_->SetAttribute("-x-auto-font-size-preset-sizes",
                                  make_preset_sizes({20, 10}));
  EXPECT_EQ(text_shadow_node_->auto_font_size_preset_sizes_,
            (std::vector<double>{10, 20}));
}

TEST_F_UI(TextTest, AutoFontSizeAttributeUpdatesConfiguration) {
  auto make_config = [](bool enabled, double min_size, double max_size,
                        double step) {
    clay::Value::Array array;
    array.emplace_back(enabled);
    array.emplace_back(min_size);
    array.emplace_back(max_size);
    array.emplace_back(step);
    return clay::Value(std::move(array));
  };

  text_shadow_node_->SetAttribute("-x-auto-font-size",
                                  make_config(true, 10, 30, 2));
  EXPECT_TRUE(text_shadow_node_->enable_auto_font_size_);
  EXPECT_DOUBLE_EQ(text_shadow_node_->auto_font_size_min_size_, 10);
  EXPECT_DOUBLE_EQ(text_shadow_node_->auto_font_size_max_size_, 30);
  EXPECT_DOUBLE_EQ(text_shadow_node_->auto_font_size_step_granularity_, 2);
}

TEST_F_UI(TextTest, AutoFontPresetAttributeSelectsLargestFittingSize) {
  clay::Value::Array config;
  config.emplace_back(true);
  config.emplace_back(10.0);
  config.emplace_back(50.0);
  config.emplace_back(1.0);
  text_shadow_node_->SetAttribute("-x-auto-font-size",
                                  clay::Value(std::move(config)));
  clay::Value::Array preset_sizes;
  preset_sizes.emplace_back(40.0);
  preset_sizes.emplace_back(20.0);
  preset_sizes.emplace_back(30.0);
  text_shadow_node_->SetAttribute("-x-auto-font-size-preset-sizes",
                                  clay::Value(std::move(preset_sizes)));
  text_shadow_node_->SetFontSize(20.f);
  raw_text_shadow_node_->SetText("A");
  MeasureConstraint constraint{1000.f, MeasureMode::kDefinite, 1000.f,
                               MeasureMode::kDefinite};

  const auto result = text_shadow_node_->Measure(constraint);

  EXPECT_EQ(text_shadow_node_->auto_font_size_preset_sizes_,
            (std::vector<double>{20, 30, 40}));
  EXPECT_EQ(text_shadow_node_->text_style_->font_size, 40.f);
  EXPECT_GT(result.width, 0.f);
  EXPECT_GT(result.height, 0.f);
}

TEST_F_UI(TextTest, InlineTruncationDoesNotMountWhenContentFits) {
  MeasureConstraint constraint{500, MeasureMode::kDefinite, 100,
                               MeasureMode::kDefinite};
  auto inline_truncation_node = std::make_unique<InlineTruncationShadowNode>(
      owner_, std::string("inline-truncation"), -1);
  auto inline_text_node = std::make_unique<InlineTextShadowNode>(
      owner_, std::string("inline-text"), -1);
  auto inline_raw_text_shadow_node =
      std::make_unique<RawTextShadowNode>(owner_, std::string("raw-text"), -1);
  inline_raw_text_shadow_node->SetText("more");
  inline_text_node->AddChild(inline_raw_text_shadow_node.get());
  inline_truncation_node->AddChild(inline_text_node.get());
  text_shadow_node_->AddChild(inline_truncation_node.get());
  text_shadow_node_->SetTextMaxLine(1);
  raw_text_shadow_node_->SetText("short");
  TextRender text_render(text_shadow_node_.get());
  text_render.SetUpdateFlag(TextUpdateFlag::kUpdateFlagChildren);
  auto context = text_shadow_node_->CreateLayoutContext(constraint);

  text_render.Measure(constraint, &context);

  EXPECT_FALSE(inline_truncation_node->IfNeedMount());
  EXPECT_TRUE(raw_text_shadow_node_->Text() == u"short");
}

TEST_F_UI(TextTest, InlineTruncationDoesNotMountMarkerWiderThanContainer) {
  MeasureConstraint constraint{40, MeasureMode::kDefinite, 100,
                               MeasureMode::kDefinite};
  auto inline_truncation_node = std::make_unique<InlineTruncationShadowNode>(
      owner_, std::string("inline-truncation"), -1);
  auto inline_text_node = std::make_unique<InlineTextShadowNode>(
      owner_, std::string("inline-text"), -1);
  auto inline_raw_text_shadow_node =
      std::make_unique<RawTextShadowNode>(owner_, std::string("raw-text"), -1);
  inline_raw_text_shadow_node->SetText(std::string(200, 'W'));
  inline_text_node->AddChild(inline_raw_text_shadow_node.get());
  inline_truncation_node->AddChild(inline_text_node.get());
  text_shadow_node_->AddChild(inline_truncation_node.get());
  text_shadow_node_->SetTextMaxLine(1);
  raw_text_shadow_node_->SetText(std::string(200, 'a'));
  TextRender text_render(text_shadow_node_.get());
  text_render.SetUpdateFlag(TextUpdateFlag::kUpdateFlagChildren);
  auto context = text_shadow_node_->CreateLayoutContext(constraint);

  text_render.Measure(constraint, &context);

  ASSERT_NE(text_render.GetCacheParagraph(), nullptr);
  EXPECT_TRUE(text_render.GetCacheParagraph()->DidExceedMaxLines());
  EXPECT_FALSE(inline_truncation_node->IfNeedMount());
  EXPECT_EQ(raw_text_shadow_node_->GetEndIndex(), 200u);
}

TEST_F_UI(TextTest, InlineTruncationMountsAndReportsHiddenText) {
  MeasureConstraint constraint{240, MeasureMode::kDefinite, 100,
                               MeasureMode::kDefinite};
  auto inline_truncation_node = std::make_unique<InlineTruncationShadowNode>(
      owner_, std::string("inline-truncation"), -1);
  auto inline_text_node = std::make_unique<InlineTextShadowNode>(
      owner_, std::string("inline-text"), -1);
  auto inline_raw_text_shadow_node =
      std::make_unique<RawTextShadowNode>(owner_, std::string("raw-text"), -1);
  inline_raw_text_shadow_node->SetText("...");
  inline_text_node->AddChild(inline_raw_text_shadow_node.get());
  inline_truncation_node->AddChild(inline_text_node.get());
  text_shadow_node_->AddChild(inline_truncation_node.get());
  text_shadow_node_->SetTextMaxLine(1);
  text_shadow_node_->SetTextOverflow(TextOverflow::kEllipsis);
  const std::string text =
      "This is a test text. We will use this text to test the function of "
      "inline-truncation. If this text exceeds the limited width, it will be "
      "truncated.";
  raw_text_shadow_node_->SetText(text);
  TextRender text_render(text_shadow_node_.get());
  text_render.SetUpdateFlag(TextUpdateFlag::kUpdateFlagChildren);
  auto context = text_shadow_node_->CreateLayoutContext(constraint);

  text_render.Measure(constraint, &context);

  EXPECT_TRUE(inline_truncation_node->IfNeedMount());
  EXPECT_LT(raw_text_shadow_node_->GetEndIndex(), text.size());
  auto lines = text_render.GetLineInfo();
  ASSERT_EQ(lines.size(), 1u);
  EXPECT_EQ(
      lines.front().ellipsis_count,
      static_cast<int>(text.size() - raw_text_shadow_node_->GetEndIndex()));
}

TEST_F_UI(TextTest,
          InlineTruncationExplicitRtlReachesParagraphAndReportsHiddenText) {
  MeasureConstraint constraint{240, MeasureMode::kDefinite, 100,
                               MeasureMode::kDefinite};
  auto inline_truncation_node = std::make_unique<InlineTruncationShadowNode>(
      owner_, std::string("inline-truncation"), -1);
  auto inline_text_node = std::make_unique<InlineTextShadowNode>(
      owner_, std::string("inline-text"), -1);
  auto inline_raw_text_shadow_node =
      std::make_unique<RawTextShadowNode>(owner_, std::string("raw-text"), -1);
  inline_raw_text_shadow_node->SetText("...");
  inline_text_node->AddChild(inline_raw_text_shadow_node.get());
  inline_truncation_node->AddChild(inline_text_node.get());
  text_shadow_node_->AddChild(inline_truncation_node.get());
  text_shadow_node_->SetTextDirection(TextDirection::kRtl);
  text_shadow_node_->SetTextMaxLine(1);
  const std::u16string text =
      u"\u05d0\u05d7\u05d3 \u05e9\u05ea\u05d9\u05d9\u05dd "
      u"\u05e9\u05dc\u05d5\u05e9 \u05d0\u05e8\u05d1\u05e2 \u05d7\u05de\u05e9 "
      u"\u05e9\u05e9 \u05e9\u05d1\u05e2 "
      u"\u05e9\u05de\u05d5\u05e0\u05d4 \u05ea\u05e9\u05e2 \u05e2\u05e9\u05e8 "
      u"\u05d0\u05d7\u05ea \u05e2\u05e9\u05e8\u05d4 \u05e9\u05ea\u05d9\u05dd "
      u"\u05e2\u05e9\u05e8\u05d4";
  raw_text_shadow_node_->SetText(text);
  TextRender text_render(text_shadow_node_.get());
  text_render.SetUpdateFlag(TextUpdateFlag::kUpdateFlagChildren);
  auto context = text_shadow_node_->CreateLayoutContext(constraint);

  text_render.BuildTextLayout(constraint, &context);
  ASSERT_NE(text_render.GetCacheParagraph(), nullptr);
  auto rtl_boxes = text_render.GetCacheParagraph()->GetRectsForRange(
      0, text.size(), txt::Paragraph::RectHeightStyle::kTight,
      txt::Paragraph::RectWidthStyle::kTight);
  ASSERT_FALSE(rtl_boxes.empty());
  EXPECT_EQ(rtl_boxes.front().direction, txt::TextDirection::rtl);

  text_render.HandleInlineTruncation(constraint, &context);

  EXPECT_TRUE(inline_truncation_node->IfNeedMount());
  EXPECT_LT(raw_text_shadow_node_->GetEndIndex(), text.size());
  auto lines = text_render.GetLineInfo();
  ASSERT_EQ(lines.size(), 1u);
  EXPECT_GT(lines.front().ellipsis_count, 0);
}

TEST_F_UI(TextTest, InlineTruncationHeightOverflowReportsHiddenText) {
  MeasureConstraint constraint{500, MeasureMode::kDefinite, 65,
                               MeasureMode::kDefinite};
  auto inline_truncation_node = std::make_unique<InlineTruncationShadowNode>(
      owner_, std::string("inline-truncation"), -1);
  auto inline_text_node = std::make_unique<InlineTextShadowNode>(
      owner_, std::string("inline-text"), -1);
  auto inline_raw_text_shadow_node =
      std::make_unique<RawTextShadowNode>(owner_, std::string("raw-text"), -1);
  inline_raw_text_shadow_node->SetText("...");
  inline_text_node->AddChild(inline_raw_text_shadow_node.get());
  inline_truncation_node->AddChild(inline_text_node.get());
  text_shadow_node_->AddChild(inline_truncation_node.get());
  const std::string text =
      "first hard-broken line\nsecond line that is hidden by the height";
  raw_text_shadow_node_->SetText(text);
  TextRender text_render(text_shadow_node_.get());
  text_render.SetUpdateFlag(TextUpdateFlag::kUpdateFlagChildren);
  auto context = text_shadow_node_->CreateLayoutContext(constraint);

  text_render.BuildTextLayout(constraint, &context);
  ASSERT_NE(text_render.GetCacheParagraph(), nullptr);
  EXPECT_FALSE(text_render.GetCacheParagraph()->DidExceedMaxLines());
  EXPECT_GT(text_render.GetCacheParagraph()->GetLineMetrics().size(), 1u);
  EXPECT_GT(text_render.GetCacheParagraph()->GetHeight(),
            constraint.height.value());

  text_render.HandleInlineTruncation(constraint, &context);

  EXPECT_TRUE(inline_truncation_node->IfNeedMount());
  EXPECT_LT(raw_text_shadow_node_->GetEndIndex(), text.size());
  auto lines = text_render.GetLineInfo();
  ASSERT_FALSE(lines.empty());
  EXPECT_GT(lines.back().ellipsis_count, 0);
}

TEST_F_UI(TextTest, InlineTruncationFitsNestedTextAndViewAfterEmoji) {
  MeasureConstraint constraint{260, MeasureMode::kDefinite, 100,
                               MeasureMode::kDefinite};
  auto inline_truncation_node = std::make_unique<InlineTruncationShadowNode>(
      owner_, std::string("inline-truncation"), -1);
  auto inline_text_node = std::make_unique<InlineTextShadowNode>(
      owner_, std::string("inline-text"), -1);
  auto inline_raw_text_shadow_node =
      std::make_unique<RawTextShadowNode>(owner_, std::string("raw-text"), -1);
  auto inline_view =
      std::make_unique<InlineViewShadowNode>(owner_, std::string("view"), 7);
  inline_raw_text_shadow_node->SetText("more");
  inline_view->SetWidth(16.f);
  inline_view->SetHeight(16.f);
  inline_text_node->AddChild(inline_raw_text_shadow_node.get());
  inline_text_node->AddChild(inline_view.get());
  inline_truncation_node->AddChild(inline_text_node.get());
  text_shadow_node_->AddChild(inline_truncation_node.get());
  text_shadow_node_->SetTextMaxLine(1);
  text_shadow_node_->SetTextOverflow(TextOverflow::kEllipsis);
  const std::u16string text =
      u"A\U0001F600 one two three four five six seven eight nine ten eleven";
  raw_text_shadow_node_->SetText(text);
  TextRender text_render(text_shadow_node_.get());
  text_render.SetUpdateFlag(TextUpdateFlag::kUpdateFlagChildren);
  auto context = text_shadow_node_->CreateLayoutContext(constraint);

  text_render.Measure(constraint, &context);

  EXPECT_TRUE(inline_truncation_node->IfNeedMount());
  ASSERT_LT(raw_text_shadow_node_->GetEndIndex(), text.size());
  size_t end = raw_text_shadow_node_->GetEndIndex();
  bool splits_surrogate_pair =
      end > 0 && end < text.size() && text[end - 1] >= 0xD800 &&
      text[end - 1] <= 0xDBFF && text[end] >= 0xDC00 && text[end] <= 0xDFFF;
  EXPECT_FALSE(splits_surrogate_pair);
  EXPECT_GE(inline_view->placeholder_index(), 0);
  EXPECT_LT(inline_view->StartGlyph(), inline_view->EndGlyph());
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

TEST_F_UI(TextTest, CalculateBaselineOffsetCoversAllVerticalAlignModes) {
  const FontMetrics metrics{/* ascent */ -8.0,
                            /* descent */ 2.0,
                            /* x_height */ 4.0f,
                            /* line_height */ 12.0f,
                            /* top */ -10.0f,
                            /* bottom */ 3.0f,
                            /* glyph_top */ -7.0f,
                            /* glyph_bottom */ 2.0f};
  constexpr double kChildDescent = 3.0;
  constexpr double kChildAscent = -6.0;
  struct TestCase {
    VerticalAlignType type;
    float length;
    double expected;
  };
  const std::vector<TestCase> cases = {
      {kVerticalAlignDefault, 0.f, 0.0},    {kVerticalAlignBaseline, 0.f, 0.0},
      {kVerticalAlignSub, 0.f, -0.9},       {kVerticalAlignSuper, 0.f, 0.9},
      {kVerticalAlignTop, 0.f, 2.0},        {kVerticalAlignTextTop, 0.f, 9.0},
      {kVerticalAlignMiddle, 0.f, 0.5},     {kVerticalAlignBottom, 0.f, 1.0},
      {kVerticalAlignTextBottom, 0.f, 9.0}, {kVerticalAlignLength, 25.f, -25.0},
      {kVerticalAlignPercent, 50.f, 6.0},   {kVerticalAlignCenter, 0.f, 3.0},
  };

  for (const auto& test_case : cases) {
    clay::Value::Array value;
    value.emplace_back(static_cast<int>(test_case.type));
    value.emplace_back(test_case.length);
    inline_text_shadow_node_->SetAttribute("vertical-align",
                                           clay::Value(std::move(value)));
    EXPECT_NEAR(inline_text_shadow_node_->CalculateBaselineOffset(
                    metrics, kChildDescent, kChildAscent),
                test_case.expected, 1e-6)
        << "vertical-align type " << test_case.type;
  }
}

TEST_F_UI(TextTest, SingleLineVerticalAlignMapsTopCenterAndBottom) {
  const std::vector<std::pair<const char*, VerticalAlignType>> cases = {
      {"top", kVerticalAlignTop},
      {"center", kVerticalAlignCenter},
      {"bottom", kVerticalAlignBottom},
  };

  for (const auto& [value, expected] : cases) {
    text_shadow_node_->SetTextSingleLineVerticalAlign(clay::Value(value));
    ASSERT_TRUE(text_shadow_node_->text_style_->align_type.has_value());
    EXPECT_EQ(text_shadow_node_->text_style_->align_type.value(), expected);
    EXPECT_EQ(text_shadow_node_->text_style_->enable_text_bounds, true);
  }
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
