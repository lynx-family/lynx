// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/css_style_utils.h"

#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/style/filter_data.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace starlight {
TEST(CssStyleUtils, ComputeFilter) {
  ComputedCSSStyle computedCssStyle(1.f, 1.f);
  tasm::CSSParserConfigs configs;
  tasm::CssMeasureContext length_context = computedCssStyle.GetMeasureContext();
  std::optional<FilterData> output;
  auto arr = lepus::CArray::Create();
  bool ret = false;
  // CSSValue to data
  arr->push_back(lepus::Value(static_cast<int>(FilterType::kNone)));
  auto input = tasm::CSSValue(lepus::Value(arr), tasm::CSSValuePattern::ARRAY);

  ret = CSSStyleUtils::ComputeFilter(input, false, output, length_context,
                                     configs);
  EXPECT_TRUE(ret);
  EXPECT_EQ((*output).type, FilterType::kNone);

  arr->Erase(0);
  arr->push_back(lepus::Value(static_cast<int>(FilterType::kGrayscale)));

  ret = CSSStyleUtils::ComputeFilter(input, false, output, length_context,
                                     configs);
  EXPECT_FALSE(ret);

  // grayscale(80%)
  arr->push_back(lepus::Value(80));
  arr->push_back(
      lepus::Value(static_cast<int>(tasm::CSSValuePattern::PERCENT)));
  ret = CSSStyleUtils::ComputeFilter(input, false, output, length_context,
                                     configs);
  EXPECT_TRUE(ret);
  EXPECT_EQ((*output).type, FilterType::kGrayscale);
  EXPECT_EQ((*output).amount.GetType(), NLengthType::kNLengthPercentage);
  EXPECT_EQ((*output).amount.GetRawValue(), 80.0f);

  arr->Erase(0, 3);
  // blur(20px)
  arr->push_back(lepus::Value(static_cast<int>(FilterType::kBlur)));
  arr->push_back(lepus::Value(20));
  arr->push_back(lepus::Value(static_cast<int>(tasm::CSSValuePattern::PX)));
  ret = CSSStyleUtils::ComputeFilter(input, false, output, length_context,
                                     configs);
  EXPECT_TRUE(ret);
  EXPECT_EQ((*output).type, FilterType::kBlur);
  EXPECT_EQ((*output).amount.GetType(), NLengthType::kNLengthUnit);
  EXPECT_EQ((*output).amount.GetRawValue(), 20);

  // data to lepus
  auto lepus_value = CSSStyleUtils::FilterToLepus(output);
  EXPECT_TRUE(lepus_value.IsArray());
  arr = lepus_value.Array();
  EXPECT_EQ(arr->size(), 3);
  EXPECT_EQ(arr->get(0).Number(), static_cast<int>(FilterType::kBlur));
  EXPECT_EQ(arr->get(1).Number(), 20);
  EXPECT_EQ(arr->get(2).Number(), static_cast<int>(PlatformLengthUnit::NUMBER));

  // data cmp ==
  FilterData target;
  target.Reset();
  EXPECT_FALSE(output == target);

  // reset
  ret = CSSStyleUtils::ComputeFilter(input, true, output, length_context,
                                     configs);
  EXPECT_TRUE(ret);
  // data cmp !=
  EXPECT_TRUE(output == std::nullopt);
  CSSStyleUtils::PrepareOptional(output);
  EXPECT_FALSE(output != target);
}

