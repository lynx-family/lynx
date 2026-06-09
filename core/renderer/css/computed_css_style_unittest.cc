// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/css/computed_css_style.h"

#include <unordered_set>
#include <utility>
#include <variant>

#include "core/renderer/css/parser/css_string_parser.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

namespace {

CSSValue ParseVariableValue(const char* raw_value) {
  CSSParserConfigs configs;
  lepus::Value value(raw_value);
  CSSStringParser parser = CSSStringParser::FromLepusString(value, configs);
  return parser.ParseVariable();
}

lepus::Value MakeTextGradientValue(int marker) {
  auto gradient = lepus::CArray::Create();
  gradient->emplace_back(marker);
  return lepus::Value(std::move(gradient));
}

CSSValue MakeBackgroundPositionValue(float x, float y) {
  auto layer = lepus::CArray::Create();
  layer->emplace_back(static_cast<uint32_t>(CSSValuePattern::PERCENT));
  layer->emplace_back(x);
  layer->emplace_back(static_cast<uint32_t>(CSSValuePattern::PERCENT));
  layer->emplace_back(y);

  auto position = lepus::CArray::Create();
  position->emplace_back(std::move(layer));
  return CSSValue(std::move(position));
}

CSSValue MakeTextDecorationValue(bool with_thickness, double thickness = 2.0) {
  auto decoration = lepus::CArray::Create();
  decoration->emplace_back(
      static_cast<uint32_t>(starlight::TextDecorationType::kUnderLine));
  if (with_thickness) {
    decoration->emplace_back(
        static_cast<uint32_t>(starlight::TextDecorationType::kThickness));
    decoration->emplace_back(thickness);
    decoration->emplace_back(static_cast<int>(CSSValuePattern::PX));
  }
  return CSSValue(std::move(decoration));
}

}  // namespace

TEST(ComputedCSSStyleTest, TracksResolvedValuesOnSetAndReset) {
  starlight::ComputedCSSStyle style{1.f, 1.f};
  CSSValue font_size_value(18.0, CSSValuePattern::PX);

  style.SetValue(CSSPropertyID::kPropertyIDFontSize, font_size_value, false);

  const auto& resolved_values = style.GetResolvedValues();
  auto resolved_it = resolved_values.find(CSSPropertyID::kPropertyIDFontSize);
  ASSERT_TRUE(resolved_it != resolved_values.end());
  EXPECT_EQ(resolved_it->second, font_size_value);

  style.ResetValue(CSSPropertyID::kPropertyIDFontSize);
  EXPECT_FALSE(
      style.GetResolvedValues().contains(CSSPropertyID::kPropertyIDFontSize));
}

TEST(ComputedCSSStyleTest, ResetClearsOptionalStateAndDirtyBits) {
  starlight::ComputedCSSStyle style{1.f, 1.f};

  style.SetValue(
      CSSPropertyID::kPropertyIDBackgroundColor,
      CSSValue(static_cast<uint32_t>(0xFFFF0000), CSSValuePattern::NUMBER),
      false);
  style.MarkReset(CSSPropertyID::kPropertyIDHeight);

  ASSERT_FALSE(style.BackgroundColorToLepus().IsEmpty());
  ASSERT_FALSE(style.GetResolvedValues().empty());
  ASSERT_FALSE(style.IsClean());

  style.Reset();

  EXPECT_TRUE(style.BackgroundColorToLepus().IsEmpty());
  EXPECT_TRUE(style.GetResolvedValues().empty());
  EXPECT_TRUE(style.IsClean());
}

TEST(ComputedCSSStyleTest, StoresTextDecorationExtensionValues) {
  starlight::ComputedCSSStyle style{1.f, 1.f};

  EXPECT_TRUE(style.SetValue(CSSPropertyID::kPropertyIDTextDecorationThickness,
                             CSSValue(2.0, CSSValuePattern::PX), false));
  ASSERT_TRUE(style.text_attributes_.has_value());
  ASSERT_TRUE(style.text_attributes_->text_decoration_thickness.has_value());
  EXPECT_FLOAT_EQ(*style.text_attributes_->text_decoration_thickness, 2.f);

  EXPECT_TRUE(style.SetValue(CSSPropertyID::kPropertyIDXTextDecorationWidth,
                             CSSValue(8.0, CSSValuePattern::PX), false));
  ASSERT_TRUE(style.text_attributes_->text_decoration_width.has_value());
  EXPECT_FLOAT_EQ(*style.text_attributes_->text_decoration_width, 8.f);

  EXPECT_TRUE(style.SetValue(CSSPropertyID::kPropertyIDXTextDecorationGap,
                             CSSValue(4.0, CSSValuePattern::PX), false));
  ASSERT_TRUE(style.text_attributes_->text_decoration_gap.has_value());
  EXPECT_FLOAT_EQ(*style.text_attributes_->text_decoration_gap, 4.f);

  EXPECT_TRUE(
      style.ResetValue(CSSPropertyID::kPropertyIDTextDecorationThickness));
  EXPECT_FALSE(style.text_attributes_->text_decoration_thickness.has_value());

  EXPECT_TRUE(style.ResetValue(CSSPropertyID::kPropertyIDXTextDecorationWidth));
  EXPECT_FALSE(style.text_attributes_->text_decoration_width.has_value());

  EXPECT_TRUE(style.ResetValue(CSSPropertyID::kPropertyIDXTextDecorationGap));
  EXPECT_FALSE(style.text_attributes_->text_decoration_gap.has_value());
}

