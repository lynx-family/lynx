// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/css/computed_css_style_css_text_helper.h"

#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/starlight/types/layout_result.h"
#include "core/style/text_attributes.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

namespace {
CSSValue parseBackgroundPositionStringValue(const lepus::Value& value_str,
                                            const CSSParserConfigs& configs) {
  CSSStringParser parser = CSSStringParser::FromLepusString(value_str, configs);
  return parser.ParseBackgroundPosition();
}
}  // namespace

TEST(ComputedCSSStyleCssTextHelperTest, floatToPixelString) {
  auto helper = ComputedCSSStyleCssTextHelper();
  EXPECT_EQ(helper.FloatToPixelString(1.0f), "1px");
  EXPECT_EQ(helper.FloatToPixelString(1.00000000000f), "1px");
  EXPECT_EQ(helper.FloatToPixelString(1.56f), "1.56px");
  EXPECT_EQ(helper.FloatToPixelString(0.0f), "0px");
  EXPECT_EQ(helper.FloatToPixelString(-1.0f), "-1px");
}

TEST(ComputedCSSStyleCssTextHelperTest, Uint32ToRGBString) {
  auto helper = ComputedCSSStyleCssTextHelper();
  // uint32_t red = 0xFFFF0000;
  EXPECT_EQ(helper.Uint32ToRGBString(0xFFFF0000), "rgb(255, 0, 0)");
  EXPECT_EQ(helper.Uint32ToRGBString(0xFF0000FF), "rgb(0, 0, 255)");
  EXPECT_EQ(helper.Uint32ToRGBString(0xFF00FF00), "rgb(0, 255, 0)");
  EXPECT_EQ(helper.Uint32ToRGBString(0xFFFFFFFF), "rgb(255, 255, 255)");
}

TEST(ComputedCSSStyleCssTextHelperTest, ConcatStringsWithWhitespace) {
  auto helper = ComputedCSSStyleCssTextHelper();
  EXPECT_EQ(helper.ConcatStringsWithWhitespace({}), "");
  EXPECT_EQ(helper.ConcatStringsWithWhitespace({"a"}), "a");
  EXPECT_EQ(helper.ConcatStringsWithWhitespace({"a", "b", "c"}), "a b c");
}

TEST(ComputedCSSStyleCssTextHelperTest, OpacityCSSText) {
  auto helper = ComputedCSSStyleCssTextHelper();
  starlight::ComputedCSSStyle computed_css_style{1.f, 1.f};
  computed_css_style.opacity_ = 0.5f;
  EXPECT_EQ(helper.OpacityCSSText(&computed_css_style,
                                  starlight::LayoutResultForRendering()),
            "0.5");
  computed_css_style.opacity_ = 0.900000f;
  EXPECT_EQ(helper.OpacityCSSText(&computed_css_style,
                                  starlight::LayoutResultForRendering()),
            "0.9");
}

TEST(ComputedCSSStyleCssTextHelperTest, TransformOriginCSSText) {
  auto helper = ComputedCSSStyleCssTextHelper();
  starlight::ComputedCSSStyle computed_css_style{1.f, 1.f};
  starlight::CSSStyleUtils::PrepareOptional(
      computed_css_style.transform_origin_);
  computed_css_style.transform_origin_->x =
      starlight::NLength::MakeUnitNLength(100.f);
  computed_css_style.transform_origin_->y =
      starlight::NLength::MakeUnitNLength(100.f);
  EXPECT_EQ(helper.TransformOriginCSSText(
                &computed_css_style, starlight::LayoutResultForRendering()),
            "100px 100px");
}

TEST(ComputedCSSStyleCssTextHelperTest, SizeCSSText) {
  auto helper = ComputedCSSStyleCssTextHelper();
  starlight::ComputedCSSStyle computed_css_style{1.f, 1.f};
  starlight::LayoutResultForRendering layout_result = {};
  layout_result.size_ = FloatSize(100.f, 100.f);
  EXPECT_EQ(helper.HeightCSSText(&computed_css_style, layout_result), "100px");
  EXPECT_EQ(helper.WidthCSSText(&computed_css_style, layout_result), "100px");
}

TEST(ComputedCSSStyleCssTextHelperTest, FourSidesCSSText) {
  auto helper = ComputedCSSStyleCssTextHelper();
  starlight::ComputedCSSStyle computed_css_style{1.f, 1.f};
  starlight::LayoutResultForRendering layout_result = {};
  layout_result.size_ = FloatSize(100.f, 100.f);
  layout_result.offset_ = starlight::FloatPoint(20.f, 10.f);
  EXPECT_EQ(helper.LeftCSSText(&computed_css_style, layout_result), "20px");
  EXPECT_EQ(helper.TopCSSText(&computed_css_style, layout_result), "10px");
  EXPECT_EQ(helper.RightCSSText(&computed_css_style, layout_result), "120px");
  EXPECT_EQ(helper.BottomCSSText(&computed_css_style, layout_result), "110px");
}

