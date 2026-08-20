// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/parser/media_query_parser.h"

#include <string>

#include "base/include/value/base_value.h"
#include "core/renderer/css/ng/media_query/media_feature.h"
#include "core/renderer/css/ng/media_query/media_query.h"
#include "core/renderer/css/ng/media_query/media_query_exp.h"
#include "core/renderer/css/ng/media_query/media_query_set.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace css {

namespace {

// Returns true if `set` is the spec-mandated `not all` placeholder used
// by the parser to encode wholly invalid input.
bool IsNotAllSet(const MediaQuerySet& set) {
  if (set.Queries().size() != 1u) return false;
  const auto& q = *set.Queries()[0];
  return q.Restrictor() == MediaQueryRestrictor::kNot &&
         q.MediaType() == MediaQuery::kTypeAll && q.Condition() == nullptr;
}

}  // namespace

// ---------------------------------------------------------------------------
// ParseMediaQuerySet — basic structural tests
// ---------------------------------------------------------------------------

TEST(MediaQueryParserTest, EmptyInput) {
  // Empty input -> valid empty list (spec: matches all).
  auto set = MediaQueryParser::ParseMediaQuerySet("");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(set->IsEmpty());
}

TEST(MediaQueryParserTest, WhitespaceOnlyInput) {
  auto set = MediaQueryParser::ParseMediaQuerySet("   \t\n  ");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(set->IsEmpty());
}

TEST(MediaQueryParserTest, WhollyInvalidInputProducesNotAllPlaceholder) {
  // Wholly invalid input -> `not all` placeholder (spec).
  auto set = MediaQueryParser::ParseMediaQuerySet("(invalid!)");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(IsNotAllSet(*set));
}

TEST(MediaQueryParserTest, SingleMediaType) {
  auto set = MediaQueryParser::ParseMediaQuerySet("screen");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  const auto& q = *set->Queries()[0];
  EXPECT_EQ(q.Restrictor(), MediaQueryRestrictor::kNone);
  EXPECT_EQ(q.MediaType(), "screen");
  EXPECT_EQ(q.Condition(), nullptr);
}

TEST(MediaQueryParserTest, MediaTypeAll) {
  auto set = MediaQueryParser::ParseMediaQuerySet("all");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  EXPECT_EQ(set->Queries()[0]->MediaType(), "all");
}

TEST(MediaQueryParserTest, NotMediaType) {
  auto set = MediaQueryParser::ParseMediaQuerySet("not print");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  const auto& q = *set->Queries()[0];
  EXPECT_EQ(q.Restrictor(), MediaQueryRestrictor::kNot);
  EXPECT_EQ(q.MediaType(), "print");
}

TEST(MediaQueryParserTest, OnlyMediaType) {
  auto set = MediaQueryParser::ParseMediaQuerySet("only screen");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  const auto& q = *set->Queries()[0];
  EXPECT_EQ(q.Restrictor(), MediaQueryRestrictor::kOnly);
  EXPECT_EQ(q.MediaType(), "screen");
}

// ---------------------------------------------------------------------------
// Single feature tests
// ---------------------------------------------------------------------------

TEST(MediaQueryParserTest, BooleanFeature) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(hover)");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  const auto& q = *set->Queries()[0];
  EXPECT_EQ(q.MediaType(), "");
  ASSERT_NE(q.Condition(), nullptr);
  EXPECT_EQ(q.Condition()->GetType(), MediaQueryExpNode::Type::kFeature);
  const auto& feature =
      static_cast<const MediaQueryFeatureExpNode&>(*q.Condition()).Feature();
  EXPECT_EQ(feature.Name(), "hover");
  EXPECT_TRUE(feature.IsBoolean());
}

TEST(MediaQueryParserTest, PlainFeatureWithValue) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(min-width: 600px)");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  const auto& q = *set->Queries()[0];
  ASSERT_NE(q.Condition(), nullptr);
  EXPECT_EQ(q.Condition()->GetType(), MediaQueryExpNode::Type::kFeature);
  const auto& feature =
      static_cast<const MediaQueryFeatureExpNode&>(*q.Condition()).Feature();
  EXPECT_EQ(feature.Name(), "min-width");
  EXPECT_EQ(feature.LeftOperator(), MediaFeatureOperator::kNone);
  EXPECT_EQ(feature.LeftValue().Type(), MediaFeatureType::kDimension);
  EXPECT_EQ(feature.LeftValue().Unit(), MediaFeatureUnit::kPixels);
  EXPECT_DOUBLE_EQ(feature.LeftValue().Numeric(), 600.0);
}