TEST(ComputedCSSStyleTest, RejectsInvalidTextDecorationExtensionValues) {
  starlight::ComputedCSSStyle style{1.f, 1.f};

  EXPECT_FALSE(style.SetValue(CSSPropertyID::kPropertyIDTextDecorationThickness,
                              CSSValue(-1.0, CSSValuePattern::PX), false));
  ASSERT_TRUE(style.text_attributes_.has_value());
  EXPECT_FALSE(style.text_attributes_->text_decoration_thickness.has_value());

  EXPECT_FALSE(style.SetValue(CSSPropertyID::kPropertyIDXTextDecorationWidth,
                              CSSValue(-1.0, CSSValuePattern::PX), false));
  EXPECT_FALSE(style.text_attributes_->text_decoration_width.has_value());

  EXPECT_FALSE(style.SetValue(CSSPropertyID::kPropertyIDXTextDecorationGap,
                              CSSValue(-1.0, CSSValuePattern::PX), false));
  EXPECT_FALSE(style.text_attributes_->text_decoration_gap.has_value());

  EXPECT_TRUE(style.SetValue(CSSPropertyID::kPropertyIDXTextDecorationGap,
                             CSSValue(0.0, CSSValuePattern::PX), false));
  ASSERT_TRUE(style.text_attributes_->text_decoration_gap.has_value());
  EXPECT_FLOAT_EQ(*style.text_attributes_->text_decoration_gap, 0.f);
}

TEST(ComputedCSSStyleTest, AppliesTextDecorationThicknessFromShorthand) {
  starlight::ComputedCSSStyle style{1.f, 1.f};

  EXPECT_TRUE(style.SetValue(CSSPropertyID::kPropertyIDTextDecoration,
                             MakeTextDecorationValue(true), false));
  ASSERT_TRUE(style.text_attributes_.has_value());
  EXPECT_TRUE(style.text_attributes_->underline_decoration);
  ASSERT_TRUE(style.text_attributes_->text_decoration_thickness.has_value());
  EXPECT_FLOAT_EQ(*style.text_attributes_->text_decoration_thickness, 2.f);
  EXPECT_TRUE(
      style.GetChangedBitset().Has(CSSPropertyID::kPropertyIDTextDecoration));
  EXPECT_TRUE(style.GetChangedBitset().Has(
      CSSPropertyID::kPropertyIDTextDecorationThickness));

  style.ClearDirtyBits();
  EXPECT_TRUE(style.SetValue(CSSPropertyID::kPropertyIDTextDecoration,
                             MakeTextDecorationValue(false), false));
  EXPECT_TRUE(style.text_attributes_->underline_decoration);
  EXPECT_FALSE(style.text_attributes_->text_decoration_thickness.has_value());
  EXPECT_TRUE(style.GetChangedBitset().Has(
      CSSPropertyID::kPropertyIDTextDecorationThickness));
}

TEST(ComputedCSSStyleTest, RejectsInvalidTextDecorationThicknessInShorthand) {
  starlight::ComputedCSSStyle style{1.f, 1.f};

  EXPECT_FALSE(style.SetValue(CSSPropertyID::kPropertyIDTextDecoration,
                              MakeTextDecorationValue(true, -1.0), false));
  ASSERT_TRUE(style.text_attributes_.has_value());
  EXPECT_FALSE(style.text_attributes_->text_decoration_thickness.has_value());
}

TEST(ComputedCSSStyleTest, IsPlatformPropertyMatchesGetterSurface) {
  EXPECT_TRUE(starlight::ComputedCSSStyle::IsPlatformProperty(
      CSSPropertyID::kPropertyIDOpacity));
  EXPECT_TRUE(starlight::ComputedCSSStyle::IsPlatformProperty(
      CSSPropertyID::kPropertyIDBackgroundColor));
  EXPECT_TRUE(starlight::ComputedCSSStyle::IsPlatformProperty(
      CSSPropertyID::kPropertyIDFontSize));
  EXPECT_FALSE(starlight::ComputedCSSStyle::IsPlatformProperty(
      CSSPropertyID::kPropertyIDWidth));
  EXPECT_FALSE(starlight::ComputedCSSStyle::IsPlatformProperty(
      CSSPropertyID::kPropertyIDHeight));
}