TEST(CssStyleUtils, GetLengthData) {
  ComputedCSSStyle computedCssStyle(1.0f, 3.0f);
  tasm::CSSParserConfigs configs;
  tasm::CssMeasureContext length_context = computedCssStyle.GetMeasureContext();
  // test 0.25px
  NLength len = NLength::MakeAutoNLength();
  lepus::Value unit = lepus::Value(static_cast<int>(tasm::CSSValuePattern::PX));
  lepus::Value value = lepus::Value(0.25);
  extern void GetLengthData(NLength & length, const lepus_value& value,
                            const lepus_value& unit,
                            const tasm::CssMeasureContext& context,
                            const tasm::CSSParserConfigs& configs);
  GetLengthData(len, value, unit, length_context, configs);
  EXPECT_TRUE(len.IsUnit());
  EXPECT_FLOAT_EQ(len.GetRawValue(), 1.0f / 3.0f);

  // test 25%
  unit = lepus::Value(static_cast<int>(tasm::CSSValuePattern::PERCENT));
  value = lepus::Value(0.25);
  extern void GetLengthData(NLength & length, const lepus_value& value,
                            const lepus_value& unit,
                            const tasm::CssMeasureContext& context,
                            const tasm::CSSParserConfigs& configs);
  GetLengthData(len, value, unit, length_context, configs);
  EXPECT_TRUE(len.IsPercent());
  EXPECT_FLOAT_EQ(len.GetRawValue(), 0.25);

  // test 0.25ppx
  unit = lepus::Value(static_cast<int>(tasm::CSSValuePattern::PPX));
  value = lepus::Value(0.25);
  extern void GetLengthData(NLength & length, const lepus_value& value,
                            const lepus_value& unit,
                            const tasm::CssMeasureContext& context,
                            const tasm::CSSParserConfigs& configs);
  GetLengthData(len, value, unit, length_context, configs);
  EXPECT_TRUE(len.IsUnit());
  EXPECT_FLOAT_EQ(len.GetRawValue(), 0);

  // test 0.55ppx
  unit = lepus::Value(static_cast<int>(tasm::CSSValuePattern::PPX));
  value = lepus::Value(0.55);
  extern void GetLengthData(NLength & length, const lepus_value& value,
                            const lepus_value& unit,
                            const tasm::CssMeasureContext& context,
                            const tasm::CSSParserConfigs& configs);
  GetLengthData(len, value, unit, length_context, configs);
  EXPECT_TRUE(len.IsUnit());
  EXPECT_FLOAT_EQ(len.GetRawValue(), 1.0f / 3.0f);
}

TEST(CSSStyleUtils, ComputeBasicShapes) {
  tasm::CSSParserConfigs configs;
  ComputedCSSStyle computed_css_style(1.0f, 1.0f);
  tasm::CssMeasureContext context = computed_css_style.GetMeasureContext();
  // Test compute path
  auto path =
      lepus::Value(R"(path('M 0 200 L 0,75 A 5,5 0,0,1 150,75 L 200 200 z')");
  auto input_arr = lepus::CArray::Create();
  input_arr->push_back(
      lepus::Value(static_cast<uint32_t>(starlight::BasicShapeType::kPath)));
  input_arr->push_back(path);
  fml::RefPtr<lepus::CArray> out_arr{nullptr};
  CSSStyleUtils::ComputeBasicShapePath(input_arr, false, out_arr);
  EXPECT_EQ(out_arr->size(), 2);
  EXPECT_EQ(out_arr->get(0).Number(),
            static_cast<uint32_t>(starlight::BasicShapeType::kPath));
  out_arr = lepus::CArray::Create();
  CSSStyleUtils::ComputeBasicShapePath(input_arr, true, out_arr);
  EXPECT_EQ(out_arr->size(), 0);

  // Test compute circle
  lepus::Value circle = lepus::Value(R"(circle(20px at left bottom))");
  auto circle_value =
      tasm::UnitHandler::Process(tasm::kPropertyIDClipPath, circle, configs);
  auto circle_raw = circle_value[tasm::kPropertyIDClipPath].GetValue().Array();
  out_arr = lepus::CArray::Create();
  CSSStyleUtils::ComputeBasicShapeCircle(circle_raw, false, out_arr, context,
                                         configs);
  EXPECT_EQ(out_arr->size(), 7);
  EXPECT_EQ(out_arr->get(0).Number(),
            static_cast<uint32_t>(starlight::BasicShapeType::kCircle));
  out_arr = lepus::CArray::Create();
  CSSStyleUtils::ComputeBasicShapeCircle(circle_raw, true, out_arr, context,
                                         configs);
  EXPECT_EQ(out_arr->size(), 0);

  // Test compute ellipse
  lepus::Value ellipse = lepus::Value(R"(ellipse(20px 30px at left bottom))");
  auto ellipse_value =
      tasm::UnitHandler::Process(tasm::kPropertyIDClipPath, ellipse, configs);
  auto ellipse_raw =
      ellipse_value[tasm::kPropertyIDClipPath].GetValue().Array();
  out_arr = lepus::CArray::Create();
  CSSStyleUtils::ComputeBasicShapeEllipse(ellipse_raw, false, out_arr, context,
                                          configs);
  EXPECT_EQ(out_arr->size(), 9);
  EXPECT_EQ(out_arr->get(0).Number(),
            static_cast<uint32_t>(starlight::BasicShapeType::kEllipse));

  out_arr = lepus::CArray::Create();
  CSSStyleUtils::ComputeBasicShapeEllipse(ellipse_raw, true, out_arr, context,
                                          configs);
  EXPECT_TRUE(out_arr->size() == 0);
}