TEST(MediaQueryParserTest, RangeFeatureSingleOp) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(width >= 1024px)");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  const auto& q = *set->Queries()[0];
  ASSERT_NE(q.Condition(), nullptr);
  const auto& feature =
      static_cast<const MediaQueryFeatureExpNode&>(*q.Condition()).Feature();
  EXPECT_EQ(feature.Name(), "width");
  EXPECT_EQ(feature.LeftOperator(), MediaFeatureOperator::kGe);
  EXPECT_DOUBLE_EQ(feature.LeftValue().Numeric(), 1024.0);
  EXPECT_FALSE(feature.HasRightBound());
}

TEST(MediaQueryParserTest, RangeFeatureDoubleOp) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(100px < width < 500px)");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  const auto& q = *set->Queries()[0];
  ASSERT_NE(q.Condition(), nullptr);
  const auto& feature =
      static_cast<const MediaQueryFeatureExpNode&>(*q.Condition()).Feature();
  EXPECT_EQ(feature.Name(), "width");
  EXPECT_EQ(feature.LeftOperator(), MediaFeatureOperator::kGt);
  EXPECT_DOUBLE_EQ(feature.LeftValue().Numeric(), 100.0);
  EXPECT_TRUE(feature.HasRightBound());
  EXPECT_EQ(feature.RightOperator(), MediaFeatureOperator::kLt);
  EXPECT_DOUBLE_EQ(feature.RightValue().Numeric(), 500.0);
}

TEST(MediaQueryParserTest, FeatureWithIdentValue) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(orientation: landscape)");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  const auto& feature = static_cast<const MediaQueryFeatureExpNode&>(
                            *set->Queries()[0]->Condition())
                            .Feature();
  EXPECT_EQ(feature.Name(), "orientation");
  EXPECT_EQ(feature.LeftValue().Type(), MediaFeatureType::kIdent);
  EXPECT_EQ(feature.LeftValue().Text(), "landscape");
}

TEST(MediaQueryParserTest, FeatureWithNumberValue) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(color: 8)");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  const auto& feature = static_cast<const MediaQueryFeatureExpNode&>(
                            *set->Queries()[0]->Condition())
                            .Feature();
  EXPECT_EQ(feature.Name(), "color");
  EXPECT_EQ(feature.LeftValue().Type(), MediaFeatureType::kNumber);
  EXPECT_DOUBLE_EQ(feature.LeftValue().Numeric(), 8.0);
}

TEST(MediaQueryParserTest, FeatureWithRatioValue) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(aspect-ratio: 16/9)");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  const auto& feature = static_cast<const MediaQueryFeatureExpNode&>(
                            *set->Queries()[0]->Condition())
                            .Feature();
  EXPECT_EQ(feature.Name(), "aspect-ratio");
  EXPECT_EQ(feature.LeftValue().Type(), MediaFeatureType::kRatio);
  EXPECT_DOUBLE_EQ(feature.LeftValue().Numeric(), 16.0);
  EXPECT_DOUBLE_EQ(feature.LeftValue().Denominator(), 9.0);
}

TEST(MediaQueryParserTest, FeatureWithRemUnit) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(min-width: 2.5rem)");
  ASSERT_NE(set, nullptr);
  const auto& feature = static_cast<const MediaQueryFeatureExpNode&>(
                            *set->Queries()[0]->Condition())
                            .Feature();
  EXPECT_EQ(feature.LeftValue().Type(), MediaFeatureType::kDimension);
  EXPECT_EQ(feature.LeftValue().Unit(), MediaFeatureUnit::kRem);
  EXPECT_DOUBLE_EQ(feature.LeftValue().Numeric(), 2.5);
}

// ---------------------------------------------------------------------------
// Combinators: and / or / not
// ---------------------------------------------------------------------------