TEST(ComputedCSSStyleTest, IteratesChangedAndResetProperties) {
  starlight::ComputedCSSStyle style{1.f, 1.f};

  style.MarkChanged(CSSPropertyID::kPropertyIDWidth);
  style.MarkChanged(CSSPropertyID::kPropertyIDOpacity);
  style.MarkReset(CSSPropertyID::kPropertyIDHeight);

  std::unordered_set<CSSPropertyID> changed_properties;
  style.ForEachChangedProperty(
      [&](CSSPropertyID id) { changed_properties.insert(id); });

  std::unordered_set<CSSPropertyID> reset_properties;
  style.ForEachResetProperty(
      [&](CSSPropertyID id) { reset_properties.insert(id); });

  EXPECT_EQ(changed_properties.size(), 2U);
  EXPECT_TRUE(changed_properties.find(CSSPropertyID::kPropertyIDWidth) !=
              changed_properties.end());
  EXPECT_TRUE(changed_properties.find(CSSPropertyID::kPropertyIDOpacity) !=
              changed_properties.end());
  EXPECT_EQ(reset_properties.size(), 1U);
  EXPECT_TRUE(reset_properties.find(CSSPropertyID::kPropertyIDHeight) !=
              reset_properties.end());
}

TEST(ComputedCSSStyleTest, CanonicalComputedValueComparesByKindAndStorage) {
  using CanonicalComputedValue = starlight::CanonicalComputedValue;
  using Kind = CanonicalComputedValue::Kind;

  EXPECT_EQ(CanonicalComputedValue::Number(1.0f),
            CanonicalComputedValue::Number(1.0f));
  EXPECT_NE(CanonicalComputedValue::Number(1.0f),
            CanonicalComputedValue::Number(0.995f));
  EXPECT_EQ(CanonicalComputedValue::ResolvedLength(2.0f),
            CanonicalComputedValue::ResolvedLength(2.0f));
  EXPECT_NE(CanonicalComputedValue::ResolvedLength(2.0f),
            CanonicalComputedValue::ResolvedLength(2.005f));
  EXPECT_NE(CanonicalComputedValue::Number(1.0f),
            CanonicalComputedValue::ResolvedLength(1.0f));

  EXPECT_EQ(CanonicalComputedValue::Color(0xFF000000),
            CanonicalComputedValue::Color(0xFF000000));
  EXPECT_NE(CanonicalComputedValue::Color(0xFF000000),
            CanonicalComputedValue::Color(0xFFFFFFFF));

  const auto text_gradient_a = MakeTextGradientValue(1);
  const auto text_gradient_a_same = MakeTextGradientValue(1);
  const auto text_gradient_b = MakeTextGradientValue(2);
  EXPECT_EQ(CanonicalComputedValue::TextGradient(text_gradient_a),
            CanonicalComputedValue::TextGradient(text_gradient_a_same));
  EXPECT_NE(CanonicalComputedValue::TextGradient(text_gradient_a),
            CanonicalComputedValue::TextGradient(text_gradient_b));
  EXPECT_NE(CanonicalComputedValue::TextGradient(text_gradient_a),
            CanonicalComputedValue::Color(
                starlight::DefaultColor::DEFAULT_TEXT_COLOR));

  EXPECT_EQ(
      CanonicalComputedValue::Length(starlight::NLength::MakeUnitNLength(10.f)),
      CanonicalComputedValue::Length(
          starlight::NLength::MakeUnitNLength(10.f)));
  EXPECT_NE(
      CanonicalComputedValue::Length(starlight::NLength::MakeUnitNLength(10.f)),
      CanonicalComputedValue::Length(
          starlight::NLength::MakePercentageNLength(10.f)));

  CanonicalComputedValue::TransformValue transform_a;
  transform_a.emplace_back();
  CanonicalComputedValue::TransformValue transform_b;
  transform_b.emplace_back();
  transform_b.front().p0 = starlight::NLength::MakeUnitNLength(1.f);
  EXPECT_EQ(CanonicalComputedValue::Transform(transform_a),
            CanonicalComputedValue::Transform(transform_a));
  EXPECT_NE(CanonicalComputedValue::Transform(transform_a),
            CanonicalComputedValue::Transform(transform_b));

  starlight::FilterData filter_a;
  starlight::FilterData filter_b;
  filter_b.type = starlight::FilterType::kBlur;
  EXPECT_EQ(CanonicalComputedValue::Filter(filter_a),
            CanonicalComputedValue::Filter(filter_a));
  EXPECT_NE(CanonicalComputedValue::Filter(filter_a),
            CanonicalComputedValue::Filter(filter_b));

  CanonicalComputedValue::BackgroundPositionValue position_a;
  position_a.emplace_back(starlight::NLength::MakePercentageNLength(0.f));
  position_a.emplace_back(starlight::NLength::MakePercentageNLength(0.f));
  CanonicalComputedValue::BackgroundPositionValue position_b;
  position_b.emplace_back(starlight::NLength::MakePercentageNLength(50.f));
  position_b.emplace_back(starlight::NLength::MakePercentageNLength(0.f));
  EXPECT_EQ(CanonicalComputedValue::BackgroundPosition(position_a),
            CanonicalComputedValue::BackgroundPosition(position_a));
  EXPECT_NE(CanonicalComputedValue::BackgroundPosition(position_a),
            CanonicalComputedValue::BackgroundPosition(position_b));

  EXPECT_EQ(
      CanonicalComputedValue::TransformOrigin(starlight::TransformOriginData()),
      CanonicalComputedValue::TransformOrigin(
          starlight::TransformOriginData()));

  auto enum_value = CanonicalComputedValue::Enum(
      static_cast<int32_t>(starlight::VisibilityType::kHidden));
  EXPECT_EQ(enum_value.kind(), Kind::kEnum);
  const auto* stored_enum =
      std::get_if<CanonicalComputedValue::kEnumIndex>(&enum_value.storage());
  ASSERT_NE(stored_enum, nullptr);
  EXPECT_EQ(*stored_enum,
            static_cast<int32_t>(starlight::VisibilityType::kHidden));
  EXPECT_NE(enum_value, CanonicalComputedValue::Enum(static_cast<int32_t>(
                            starlight::VisibilityType::kVisible)));
}