TEST(CSSStyleUtils, ComputeSuperEllipse) {
#define UNIT_PX static_cast<uint32_t>(lynx::tasm::CSSValuePattern::PX)
#define UNIT_PERCENT static_cast<uint32_t>(lynx::tasm::CSSValuePattern::PERCENT)
#define PLATFORM_NUMBER \
  static_cast<uint32_t>(lynx::starlight::PlatformLengthUnit::NUMBER)
#define PLATFORM_PERCENT \
  static_cast<uint32_t>(lynx::starlight::PlatformLengthUnit::PERCENTAGE)
#define BUILD_ELLIPSE_ARR(arr, rx, urx, ry, ury, ex, ey, cx, ucx, cy, ucy) \
  arr->push_back(lynx::lepus::Value(rx));                                  \
  arr->push_back(lynx::lepus::Value(urx));                                 \
  arr->push_back(lynx::lepus::Value(ry));                                  \
  arr->push_back(lynx::lepus::Value(ury));                                 \
  arr->push_back(lynx::lepus::Value(ex));                                  \
  arr->push_back(lynx::lepus::Value(ey));                                  \
  arr->push_back(lynx::lepus::Value(cx));                                  \
  arr->push_back(lynx::lepus::Value(ucx));                                 \
  arr->push_back(lynx::lepus::Value(cy));                                  \
  arr->push_back(lynx::lepus::Value(ucy));

  auto arr = lepus::CArray::Create();
  arr->push_back(lepus::Value(
      static_cast<uint32_t>(starlight::BasicShapeType::kSuperEllipse)));

#define CHECK_COMPUTED_SUPER_ELLIPSE(arr, rx, urx, ry, ury, ex, ey, cx, ucx, \
                                     cy, ucy)                                \
  EXPECT_FLOAT_EQ(arr->get(1).Number(), rx);                                 \
  EXPECT_EQ(arr->get(2).Number(), urx);                                      \
  EXPECT_FLOAT_EQ(arr->get(3).Number(), ry);                                 \
  EXPECT_EQ(arr->get(4).Number(), ury);                                      \
  EXPECT_FLOAT_EQ(arr->get(5).Number(), ex);                                 \
  EXPECT_EQ(arr->get(6).Number(), ey);                                       \
  EXPECT_FLOAT_EQ(arr->get(7).Number(), cx);                                 \
  EXPECT_EQ(arr->get(8).Number(), ucx);                                      \
  EXPECT_FLOAT_EQ(arr->get(9).Number(), cy);                                 \
  EXPECT_EQ(arr->get(10).Number(), ucy);

  BUILD_ELLIPSE_ARR(arr, 30, UNIT_PX, 40, UNIT_PERCENT, 5, 9, 100, UNIT_PERCENT,
                    20, UNIT_PX);

  auto out = lepus::CArray::Create();
  ComputedCSSStyle computedCssStyle(1.0f, 1.0f);
  tasm::CSSParserConfigs configs;

  // Test normal compute
  tasm::CssMeasureContext length_context = computedCssStyle.GetMeasureContext();
  CSSStyleUtils::ComputeSuperEllipse(arr, false, out, length_context, configs);
  CHECK_COMPUTED_SUPER_ELLIPSE(out, 30, PLATFORM_NUMBER, 0.4, PLATFORM_PERCENT,
                               5, 9, 1, PLATFORM_PERCENT, 20, PLATFORM_NUMBER)

  // test reset
  out = lepus::CArray::Create();
  arr = lepus::CArray::Create();
  arr->push_back(lepus::Value(
      static_cast<uint32_t>(starlight::BasicShapeType::kSuperEllipse)));
  BUILD_ELLIPSE_ARR(arr, 30, UNIT_PX, 40, UNIT_PERCENT, 5, 9, 100, UNIT_PERCENT,
                    20, UNIT_PX);
  CSSStyleUtils::ComputeSuperEllipse(arr, true, out, length_context, configs);
  EXPECT_EQ(out->size(), 0);

#undef CHECK_COMPUTED_SUPER_ELLIPSE
#undef BUILD_ELLIPSE_ARR
#undef UNIT_PX
#undef UNIT_PERCENT
#undef PLATFORM_PERCENT
#undef PLATFORM_NUMBER
}

