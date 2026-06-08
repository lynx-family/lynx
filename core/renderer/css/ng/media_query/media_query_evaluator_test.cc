// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/media_query/media_query_evaluator.h"

#include <string>
#include <utility>

#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "core/renderer/css/ng/media_query/media_feature.h"
#include "core/renderer/css/ng/media_query/media_query.h"
#include "core/renderer/css/ng/media_query/media_query_exp.h"
#include "core/renderer/css/ng/media_query/media_query_set.h"
#include "core/renderer/css/ng/media_query/media_values.h"
#include "core/renderer/css/ng/parser/media_query_parser.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace css {

namespace {

MediaFeature MakeFeature(const std::string& name, MediaFeatureOperator op,
                         MediaFeatureValue value) {
  MediaFeatureId id = ResolveMediaFeatureId(name);
  return MediaFeature(id, name, op, std::move(value));
}

}  // namespace

// ===========================================================================
// MediaValues
// ===========================================================================

TEST(MediaValuesTest, DefaultValues) {
  MediaValues v;
  EXPECT_DOUBLE_EQ(v.ViewportWidth(), 0.0);
  EXPECT_DOUBLE_EQ(v.ViewportHeight(), 0.0);
  EXPECT_DOUBLE_EQ(v.DevicePixelRatio(), 1.0);
  EXPECT_DOUBLE_EQ(v.RootFontSize(), 16.0);
  EXPECT_DOUBLE_EQ(v.FontSize(), 16.0);
  EXPECT_EQ(v.Hover(), MediaTristate::kUnknown);
  EXPECT_EQ(v.Pointer(), MediaTristate::kUnknown);
  EXPECT_EQ(v.PreferredColorScheme(), MediaPreferredColorScheme::kLight);
}

TEST(MediaValuesTest, WithViewportFactory) {
  auto v = MediaValues::WithViewport(375.0, 812.0, 3.0);
  EXPECT_DOUBLE_EQ(v.ViewportWidth(), 375.0);
  EXPECT_DOUBLE_EQ(v.ViewportHeight(), 812.0);
  EXPECT_DOUBLE_EQ(v.DevicePixelRatio(), 3.0);
}

TEST(MediaValuesTest, OrientationDerivedFromViewport) {
  MediaValues v;
  v.SetViewportWidth(1024.0);
  v.SetViewportHeight(768.0);
  EXPECT_EQ(v.Orientation(), MediaOrientation::kLandscape);

  v.SetViewportWidth(768.0);
  v.SetViewportHeight(1024.0);
  EXPECT_EQ(v.Orientation(), MediaOrientation::kPortrait);
}

TEST(MediaValuesTest, OrientationExplicitOverridesViewport) {
  MediaValues v;
  v.SetViewportWidth(1024.0);
  v.SetViewportHeight(768.0);
  v.SetOrientation(MediaOrientation::kPortrait);
  EXPECT_EQ(v.Orientation(), MediaOrientation::kPortrait);
}

TEST(MediaValuesTest, OrientationUnknownWhenZeroViewport) {
  MediaValues v;
  EXPECT_EQ(v.Orientation(), MediaOrientation::kUnknown);
}

// ===========================================================================
// MediaQueryEvaluator
// ===========================================================================

class MediaQueryEvaluatorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    values_ = MediaValues::WithViewport(375.0, 812.0, 3.0);
    values_.SetRootFontSize(16.0);
    values_.SetFontSize(16.0);
    values_.SetHover(MediaTristate::kPresent);
    values_.SetPointer(MediaTristate::kPresent);
    values_.SetPreferredColorScheme(MediaPreferredColorScheme::kDark);
    evaluator_ = MediaQueryEvaluator(values_);
  }

  MediaValues values_;
  MediaQueryEvaluator evaluator_;
};

TEST_F(MediaQueryEvaluatorTest, EmptySetMatches) {
  // Spec: empty <media-query-list> evaluates to true (e.g. `@media {}`).
  auto set = fml::MakeRefCounted<MediaQuerySet>();
  EXPECT_TRUE(evaluator_.Eval(*set));
}

TEST_F(MediaQueryEvaluatorTest, EmptyTextParsesAndMatches) {
  auto set = MediaQueryParser::ParseMediaQuerySet("");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(evaluator_.Eval(*set));
}

TEST_F(MediaQueryEvaluatorTest, WhitespaceOnlyTextMatches) {
  auto set = MediaQueryParser::ParseMediaQuerySet("   \t\n  ");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(evaluator_.Eval(*set));
}

