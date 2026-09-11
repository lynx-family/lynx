// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/css_property.h"

#include "core/renderer/css/css_property_id.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {

extern const char* GetPropertyNameCStr(CSSPropertyID id);

namespace test {

TEST(CSSProperty, GetComputeStyleMap) {
  auto map = CSSProperty::GetComputeStyleMap();
  EXPECT_EQ(map.size(),
            CSSPropertyID::kPropertyEnd - CSSPropertyID::kPropertyStart);
}

TEST(CSSProperty, GetPropertyID) {
  EXPECT_EQ(CSSProperty::GetPropertyID("justify-content"),
            kPropertyIDJustifyContent);

  std::string name = "justify-content";
  EXPECT_EQ(CSSProperty::GetPropertyID(name), kPropertyIDJustifyContent);

  base::String name_lepus("justify-content");
  EXPECT_EQ(CSSProperty::GetPropertyID(name_lepus), kPropertyIDJustifyContent);
}

TEST(CSSProperty, GetPropertyName) {
  EXPECT_TRUE(
      strcmp(CSSProperty::GetPropertyName(kPropertyIDJustifyContent).c_str(),
             "justify-content") == 0);
  EXPECT_EQ(CSSProperty::GetPropertyName(kPropertyIDJustifyContent).str(),
            "justify-content");

  base::String lepus_name =
      CSSProperty::GetPropertyName(kPropertyIDJustifyContent);
  EXPECT_TRUE(lepus_name == "justify-content");

  EXPECT_TRUE(
      strcmp(CSSProperty::GetPropertyNameCStr(kPropertyIDJustifyContent),
             "justify-content") == 0);

  EXPECT_TRUE(strcmp(GetPropertyNameCStr(kPropertyIDJustifyContent),
                     "justify-content") == 0);

  EXPECT_EQ(
      CSSProperty::GetPropertyName(static_cast<CSSPropertyID>(0xFFFF)).str(),
      "");
}

TEST(CSSProperty, IsInspectorFilteredProperty) {
  const CSSPropertyID inspectorFilteredProperties[] = {
      kPropertyIDBorder,
      kPropertyIDBorderTop,
      kPropertyIDBorderRight,
      kPropertyIDBorderBottom,
      kPropertyIDBorderLeft,
      kPropertyIDMarginInlineStart,
      kPropertyIDMarginInlineEnd,
      kPropertyIDPaddingInlineStart,
      kPropertyIDPaddingInlineEnd,
      kPropertyIDBorderInlineStartWidth,
      kPropertyIDBorderInlineEndWidth,
      kPropertyIDBorderInlineStartColor,
      kPropertyIDBorderInlineEndColor,
      kPropertyIDBorderInlineStartStyle,
      kPropertyIDBorderInlineEndStyle,
      kPropertyIDBorderStartStartRadius,
      kPropertyIDBorderEndStartRadius,
      kPropertyIDBorderStartEndRadius,
      kPropertyIDBorderEndEndRadius,
      kPropertyIDFlex,
      kPropertyIDFlexFlow,
      kPropertyIDPadding,
      kPropertyIDMargin,
      kPropertyIDInsetInlineStart,
      kPropertyIDInsetInlineEnd,
      kPropertyIDBorderWidth,
      kPropertyIDBackground,
      kPropertyIDBorderColor,
      kPropertyIDBorderStyle,
      kPropertyIDOutline};
  for (size_t i = 0;
       i < sizeof(inspectorFilteredProperties) / sizeof(CSSPropertyID); i++) {
    EXPECT_TRUE(CSSProperty::IsInspectorFilteredProperty(
        inspectorFilteredProperties[i]));
  }
}

TEST(CSSProperty, IsShorthandProperty) {
  const CSSPropertyID shorthands[] = {
      kPropertyIDPadding,      kPropertyIDMargin,      kPropertyIDFlex,
      kPropertyIDBackground,   kPropertyIDBorder,      kPropertyIDBorderWidth,
      kPropertyIDBorderRadius, kPropertyIDBorderColor, kPropertyIDBorderStyle,
      kPropertyIDBorderRight,  kPropertyIDBorderLeft,  kPropertyIDBorderTop,
      kPropertyIDBorderBottom, kPropertyIDOutline,     kPropertyIDFlexFlow,
      kPropertyIDTransition,   kPropertyIDMask,        kPropertyIDAnimation};
  for (size_t i = 0; i < sizeof(shorthands) / sizeof(CSSPropertyID); i++) {
    EXPECT_TRUE(CSSProperty::IsShorthandProperty(shorthands[i]));
  }
}

TEST(CSSProperty, GetExpandedLonghands) {
  size_t count = 0;
  const auto* margin_longhands =
      CSSProperty::GetExpandedLonghands(kPropertyIDMargin, &count);
  ASSERT_NE(margin_longhands, nullptr);
  ASSERT_EQ(count, 4u);
  EXPECT_EQ(margin_longhands[0], kPropertyIDMarginTop);
  EXPECT_EQ(margin_longhands[1], kPropertyIDMarginRight);
  EXPECT_EQ(margin_longhands[2], kPropertyIDMarginBottom);
  EXPECT_EQ(margin_longhands[3], kPropertyIDMarginLeft);

  const auto* transition_longhands =
      CSSProperty::GetExpandedLonghands(kPropertyIDTransition, &count);
  ASSERT_NE(transition_longhands, nullptr);
  ASSERT_EQ(count, 4u);
  EXPECT_EQ(transition_longhands[0], kPropertyIDTransitionProperty);
  EXPECT_EQ(transition_longhands[1], kPropertyIDTransitionDuration);
  EXPECT_EQ(transition_longhands[2], kPropertyIDTransitionDelay);
  EXPECT_EQ(transition_longhands[3], kPropertyIDTransitionTimingFunction);

  const auto* width_longhands =
      CSSProperty::GetExpandedLonghands(kPropertyIDWidth, &count);
  EXPECT_EQ(width_longhands, nullptr);
  EXPECT_EQ(count, 0u);
}

TEST(CSSProperty, CornerShapePropertyLookup) {
  const struct {
    CSSPropertyID id;
    const char* name;
    int numeric_id;
  } properties[] = {
      {kPropertyIDCornerShape, "corner-shape", 239},
      {kPropertyIDCornerTopLeftShape, "corner-top-left-shape", 240},
      {kPropertyIDCornerTopRightShape, "corner-top-right-shape", 241},
      {kPropertyIDCornerBottomRightShape, "corner-bottom-right-shape", 242},
      {kPropertyIDCornerBottomLeftShape, "corner-bottom-left-shape", 243},
  };
  const auto& defaults = CSSProperty::GetComputeStyleMap();
  for (const auto& property : properties) {
    SCOPED_TRACE(property.name);
    EXPECT_EQ(static_cast<int>(property.id), property.numeric_id);
    EXPECT_TRUE(CSSProperty::IsPropertyValid(property.id));
    EXPECT_EQ(CSSProperty::GetPropertyID(property.name), property.id);
    EXPECT_EQ(CSSProperty::GetPropertyID(std::string(property.name)),
              property.id);
    EXPECT_EQ(CSSProperty::GetPropertyID(base::String(property.name)),
              property.id);
    EXPECT_EQ(CSSProperty::GetPropertyName(property.id).str(), property.name);
    EXPECT_STREQ(CSSProperty::GetPropertyNameCStr(property.id), property.name);
    EXPECT_STREQ(GetPropertyNameCStr(property.id), property.name);
    auto it = defaults.find(property.name);
    ASSERT_NE(it, defaults.end());
    EXPECT_EQ(it->second, "round");
  }
}

TEST(CSSProperty, CornerShapeShorthandMetadata) {
  const CSSPropertyID corners[] = {
      kPropertyIDCornerTopLeftShape, kPropertyIDCornerTopRightShape,
      kPropertyIDCornerBottomRightShape, kPropertyIDCornerBottomLeftShape};
  EXPECT_EQ(CSSProperty::GetShorthandExpand(kPropertyIDCornerShape), 4u);
  EXPECT_TRUE(CSSProperty::IsShorthandProperty(kPropertyIDCornerShape));
  size_t count = 0;
  const auto* longhands =
      CSSProperty::GetExpandedLonghands(kPropertyIDCornerShape, &count);
  ASSERT_NE(longhands, nullptr);
  ASSERT_EQ(count, 4u);
  for (size_t i = 0; i < count; ++i) {
    EXPECT_EQ(longhands[i], corners[i]);
  }
  for (auto id : corners) {
    SCOPED_TRACE(CSSProperty::GetPropertyNameCStr(id));
    EXPECT_EQ(CSSProperty::GetShorthandExpand(id), 0u);
    EXPECT_FALSE(CSSProperty::IsShorthandProperty(id));
    size_t longhand_count = 42;
    EXPECT_EQ(CSSProperty::GetExpandedLonghands(id, &longhand_count), nullptr);
    EXPECT_EQ(longhand_count, 0u);
  }
}

TEST(CSSProperty, IsCustomProperty) {
  EXPECT_FALSE(CSSProperty::IsCustomProperty("", 0));
  EXPECT_FALSE(CSSProperty::IsCustomProperty("--", 2));
  EXPECT_TRUE(CSSProperty::IsCustomProperty("--x", 3));
  EXPECT_FALSE(CSSProperty::IsCustomProperty("custom-property", 15));
  EXPECT_TRUE(CSSProperty::IsCustomProperty("--custom-property", 17));
}
}  // namespace test
}  // namespace tasm
}  // namespace lynx
