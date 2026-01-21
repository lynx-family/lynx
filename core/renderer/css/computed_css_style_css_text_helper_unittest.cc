// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/css/computed_css_style_css_text_helper.h"

#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/text_attributes.h"
#include "core/renderer/starlight/types/layout_result.h"
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

TEST(ComputedCSSStyleCssTextHelperTest, PaddingCSSText) {
  auto helper = ComputedCSSStyleCssTextHelper();
  starlight::ComputedCSSStyle computed_css_style{1.f, 1.f};
  starlight::LayoutResultForRendering layout_result;

  layout_result.padding_[starlight::Direction::kLeft] = 10.0f;
  layout_result.padding_[starlight::Direction::kRight] = 20.0f;
  layout_result.padding_[starlight::Direction::kTop] = 5.0f;
  layout_result.padding_[starlight::Direction::kBottom] = 15.0f;

  EXPECT_EQ(helper.PaddingLeftCSSText(&computed_css_style, layout_result),
            "10px");
  EXPECT_EQ(helper.PaddingRightCSSText(&computed_css_style, layout_result),
            "20px");
  EXPECT_EQ(helper.PaddingTopCSSText(&computed_css_style, layout_result),
            "5px");
  EXPECT_EQ(helper.PaddingBottomCSSText(&computed_css_style, layout_result),
            "15px");

  // top right bottom left order
  EXPECT_EQ(helper.PaddingCSSText(&computed_css_style, layout_result),
            "5px 20px 15px 10px");

  layout_result.padding_[starlight::Direction::kLeft] = 10.0f;
  layout_result.padding_[starlight::Direction::kRight] = 10.0f;
  layout_result.padding_[starlight::Direction::kTop] = 10.0f;
  layout_result.padding_[starlight::Direction::kBottom] = 10.0f;
  EXPECT_EQ(helper.PaddingCSSText(&computed_css_style, layout_result),
            "10px 10px 10px 10px");
}

TEST(ComputedCSSStyleCssTextHelperTest, MarginCSSText) {
  auto helper = ComputedCSSStyleCssTextHelper();
  starlight::ComputedCSSStyle computed_css_style{1.f, 1.f};
  starlight::LayoutResultForRendering layout_result;

  layout_result.margin_[starlight::Direction::kLeft] = 10.0f;
  layout_result.margin_[starlight::Direction::kRight] = 20.0f;
  layout_result.margin_[starlight::Direction::kTop] = 5.0f;
  layout_result.margin_[starlight::Direction::kBottom] = 15.0f;

  EXPECT_EQ(helper.MarginLeftCSSText(&computed_css_style, layout_result),
            "10px");
  EXPECT_EQ(helper.MarginRightCSSText(&computed_css_style, layout_result),
            "20px");
  EXPECT_EQ(helper.MarginTopCSSText(&computed_css_style, layout_result), "5px");
  EXPECT_EQ(helper.MarginBottomCSSText(&computed_css_style, layout_result),
            "15px");

  // top right bottom left order
  EXPECT_EQ(helper.MarginCSSText(&computed_css_style, layout_result),
            "5px 20px 15px 10px");

  layout_result.margin_[starlight::Direction::kLeft] = -10.0f;
  layout_result.margin_[starlight::Direction::kRight] = -20.0f;
  layout_result.margin_[starlight::Direction::kTop] = -5.0f;
  layout_result.margin_[starlight::Direction::kBottom] = -15.0f;
  EXPECT_EQ(helper.MarginLeftCSSText(&computed_css_style, layout_result),
            "-10px");
  EXPECT_EQ(helper.MarginCSSText(&computed_css_style, layout_result),
            "-5px -20px -15px -10px");

  layout_result.margin_[starlight::Direction::kLeft] = 0.0f;
  layout_result.margin_[starlight::Direction::kRight] = 0.0f;
  layout_result.margin_[starlight::Direction::kTop] = 0.0f;
  layout_result.margin_[starlight::Direction::kBottom] = 0.0f;
  EXPECT_EQ(helper.MarginCSSText(&computed_css_style, layout_result),
            "0px 0px 0px 0px");
}

