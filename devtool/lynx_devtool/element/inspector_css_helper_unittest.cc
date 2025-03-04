// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/element/inspector_css_helper.h"

#include "core/renderer/css/css_property.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

TEST(InspectorCSSHelperTest, IsColorTest) {
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsColor(
      lynx::tasm::CSSPropertyID::kPropertyIDColor));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsColor(
      lynx::tasm::CSSPropertyID::kPropertyIDBackgroundColor));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsColor(
      lynx::tasm::CSSPropertyID::kPropertyIDBorderBottomColor));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsColor(
      lynx::tasm::CSSPropertyID::kPropertyIDBorderTopColor));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsColor(
      lynx::tasm::CSSPropertyID::kPropertyIDBorderLeftColor));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsColor(
      lynx::tasm::CSSPropertyID::kPropertyIDBorderRightColor));
  EXPECT_FALSE(lynx::devtool::InspectorCSSHelper::IsColor(
      lynx::tasm::CSSPropertyID::kPropertyIDLineSpacing));
  EXPECT_FALSE(lynx::devtool::InspectorCSSHelper::IsColor(
      lynx::tasm::CSSPropertyID::kPropertyIDBottom));
}

TEST(InspectorCSSHelperTest, IsDimensionTest) {
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDLineSpacing));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDLetterSpacing));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDBorderWidth));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDBorderLeftWidth));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDBorderRightWidth));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDBorderTopWidth));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDBorderBottomWidth));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDBorderRadius));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDBorderTopLeftRadius));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDBorderBottomLeftRadius));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDBorderTopRightRadius));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDBorderBottomRightRadius));
  EXPECT_FALSE(lynx::devtool::InspectorCSSHelper::IsDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDBottom));
  EXPECT_FALSE(lynx::devtool::InspectorCSSHelper::IsDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDColor));
}

TEST(InspectorCSSHelperTest, IsAutoDimensionTest) {
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAutoDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDTop));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAutoDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDBottom));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAutoDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDLeft));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAutoDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDRight));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAutoDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDHeight));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAutoDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDWidth));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAutoDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDMaxHeight));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAutoDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDMaxWidth));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAutoDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDMinHeight));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAutoDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDMinWidth));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAutoDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDPadding));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAutoDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDPaddingTop));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAutoDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDPaddingBottom));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAutoDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDPaddingLeft));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAutoDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDPaddingRight));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAutoDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDMargin));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAutoDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDMarginTop));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAutoDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDMarginBottom));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAutoDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDMarginLeft));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAutoDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDMarginRight));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAutoDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDFlexBasis));
  EXPECT_FALSE(lynx::devtool::InspectorCSSHelper::IsAutoDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDLineSpacing));
  EXPECT_FALSE(lynx::devtool::InspectorCSSHelper::IsAutoDimension(
      lynx::tasm::CSSPropertyID::kPropertyIDColor));
}

TEST(InspectorCSSHelperTest, IsStringPropTest) {
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsStringProp(
      lynx::tasm::CSSPropertyID::kPropertyIDPosition));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsStringProp(
      lynx::tasm::CSSPropertyID::kPropertyIDDisplay));
  EXPECT_FALSE(lynx::devtool::InspectorCSSHelper::IsStringProp(
      lynx::tasm::CSSPropertyID::kPropertyIDBackgroundPosition));
}

TEST(InspectorCSSHelperTest, IsIntPropTest) {
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsIntProp(
      lynx::tasm::CSSPropertyID::kPropertyIDOrder));
  EXPECT_FALSE(lynx::devtool::InspectorCSSHelper::IsIntProp(
      lynx::tasm::CSSPropertyID::kPropertyIDTop));
  EXPECT_FALSE(lynx::devtool::InspectorCSSHelper::IsIntProp(
      lynx::tasm::CSSPropertyID::kPropertyIDLineSpacing));
  EXPECT_FALSE(lynx::devtool::InspectorCSSHelper::IsIntProp(
      lynx::tasm::CSSPropertyID::kPropertyIDColor));
}

TEST(InspectorCSSHelperTest, IsFloatPropTest) {
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsFloatProp(
      lynx::tasm::CSSPropertyID::kPropertyIDOpacity));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsFloatProp(
      lynx::tasm::CSSPropertyID::kPropertyIDFlex));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsFloatProp(
      lynx::tasm::CSSPropertyID::kPropertyIDFlexGrow));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsFloatProp(
      lynx::tasm::CSSPropertyID::kPropertyIDFlexShrink));
  EXPECT_FALSE(lynx::devtool::InspectorCSSHelper::IsFloatProp(
      lynx::tasm::CSSPropertyID::kPropertyIDTop));
  EXPECT_FALSE(lynx::devtool::InspectorCSSHelper::IsFloatProp(
      lynx::tasm::CSSPropertyID::kPropertyIDLineSpacing));
  EXPECT_FALSE(lynx::devtool::InspectorCSSHelper::IsFloatProp(
      lynx::tasm::CSSPropertyID::kPropertyIDColor));
}