TEST(ComputedCSSStyleCssTextHelperTest, BackgroundColorCSSText) {
  auto helper = ComputedCSSStyleCssTextHelper();
  starlight::ComputedCSSStyle computed_css_style{1.f, 1.f};
  starlight::CSSStyleUtils::PrepareOptional(
      computed_css_style.background_data_);
  computed_css_style.background_data_->color = 0xFF0000FF;
  EXPECT_EQ(helper.BackgroundColorCSSText(
                &computed_css_style, starlight::LayoutResultForRendering()),
            "rgb(0, 0, 255)");
}

TEST(ComputedCSSStyleCssTextHelperTest, ColorCSSText) {
  auto helper = ComputedCSSStyleCssTextHelper();
  starlight::ComputedCSSStyle computed_css_style{1.f, 1.f};
  computed_css_style.text_attributes_ = starlight::TextAttributes(14);
  computed_css_style.text_attributes_->color = 0xFF0000FF;
  EXPECT_EQ(helper.ColorCSSText(&computed_css_style,
                                starlight::LayoutResultForRendering()),
            "rgb(0, 0, 255)");
}

TEST(ComputedCSSStyleCssTextHelperTest, UnsupportedCSSProperty) {
  auto helper = ComputedCSSStyleCssTextHelper();
  starlight::ComputedCSSStyle computed_css_style{1.f, 1.f};
  EXPECT_EQ(helper.GetComputedStyleByPropertyID(
                tasm::CSSPropertyID::kPropertyEnd, &computed_css_style,
                starlight::LayoutResultForRendering()),
            "");
  EXPECT_EQ(helper.GetComputedStyleByPropertyID(
                tasm::CSSPropertyID::kPropertyStart, &computed_css_style,
                starlight::LayoutResultForRendering()),
            "");
}

TEST(ComputedCSSStyleCssTextHelperTest, GetComputedStyleByPropertyID) {
  auto helper = ComputedCSSStyleCssTextHelper();
  starlight::ComputedCSSStyle computed_css_style{1.f, 1.f};
  computed_css_style.opacity_ = 0.5f;
  EXPECT_EQ(helper.GetComputedStyleByPropertyID(
                tasm::CSSPropertyID::kPropertyIDOpacity, &computed_css_style,
                starlight::LayoutResultForRendering()),
            "0.5");
}

TEST(ComputedCSSStyleCssTextHelperTest, ZIndexCSSText) {
  auto helper = ComputedCSSStyleCssTextHelper();
  starlight::ComputedCSSStyle computed_css_style{1.f, 1.f};
  computed_css_style.SetEnableZIndex(true);
  computed_css_style.SetZIndex(
      CSSValue(lepus::Value(100), CSSValuePattern::NUMBER,
               CSSValueType::DEFAULT),
      false);
  EXPECT_EQ(helper.GetComputedStyleByPropertyID(
                tasm::CSSPropertyID::kPropertyIDZIndex, &computed_css_style,
                starlight::LayoutResultForRendering()),
            "100");

  computed_css_style.SetZIndex(
      CSSValue(lepus::Value(0), CSSValuePattern::NUMBER, CSSValueType::DEFAULT),
      true);
  EXPECT_EQ(helper.GetComputedStyleByPropertyID(
                tasm::CSSPropertyID::kPropertyIDZIndex, &computed_css_style,
                starlight::LayoutResultForRendering()),
            "0");
}