TEST(ComputedCSSStyleCssTextHelperTest, BorderWidthCSSText) {
  auto helper = ComputedCSSStyleCssTextHelper();
  starlight::ComputedCSSStyle computed_css_style{1.f, 1.f};
  starlight::LayoutResultForRendering layout_result;

  // Test case: no border data, should return default values
  EXPECT_EQ(helper.BorderTopWidthCSSText(&computed_css_style, layout_result),
            "0px");
  EXPECT_EQ(helper.BorderBottomWidthCSSText(&computed_css_style, layout_result),
            "0px");
  EXPECT_EQ(helper.BorderLeftWidthCSSText(&computed_css_style, layout_result),
            "0px");
  EXPECT_EQ(helper.BorderRightWidthCSSText(&computed_css_style, layout_result),
            "0px");
  EXPECT_EQ(helper.BorderWidthCSSText(&computed_css_style, layout_result),
            "0px 0px 0px 0px");

  computed_css_style.css_align_with_legacy_w3c_ = true;
  // Test case: no border data, should return default values
  EXPECT_EQ(helper.BorderTopWidthCSSText(&computed_css_style, layout_result),
            "3px");
  EXPECT_EQ(helper.BorderBottomWidthCSSText(&computed_css_style, layout_result),
            "3px");
  EXPECT_EQ(helper.BorderLeftWidthCSSText(&computed_css_style, layout_result),
            "3px");
  EXPECT_EQ(helper.BorderRightWidthCSSText(&computed_css_style, layout_result),
            "3px");
  EXPECT_EQ(helper.BorderWidthCSSText(&computed_css_style, layout_result),
            "3px 3px 3px 3px");

  // Test case: with border data
  starlight::CSSStyleUtils::PrepareOptional(
      computed_css_style.layout_computed_style_.surround_data_.border_data_);
  auto& border_data = computed_css_style.layout_computed_style_.surround_data_
                          .border_data_.value();

  border_data.width_top = 10.0f;
  border_data.width_right = 20.0f;
  border_data.width_bottom = 30.0f;
  border_data.width_left = 40.0f;

  EXPECT_EQ(helper.BorderTopWidthCSSText(&computed_css_style, layout_result),
            "10px");
  EXPECT_EQ(helper.BorderRightWidthCSSText(&computed_css_style, layout_result),
            "20px");
  EXPECT_EQ(helper.BorderBottomWidthCSSText(&computed_css_style, layout_result),
            "30px");
  EXPECT_EQ(helper.BorderLeftWidthCSSText(&computed_css_style, layout_result),
            "40px");
  EXPECT_EQ(helper.BorderWidthCSSText(&computed_css_style, layout_result),
            "10px 20px 30px 40px");

  // Test case: equal border widths
  border_data.width_top = 5.0f;
  border_data.width_right = 5.0f;
  border_data.width_bottom = 5.0f;
  border_data.width_left = 5.0f;

  EXPECT_EQ(helper.BorderWidthCSSText(&computed_css_style, layout_result),
            "5px 5px 5px 5px");

  // Test case: floating point values
  border_data.width_top = 2.5f;
  border_data.width_right = 3.75f;
  border_data.width_bottom = 1.25f;
  border_data.width_left = 0.5f;

  EXPECT_EQ(helper.BorderTopWidthCSSText(&computed_css_style, layout_result),
            "2.5px");
  EXPECT_EQ(helper.BorderRightWidthCSSText(&computed_css_style, layout_result),
            "3.75px");
  EXPECT_EQ(helper.BorderBottomWidthCSSText(&computed_css_style, layout_result),
            "1.25px");
  EXPECT_EQ(helper.BorderLeftWidthCSSText(&computed_css_style, layout_result),
            "0.5px");
  EXPECT_EQ(helper.BorderWidthCSSText(&computed_css_style, layout_result),
            "2.5px 3.75px 1.25px 0.5px");
}