TEST(InspectorCSSHelperTest, IsBorderPropTest) {
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsBorderProp(
      lynx::tasm::CSSPropertyID::kPropertyIDBorder));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsBorderProp(
      lynx::tasm::CSSPropertyID::kPropertyIDBorderRight));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsBorderProp(
      lynx::tasm::CSSPropertyID::kPropertyIDBorderLeft));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsBorderProp(
      lynx::tasm::CSSPropertyID::kPropertyIDBorderTop));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsBorderProp(
      lynx::tasm::CSSPropertyID::kPropertyIDBorderBottom));
  EXPECT_FALSE(lynx::devtool::InspectorCSSHelper::IsFloatProp(
      lynx::tasm::CSSPropertyID::kPropertyIDTop));
  EXPECT_FALSE(lynx::devtool::InspectorCSSHelper::IsBorderProp(
      lynx::tasm::CSSPropertyID::kPropertyIDLineSpacing));
  EXPECT_FALSE(lynx::devtool::InspectorCSSHelper::IsBorderProp(
      lynx::tasm::CSSPropertyID::kPropertyIDColor));
}

TEST(InspectorCSSHelperTest, IsSupportedPropTest) {
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsSupportedProp(
      lynx::tasm::CSSPropertyID::kPropertyIDPosition));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsSupportedProp(
      lynx::tasm::CSSPropertyID::kPropertyIDBackgroundPosition));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsSupportedProp(
      lynx::tasm::CSSPropertyID::kPropertyIDBorder));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsSupportedProp(
      lynx::tasm::CSSPropertyID::kPropertyIDFlexGrow));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsSupportedProp(
      lynx::tasm::CSSPropertyID::kPropertyIDMaxWidth));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsSupportedProp(
      lynx::tasm::CSSPropertyID::kPropertyIDBorderRadius));
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsSupportedProp(
      lynx::tasm::CSSPropertyID::kPropertyIDBackgroundColor));
}

TEST(InspectorCSSHelperTest, IsLegalTest) {
  std::string name1 = "background", value1 = "red";
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsLegal(name1, value1));
  std::string name2 = "color", value2 = "black";
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsLegal(name2, value2));

  std::string name3 = "border-width", value3 = "14px";
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsLegal(name3, value3));

  std::string name4 = "outline-width", value4 = "blue";
  EXPECT_FALSE(lynx::devtool::InspectorCSSHelper::IsLegal(name4, value4));
}

TEST(InspectorCSSHelperTest, IsAnimationLegalTest) {
  std::string name = "animation-duration", value = "0";
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAnimationLegal(name, value));
  name = "animation-delay", value = "15ms";
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAnimationLegal(name, value));
  name = "animation-duration", value = "1s";
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAnimationLegal(name, value));
  name = "animation-duration", value = "1.9s";
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAnimationLegal(name, value));
  name = "animation-delay", value = "3.5ms";
  EXPECT_FALSE(
      lynx::devtool::InspectorCSSHelper::IsAnimationLegal(name, value));

  name = "animation-timing-function", value = "linear";
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAnimationLegal(name, value));
  name = "animation-timing-function", value = "ease-in-out";
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAnimationLegal(name, value));
  name = "animation-timing-function", value = "cubic-bezier(0, 0, 1, 1)";
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAnimationLegal(name, value));
  name = "animation-timing-function", value = "linear11";
  EXPECT_FALSE(
      lynx::devtool::InspectorCSSHelper::IsAnimationLegal(name, value));

  name = "animation-iteration-count", value = "infinite";
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAnimationLegal(name, value));
  name = "animation-iteration-count", value = "1000000000";
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAnimationLegal(name, value));
  name = "animation-iteration-count", value = "100-infinite";
  EXPECT_FALSE(
      lynx::devtool::InspectorCSSHelper::IsAnimationLegal(name, value));

  name = "animation-direction", value = "normal";
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAnimationLegal(name, value));
  name = "animation-direction", value = "alternate-reverse";
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAnimationLegal(name, value));
  name = "animation-direction", value = "reverse";
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAnimationLegal(name, value));
  name = "animation-direction", value = "converse";
  EXPECT_FALSE(
      lynx::devtool::InspectorCSSHelper::IsAnimationLegal(name, value));

  name = "animation-fill-mode", value = "none";
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAnimationLegal(name, value));
  name = "animation-fill-mode", value = "forwards";
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAnimationLegal(name, value));
  name = "animation-fill-mode", value = "backwards";
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAnimationLegal(name, value));
  name = "animation-fill-mode", value = "both";
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAnimationLegal(name, value));
  name = "animation-fill-mode", value = "mode0";
  EXPECT_FALSE(
      lynx::devtool::InspectorCSSHelper::IsAnimationLegal(name, value));

  name = "animation-play-state", value = "running";
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAnimationLegal(name, value));
  name = "animation-play-state", value = "paused";
  EXPECT_TRUE(lynx::devtool::InspectorCSSHelper::IsAnimationLegal(name, value));
  name = "animation-play-state", value = "done";
  EXPECT_FALSE(
      lynx::devtool::InspectorCSSHelper::IsAnimationLegal(name, value));

  name = "animation-status", value = "running";
  EXPECT_FALSE(
      lynx::devtool::InspectorCSSHelper::IsAnimationLegal(name, value));
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