TEST_F(MediaQueryEvaluatorTest, InvalidTextDoesNotMatch) {
  // Wholly invalid input is replaced by `not all`.
  auto set = MediaQueryParser::ParseMediaQuerySet("invalid!");
  ASSERT_NE(set, nullptr);
  EXPECT_FALSE(evaluator_.Eval(*set));
}

TEST_F(MediaQueryEvaluatorTest, NullSetDoesNotMatch) {
  EXPECT_FALSE(evaluator_.Eval(static_cast<const MediaQuerySet*>(nullptr)));
}

TEST_F(MediaQueryEvaluatorTest, ScreenMatches) {
  auto query = fml::MakeRefCounted<MediaQuery>(MediaQueryRestrictor::kNone,
                                               "screen", nullptr);
  EXPECT_TRUE(evaluator_.Eval(*query));
}

TEST_F(MediaQueryEvaluatorTest, AllMatches) {
  auto query = fml::MakeRefCounted<MediaQuery>(MediaQueryRestrictor::kNone,
                                               "all", nullptr);
  EXPECT_TRUE(evaluator_.Eval(*query));
}

TEST_F(MediaQueryEvaluatorTest, PrintDoesNotMatch) {
  auto query = fml::MakeRefCounted<MediaQuery>(MediaQueryRestrictor::kNone,
                                               "print", nullptr);
  EXPECT_FALSE(evaluator_.Eval(*query));
}

TEST_F(MediaQueryEvaluatorTest, NotPrintMatches) {
  auto query = fml::MakeRefCounted<MediaQuery>(MediaQueryRestrictor::kNot,
                                               "print", nullptr);
  EXPECT_TRUE(evaluator_.Eval(*query));
}

TEST_F(MediaQueryEvaluatorTest, NotScreenDoesNotMatch) {
  auto query = fml::MakeRefCounted<MediaQuery>(MediaQueryRestrictor::kNot,
                                               "screen", nullptr);
  EXPECT_FALSE(evaluator_.Eval(*query));
}

TEST_F(MediaQueryEvaluatorTest, OnlyScreenMatches) {
  auto query = fml::MakeRefCounted<MediaQuery>(MediaQueryRestrictor::kOnly,
                                               "screen", nullptr);
  EXPECT_TRUE(evaluator_.Eval(*query));
}

// ---- width features -------------------------------------------------------

TEST_F(MediaQueryEvaluatorTest, WidthExact) {
  auto f = MakeFeature(
      "width", MediaFeatureOperator::kNone,
      MediaFeatureValue::Dimension(375.0, MediaFeatureUnit::kPixels));
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, WidthExactMismatch) {
  auto f = MakeFeature(
      "width", MediaFeatureOperator::kNone,
      MediaFeatureValue::Dimension(400.0, MediaFeatureUnit::kPixels));
  EXPECT_FALSE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, MinWidth) {
  auto f = MakeFeature(
      "min-width", MediaFeatureOperator::kNone,
      MediaFeatureValue::Dimension(300.0, MediaFeatureUnit::kPixels));
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, MinWidthFail) {
  auto f = MakeFeature(
      "min-width", MediaFeatureOperator::kNone,
      MediaFeatureValue::Dimension(400.0, MediaFeatureUnit::kPixels));
  EXPECT_FALSE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, MaxWidth) {
  auto f = MakeFeature(
      "max-width", MediaFeatureOperator::kNone,
      MediaFeatureValue::Dimension(500.0, MediaFeatureUnit::kPixels));
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, MaxWidthFail) {
  auto f = MakeFeature(
      "max-width", MediaFeatureOperator::kNone,
      MediaFeatureValue::Dimension(300.0, MediaFeatureUnit::kPixels));
  EXPECT_FALSE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, WidthRangeGe) {
  auto f = MakeFeature(
      "width", MediaFeatureOperator::kGe,
      MediaFeatureValue::Dimension(375.0, MediaFeatureUnit::kPixels));
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, WidthRangeGt) {
  auto f = MakeFeature(
      "width", MediaFeatureOperator::kGt,
      MediaFeatureValue::Dimension(375.0, MediaFeatureUnit::kPixels));
  EXPECT_FALSE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, WidthRangeLt) {
  auto f = MakeFeature(
      "width", MediaFeatureOperator::kLt,
      MediaFeatureValue::Dimension(400.0, MediaFeatureUnit::kPixels));
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, WidthDoubleRange) {
  auto f = MakeFeature(
      "width", MediaFeatureOperator::kGe,
      MediaFeatureValue::Dimension(300.0, MediaFeatureUnit::kPixels));
  f.SetRightBound(
      MediaFeatureOperator::kLe,
      MediaFeatureValue::Dimension(500.0, MediaFeatureUnit::kPixels));
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, WidthDoubleRangeFail) {
  auto f = MakeFeature(
      "width", MediaFeatureOperator::kGe,
      MediaFeatureValue::Dimension(400.0, MediaFeatureUnit::kPixels));
  f.SetRightBound(
      MediaFeatureOperator::kLe,
      MediaFeatureValue::Dimension(500.0, MediaFeatureUnit::kPixels));
  EXPECT_FALSE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, WidthBooleanTrue) {
  auto f = MakeFeature("width", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Boolean());
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, WidthBooleanFalseWhenZero) {
  MediaValues v;
  MediaQueryEvaluator e(v);
  auto f = MakeFeature("width", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Boolean());
  EXPECT_FALSE(e.EvalFeature(f));
}

