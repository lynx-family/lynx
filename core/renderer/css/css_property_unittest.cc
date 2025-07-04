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
  EXPECT_EQ(map.size(), 217);
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

}  // namespace test
}  // namespace tasm
}  // namespace lynx