TEST(MediaQueryParserTest, AndCombinator) {
  auto set = MediaQueryParser::ParseMediaQuerySet(
      "(min-width: 600px) and (orientation: landscape)");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  const auto& cond = *set->Queries()[0]->Condition();
  EXPECT_EQ(cond.GetType(), MediaQueryExpNode::Type::kAnd);
  const auto& and_node = static_cast<const MediaQueryAndExpNode&>(cond);
  EXPECT_EQ(and_node.Left()->GetType(), MediaQueryExpNode::Type::kFeature);
  EXPECT_EQ(and_node.Right()->GetType(), MediaQueryExpNode::Type::kFeature);
}

TEST(MediaQueryParserTest, OrCombinator) {
  auto set = MediaQueryParser::ParseMediaQuerySet(
      "(min-width: 600px) or (orientation: portrait)");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  const auto& cond = *set->Queries()[0]->Condition();
  EXPECT_EQ(cond.GetType(), MediaQueryExpNode::Type::kOr);
}

TEST(MediaQueryParserTest, NotCondition) {
  auto set = MediaQueryParser::ParseMediaQuerySet("not (hover)");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  const auto& cond = *set->Queries()[0]->Condition();
  EXPECT_EQ(cond.GetType(), MediaQueryExpNode::Type::kNot);
  const auto& not_node = static_cast<const MediaQueryNotExpNode&>(cond);
  EXPECT_NE(not_node.Operand(), nullptr);
  EXPECT_EQ(not_node.Operand()->GetType(), MediaQueryExpNode::Type::kFeature);
}

TEST(MediaQueryParserTest, NestedParens) {
  auto set = MediaQueryParser::ParseMediaQuerySet("((min-width: 600px))");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  const auto& cond = *set->Queries()[0]->Condition();
  EXPECT_EQ(cond.GetType(), MediaQueryExpNode::Type::kNested);
  const auto& nested = static_cast<const MediaQueryNestedExpNode&>(cond);
  ASSERT_NE(nested.Inner(), nullptr);
  EXPECT_EQ(nested.Inner()->GetType(), MediaQueryExpNode::Type::kFeature);
}

TEST(MediaQueryParserTest, MultipleAndNodes) {
  auto set =
      MediaQueryParser::ParseMediaQuerySet("(a: 1) and (b: 2) and (c: 3)");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  const auto& cond = *set->Queries()[0]->Condition();
  EXPECT_EQ(cond.GetType(), MediaQueryExpNode::Type::kAnd);
  const auto& outer = static_cast<const MediaQueryAndExpNode&>(cond);
  EXPECT_EQ(outer.Left()->GetType(), MediaQueryExpNode::Type::kAnd);
  EXPECT_EQ(outer.Right()->GetType(), MediaQueryExpNode::Type::kFeature);
}

// ---------------------------------------------------------------------------
// Media type + condition
// ---------------------------------------------------------------------------

TEST(MediaQueryParserTest, MediaTypeAndCondition) {
  auto set =
      MediaQueryParser::ParseMediaQuerySet("screen and (min-width: 768px)");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  const auto& q = *set->Queries()[0];
  EXPECT_EQ(q.Restrictor(), MediaQueryRestrictor::kNone);
  EXPECT_EQ(q.MediaType(), "screen");
  ASSERT_NE(q.Condition(), nullptr);
  EXPECT_EQ(q.Condition()->GetType(), MediaQueryExpNode::Type::kFeature);
}

TEST(MediaQueryParserTest, NotMediaTypeAndCondition) {
  auto set = MediaQueryParser::ParseMediaQuerySet("not print and (color)");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  const auto& q = *set->Queries()[0];
  EXPECT_EQ(q.Restrictor(), MediaQueryRestrictor::kNot);
  EXPECT_EQ(q.MediaType(), "print");
  ASSERT_NE(q.Condition(), nullptr);
}

// ---------------------------------------------------------------------------
// Comma-separated list
// ---------------------------------------------------------------------------

TEST(MediaQueryParserTest, CommaSeparatedList) {
  auto set =
      MediaQueryParser::ParseMediaQuerySet("screen, (min-width: 600px), print");
  ASSERT_NE(set, nullptr);
  EXPECT_EQ(set->Queries().size(), 3u);
  EXPECT_EQ(set->Queries()[0]->MediaType(), "screen");
  EXPECT_NE(set->Queries()[1]->Condition(), nullptr);
  EXPECT_EQ(set->Queries()[2]->MediaType(), "print");
}