// Regression: a manually-constructed feature with kInvalid type must NOT be
// treated as boolean and must NOT match. This verifies the CompareRange guard.
TEST_F(MediaQueryEvaluatorTest, WidthWithUnknownUnitDoesNotMatch) {
  auto f = MakeFeature("width", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Invalid("10foo"));
  EXPECT_FALSE(f.IsBoolean());
  EXPECT_FALSE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, MinWidthWithUnknownUnitDoesNotMatch) {
  auto f = MakeFeature("min-width", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Invalid("10foo"));
  EXPECT_FALSE(f.IsBoolean());
  EXPECT_FALSE(evaluator_.EvalFeature(f));
}

// (width: 10vh) → 10vh = 0.1 * 812 = 81.2px; viewport width is 375 ≠ 81.2.
TEST_F(MediaQueryEvaluatorTest, ParsedWidthWithVhUnitDoesNotMatch) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(width: 10vh)");
  ASSERT_NE(set, nullptr);
  EXPECT_FALSE(evaluator_.Eval(set.get()));
}

// Ensure genuine boolean form `(width)` still matches as before.
TEST_F(MediaQueryEvaluatorTest, ParsedWidthBooleanStillMatches) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(width)");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(evaluator_.Eval(set.get()));
}

// ---- height features ------------------------------------------------------

TEST_F(MediaQueryEvaluatorTest, HeightExact) {
  auto f = MakeFeature(
      "height", MediaFeatureOperator::kNone,
      MediaFeatureValue::Dimension(812.0, MediaFeatureUnit::kPixels));
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, MinHeight) {
  auto f = MakeFeature(
      "min-height", MediaFeatureOperator::kNone,
      MediaFeatureValue::Dimension(600.0, MediaFeatureUnit::kPixels));
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, MaxHeight) {
  auto f = MakeFeature(
      "max-height", MediaFeatureOperator::kNone,
      MediaFeatureValue::Dimension(900.0, MediaFeatureUnit::kPixels));
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

// ---- rem / em unit support ------------------------------------------------

TEST_F(MediaQueryEvaluatorTest, WidthRemUnit) {
  auto f =
      MakeFeature("min-width", MediaFeatureOperator::kNone,
                  MediaFeatureValue::Dimension(20.0, MediaFeatureUnit::kRem));
  // 20rem * 16 = 320px <= 375
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, WidthEmUnit) {
  auto f =
      MakeFeature("min-width", MediaFeatureOperator::kNone,
                  MediaFeatureValue::Dimension(25.0, MediaFeatureUnit::kEm));
  // 25em * 16 = 400px > 375
  EXPECT_FALSE(evaluator_.EvalFeature(f));
}

// ---- vw / vh unit support -------------------------------------------------

TEST_F(MediaQueryEvaluatorTest, MinWidthVwMatches) {
  auto f = MakeFeature(
      "min-width", MediaFeatureOperator::kNone,
      MediaFeatureValue::Dimension(50.0, MediaFeatureUnit::kViewportWidth));
  // 50vw = 0.5 * 375 = 187.5px <= 375
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, MinWidthVwExceeds) {
  auto f = MakeFeature(
      "min-width", MediaFeatureOperator::kNone,
      MediaFeatureValue::Dimension(200.0, MediaFeatureUnit::kViewportWidth));
  // 200vw = 2.0 * 375 = 750px > 375
  EXPECT_FALSE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, MinHeightVhMatches) {
  auto f = MakeFeature(
      "min-height", MediaFeatureOperator::kNone,
      MediaFeatureValue::Dimension(50.0, MediaFeatureUnit::kViewportHeight));
  // 50vh = 0.5 * 812 = 406px <= 812
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, MinHeightVhExceeds) {
  auto f = MakeFeature(
      "min-height", MediaFeatureOperator::kNone,
      MediaFeatureValue::Dimension(150.0, MediaFeatureUnit::kViewportHeight));
  // 150vh = 1.5 * 812 = 1218px > 812
  EXPECT_FALSE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, ParsedMinWidth50VwMatches) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(min-width: 50vw)");
  ASSERT_NE(set, nullptr);
  // 50vw = 187.5px <= viewport width 375
  EXPECT_TRUE(evaluator_.Eval(set.get()));
}