TEST(ComputedCSSStyleCssTextHelperTest, BorderCornerRadiusCSSText_Defaults) {
  auto helper = ComputedCSSStyleCssTextHelper();
  starlight::ComputedCSSStyle computed_css_style{1.f, 1.f};
  starlight::LayoutResultForRendering layout_result;
  layout_result.size_ = FloatSize(200.f, 100.f);

  EXPECT_EQ(
      helper.BorderTopLeftRadiusCSSText(&computed_css_style, layout_result),
      "0px 0px");
  EXPECT_EQ(
      helper.BorderTopRightRadiusCSSText(&computed_css_style, layout_result),
      "0px 0px");
  EXPECT_EQ(
      helper.BorderBottomRightRadiusCSSText(&computed_css_style, layout_result),
      "0px 0px");
  EXPECT_EQ(
      helper.BorderBottomLeftRadiusCSSText(&computed_css_style, layout_result),
      "0px 0px");
  EXPECT_EQ(helper.BorderRadiusCSSText(&computed_css_style, layout_result),
            "0px 0px 0px 0px / 0px 0px 0px 0px");
}

TEST(ComputedCSSStyleCssTextHelperTest,
     BorderCornerRadiusCSSText_ValuesAndSlashCombine) {
  auto helper = ComputedCSSStyleCssTextHelper();
  starlight::ComputedCSSStyle computed_css_style{1.f, 1.f};
  starlight::LayoutResultForRendering layout_result;
  layout_result.size_ = FloatSize(200.f, 100.f);

  starlight::CSSStyleUtils::PrepareOptional(
      computed_css_style.layout_computed_style_.surround_data_.border_data_);
  auto& border_data = computed_css_style.layout_computed_style_.surround_data_
                          .border_data_.value();

  border_data.radius_x_top_left = starlight::NLength::MakeUnitNLength(10.f);
  border_data.radius_y_top_left = starlight::NLength::MakeUnitNLength(5.f);

  border_data.radius_x_top_right =
      starlight::NLength::MakePercentageNLength(25.f);
  border_data.radius_y_top_right = starlight::NLength::MakeUnitNLength(6.f);

  border_data.radius_x_bottom_right = starlight::NLength::MakeUnitNLength(30.f);
  border_data.radius_y_bottom_right =
      starlight::NLength::MakePercentageNLength(20.f);

  border_data.radius_x_bottom_left =
      starlight::NLength::MakePercentageNLength(11.f);
  border_data.radius_y_bottom_left = starlight::NLength::MakeUnitNLength(11.f);

  EXPECT_EQ(
      helper.BorderTopLeftRadiusCSSText(&computed_css_style, layout_result),
      "10px 5px");
  EXPECT_EQ(
      helper.BorderTopRightRadiusCSSText(&computed_css_style, layout_result),
      "50px 6px");
  EXPECT_EQ(
      helper.BorderBottomRightRadiusCSSText(&computed_css_style, layout_result),
      "30px 20px");
  EXPECT_EQ(
      helper.BorderBottomLeftRadiusCSSText(&computed_css_style, layout_result),
      "22px 11px");

  EXPECT_EQ(helper.BorderRadiusCSSText(&computed_css_style, layout_result),
            "10px 50px 30px 22px / 5px 6px 20px 11px");
}

TEST(ComputedCSSStyleCssTextHelperTest, BorderRadiusPairCSSText) {
  auto helper = ComputedCSSStyleCssTextHelper();
  starlight::LayoutResultForRendering layout_result;
  layout_result.size_ = FloatSize(100.f, 80.f);

  auto rx = starlight::NLength::MakePercentageNLength(50.f);
  auto ry = starlight::NLength::MakeUnitNLength(24.f);
  EXPECT_EQ(helper.BorderRadiusPairCSSText(rx, ry, layout_result), "50px 24px");
}

TEST(ComputedCSSStyleCssTextHelperTest, BorderRadiusPairCSSTextComponents) {
  auto helper = ComputedCSSStyleCssTextHelper();
  starlight::LayoutResultForRendering layout_result;
  layout_result.size_ = FloatSize(100.f, 80.f);

  auto rx = starlight::NLength::MakePercentageNLength(50.f);
  auto ry = starlight::NLength::MakeUnitNLength(24.f);
  auto tuple = helper.BorderRadiusPairCSSTextComponents(rx, ry, layout_result);
  EXPECT_EQ(std::get<0>(tuple), "50px");
  EXPECT_EQ(std::get<1>(tuple), "24px");
}