TEST(MediaQueryParserTest, CommaWithInvalidEntrySkipped) {
  auto set = MediaQueryParser::ParseMediaQuerySet("screen, ???, (hover)");
  ASSERT_NE(set, nullptr);
  EXPECT_EQ(set->Queries().size(), 2u);
  EXPECT_EQ(set->Queries()[0]->MediaType(), "screen");
  EXPECT_NE(set->Queries()[1]->Condition(), nullptr);
}

// ---------------------------------------------------------------------------
// Serialize roundtrip
// ---------------------------------------------------------------------------

TEST(MediaQueryParserTest, SerializeRoundtripPlainFeature) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(min-width: 600px)");
  ASSERT_NE(set, nullptr);
  std::string serialized = set->Serialize();
  EXPECT_EQ(serialized, "(min-width: 600px)");
}

TEST(MediaQueryParserTest, SerializeRoundtripMediaTypeAndCondition) {
  auto set = MediaQueryParser::ParseMediaQuerySet("not print and (color)");
  ASSERT_NE(set, nullptr);
  std::string serialized = set->Serialize();
  EXPECT_EQ(serialized, "not print and (color)");
}

TEST(MediaQueryParserTest, SerializeRoundtripCommaList) {
  auto set = MediaQueryParser::ParseMediaQuerySet("screen, (hover)");
  ASSERT_NE(set, nullptr);
  std::string serialized = set->Serialize();
  EXPECT_EQ(serialized, "screen, (hover)");
}

TEST(MediaQueryParserTest, SerializeRangeFeature) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(width >= 1024px)");
  ASSERT_NE(set, nullptr);
  std::string serialized = set->Serialize();
  EXPECT_EQ(serialized, "(width >= 1024px)");
}

// ---------------------------------------------------------------------------
// ToLepus / FromLepus roundtrip (parse -> serialize -> deserialize -> check)
// ---------------------------------------------------------------------------

TEST(MediaQueryParserTest, ToFromLepusRoundtripSimple) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(min-width: 600px)");
  ASSERT_NE(set, nullptr);
  auto lepus = set->ToLepus();
  auto restored = MediaQuerySet::FromLepus(lepus);
  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->Serialize(), set->Serialize());
}

TEST(MediaQueryParserTest, ToFromLepusRoundtripComplex) {
  auto set = MediaQueryParser::ParseMediaQuerySet(
      "not print and (color), screen and (min-width: 600px) and "
      "(orientation: landscape)");
  ASSERT_NE(set, nullptr);
  auto lepus = set->ToLepus();
  auto restored = MediaQuerySet::FromLepus(lepus);
  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->Serialize(), set->Serialize());
}

TEST(MediaQueryParserTest, ToFromLepusRoundtripRange) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(100px < width < 500px)");
  ASSERT_NE(set, nullptr);
  auto lepus = set->ToLepus();
  auto restored = MediaQuerySet::FromLepus(lepus);
  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->Serialize(), set->Serialize());
}

TEST(MediaQueryParserTest, ToFromLepusRoundtripNotOr) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(hover) or (pointer: fine)");
  ASSERT_NE(set, nullptr);
  auto lepus = set->ToLepus();
  auto restored = MediaQuerySet::FromLepus(lepus);
  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->Serialize(), set->Serialize());
}

TEST(MediaQueryParserTest, FromLepusInvalidReturnsNull) {
  lepus::Value invalid(42);
  auto result = MediaQuerySet::FromLepus(invalid);
  EXPECT_EQ(result, nullptr);
}

// ---------------------------------------------------------------------------
// ParseMediaCondition entry point
// ---------------------------------------------------------------------------

TEST(MediaQueryParserTest, ParseMediaConditionSimple) {
  auto node = MediaQueryParser::ParseMediaCondition("(hover)");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->GetType(), MediaQueryExpNode::Type::kFeature);
}

TEST(MediaQueryParserTest, ParseMediaConditionComplex) {
  auto node = MediaQueryParser::ParseMediaCondition(
      "(min-width: 600px) and (max-width: 1200px)");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->GetType(), MediaQueryExpNode::Type::kAnd);
}