TEST_F(MediaQueryEvaluatorTest, ParsedMaxHeight50VhMatches) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(max-height: 200vh)");
  ASSERT_NE(set, nullptr);
  // 200vh = 1624px >= viewport height 812
  EXPECT_TRUE(evaluator_.Eval(set.get()));
}

// ---- orientation ----------------------------------------------------------

TEST_F(MediaQueryEvaluatorTest, OrientationPortrait) {
  auto f = MakeFeature("orientation", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Ident("portrait"));
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, OrientationLandscape) {
  auto f = MakeFeature("orientation", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Ident("landscape"));
  EXPECT_FALSE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, OrientationBoolean) {
  auto f = MakeFeature("orientation", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Boolean());
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, OrientationPortraitWhenSquare) {
  MediaValues v = MediaValues::WithViewport(500.0, 500.0, 2.0);
  MediaQueryEvaluator e(v);
  auto f = MakeFeature("orientation", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Ident("portrait"));
  EXPECT_TRUE(e.EvalFeature(f));
}

// ---- resolution -----------------------------------------------------------

TEST_F(MediaQueryEvaluatorTest, MinResolutionDpi) {
  auto f =
      MakeFeature("min-resolution", MediaFeatureOperator::kNone,
                  MediaFeatureValue::Dimension(200.0, MediaFeatureUnit::kDpi));
  // 200dpi = 200/96 ≈ 2.08 dppx; actual is 3.0 => pass
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, ResolutionDppx) {
  auto f =
      MakeFeature("resolution", MediaFeatureOperator::kGe,
                  MediaFeatureValue::Dimension(2.0, MediaFeatureUnit::kDppx));
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, ResolutionX) {
  auto f =
      MakeFeature("resolution", MediaFeatureOperator::kGe,
                  MediaFeatureValue::Dimension(2.0, MediaFeatureUnit::kDppx));
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

// ---- aspect-ratio ---------------------------------------------------------

TEST_F(MediaQueryEvaluatorTest, AspectRatioMatch) {
  // viewport = 375 x 812, ratio = 375/812 ≈ 0.462
  auto f = MakeFeature("max-aspect-ratio", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Ratio(1.0, 1.0));
  // 0.462 <= 1.0 => pass
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, AspectRatioFail) {
  auto f = MakeFeature("min-aspect-ratio", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Ratio(16.0, 9.0));
  // 0.462 >= 16/9 ≈ 1.78 => fail
  EXPECT_FALSE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, AspectRatioDenominatorZero) {
  // 1/0 is a degenerate ratio representing infinity; no finite viewport
  // can have an aspect-ratio >= infinity, so min-aspect-ratio must fail.
  auto f = MakeFeature("min-aspect-ratio", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Ratio(1.0, 0.0));
  EXPECT_FALSE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, AspectRatioExactMatch) {
  MediaValues v = MediaValues::WithViewport(160.0, 90.0, 2.0);
  MediaQueryEvaluator e(v);
  auto f = MakeFeature("aspect-ratio", MediaFeatureOperator::kEq,
                       MediaFeatureValue::Ratio(16.0, 9.0));
  EXPECT_TRUE(e.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, AspectRatioExactMismatch) {
  MediaValues v = MediaValues::WithViewport(160.0, 91.0, 2.0);
  MediaQueryEvaluator e(v);
  auto f = MakeFeature("aspect-ratio", MediaFeatureOperator::kEq,
                       MediaFeatureValue::Ratio(16.0, 9.0));
  EXPECT_FALSE(e.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, AspectRatioBoolean) {
  auto f = MakeFeature("aspect-ratio", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Boolean());
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, AspectRatioBooleanZeroWidth) {
  MediaValues v = MediaValues::WithViewport(0.0, 812.0, 2.0);
  MediaQueryEvaluator e(v);
  auto f = MakeFeature("aspect-ratio", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Boolean());
  EXPECT_FALSE(e.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, AspectRatioZeroHeight) {
  MediaValues v = MediaValues::WithViewport(375.0, 0.0, 2.0);
  MediaQueryEvaluator e(v);
  auto f = MakeFeature("aspect-ratio", MediaFeatureOperator::kEq,
                       MediaFeatureValue::Ratio(1.0, 1.0));
  EXPECT_FALSE(e.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, AspectRatioNegativeDenominator) {
  auto f = MakeFeature("min-aspect-ratio", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Ratio(1.0, -1.0));
  EXPECT_FALSE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, AspectRatioGtPass) {
  MediaValues v = MediaValues::WithViewport(1920.0, 1080.0, 1.0);
  MediaQueryEvaluator e(v);
  auto f = MakeFeature("aspect-ratio", MediaFeatureOperator::kGt,
                       MediaFeatureValue::Ratio(4.0, 3.0));
  EXPECT_TRUE(e.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, AspectRatioGtBoundary) {
  MediaValues v = MediaValues::WithViewport(160.0, 90.0, 1.0);
  MediaQueryEvaluator e(v);
  auto f = MakeFeature("aspect-ratio", MediaFeatureOperator::kGt,
                       MediaFeatureValue::Ratio(16.0, 9.0));
  EXPECT_FALSE(e.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, AspectRatioLtPass) {
  auto f = MakeFeature("aspect-ratio", MediaFeatureOperator::kLt,
                       MediaFeatureValue::Ratio(1.0, 1.0));
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, AspectRatioLtBoundary) {
  MediaValues v = MediaValues::WithViewport(160.0, 90.0, 1.0);
  MediaQueryEvaluator e(v);
  auto f = MakeFeature("aspect-ratio", MediaFeatureOperator::kLt,
                       MediaFeatureValue::Ratio(16.0, 9.0));
  EXPECT_FALSE(e.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, AspectRatioGeEqualBoundary) {
  MediaValues v = MediaValues::WithViewport(160.0, 90.0, 1.0);
  MediaQueryEvaluator e(v);
  auto f = MakeFeature("aspect-ratio", MediaFeatureOperator::kGe,
                       MediaFeatureValue::Ratio(16.0, 9.0));
  EXPECT_TRUE(e.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, AspectRatioLeEqualBoundary) {
  MediaValues v = MediaValues::WithViewport(160.0, 90.0, 1.0);
  MediaQueryEvaluator e(v);
  auto f = MakeFeature("aspect-ratio", MediaFeatureOperator::kLe,
                       MediaFeatureValue::Ratio(16.0, 9.0));
  EXPECT_TRUE(e.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, AspectRatioDoubleRange) {
  MediaValues v = MediaValues::WithViewport(160.0, 90.0, 1.0);
  MediaQueryEvaluator e(v);
  auto f = MakeFeature("aspect-ratio", MediaFeatureOperator::kGe,
                       MediaFeatureValue::Ratio(4.0, 3.0));
  f.SetRightBound(MediaFeatureOperator::kLe,
                  MediaFeatureValue::Ratio(2.0, 1.0));
  EXPECT_TRUE(e.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, AspectRatioDoubleRangeFailLeft) {
  MediaValues v = MediaValues::WithViewport(100.0, 100.0, 1.0);
  MediaQueryEvaluator e(v);
  auto f = MakeFeature("aspect-ratio", MediaFeatureOperator::kGe,
                       MediaFeatureValue::Ratio(4.0, 3.0));
  f.SetRightBound(MediaFeatureOperator::kLe,
                  MediaFeatureValue::Ratio(2.0, 1.0));
  EXPECT_FALSE(e.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, AspectRatioDoubleRangeFailRight) {
  MediaValues v = MediaValues::WithViewport(300.0, 100.0, 1.0);
  MediaQueryEvaluator e(v);
  auto f = MakeFeature("aspect-ratio", MediaFeatureOperator::kGe,
                       MediaFeatureValue::Ratio(4.0, 3.0));
  f.SetRightBound(MediaFeatureOperator::kLe,
                  MediaFeatureValue::Ratio(2.0, 1.0));
  EXPECT_FALSE(e.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, AspectRatioPlainNumber) {
  MediaValues v = MediaValues::WithViewport(200.0, 100.0, 1.0);
  MediaQueryEvaluator e(v);
  auto f = MakeFeature("min-aspect-ratio", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Number(2.0));
  EXPECT_TRUE(e.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, AspectRatioPlainNumberFail) {
  MediaValues v = MediaValues::WithViewport(200.0, 100.0, 1.0);
  MediaQueryEvaluator e(v);
  auto f = MakeFeature("min-aspect-ratio", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Number(3.0));
  EXPECT_FALSE(e.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, AspectRatioFloatPrecisionEqual) {
  MediaValues v = MediaValues::WithViewport(1.0, 3.0, 1.0);
  MediaQueryEvaluator e(v);
  auto f = MakeFeature("aspect-ratio", MediaFeatureOperator::kEq,
                       MediaFeatureValue::Ratio(1.0, 3.0));
  EXPECT_TRUE(e.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, AspectRatioLargeValues) {
  MediaValues v = MediaValues::WithViewport(3840.0, 2160.0, 1.0);
  MediaQueryEvaluator e(v);
  auto f = MakeFeature("aspect-ratio", MediaFeatureOperator::kEq,
                       MediaFeatureValue::Ratio(16.0, 9.0));
  EXPECT_TRUE(e.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, DeviceAspectRatioMinMatch) {
  auto f = MakeFeature("min-device-aspect-ratio", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Ratio(1.0, 3.0));
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, DeviceAspectRatioMinFail) {
  auto f = MakeFeature("min-device-aspect-ratio", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Ratio(1.0, 1.0));
  EXPECT_FALSE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, DeviceAspectRatioExact) {
  MediaValues v = MediaValues::WithViewport(1920.0, 1080.0, 1.0);
  MediaQueryEvaluator e(v);
  auto f = MakeFeature("device-aspect-ratio", MediaFeatureOperator::kEq,
                       MediaFeatureValue::Ratio(16.0, 9.0));
  EXPECT_TRUE(e.EvalFeature(f));
}

// ---- hover / pointer ------------------------------------------------------

TEST_F(MediaQueryEvaluatorTest, HoverPresent) {
  auto f = MakeFeature("hover", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Ident("hover"));
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, HoverNone) {
  auto f = MakeFeature("hover", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Ident("none"));
  EXPECT_FALSE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, HoverBoolean) {
  auto f = MakeFeature("hover", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Boolean());
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, PointerFine) {
  auto f = MakeFeature("pointer", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Ident("fine"));
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, PointerNone) {
  auto f = MakeFeature("pointer", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Ident("none"));
  EXPECT_FALSE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, PointerBoolean) {
  auto f = MakeFeature("pointer", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Boolean());
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

// ---- prefers-color-scheme -------------------------------------------------

TEST_F(MediaQueryEvaluatorTest, PrefersColorSchemeDark) {
  auto f = MakeFeature("prefers-color-scheme", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Ident("dark"));
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, PrefersColorSchemeLight) {
  auto f = MakeFeature("prefers-color-scheme", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Ident("light"));
  EXPECT_FALSE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, PrefersColorSchemeBoolean) {
  auto f = MakeFeature("prefers-color-scheme", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Boolean());
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

// ---- color ----------------------------------------------------------------

TEST_F(MediaQueryEvaluatorTest, ColorBoolean) {
  auto f = MakeFeature("color", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Boolean());
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, ColorExactMatch) {
  auto f = MakeFeature("color", MediaFeatureOperator::kEq,
                       MediaFeatureValue::Number(8.0));
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, ColorExactMismatch) {
  auto f = MakeFeature("color", MediaFeatureOperator::kEq,
                       MediaFeatureValue::Number(16.0));
  EXPECT_FALSE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, MinColorPass) {
  auto f = MakeFeature("min-color", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Number(4.0));
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, MinColorFail) {
  auto f = MakeFeature("min-color", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Number(10.0));
  EXPECT_FALSE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, MaxColorPass) {
  auto f = MakeFeature("max-color", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Number(8.0));
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, MaxColorFail) {
  auto f = MakeFeature("max-color", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Number(4.0));
  EXPECT_FALSE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, ParsedColorBooleanMatches) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(color)");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(evaluator_.Eval(set.get()));
}

TEST_F(MediaQueryEvaluatorTest, ParsedMinColor8Matches) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(min-color: 8)");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(evaluator_.Eval(set.get()));
}

// ---- device-pixel-ratio ---------------------------------------------------

TEST_F(MediaQueryEvaluatorTest, DevicePixelRatioBoolean) {
  auto f = MakeFeature("device-pixel-ratio", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Boolean());
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, DevicePixelRatioExactMatch) {
  auto f = MakeFeature("device-pixel-ratio", MediaFeatureOperator::kEq,
                       MediaFeatureValue::Number(3.0));
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, DevicePixelRatioExactMismatch) {
  auto f = MakeFeature("device-pixel-ratio", MediaFeatureOperator::kEq,
                       MediaFeatureValue::Number(2.0));
  EXPECT_FALSE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, MinDevicePixelRatioPass) {
  auto f = MakeFeature("min-device-pixel-ratio", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Number(2.0));
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, MinDevicePixelRatioFail) {
  auto f = MakeFeature("min-device-pixel-ratio", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Number(4.0));
  EXPECT_FALSE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, MaxDevicePixelRatioPass) {
  auto f = MakeFeature("max-device-pixel-ratio", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Number(3.0));
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, MaxDevicePixelRatioFail) {
  auto f = MakeFeature("max-device-pixel-ratio", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Number(2.0));
  EXPECT_FALSE(evaluator_.EvalFeature(f));
}

TEST_F(MediaQueryEvaluatorTest, ParsedMinDevicePixelRatio2Matches) {
  auto set =
      MediaQueryParser::ParseMediaQuerySet("(min-device-pixel-ratio: 2)");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(evaluator_.Eval(set.get()));
}

TEST_F(MediaQueryEvaluatorTest, ParsedMaxDevicePixelRatio2Fails) {
  auto set =
      MediaQueryParser::ParseMediaQuerySet("(max-device-pixel-ratio: 2)");
  ASSERT_NE(set, nullptr);
  EXPECT_FALSE(evaluator_.Eval(set.get()));
}

// ---- unknown feature ------------------------------------------------------

TEST_F(MediaQueryEvaluatorTest, UnknownFeatureFails) {
  auto f = MakeFeature("some-unknown-feature", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Number(1.0));
  EXPECT_FALSE(evaluator_.EvalFeature(f));
}

// ---- node evaluation (compound expressions) -------------------------------

TEST_F(MediaQueryEvaluatorTest, EvalNotNode) {
  auto f = MakeFeature(
      "width", MediaFeatureOperator::kGe,
      MediaFeatureValue::Dimension(1000.0, MediaFeatureUnit::kPixels));
  auto inner = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f));
  auto not_node = fml::MakeRefCounted<MediaQueryNotExpNode>(std::move(inner));
  EXPECT_TRUE(evaluator_.EvalNode(*not_node));
}

TEST_F(MediaQueryEvaluatorTest, EvalAndNodeBothTrue) {
  auto f1 = MakeFeature(
      "min-width", MediaFeatureOperator::kNone,
      MediaFeatureValue::Dimension(300.0, MediaFeatureUnit::kPixels));
  auto f2 = MakeFeature(
      "max-width", MediaFeatureOperator::kNone,
      MediaFeatureValue::Dimension(500.0, MediaFeatureUnit::kPixels));
  auto left = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f1));
  auto right = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f2));
  auto and_node = fml::MakeRefCounted<MediaQueryAndExpNode>(std::move(left),
                                                            std::move(right));
  EXPECT_TRUE(evaluator_.EvalNode(*and_node));
}

TEST_F(MediaQueryEvaluatorTest, EvalAndNodeOneFails) {
  auto f1 = MakeFeature(
      "min-width", MediaFeatureOperator::kNone,
      MediaFeatureValue::Dimension(300.0, MediaFeatureUnit::kPixels));
  auto f2 = MakeFeature(
      "max-width", MediaFeatureOperator::kNone,
      MediaFeatureValue::Dimension(350.0, MediaFeatureUnit::kPixels));
  auto left = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f1));
  auto right = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f2));
  auto and_node = fml::MakeRefCounted<MediaQueryAndExpNode>(std::move(left),
                                                            std::move(right));
  EXPECT_FALSE(evaluator_.EvalNode(*and_node));
}

TEST_F(MediaQueryEvaluatorTest, EvalOrNodeOneTrue) {
  auto f1 = MakeFeature(
      "min-width", MediaFeatureOperator::kNone,
      MediaFeatureValue::Dimension(1000.0, MediaFeatureUnit::kPixels));
  auto f2 = MakeFeature(
      "max-width", MediaFeatureOperator::kNone,
      MediaFeatureValue::Dimension(500.0, MediaFeatureUnit::kPixels));
  auto left = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f1));
  auto right = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f2));
  auto or_node = fml::MakeRefCounted<MediaQueryOrExpNode>(std::move(left),
                                                          std::move(right));
  EXPECT_TRUE(evaluator_.EvalNode(*or_node));
}

TEST_F(MediaQueryEvaluatorTest, EvalOrNodeBothFail) {
  auto f1 = MakeFeature(
      "min-width", MediaFeatureOperator::kNone,
      MediaFeatureValue::Dimension(1000.0, MediaFeatureUnit::kPixels));
  auto f2 = MakeFeature(
      "min-width", MediaFeatureOperator::kNone,
      MediaFeatureValue::Dimension(800.0, MediaFeatureUnit::kPixels));
  auto left = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f1));
  auto right = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f2));
  auto or_node = fml::MakeRefCounted<MediaQueryOrExpNode>(std::move(left),
                                                          std::move(right));
  EXPECT_FALSE(evaluator_.EvalNode(*or_node));
}