TEST(ComputedCSSStyleCssTextHelperTest, FilterCSSText) {
  auto helper = ComputedCSSStyleCssTextHelper();
  starlight::ComputedCSSStyle computed_css_style{1.f, 1.f};
  EXPECT_EQ(helper.GetComputedStyleByPropertyID(
                tasm::CSSPropertyID::kPropertyIDFilter, &computed_css_style,
                starlight::LayoutResultForRendering()),
            "none");

  starlight::CSSStyleUtils::PrepareOptional(computed_css_style.filter_);
  (*(computed_css_style.filter_)).type = starlight::FilterType::kBlur;
  (*(computed_css_style.filter_)).amount =
      starlight::NLength::MakeUnitNLength(10);
  EXPECT_EQ(helper.GetComputedStyleByPropertyID(
                tasm::CSSPropertyID::kPropertyIDFilter, &computed_css_style,
                starlight::LayoutResultForRendering()),
            "blur(10px)");

  (*(computed_css_style.filter_)).type = starlight::FilterType::kGrayscale;
  (*(computed_css_style.filter_)).amount =
      starlight::NLength::MakePercentageNLength(28);
  EXPECT_EQ(helper.GetComputedStyleByPropertyID(
                tasm::CSSPropertyID::kPropertyIDFilter, &computed_css_style,
                starlight::LayoutResultForRendering()),
            "grayscale(0.28)");

  computed_css_style.SetFilter(
      CSSValue(lepus::Value(0), CSSValuePattern::NUMBER, CSSValueType::DEFAULT),
      true);
  EXPECT_EQ(helper.GetComputedStyleByPropertyID(
                tasm::CSSPropertyID::kPropertyIDFilter, &computed_css_style,
                starlight::LayoutResultForRendering()),
            "none");
}

TEST(ComputedCSSStyleCssTextHelperTest, DirectionCSSText) {
  auto helper = ComputedCSSStyleCssTextHelper();
  starlight::ComputedCSSStyle computed_css_style{1.f, 1.f};

  // Test kNormal (should return "ltr")
  computed_css_style.SetValue(tasm::CSSPropertyID::kPropertyIDDirection,
                              CSSValue(starlight::DirectionType::kNormal),
                              false);
  EXPECT_EQ(helper.DirectionCSSText(&computed_css_style,
                                    starlight::LayoutResultForRendering()),
            "ltr");

  // Test kLtr (should return "ltr")
  computed_css_style.SetValue(tasm::CSSPropertyID::kPropertyIDDirection,
                              CSSValue(starlight::DirectionType::kLtr), false);
  EXPECT_EQ(helper.DirectionCSSText(&computed_css_style,
                                    starlight::LayoutResultForRendering()),
            "ltr");

  // Test kLynxRtl (should return "rtl")
  computed_css_style.SetValue(tasm::CSSPropertyID::kPropertyIDDirection,
                              CSSValue(starlight::DirectionType::kLynxRtl),
                              false);
  EXPECT_EQ(helper.DirectionCSSText(&computed_css_style,
                                    starlight::LayoutResultForRendering()),
            "rtl");

  // Test kRtl (should return "rtl")
  computed_css_style.SetValue(tasm::CSSPropertyID::kPropertyIDDirection,
                              CSSValue(starlight::DirectionType::kRtl), false);
  EXPECT_EQ(helper.DirectionCSSText(&computed_css_style,
                                    starlight::LayoutResultForRendering()),
            "rtl");
}

TEST(ComputedCSSStyleCssTextHelperTest,
     BackgroundPositionCSSTextOneValueSyntax) {
  auto helper = ComputedCSSStyleCssTextHelper();
  starlight::ComputedCSSStyle computed_css_style{1.f, 1.f};
  starlight::LayoutResultForRendering layout_result;
  CSSParserConfigs configs;

  // Prepare background data and image data
  starlight::CSSStyleUtils::PrepareOptional(
      computed_css_style.background_data_);
  starlight::CSSStyleUtils::PrepareOptional(
      computed_css_style.background_data_->image_data);
  auto& image_data = computed_css_style.background_data_->image_data.value();

  // Default value as center
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "0% 0%");

  image_data.image_count = 1;

  // Test 1-value syntax with `top` keyword (x should default to 50%, y = 0%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("top"), configs), false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "50% 0%");

  // Test 1-value syntax with `bottom` keyword (x=should default to 50%, y =
  // 100%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("bottom"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "50% 100%");

  // Test 1-value syntax with `left` keyword (x=0%, y should default to 50%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("left"), configs), false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "0% 50%");

  // Test 1-value syntax with `right` keyword (x=100%, y should default to 50%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("right"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "100% 50%");

  // Test 1-value syntax with `center` keyword
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("center"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "50% 50%");

  // Test 1-value syntax with percentage value(x = assigned percentage value, y
  // should default to 50%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("25%"), configs), false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "25% 50%");

  // Test 1-value syntax with numeric value(x = assigned numeric value, y should
  // default to 50%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("25px"), configs), false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "25px 50%");
}