TEST(ComputedCSSStyleTest, CanonicalComputedValueSupportsTransitionProperties) {
  starlight::ComputedCSSStyle style{1.f, 1.f};
  const std::unordered_set<CSSPropertyID> supported = {
      ALL_ANIMATABLE_PROPERTY_ID};

  for (const auto id : supported) {
    EXPECT_TRUE(starlight::ComputedCSSStyle::SupportsCanonicalComputedValue(id))
        << static_cast<int>(id);
    EXPECT_TRUE(style.ExtractCanonicalComputedValue(id).has_value())
        << static_cast<int>(id);
  }

  EXPECT_FALSE(starlight::ComputedCSSStyle::SupportsCanonicalComputedValue(
      CSSPropertyID::kPropertyIDFontSize));
  EXPECT_FALSE(
      style.ExtractCanonicalComputedValue(CSSPropertyID::kPropertyIDFontSize)
          .has_value());
}

TEST(ComputedCSSStyleTest, CanonicalTransformUsesLegacyTransitionDefault) {
  using CanonicalComputedValue = starlight::CanonicalComputedValue;

  starlight::ComputedCSSStyle unset_style{1.f, 1.f};
  auto unset_transform = unset_style.ExtractCanonicalComputedValue(
      CSSPropertyID::kPropertyIDTransform);
  ASSERT_TRUE(unset_transform.has_value());
  const auto* unset_transform_value =
      std::get_if<CanonicalComputedValue::kTransformIndex>(
          &unset_transform->storage());
  ASSERT_NE(unset_transform_value, nullptr);
  ASSERT_EQ(unset_transform_value->size(), 1U);
  EXPECT_EQ(unset_transform_value->front().type,
            starlight::TransformType::kRotateZ);
  EXPECT_EQ(unset_transform_value->front().p0,
            starlight::NLength::MakeUnitNLength(0.f));

  auto transform_array = lepus::CArray::Create();
  auto rotate_z = lepus::CArray::Create();
  rotate_z->emplace_back(static_cast<int>(starlight::TransformType::kRotateZ));
  rotate_z->emplace_back(0.0f);
  transform_array->emplace_back(std::move(rotate_z));

  starlight::ComputedCSSStyle rotate_zero_style{1.f, 1.f};
  rotate_zero_style.SetValue(CSSPropertyID::kPropertyIDTransform,
                             CSSValue(std::move(transform_array)));
  auto rotate_zero_transform = rotate_zero_style.ExtractCanonicalComputedValue(
      CSSPropertyID::kPropertyIDTransform);
  ASSERT_TRUE(rotate_zero_transform.has_value());
  EXPECT_EQ(*unset_transform, *rotate_zero_transform);
}

TEST(ComputedCSSStyleTest, ExtractsTextGradientAsCanonicalColorState) {
  using CanonicalComputedValue = starlight::CanonicalComputedValue;
  using Kind = CanonicalComputedValue::Kind;

  const auto text_gradient = MakeTextGradientValue(1);
  starlight::ComputedCSSStyle style{1.f, 1.f};
  style.SetValue(
      CSSPropertyID::kPropertyIDColor,
      CSSValue(lepus::Value::Clone(text_gradient), CSSValuePattern::ARRAY));

  auto color =
      style.ExtractCanonicalComputedValue(CSSPropertyID::kPropertyIDColor);
  ASSERT_TRUE(color.has_value());
  EXPECT_EQ(color->kind(), Kind::kTextGradient);
  const auto* text_gradient_value =
      std::get_if<CanonicalComputedValue::kTextGradientIndex>(
          &color->storage());
  ASSERT_NE(text_gradient_value, nullptr);
  EXPECT_EQ(*text_gradient_value, text_gradient);
  EXPECT_NE(*color, CanonicalComputedValue::Color(
                        starlight::DefaultColor::DEFAULT_TEXT_COLOR));
}