TEST_F(MediaQueryEvaluatorTest, EvalNestedNode) {
  auto f = MakeFeature(
      "width", MediaFeatureOperator::kGe,
      MediaFeatureValue::Dimension(300.0, MediaFeatureUnit::kPixels));
  auto inner = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f));
  auto nested = fml::MakeRefCounted<MediaQueryNestedExpNode>(std::move(inner));
  EXPECT_TRUE(evaluator_.EvalNode(*nested));
}

TEST_F(MediaQueryEvaluatorTest, EvalNullNodeFails) {
  EXPECT_FALSE(
      evaluator_.EvalNode(static_cast<const MediaQueryExpNode*>(nullptr)));
}

// ---- full query evaluation ------------------------------------------------

TEST_F(MediaQueryEvaluatorTest, EvalQuerySetOneMatch) {
  auto set = fml::MakeRefCounted<MediaQuerySet>();
  auto q1 = fml::MakeRefCounted<MediaQuery>(MediaQueryRestrictor::kNone,
                                            "print", nullptr);
  auto f = MakeFeature(
      "min-width", MediaFeatureOperator::kNone,
      MediaFeatureValue::Dimension(300.0, MediaFeatureUnit::kPixels));
  auto cond = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f));
  auto q2 = fml::MakeRefCounted<MediaQuery>(MediaQueryRestrictor::kNone,
                                            "screen", std::move(cond));
  set->Append(std::move(q1));
  set->Append(std::move(q2));
  EXPECT_TRUE(evaluator_.Eval(*set));
}

