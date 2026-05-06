// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/css/computed_css_style.h"

#include <unordered_set>

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