TEST(ComputedCSSStyleTest, CanonicalBackgroundPositionUsesDefaultPosition) {
  using CanonicalComputedValue = starlight::CanonicalComputedValue;

  starlight::ComputedCSSStyle unset_style{1.f, 1.f};
  auto unset_position = unset_style.ExtractCanonicalComputedValue(
      CSSPropertyID::kPropertyIDBackgroundPosition);
  ASSERT_TRUE(unset_position.has_value());
  const auto* unset_position_value =
      std::get_if<CanonicalComputedValue::kBackgroundPositionIndex>(
          &unset_position->storage());
  ASSERT_NE(unset_position_value, nullptr);
  ASSERT_EQ(unset_position_value->size(), 2U);
  EXPECT_EQ((*unset_position_value)[0],
            starlight::NLength::MakePercentageNLength(0.f));
  EXPECT_EQ((*unset_position_value)[1],
            starlight::NLength::MakePercentageNLength(0.f));

  starlight::ComputedCSSStyle explicit_default_style{1.f, 1.f};
  explicit_default_style.SetValue(CSSPropertyID::kPropertyIDBackgroundPosition,
                                  MakeBackgroundPositionValue(0.f, 0.f));
  auto explicit_position = explicit_default_style.ExtractCanonicalComputedValue(
      CSSPropertyID::kPropertyIDBackgroundPosition);
  ASSERT_TRUE(explicit_position.has_value());
  EXPECT_EQ(*unset_position, *explicit_position);
}