TEST(MediaQueryParserTest, ParseMediaConditionEmpty) {
  auto node = MediaQueryParser::ParseMediaCondition("");
  EXPECT_EQ(node, nullptr);
}

TEST(MediaQueryParserTest, ParseMediaConditionInvalid) {
  auto node = MediaQueryParser::ParseMediaCondition("screen");
  EXPECT_EQ(node, nullptr);
}

// ---------------------------------------------------------------------------
// Case insensitivity
// ---------------------------------------------------------------------------

TEST(MediaQueryParserTest, CaseInsensitiveKeywords) {
  auto set = MediaQueryParser::ParseMediaQuerySet("NOT SCREEN AND (COLOR)");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  const auto& q = *set->Queries()[0];
  EXPECT_EQ(q.Restrictor(), MediaQueryRestrictor::kNot);
  EXPECT_EQ(q.MediaType(), "screen");
  ASSERT_NE(q.Condition(), nullptr);
}

TEST(MediaQueryParserTest, CaseInsensitiveFeatureName) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(Min-Width: 100px)");
  ASSERT_NE(set, nullptr);
  const auto& feature = static_cast<const MediaQueryFeatureExpNode&>(
                            *set->Queries()[0]->Condition())
                            .Feature();
  EXPECT_EQ(feature.Name(), "min-width");
}

// ---------------------------------------------------------------------------
// Operator completeness
// ---------------------------------------------------------------------------

TEST(MediaQueryParserTest, RangeOpLt) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(width < 800px)");
  ASSERT_NE(set, nullptr);
  const auto& feature = static_cast<const MediaQueryFeatureExpNode&>(
                            *set->Queries()[0]->Condition())
                            .Feature();
  EXPECT_EQ(feature.LeftOperator(), MediaFeatureOperator::kLt);
  EXPECT_DOUBLE_EQ(feature.LeftValue().Numeric(), 800.0);
}

TEST(MediaQueryParserTest, RangeOpLe) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(width <= 800px)");
  ASSERT_NE(set, nullptr);
  const auto& feature = static_cast<const MediaQueryFeatureExpNode&>(
                            *set->Queries()[0]->Condition())
                            .Feature();
  EXPECT_EQ(feature.LeftOperator(), MediaFeatureOperator::kLe);
}

TEST(MediaQueryParserTest, RangeOpGt) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(width > 400px)");
  ASSERT_NE(set, nullptr);
  const auto& feature = static_cast<const MediaQueryFeatureExpNode&>(
                            *set->Queries()[0]->Condition())
                            .Feature();
  EXPECT_EQ(feature.LeftOperator(), MediaFeatureOperator::kGt);
  EXPECT_DOUBLE_EQ(feature.LeftValue().Numeric(), 400.0);
}

TEST(MediaQueryParserTest, RangeOpEq) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(width = 600px)");
  ASSERT_NE(set, nullptr);
  const auto& feature = static_cast<const MediaQueryFeatureExpNode&>(
                            *set->Queries()[0]->Condition())
                            .Feature();
  EXPECT_EQ(feature.LeftOperator(), MediaFeatureOperator::kEq);
  EXPECT_DOUBLE_EQ(feature.LeftValue().Numeric(), 600.0);
}

TEST(MediaQueryParserTest, DoubleEqualsInvalid) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(width == 100px)");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(IsNotAllSet(*set));
}

// ---------------------------------------------------------------------------
// Range form B: leading value with single operator (value op name)
// ---------------------------------------------------------------------------

TEST(MediaQueryParserTest, RangeFormBLeadingValue) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(600px <= width)");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  const auto& feature = static_cast<const MediaQueryFeatureExpNode&>(
                            *set->Queries()[0]->Condition())
                            .Feature();
  EXPECT_EQ(feature.Name(), "width");
  EXPECT_EQ(feature.LeftOperator(), MediaFeatureOperator::kGe);
  EXPECT_DOUBLE_EQ(feature.LeftValue().Numeric(), 600.0);
  EXPECT_FALSE(feature.HasRightBound());
}