TEST_F(MediaQueryEvaluatorTest, EvalQuerySetNoneMatch) {
  auto set = fml::MakeRefCounted<MediaQuerySet>();
  auto q1 = fml::MakeRefCounted<MediaQuery>(MediaQueryRestrictor::kNone,
                                            "print", nullptr);
  auto f = MakeFeature(
      "min-width", MediaFeatureOperator::kNone,
      MediaFeatureValue::Dimension(1000.0, MediaFeatureUnit::kPixels));
  auto cond = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f));
  auto q2 = fml::MakeRefCounted<MediaQuery>(MediaQueryRestrictor::kNone,
                                            "screen", std::move(cond));
  set->Append(std::move(q1));
  set->Append(std::move(q2));
  EXPECT_FALSE(evaluator_.Eval(*set));
}

// ---- invalid combos -------------------------------------------------------

TEST_F(MediaQueryEvaluatorTest, MinMaxPrefixWithExplicitOpFails) {
  auto f = MakeFeature(
      "min-width", MediaFeatureOperator::kGe,
      MediaFeatureValue::Dimension(300.0, MediaFeatureUnit::kPixels));
  EXPECT_FALSE(evaluator_.EvalFeature(f));
}

// ---- SetValues changes evaluation -----------------------------------------

TEST_F(MediaQueryEvaluatorTest, SetValuesUpdatesResult) {
  auto f = MakeFeature(
      "min-width", MediaFeatureOperator::kNone,
      MediaFeatureValue::Dimension(500.0, MediaFeatureUnit::kPixels));
  EXPECT_FALSE(evaluator_.EvalFeature(f));

  MediaValues new_values = MediaValues::WithViewport(600.0, 800.0, 2.0);
  evaluator_.SetValues(new_values);
  EXPECT_TRUE(evaluator_.EvalFeature(f));
}

}  // namespace css
}  // namespace lynx