TEST(ComputedCSSStyleTest, ExtractsCanonicalValuesByPropertyKind) {
  using CanonicalComputedValue = starlight::CanonicalComputedValue;
  using Kind = CanonicalComputedValue::Kind;

  starlight::ComputedCSSStyle style{1.f, 1.f};
  style.SetValue(CSSPropertyID::kPropertyIDWidth,
                 CSSValue(20.0, CSSValuePattern::PX));
  style.SetValue(CSSPropertyID::kPropertyIDOpacity,
                 CSSValue(0.3, CSSValuePattern::NUMBER));
  style.SetValue(CSSPropertyID::kPropertyIDColor,
                 CSSValue(0xFF0000FFU, CSSValuePattern::NUMBER));
  style.SetValue(CSSPropertyID::kPropertyIDBorderLeftWidth,
                 CSSValue(2.0, CSSValuePattern::PX));
  style.SetValue(CSSPropertyID::kPropertyIDVisibility,
                 CSSValue(starlight::VisibilityType::kHidden));

  starlight::CSSStyleUtils::PrepareOptional(style.filter_);
  style.filter_->type = starlight::FilterType::kBlur;
  style.filter_->amount = starlight::NLength::MakeUnitNLength(5.f);

  starlight::CSSStyleUtils::PrepareOptional(style.transform_raw_);
  style.transform_raw_->emplace_back();

  starlight::CSSStyleUtils::PrepareOptional(style.background_data_);
  style.background_data_->image_data.emplace();
  style.background_data_->image_data->position.emplace_back(
      starlight::NLength::MakePercentageNLength(25.f));
  style.background_data_->image_data->position.emplace_back(
      starlight::NLength::MakePercentageNLength(75.f));

  auto width =
      style.ExtractCanonicalComputedValue(CSSPropertyID::kPropertyIDWidth);
  ASSERT_TRUE(width.has_value());
  EXPECT_EQ(width->kind(), Kind::kLength);
  const auto* width_value =
      std::get_if<CanonicalComputedValue::kLengthIndex>(&width->storage());
  ASSERT_NE(width_value, nullptr);
  EXPECT_EQ(*width_value, starlight::NLength::MakeUnitNLength(20.f));

  auto opacity =
      style.ExtractCanonicalComputedValue(CSSPropertyID::kPropertyIDOpacity);
  ASSERT_TRUE(opacity.has_value());
  EXPECT_EQ(opacity->kind(), Kind::kNumber);
  const auto* opacity_value =
      std::get_if<CanonicalComputedValue::kFloatIndex>(&opacity->storage());
  ASSERT_NE(opacity_value, nullptr);
  EXPECT_FLOAT_EQ(*opacity_value, 0.3f);

  auto color =
      style.ExtractCanonicalComputedValue(CSSPropertyID::kPropertyIDColor);
  ASSERT_TRUE(color.has_value());
  EXPECT_EQ(color->kind(), Kind::kColor);
  const auto* color_value =
      std::get_if<CanonicalComputedValue::kColorIndex>(&color->storage());
  ASSERT_NE(color_value, nullptr);
  EXPECT_EQ(*color_value, 0xFF0000FFU);

  auto border_width = style.ExtractCanonicalComputedValue(
      CSSPropertyID::kPropertyIDBorderLeftWidth);
  ASSERT_TRUE(border_width.has_value());
  EXPECT_EQ(border_width->kind(), Kind::kResolvedLength);
  const auto* border_width_value =
      std::get_if<CanonicalComputedValue::kFloatIndex>(
          &border_width->storage());
  ASSERT_NE(border_width_value, nullptr);
  EXPECT_FLOAT_EQ(*border_width_value, 2.f);

  auto filter =
      style.ExtractCanonicalComputedValue(CSSPropertyID::kPropertyIDFilter);
  ASSERT_TRUE(filter.has_value());
  EXPECT_EQ(filter->kind(), Kind::kFilter);
  const auto* filter_value =
      std::get_if<CanonicalComputedValue::kFilterIndex>(&filter->storage());
  ASSERT_NE(filter_value, nullptr);
  EXPECT_EQ(filter_value->type, starlight::FilterType::kBlur);

  auto transform =
      style.ExtractCanonicalComputedValue(CSSPropertyID::kPropertyIDTransform);
  ASSERT_TRUE(transform.has_value());
  EXPECT_EQ(transform->kind(), Kind::kTransform);
  const auto* transform_value =
      std::get_if<CanonicalComputedValue::kTransformIndex>(
          &transform->storage());
  ASSERT_NE(transform_value, nullptr);
  EXPECT_EQ(transform_value->size(), 1U);

  auto background_position = style.ExtractCanonicalComputedValue(
      CSSPropertyID::kPropertyIDBackgroundPosition);
  ASSERT_TRUE(background_position.has_value());
  EXPECT_EQ(background_position->kind(), Kind::kBackgroundPosition);
  const auto* position_value =
      std::get_if<CanonicalComputedValue::kBackgroundPositionIndex>(
          &background_position->storage());
  ASSERT_NE(position_value, nullptr);
  ASSERT_EQ(position_value->size(), 2U);
  EXPECT_EQ((*position_value)[0],
            starlight::NLength::MakePercentageNLength(25.f));

  auto transform_origin = style.ExtractCanonicalComputedValue(
      CSSPropertyID::kPropertyIDTransformOrigin);
  ASSERT_TRUE(transform_origin.has_value());
  EXPECT_EQ(transform_origin->kind(), Kind::kTransformOrigin);
  const auto* transform_origin_value =
      std::get_if<CanonicalComputedValue::kTransformOriginIndex>(
          &transform_origin->storage());
  ASSERT_NE(transform_origin_value, nullptr);
  EXPECT_EQ(transform_origin_value->x,
            starlight::NLength::MakePercentageNLength(50.f));

  auto visibility =
      style.ExtractCanonicalComputedValue(CSSPropertyID::kPropertyIDVisibility);
  ASSERT_TRUE(visibility.has_value());
  EXPECT_EQ(visibility->kind(), Kind::kEnum);
  const auto* visibility_value =
      std::get_if<CanonicalComputedValue::kEnumIndex>(&visibility->storage());
  ASSERT_NE(visibility_value, nullptr);
  EXPECT_EQ(*visibility_value,
            static_cast<int32_t>(starlight::VisibilityType::kHidden));
}

TEST(ComputedCSSStyleTest, FinalizeCustomPropertiesResolvesVariables) {
  starlight::ComputedCSSStyle style{1.f, 1.f};

  style.SetCustomProperty(base::String("--base"),
                          CSSValue::MakePlainString("blue"));
  style.SetCustomProperty(base::String("--accent"),
                          ParseVariableValue("var(--base)"));
  style.FinalizeCustomProperties();

  const auto* raw_props = style.GetRawCustomProperties();
  ASSERT_NE(raw_props, nullptr);
  auto raw_it = raw_props->find(base::String("--accent"));
  ASSERT_TRUE(raw_it != raw_props->end());
  EXPECT_TRUE(raw_it->second.NeedsVariableResolution());

  const auto* resolved_props = style.GetCustomProperties();
  ASSERT_NE(resolved_props, nullptr);
  auto resolved_it = resolved_props->find(base::String("--accent"));
  ASSERT_TRUE(resolved_it != resolved_props->end());
  EXPECT_EQ(resolved_it->second.AsStdString(), "blue");

  auto resolved_value = style.ResolveVariable(base::String("--accent"));
  ASSERT_TRUE(resolved_value.has_value());
  EXPECT_EQ(resolved_value.value().AsStdString(), "blue");
}