TEST(MediaQueryParserTest, RangeFormBLeadingValueGt) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(1024px > width)");
  ASSERT_NE(set, nullptr);
  const auto& feature = static_cast<const MediaQueryFeatureExpNode&>(
                            *set->Queries()[0]->Condition())
                            .Feature();
  EXPECT_EQ(feature.Name(), "width");
  EXPECT_EQ(feature.LeftOperator(), MediaFeatureOperator::kLt);
  EXPECT_DOUBLE_EQ(feature.LeftValue().Numeric(), 1024.0);
  EXPECT_FALSE(feature.HasRightBound());
}

// Range form B must reject identifier as the leading value. `(foo < bar)`
// is ambiguous with (and looks like) Case A but uses an ident on the left of
// the operator, which is not allowed by the grammar.
TEST(MediaQueryParserTest, RangeFormBRejectsIdentLeadingValue) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(foo < bar)");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(IsNotAllSet(*set));
}

TEST(MediaQueryParserTest, DoubleRangeMixedDirectionInvalid) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(100px < width > 500px)");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(IsNotAllSet(*set));
}

TEST(MediaQueryParserTest, DoubleRangeEqChainInvalid) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(100px = width < 200px)");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(IsNotAllSet(*set));
}

// ---------------------------------------------------------------------------
// Syntax constraint: mixing and/or is invalid
// ---------------------------------------------------------------------------

TEST(MediaQueryParserTest, MixingAndOrFails) {
  auto set =
      MediaQueryParser::ParseMediaQuerySet("(a: 1) and (b: 2) or (c: 3)");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(IsNotAllSet(*set));
}

TEST(MediaQueryParserTest, ConditionWithoutOrConstraint) {
  auto set =
      MediaQueryParser::ParseMediaQuerySet("screen and (a: 1) or (b: 2)");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(IsNotAllSet(*set));
}

TEST(MediaQueryParserTest, MediaTypeAndRangeOrMixed) {
  auto set = MediaQueryParser::ParseMediaQuerySet(
      "screen and (width > 100px) or (color)");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(IsNotAllSet(*set));
}

// ---------------------------------------------------------------------------
// only <type> and <condition>
// ---------------------------------------------------------------------------

TEST(MediaQueryParserTest, OnlyMediaTypeAndCondition) {
  auto set = MediaQueryParser::ParseMediaQuerySet("only screen and (color)");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  const auto& q = *set->Queries()[0];
  EXPECT_EQ(q.Restrictor(), MediaQueryRestrictor::kOnly);
  EXPECT_EQ(q.MediaType(), "screen");
  ASSERT_NE(q.Condition(), nullptr);
  EXPECT_EQ(q.Condition()->GetType(), MediaQueryExpNode::Type::kFeature);
}

// ---------------------------------------------------------------------------
// Nested not inside parens: (not (hover))
// ---------------------------------------------------------------------------

TEST(MediaQueryParserTest, NestedNotInParens) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(not (hover))");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  const auto& cond = *set->Queries()[0]->Condition();
  EXPECT_EQ(cond.GetType(), MediaQueryExpNode::Type::kNested);
  const auto& nested = static_cast<const MediaQueryNestedExpNode&>(cond);
  ASSERT_NE(nested.Inner(), nullptr);
  EXPECT_EQ(nested.Inner()->GetType(), MediaQueryExpNode::Type::kNot);
}

// ---------------------------------------------------------------------------
// Additional value types: percentage, em, resolution, unknown unit
// ---------------------------------------------------------------------------

TEST(MediaQueryParserTest, FeatureWithPercentValue) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(width: 50%)");
  ASSERT_NE(set, nullptr);
  const auto& feature = static_cast<const MediaQueryFeatureExpNode&>(
                            *set->Queries()[0]->Condition())
                            .Feature();
  EXPECT_EQ(feature.LeftValue().Type(), MediaFeatureType::kDimension);
  EXPECT_EQ(feature.LeftValue().Unit(), MediaFeatureUnit::kPercent);
  EXPECT_DOUBLE_EQ(feature.LeftValue().Numeric(), 50.0);
}

TEST(MediaQueryParserTest, FeatureWithEmUnit) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(min-width: 1.5em)");
  ASSERT_NE(set, nullptr);
  const auto& feature = static_cast<const MediaQueryFeatureExpNode&>(
                            *set->Queries()[0]->Condition())
                            .Feature();
  EXPECT_EQ(feature.LeftValue().Type(), MediaFeatureType::kDimension);
  EXPECT_EQ(feature.LeftValue().Unit(), MediaFeatureUnit::kEm);
  EXPECT_DOUBLE_EQ(feature.LeftValue().Numeric(), 1.5);
}

