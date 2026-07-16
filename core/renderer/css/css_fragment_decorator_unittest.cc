// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/css/css_fragment_decorator.h"

#include "core/renderer/css/shared_css_fragment.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

class CSSFragmentDecoratorTest : public ::testing::Test {
 protected:
  CSSFragmentDecoratorTest() = default;
  ~CSSFragmentDecoratorTest() override = default;
};

TEST_F(CSSFragmentDecoratorTest,
       MarkFontFacesResolvedDoesNotPolluteSharedIntrinsicFragment) {
  CSSFontFaceRuleMap fontfaces;
  auto font_rule = std::make_shared<CSSFontFaceRule>(
      "custom-font", CSSFontFaceAttrsMap{{"src", "url(custom.woff2)"}});
  fontfaces["custom-font"].push_back(font_rule);

  SharedCSSFragment shared(1, std::vector<int32_t>{}, CSSParserTokenMap{},
                           CSSKeyframesTokenMap{}, std::move(fontfaces));

  CSSFragmentDecorator decorator_a(&shared);
  CSSFragmentDecorator decorator_b(&shared);

  // Precondition: neither decorator nor the shared fragment is resolved.
  EXPECT_FALSE(decorator_a.HasFontFacesResolved());
  EXPECT_FALSE(decorator_b.HasFontFacesResolved());
  EXPECT_FALSE(shared.HasFontFacesResolved());

  decorator_a.MarkFontFacesResolved(true);

  // The resolved state must be local to decorator_a. The shared intrinsic
  // fragment and decorator_b must remain unresolved so that another LynxView
  // sharing the same predecoded CSS data can still resolve its own font-faces.
  EXPECT_TRUE(decorator_a.HasFontFacesResolved());
  EXPECT_FALSE(decorator_b.HasFontFacesResolved());
  EXPECT_FALSE(shared.HasFontFacesResolved());
}

TEST_F(CSSFragmentDecoratorTest,
       ForEachUnresolvedFontFaceMapIsolatedAcrossDecoratorsSharingIntrinsic) {
  CSSFontFaceRuleMap fontfaces;
  auto font_rule = std::make_shared<CSSFontFaceRule>(
      "custom-font", CSSFontFaceAttrsMap{{"src", "url(custom.woff2)"}});
  fontfaces["custom-font"].push_back(font_rule);

  SharedCSSFragment shared(1, std::vector<int32_t>{}, CSSParserTokenMap{},
                           CSSKeyframesTokenMap{}, std::move(fontfaces));

  CSSFragmentDecorator decorator_a(&shared);
  CSSFragmentDecorator decorator_b(&shared);

  size_t visit_count_a = 0;
  decorator_a.ForEachUnresolvedFontFaceMap(
      [](const CSSFontFaceRuleMap& map, void* cb_data) {
        if (!map.empty()) {
          ++*static_cast<size_t*>(cb_data);
        }
      },
      &visit_count_a);
  EXPECT_EQ(visit_count_a, 1u);

  // Marking the first decorator resolved must not prevent the second decorator
  // from visiting the shared intrinsic font-face map.
  decorator_a.MarkFontFacesResolved(true);

  size_t visit_count_b = 0;
  decorator_b.ForEachUnresolvedFontFaceMap(
      [](const CSSFontFaceRuleMap& map, void* cb_data) {
        if (!map.empty()) {
          ++*static_cast<size_t*>(cb_data);
        }
      },
      &visit_count_b);
  EXPECT_EQ(visit_count_b, 1u);

  // After decorator_b resolves, re-visiting decorator_b should skip.
  decorator_b.MarkFontFacesResolved(true);
  size_t visit_count_b_again = 0;
  decorator_b.ForEachUnresolvedFontFaceMap(
      [](const CSSFontFaceRuleMap& map, void* cb_data) {
        if (!map.empty()) {
          ++*static_cast<size_t*>(cb_data);
        }
      },
      &visit_count_b_again);
  EXPECT_EQ(visit_count_b_again, 0u);
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