TEST(ComputedCSSStyleTest, CopyFromPreservesComputedCssStateIndependently) {
  starlight::ComputedCSSStyle source{1.f, 1.f};
  source.SetValue(CSSPropertyID::kPropertyIDFontSize,
                  CSSValue(24.0, CSSValuePattern::PX), false);
  source.SetCustomProperty(base::String("--base"),
                           CSSValue::MakePlainString("blue"));
  source.SetCustomProperty(base::String("--accent"),
                           ParseVariableValue("var(--base)"));
  source.FinalizeCustomProperties();

  starlight::ComputedCSSStyle copied{1.f, 1.f};
  copied.CopyFrom(source);

  auto copied_font_size =
      copied.GetResolvedValues().find(CSSPropertyID::kPropertyIDFontSize);
  ASSERT_TRUE(copied_font_size != copied.GetResolvedValues().end());
  EXPECT_EQ(copied_font_size->second, CSSValue(24.0, CSSValuePattern::PX));

  auto copied_accent = copied.ResolveVariable(base::String("--accent"));
  ASSERT_TRUE(copied_accent.has_value());
  EXPECT_EQ(copied_accent.value().AsStdString(), "blue");

  copied.SetCustomProperty(base::String("--base"),
                           CSSValue::MakePlainString("green"));
  copied.FinalizeCustomProperties();

  auto updated_copy = copied.ResolveVariable(base::String("--accent"));
  ASSERT_TRUE(updated_copy.has_value());
  EXPECT_EQ(updated_copy.value().AsStdString(), "green");

  auto unchanged_source = source.ResolveVariable(base::String("--accent"));
  ASSERT_TRUE(unchanged_source.has_value());
  EXPECT_EQ(unchanged_source.value().AsStdString(), "blue");
}

TEST(ComputedCSSStyleTest, InheritHelpersCopyOnlyRequestedState) {
  starlight::ComputedCSSStyle parent{1.f, 1.f};
  starlight::ComputedCSSStyle child{1.f, 1.f};

  parent.SetFontSize(20.0, DEFAULT_FONT_SIZE_DP);
  parent.SetValue(CSSPropertyID::kPropertyIDFontSize,
                  CSSValue(20.0, CSSValuePattern::PX), false);
  parent.SetValue(CSSPropertyID::kPropertyIDDirection,
                  CSSValue(starlight::DirectionType::kRtl), false);
  parent.SetValue(CSSPropertyID::kPropertyIDOpacity,
                  CSSValue(0.3, CSSValuePattern::NUMBER), false);
  parent.SetCustomProperty(base::String("--base"),
                           CSSValue::MakePlainString("blue"));
  parent.SetCustomProperty(base::String("--accent"),
                           ParseVariableValue("var(--base)"));
  parent.FinalizeCustomProperties();

  const std::unordered_set<CSSPropertyID> inheritable_props = {
      CSSPropertyID::kPropertyIDFontSize,
      CSSPropertyID::kPropertyIDDirection,
  };
  child.InheritCustomPropertiesFrom(parent);
  child.InheritNormalPropertiesFrom(parent, inheritable_props);
  child.InheritResolvedValuesFrom(parent, inheritable_props);

  ASSERT_TRUE(child.text_attributes_.has_value());
  EXPECT_EQ(child.text_attributes_->font_size,
            parent.text_attributes_->font_size);
  EXPECT_EQ(child.layout_computed_style_.GetDirection(),
            parent.layout_computed_style_.GetDirection());

  auto inherited_font_size =
      child.GetResolvedValues().find(CSSPropertyID::kPropertyIDFontSize);
  ASSERT_TRUE(inherited_font_size != child.GetResolvedValues().end());
  EXPECT_EQ(inherited_font_size->second, CSSValue(20.0, CSSValuePattern::PX));

  auto inherited_direction =
      child.GetResolvedValues().find(CSSPropertyID::kPropertyIDDirection);
  ASSERT_TRUE(inherited_direction != child.GetResolvedValues().end());
  EXPECT_EQ(inherited_direction->second,
            CSSValue(starlight::DirectionType::kRtl));

  EXPECT_FALSE(
      child.GetResolvedValues().contains(CSSPropertyID::kPropertyIDOpacity));

  auto inherited_custom = child.ResolveVariable(base::String("--accent"));
  ASSERT_TRUE(inherited_custom.has_value());
  EXPECT_EQ(inherited_custom.value().AsStdString(), "blue");
}