TEST(MediaQueryParserTest, FeatureWithResolutionDpi) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(resolution: 96dpi)");
  ASSERT_NE(set, nullptr);
  const auto& feature = static_cast<const MediaQueryFeatureExpNode&>(
                            *set->Queries()[0]->Condition())
                            .Feature();
  EXPECT_EQ(feature.LeftValue().Type(), MediaFeatureType::kDimension);
  EXPECT_EQ(feature.LeftValue().Unit(), MediaFeatureUnit::kDpi);
  EXPECT_DOUBLE_EQ(feature.LeftValue().Numeric(), 96.0);
}

TEST(MediaQueryParserTest, FeatureWithResolutionDppx) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(resolution: 2dppx)");
  ASSERT_NE(set, nullptr);
  const auto& feature = static_cast<const MediaQueryFeatureExpNode&>(
                            *set->Queries()[0]->Condition())
                            .Feature();
  EXPECT_EQ(feature.LeftValue().Type(), MediaFeatureType::kDimension);
  EXPECT_EQ(feature.LeftValue().Unit(), MediaFeatureUnit::kDppx);
  EXPECT_DOUBLE_EQ(feature.LeftValue().Numeric(), 2.0);
}

TEST(MediaQueryParserTest, FeatureWithUnknownUnitFallback) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(width: 10foo)");
  ASSERT_NE(set, nullptr);
  const auto& feature = static_cast<const MediaQueryFeatureExpNode&>(
                            *set->Queries()[0]->Condition())
                            .Feature();
  EXPECT_FALSE(feature.LeftValue().IsValid());
  EXPECT_EQ(feature.LeftValue().Type(), MediaFeatureType::kInvalid);
}

// ---------------------------------------------------------------------------
// Error recovery / invalid inputs
// ---------------------------------------------------------------------------

TEST(MediaQueryParserTest, EmptyParensInvalid) {
  auto set = MediaQueryParser::ParseMediaQuerySet("()");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(IsNotAllSet(*set));
}

TEST(MediaQueryParserTest, MissingValueAfterColon) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(min-width:)");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(IsNotAllSet(*set));
}

TEST(MediaQueryParserTest, UnclosedParen) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(width");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  const auto& feature = static_cast<const MediaQueryFeatureExpNode&>(
                            *set->Queries()[0]->Condition())
                            .Feature();
  EXPECT_EQ(feature.Name(), "width");
  EXPECT_TRUE(feature.IsBoolean());
}

TEST(MediaQueryParserTest, ExtraTokensInsideParens) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(width: 100px foo)");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(IsNotAllSet(*set));
}

TEST(MediaQueryParserTest, JustComma) {
  auto set = MediaQueryParser::ParseMediaQuerySet(",");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(IsNotAllSet(*set));
}

TEST(MediaQueryParserTest, RandomGarbage) {
  auto set = MediaQueryParser::ParseMediaQuerySet("!@#$%^&");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(IsNotAllSet(*set));
}

TEST(MediaQueryParserTest, MissingMediaTypeAfterNot) {
  auto set = MediaQueryParser::ParseMediaQuerySet("not");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(IsNotAllSet(*set));
}

TEST(MediaQueryParserTest, MissingMediaTypeAfterOnly) {
  auto set = MediaQueryParser::ParseMediaQuerySet("only");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(IsNotAllSet(*set));
}

TEST(MediaQueryParserTest, ReservedKeywordAndAsMediaType) {
  auto set = MediaQueryParser::ParseMediaQuerySet("and and (color)");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(IsNotAllSet(*set));
}

TEST(MediaQueryParserTest, ReservedKeywordOrAsMediaType) {
  auto set = MediaQueryParser::ParseMediaQuerySet("or");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(IsNotAllSet(*set));
}

TEST(MediaQueryParserTest, InvalidRecoveryInList) {
  auto set = MediaQueryParser::ParseMediaQuerySet(
      "(width: 100px bad), screen, (invalid!), (hover)");
  ASSERT_NE(set, nullptr);
  EXPECT_EQ(set->Queries().size(), 2u);
  EXPECT_EQ(set->Queries()[0]->MediaType(), "screen");
  ASSERT_NE(set->Queries()[1]->Condition(), nullptr);
}