TEST(ComputedCSSStyleCssTextHelperTest,
     BackgroundPositionCSSTextTwoValuesSyntax) {
  auto helper = ComputedCSSStyleCssTextHelper();
  starlight::ComputedCSSStyle computed_css_style{1.f, 1.f};
  starlight::LayoutResultForRendering layout_result;
  CSSParserConfigs configs;

  // Prepare background data and image data
  starlight::CSSStyleUtils::PrepareOptional(
      computed_css_style.background_data_);
  starlight::CSSStyleUtils::PrepareOptional(
      computed_css_style.background_data_->image_data);
  auto& image_data = computed_css_style.background_data_->image_data.value();

  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "0% 0%");

  image_data.image_count = 1;

  // Test 2-value syntax: top left (x=0%, y=0%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("top left"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "0% 0%");

  // Test 2-value syntax: left top (x=0%, y=0%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("left top"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "0% 0%");

  // Test 2-value syntax: bottom left (x=0%, y=100%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("bottom left"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "0% 100%");

  // Test 2-value syntax: left bottom (x=0%, y=0%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("left bottom"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "0% 100%");

  // Test 2-value syntax: top right (x=100%, y=0%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("top right"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "100% 0%");

  // Test 2-value syntax: right top (x=100%, y=0%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("right top"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "100% 0%");

  // Test 2-value syntax: bottom right (x=100%, y=100%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("bottom right"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "100% 100%");

  // Test 2-value syntax: right bottom (x=100%, y=100%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("right bottom"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "100% 100%");

  // Test 2-value syntax: center center (x=50%, y=50%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("center center"),
                                         configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "50% 50%");

  // Test 2-value syntax: center top (x=50%, y=0%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("center top"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "50% 0%");

  // Test 2-value syntax: top center (x=50%, y=0%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("top center"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "50% 0%");

  // Test 2-value syntax: center bottom (x=50%, y=100%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("center bottom"),
                                         configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "50% 100%");

  // Test 2-value syntax: bottom center (x=50%, y=100%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("bottom center"),
                                         configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "50% 100%");

  // Test 2-value syntax: center right (x=100%, y=50%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("center right"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "100% 50%");

  // Test 2-value syntax: right center (x=100%, y=50%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("right center"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "100% 50%");

  // Test 2-value syntax: center left (x=0%, y=50%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("center left"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "0% 50%");

  // Test 2-value syntax: left center (x=0%, y=50%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("left center"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "0% 50%");

  // Test 2-value syntax: 25% center (x=25%, y=50%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("25% center"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "25% 50%");

  // Test 2-value syntax: 25px center (x=25px, y=50%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("25px center"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "25px 50%");

  // Test 2-value syntax: 25% top (x=25%, y=0%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("25% top"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "25% 0%");

  // Test 2-value syntax: 25px top (x=25px, y=0%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("25px top"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "25px 0%");

  // Test 2-value syntax: 25% bottom (x=25%, y=100%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("25% bottom"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "25% 100%");

  // Test 2-value syntax: 25px bottom (x=25px, y=100%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("25px bottom"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "25px 100%");

  // Test 2-value syntax: center 25% (x=50%, y=25%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("center 25%"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "50% 25%");

  // Test 2-value syntax: center 25px (x=50%, y=25px)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("center 25px"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "50% 25px");

  // Test 2-value syntax: left 25% (x=0%, y=25%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("left 25%"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "0% 25%");

  // Test 2-value syntax: left 25px (x=0%, y=25px)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("left 25px"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "0% 25px");

  // Test 2-value syntax: right 25% (x=100%, y=25%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("right 25%"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "100% 25%");

  // Test 2-value syntax: right 25px (x=100%, y=25px)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("right 25px"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "100% 25px");

  // Test 2-value syntax: 25px 25% (x=25px, y=25%)
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("25px 25%"), configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "25px 25%");
}

TEST(ComputedCSSStyleCssTextHelperTest,
     BackgroundPositionCSSTextMultiBackgroundImages) {
  auto helper = ComputedCSSStyleCssTextHelper();
  starlight::ComputedCSSStyle computed_css_style{1.f, 1.f};
  starlight::LayoutResultForRendering layout_result;
  CSSParserConfigs configs;

  // Prepare background data and image data
  starlight::CSSStyleUtils::PrepareOptional(
      computed_css_style.background_data_);
  starlight::CSSStyleUtils::PrepareOptional(
      computed_css_style.background_data_->image_data);
  auto& image_data = computed_css_style.background_data_->image_data.value();

  // Set multiple images
  image_data.image_count = 4;
  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("top left, right bottom"),
                                         configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "0% 0%, 100% 100%, 0% 0%, 100% 100%");

  image_data.image_count = 3;

  image_data.position.clear();
  computed_css_style.SetBackgroundPosition(
      parseBackgroundPositionStringValue(lepus::Value("top left, right bottom"),
                                         configs),
      false);
  EXPECT_EQ(
      helper.BackgroundPositionCSSText(&computed_css_style, layout_result),
      "0% 0%, 100% 100%, 0% 0%");
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