TEST(ComputedCSSStyleTest,
     HasNonDefaultInheritedResolvedValueRequiresResolvedEntryAndState) {
  starlight::ComputedCSSStyle font_size_style{1.f, 1.f};
  font_size_style.SetFontSize(20.0, DEFAULT_FONT_SIZE_DP);
  font_size_style.SetValue(CSSPropertyID::kPropertyIDFontSize,
                           CSSValue(20.0, CSSValuePattern::PX), false);
  EXPECT_TRUE(font_size_style.HasNonDefaultInheritedResolvedValue(
      CSSPropertyID::kPropertyIDFontSize));

  font_size_style.RemoveResolvedValue(CSSPropertyID::kPropertyIDFontSize);
  EXPECT_FALSE(font_size_style.HasNonDefaultInheritedResolvedValue(
      CSSPropertyID::kPropertyIDFontSize));

  starlight::ComputedCSSStyle resolved_only_style{1.f, 1.f};
  resolved_only_style.SetResolvedValue(CSSPropertyID::kPropertyIDFontSize,
                                       CSSValue(20.0, CSSValuePattern::PX));
  EXPECT_FALSE(resolved_only_style.HasNonDefaultInheritedResolvedValue(
      CSSPropertyID::kPropertyIDFontSize));

  starlight::ComputedCSSStyle direction_style{1.f, 1.f};
  direction_style.SetValue(CSSPropertyID::kPropertyIDDirection,
                           CSSValue(starlight::DirectionType::kRtl), false);
  EXPECT_TRUE(direction_style.HasNonDefaultInheritedResolvedValue(
      CSSPropertyID::kPropertyIDDirection));

  direction_style.SetValue(CSSPropertyID::kPropertyIDDirection,
                           CSSValue(starlight::DirectionType::kNormal), false);
  EXPECT_FALSE(direction_style.HasNonDefaultInheritedResolvedValue(
      CSSPropertyID::kPropertyIDDirection));
}

TEST(ComputedCSSStyleTest,
     InheritNormalPropertiesSkipsDefaultResolvedInheritedValues) {
  starlight::ComputedCSSStyle parent{1.f, 1.f};
  starlight::ComputedCSSStyle child{1.f, 1.f};

  parent.SetValue(CSSPropertyID::kPropertyIDFontSize,
                  CSSValue(DEFAULT_FONT_SIZE_DP, CSSValuePattern::PX), false);
  parent.SetValue(CSSPropertyID::kPropertyIDDirection,
                  CSSValue(starlight::DirectionType::kNormal), false);

  const std::unordered_set<CSSPropertyID> inheritable_props = {
      CSSPropertyID::kPropertyIDFontSize,
      CSSPropertyID::kPropertyIDDirection,
  };
  child.InheritNormalPropertiesFrom(parent, inheritable_props);

  EXPECT_FALSE(child.text_attributes_.has_value());
  EXPECT_EQ(child.GetDirection(), starlight::DirectionType::kNormal);
  EXPECT_TRUE(child.GetResolvedValues().empty());
}

TEST(ComputedCSSStyleTest,
     InheritNormalPropertiesCopiesOnlyNonDefaultResolvedState) {
  starlight::ComputedCSSStyle parent{1.f, 1.f};
  starlight::ComputedCSSStyle child{1.f, 1.f};

  parent.SetFontSize(24.0, DEFAULT_FONT_SIZE_DP);
  parent.SetValue(CSSPropertyID::kPropertyIDFontSize,
                  CSSValue(24.0, CSSValuePattern::PX), false);
  parent.SetValue(CSSPropertyID::kPropertyIDDirection,
                  CSSValue(starlight::DirectionType::kRtl), false);
  parent.SetResolvedValue(
      CSSPropertyID::kPropertyIDColor,
      CSSValue(static_cast<uint32_t>(0xffff0000), CSSValuePattern::NUMBER));

  const std::unordered_set<CSSPropertyID> inheritable_props = {
      CSSPropertyID::kPropertyIDFontSize,
      CSSPropertyID::kPropertyIDDirection,
      CSSPropertyID::kPropertyIDColor,
  };
  child.InheritNormalPropertiesFrom(parent, inheritable_props);

  ASSERT_TRUE(child.text_attributes_.has_value());
  EXPECT_EQ(child.text_attributes_->font_size, 24.0);
  EXPECT_FALSE(child.text_attributes_->color.has_value());
  EXPECT_EQ(child.GetDirection(), starlight::DirectionType::kRtl);
}

TEST(ComputedCSSStyleTest, InheritHelpersNoOpForEmptyInputs) {
  starlight::ComputedCSSStyle parent{1.f, 1.f};
  starlight::ComputedCSSStyle child{1.f, 1.f};
  const std::unordered_set<CSSPropertyID> inheritable_props;

  child.InheritCustomPropertiesFrom(parent);
  child.InheritNormalPropertiesFrom(parent, inheritable_props);
  child.InheritResolvedValuesFrom(parent, inheritable_props);

  EXPECT_EQ(child.GetRawCustomProperties(), nullptr);
  EXPECT_EQ(child.GetCustomProperties(), nullptr);
  EXPECT_TRUE(child.GetResolvedValues().empty());
  EXPECT_FALSE(child.text_attributes_.has_value());
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