TEST(ComputedCSSStyleCssTextHelperTest, BorderTopColorCSSText) {
  auto helper = ComputedCSSStyleCssTextHelper();
  starlight::ComputedCSSStyle computed_css_style{1.f, 1.f};
  starlight::LayoutResultForRendering layout_result;

  EXPECT_EQ(helper.BorderTopColorCSSText(&computed_css_style, layout_result),
            "rgb(0, 0, 0)");

  starlight::CSSStyleUtils::PrepareOptional(
      computed_css_style.layout_computed_style_.surround_data_.border_data_);
  computed_css_style.layout_computed_style_.surround_data_.border_data_
      ->color_top = 0xFFFF0000;
  EXPECT_EQ(helper.BorderTopColorCSSText(&computed_css_style, layout_result),
            "rgb(255, 0, 0)");
}

TEST(ComputedCSSStyleCssTextHelperTest, BorderRightColorCSSText) {
  auto helper = ComputedCSSStyleCssTextHelper();
  starlight::ComputedCSSStyle computed_css_style{1.f, 1.f};
  starlight::LayoutResultForRendering layout_result;

  EXPECT_EQ(helper.BorderRightColorCSSText(&computed_css_style, layout_result),
            "rgb(0, 0, 0)");

  starlight::CSSStyleUtils::PrepareOptional(
      computed_css_style.layout_computed_style_.surround_data_.border_data_);
  computed_css_style.layout_computed_style_.surround_data_.border_data_
      ->color_right = 0xFF0000FF;
  EXPECT_EQ(helper.BorderRightColorCSSText(&computed_css_style, layout_result),
            "rgb(0, 0, 255)");
}

TEST(ComputedCSSStyleCssTextHelperTest, BorderBottomColorCSSText) {
  auto helper = ComputedCSSStyleCssTextHelper();
  starlight::ComputedCSSStyle computed_css_style{1.f, 1.f};
  starlight::LayoutResultForRendering layout_result;

  EXPECT_EQ(helper.BorderBottomColorCSSText(&computed_css_style, layout_result),
            "rgb(0, 0, 0)");

  starlight::CSSStyleUtils::PrepareOptional(
      computed_css_style.layout_computed_style_.surround_data_.border_data_);
  computed_css_style.layout_computed_style_.surround_data_.border_data_
      ->color_bottom = 0xFF00FF00;
  EXPECT_EQ(helper.BorderBottomColorCSSText(&computed_css_style, layout_result),
            "rgb(0, 255, 0)");
}

TEST(ComputedCSSStyleCssTextHelperTest, BorderLeftColorCSSText) {
  auto helper = ComputedCSSStyleCssTextHelper();
  starlight::ComputedCSSStyle computed_css_style{1.f, 1.f};
  starlight::LayoutResultForRendering layout_result;

  EXPECT_EQ(helper.BorderLeftColorCSSText(&computed_css_style, layout_result),
            "rgb(0, 0, 0)");

  starlight::CSSStyleUtils::PrepareOptional(
      computed_css_style.layout_computed_style_.surround_data_.border_data_);
  computed_css_style.layout_computed_style_.surround_data_.border_data_
      ->color_left = 0xFFFFFFFF;
  EXPECT_EQ(helper.BorderLeftColorCSSText(&computed_css_style, layout_result),
            "rgb(255, 255, 255)");
}

TEST(ComputedCSSStyleCssTextHelperTest, BorderColorCSSText) {
  auto helper = ComputedCSSStyleCssTextHelper();
  starlight::ComputedCSSStyle computed_css_style{1.f, 1.f};
  starlight::LayoutResultForRendering layout_result;

  EXPECT_EQ(helper.BorderColorCSSText(&computed_css_style, layout_result),
            "rgb(0, 0, 0) rgb(0, 0, 0) rgb(0, 0, 0) rgb(0, 0, 0)");

  starlight::CSSStyleUtils::PrepareOptional(
      computed_css_style.layout_computed_style_.surround_data_.border_data_);
  auto& border =
      *computed_css_style.layout_computed_style_.surround_data_.border_data_;
  border.color_top = 0xFFFF0000;
  border.color_right = 0xFF0000FF;
  border.color_bottom = 0xFF00FF00;
  border.color_left = 0xFFFFFFFF;

  EXPECT_EQ(helper.BorderColorCSSText(&computed_css_style, layout_result),
            "rgb(255, 0, 0) rgb(0, 0, 255) rgb(0, 255, 0) rgb(255, 255, 255)");
}
}  // namespace test
}  // namespace tasm
}  // namespace lynx