TEST(CSSStyleUtils, SetBasicShapeInset) {
  ComputedCSSStyle computedCssStyle(1.0f, 1.0f);
  tasm::CSSParserConfigs configs;
  tasm::CssMeasureContext context = computedCssStyle.GetMeasureContext();

  {  // Test compute inset
    lepus::Value inset =
        lepus::Value(R"(inset(20px 30% super-ellipse 2 3 10px))");
    auto inset_value =
        tasm::UnitHandler::Process(tasm::kPropertyIDClipPath, inset, configs);
    auto inset_raw = inset_value[tasm::kPropertyIDClipPath].GetValue().Array();
    auto out_arr = lepus::CArray::Create();
    CSSStyleUtils::ComputeBasicShapeInset(inset_raw, false, out_arr, context,
                                          configs);
    EXPECT_EQ(out_arr->size(), 27);
    EXPECT_EQ(out_arr->get(0).Number(),
              static_cast<uint32_t>(starlight::BasicShapeType::kInset));
    // top
    EXPECT_EQ(out_arr->get(1).Number(), 20);
    EXPECT_EQ(out_arr->get(2).Number(),
              static_cast<uint32_t>(PlatformLengthUnit::NUMBER));
    // right
    EXPECT_FLOAT_EQ(out_arr->get(3).Number(), 0.3);
    EXPECT_EQ(out_arr->get(4).Number(),
              static_cast<uint32_t>(PlatformLengthUnit::PERCENTAGE));
    // bottom
    EXPECT_EQ(out_arr->get(5).Number(), 20);
    EXPECT_EQ(out_arr->get(6).Number(),
              static_cast<uint32_t>(PlatformLengthUnit::NUMBER));
    // left
    EXPECT_FLOAT_EQ(out_arr->get(7).Number(), 0.3);
    EXPECT_EQ(out_arr->get(8).Number(),
              static_cast<uint32_t>(PlatformLengthUnit::PERCENTAGE));
    // exponent x & y
    EXPECT_EQ(out_arr->get(9).Number(), 2);
    EXPECT_EQ(out_arr->get(10).Number(), 3);

    for (int i = 11; i < 27; i += 2) {
      EXPECT_EQ(out_arr->get(i).Number(), 10);
      EXPECT_EQ(out_arr->get(i + 1).Number(),
                static_cast<uint32_t>(PlatformLengthUnit::NUMBER));
    }

    // test reset.
    out_arr = lepus::CArray::Create();
    CSSStyleUtils::ComputeBasicShapeInset(inset_raw, true, out_arr, context,
                                          configs);
    EXPECT_TRUE(out_arr->size() == 0);
  }

  {
    // Test compute inset
    lepus::Value ellipse = lepus::Value(R"(inset(20px 30% round 45%))");
    auto ellipse_value =
        tasm::UnitHandler::Process(tasm::kPropertyIDClipPath, ellipse, configs);
    auto ellipse_raw =
        ellipse_value[tasm::kPropertyIDClipPath].GetValue().Array();
    auto out_arr = lepus::CArray::Create();
    CSSStyleUtils::ComputeBasicShapeInset(ellipse_raw, false, out_arr, context,
                                          configs);
    EXPECT_EQ(out_arr->size(), 25);
    EXPECT_EQ(out_arr->get(0).Number(),
              static_cast<uint32_t>(starlight::BasicShapeType::kInset));
    // top
    EXPECT_EQ(out_arr->get(1).Number(), 20);
    EXPECT_EQ(out_arr->get(2).Number(),
              static_cast<uint32_t>(PlatformLengthUnit::NUMBER));
    // right
    EXPECT_FLOAT_EQ(out_arr->get(3).Number(), 0.3);
    EXPECT_EQ(out_arr->get(4).Number(),
              static_cast<uint32_t>(PlatformLengthUnit::PERCENTAGE));
    // bottom
    EXPECT_EQ(out_arr->get(5).Number(), 20);
    EXPECT_EQ(out_arr->get(6).Number(),
              static_cast<uint32_t>(PlatformLengthUnit::NUMBER));
    // left
    EXPECT_FLOAT_EQ(out_arr->get(7).Number(), 0.3);
    EXPECT_EQ(out_arr->get(8).Number(),
              static_cast<uint32_t>(PlatformLengthUnit::PERCENTAGE));

    for (int i = 9; i < 25; i += 2) {
      EXPECT_FLOAT_EQ(0.45, out_arr->get(i).Number());
      EXPECT_EQ(out_arr->get(i + 1).Number(),
                static_cast<uint32_t>(PlatformLengthUnit::PERCENTAGE));
    }
  }
}

}  // namespace starlight
}  // namespace lynx