TEST(MediaQueryParserTest, TrailingGarbageDoesNotDiscardSubsequentQueries) {
  auto set =
      MediaQueryParser::ParseMediaQuerySet("screen and (color) foo, (hover)");
  ASSERT_NE(set, nullptr);
  EXPECT_EQ(set->Queries().size(), 1u);
  ASSERT_NE(set->Queries()[0]->Condition(), nullptr);
  EXPECT_EQ(set->Queries()[0]->Condition()->Serialize(), "(hover)");
}

// ---------------------------------------------------------------------------
// Ratio ambiguity probes (review issue #6)
// ---------------------------------------------------------------------------

TEST(MediaQueryParserTest, AspectRatioPlainNumber) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(aspect-ratio: 16)");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  const auto& feature = static_cast<const MediaQueryFeatureExpNode&>(
                            *set->Queries()[0]->Condition())
                            .Feature();
  EXPECT_EQ(feature.Name(), "aspect-ratio");
  EXPECT_EQ(feature.LeftValue().Type(), MediaFeatureType::kNumber);
  EXPECT_DOUBLE_EQ(feature.LeftValue().Numeric(), 16.0);
}

TEST(MediaQueryParserTest, AspectRatioWithInvalidDenominator) {
  // "16 / landscape" — the `/` triggers ratio detection but denominator is
  // not a number. Current behavior: the whole query is rejected.
  auto set =
      MediaQueryParser::ParseMediaQuerySet("(aspect-ratio: 16 / landscape)");
  ASSERT_NE(set, nullptr);
  // This currently produces an empty set (query rejected).
  EXPECT_TRUE(IsNotAllSet(*set));
}

TEST(MediaQueryParserTest, AspectRatioValidRatio) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(aspect-ratio: 16 / 9)");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  const auto& feature = static_cast<const MediaQueryFeatureExpNode&>(
                            *set->Queries()[0]->Condition())
                            .Feature();
  EXPECT_EQ(feature.Name(), "aspect-ratio");
  EXPECT_EQ(feature.LeftValue().Type(), MediaFeatureType::kRatio);
  EXPECT_DOUBLE_EQ(feature.LeftValue().Numeric(), 16.0);
  EXPECT_DOUBLE_EQ(feature.LeftValue().Denominator(), 9.0);
}

TEST(MediaQueryParserTest, AspectRatioTrailingTokenAfterRatio) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(aspect-ratio: 16/9 extra)");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(IsNotAllSet(*set));
}

TEST(MediaQueryParserTest, AspectRatioWithNegativeDenominator) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(aspect-ratio: 16 / -2)");
  ASSERT_NE(set, nullptr);
  EXPECT_TRUE(IsNotAllSet(*set));
}

TEST(MediaQueryParserTest, FeatureWithVwUnit) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(min-width: 50vw)");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  const auto& feature = static_cast<const MediaQueryFeatureExpNode&>(
                            *set->Queries()[0]->Condition())
                            .Feature();
  EXPECT_EQ(feature.LeftValue().Type(), MediaFeatureType::kDimension);
  EXPECT_EQ(feature.LeftValue().Unit(), MediaFeatureUnit::kViewportWidth);
  EXPECT_DOUBLE_EQ(feature.LeftValue().Numeric(), 50.0);
}

TEST(MediaQueryParserTest, FeatureWithVhUnit) {
  auto set = MediaQueryParser::ParseMediaQuerySet("(min-height: 80vh)");
  ASSERT_NE(set, nullptr);
  ASSERT_EQ(set->Queries().size(), 1u);
  const auto& feature = static_cast<const MediaQueryFeatureExpNode&>(
                            *set->Queries()[0]->Condition())
                            .Feature();
  EXPECT_EQ(feature.LeftValue().Type(), MediaFeatureType::kDimension);
  EXPECT_EQ(feature.LeftValue().Unit(), MediaFeatureUnit::kViewportHeight);
  EXPECT_DOUBLE_EQ(feature.LeftValue().Numeric(), 80.0);
}

}  // namespace css
}  // namespace lynx
