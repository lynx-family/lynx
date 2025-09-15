// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/css/computed_css_style_css_text_helper.h"

#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/starlight/types/layout_result.h"
#include "core/style/text_attributes.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

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

}  // namespace test
}  // namespace tasm
}  // namespace lynx
