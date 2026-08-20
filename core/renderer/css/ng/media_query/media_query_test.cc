// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/media_query/media_query.h"

#include <cmath>
#include <string>

#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "core/renderer/css/ng/media_query/media_feature.h"
#include "core/renderer/css/ng/media_query/media_query_exp.h"
#include "core/renderer/css/ng/media_query/media_query_set.h"
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
// MediaFeatureValue
// ===========================================================================

TEST(MediaFeatureValueTest, DefaultIsInvalid) {
  MediaFeatureValue v;
  EXPECT_FALSE(v.IsValid());
  EXPECT_EQ(v.Type(), MediaFeatureType::kInvalid);
}

TEST(MediaFeatureValueTest, NumberFactory) {
  auto v = MediaFeatureValue::Number(3.14);
  EXPECT_TRUE(v.IsValid());
  EXPECT_EQ(v.Type(), MediaFeatureType::kNumber);
  EXPECT_DOUBLE_EQ(v.Numeric(), 3.14);
}

TEST(MediaFeatureValueTest, PixelsFactory) {
  auto v = MediaFeatureValue::Dimension(720.0, MediaFeatureUnit::kPixels);
  EXPECT_TRUE(v.IsValid());
  EXPECT_EQ(v.Type(), MediaFeatureType::kDimension);
  EXPECT_EQ(v.Unit(), MediaFeatureUnit::kPixels);
  EXPECT_DOUBLE_EQ(v.Numeric(), 720.0);
}

TEST(MediaFeatureValueTest, RatioFactory) {
  auto v = MediaFeatureValue::Ratio(16.0, 9.0);
  EXPECT_TRUE(v.IsValid());
  EXPECT_EQ(v.Type(), MediaFeatureType::kRatio);
  EXPECT_DOUBLE_EQ(v.Numeric(), 16.0);
  EXPECT_DOUBLE_EQ(v.Denominator(), 9.0);
}

TEST(MediaFeatureValueTest, IdentFactory) {
  auto v = MediaFeatureValue::Ident("landscape");
  EXPECT_TRUE(v.IsValid());
  EXPECT_EQ(v.Type(), MediaFeatureType::kIdent);
  EXPECT_EQ(v.Text(), "landscape");
}

TEST(MediaFeatureValueTest, ResolutionFactory) {
  auto v = MediaFeatureValue::Dimension(96.0, MediaFeatureUnit::kDpi);
  EXPECT_TRUE(v.IsValid());
  EXPECT_EQ(v.Type(), MediaFeatureType::kDimension);
  EXPECT_EQ(v.Unit(), MediaFeatureUnit::kDpi);
  EXPECT_TRUE(v.IsResolution());
}

TEST(MediaFeatureValueTest, LengthFactory) {
  auto v = MediaFeatureValue::Dimension(1.5, MediaFeatureUnit::kRem);
  EXPECT_TRUE(v.IsValid());
  EXPECT_EQ(v.Type(), MediaFeatureType::kDimension);
  EXPECT_EQ(v.Unit(), MediaFeatureUnit::kRem);
  EXPECT_DOUBLE_EQ(v.Numeric(), 1.5);
  EXPECT_TRUE(v.IsLength());
}

TEST(MediaFeatureValueTest, ToFromLepusRoundtrip) {
  auto original =
      MediaFeatureValue::Dimension(360.0, MediaFeatureUnit::kPixels);
  auto lepus = original.ToLepus();
  auto restored = MediaFeatureValue::FromLepus(lepus);
  EXPECT_EQ(restored.Type(), MediaFeatureType::kDimension);
  EXPECT_EQ(restored.Unit(), MediaFeatureUnit::kPixels);
  EXPECT_DOUBLE_EQ(restored.Numeric(), 360.0);
}

TEST(MediaFeatureValueTest, ToFromLepusRoundtripRatio) {
  auto original = MediaFeatureValue::Ratio(4.0, 3.0);
  auto lepus = original.ToLepus();
  auto restored = MediaFeatureValue::FromLepus(lepus);
  EXPECT_EQ(restored.Type(), MediaFeatureType::kRatio);
  EXPECT_DOUBLE_EQ(restored.Numeric(), 4.0);
  EXPECT_DOUBLE_EQ(restored.Denominator(), 3.0);
}

TEST(MediaFeatureValueTest, ToFromLepusRoundtripIdent) {
  auto original = MediaFeatureValue::Ident("portrait");
  auto lepus = original.ToLepus();
  auto restored = MediaFeatureValue::FromLepus(lepus);
  EXPECT_EQ(restored.Type(), MediaFeatureType::kIdent);
  EXPECT_EQ(restored.Text(), "portrait");
}

TEST(MediaFeatureValueTest, FromLepusInvalidInput) {
  lepus_value bad(42);
  auto result = MediaFeatureValue::FromLepus(bad);
  EXPECT_FALSE(result.IsValid());
}

// ===========================================================================
// MediaFeature
// ===========================================================================

TEST(MediaFeatureTest, BooleanFeature) {
  auto f = MakeFeature("hover", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Boolean());
  EXPECT_TRUE(f.IsBoolean());
  EXPECT_EQ(f.Name(), "hover");
  EXPECT_FALSE(f.HasRightBound());
}

TEST(MediaFeatureTest, PlainFeature) {
  auto f = MakeFeature(
      "width", MediaFeatureOperator::kNone,
      MediaFeatureValue::Dimension(600.0, MediaFeatureUnit::kPixels));
  EXPECT_FALSE(f.IsBoolean());
  EXPECT_EQ(f.Name(), "width");
  EXPECT_EQ(f.LeftOperator(), MediaFeatureOperator::kNone);
  EXPECT_DOUBLE_EQ(f.LeftValue().Numeric(), 600.0);
  EXPECT_FALSE(f.HasRightBound());
}

TEST(MediaFeatureTest, RangeFeature) {
  auto f = MakeFeature(
      "width", MediaFeatureOperator::kGe,
      MediaFeatureValue::Dimension(100.0, MediaFeatureUnit::kPixels));
  f.SetRightBound(
      MediaFeatureOperator::kLe,
      MediaFeatureValue::Dimension(500.0, MediaFeatureUnit::kPixels));
  EXPECT_FALSE(f.IsBoolean());
  EXPECT_TRUE(f.HasRightBound());
  EXPECT_EQ(f.LeftOperator(), MediaFeatureOperator::kGe);
  EXPECT_DOUBLE_EQ(f.LeftValue().Numeric(), 100.0);
  EXPECT_EQ(f.RightOperator(), MediaFeatureOperator::kLe);
  EXPECT_DOUBLE_EQ(f.RightValue().Numeric(), 500.0);
}

TEST(MediaFeatureTest, ToFromLepusRoundtripSimple) {
  auto original = MakeFeature(
      "height", MediaFeatureOperator::kLt,
      MediaFeatureValue::Dimension(1024.0, MediaFeatureUnit::kPixels));
  auto lepus = original.ToLepus();
  auto restored = MediaFeature::FromLepus(lepus);
  EXPECT_EQ(restored.Name(), "height");
  EXPECT_EQ(restored.LeftOperator(), MediaFeatureOperator::kLt);
  EXPECT_DOUBLE_EQ(restored.LeftValue().Numeric(), 1024.0);
  EXPECT_FALSE(restored.HasRightBound());
}

TEST(MediaFeatureTest, ToFromLepusRoundtripRange) {
  auto original = MakeFeature(
      "width", MediaFeatureOperator::kGt,
      MediaFeatureValue::Dimension(200.0, MediaFeatureUnit::kPixels));
  original.SetRightBound(
      MediaFeatureOperator::kLt,
      MediaFeatureValue::Dimension(800.0, MediaFeatureUnit::kPixels));
  auto lepus = original.ToLepus();
  auto restored = MediaFeature::FromLepus(lepus);
  EXPECT_EQ(restored.Name(), "width");
  EXPECT_TRUE(restored.HasRightBound());
  EXPECT_EQ(restored.LeftOperator(), MediaFeatureOperator::kGt);
  EXPECT_DOUBLE_EQ(restored.LeftValue().Numeric(), 200.0);
  EXPECT_EQ(restored.RightOperator(), MediaFeatureOperator::kLt);
  EXPECT_DOUBLE_EQ(restored.RightValue().Numeric(), 800.0);
}

TEST(MediaFeatureTest, FromLepusInvalidInput) {
  lepus_value bad(false);
  auto result = MediaFeature::FromLepus(bad);
  EXPECT_FALSE(result.IsBoolean());
  EXPECT_EQ(result.Name(), "");
  EXPECT_EQ(result.LeftOperator(), MediaFeatureOperator::kNone);
}

TEST(MediaFeatureTest, FromLepusTruncatedRightBound) {
  auto arr = lepus::CArray::Create();
  arr->emplace_back(static_cast<uint32_t>(MediaFeatureId::kWidth));
  arr->emplace_back(std::string(""));
  arr->emplace_back(static_cast<uint32_t>(MediaFeatureOperator::kGt));
  arr->emplace_back(
      MediaFeatureValue::Dimension(200.0, MediaFeatureUnit::kPixels).ToLepus());
  arr->emplace_back(true);
  lepus_value truncated(std::move(arr));
  auto result = MediaFeature::FromLepus(truncated);
  EXPECT_EQ(result.Name(), "");
  EXPECT_EQ(result.LeftOperator(), MediaFeatureOperator::kNone);
}

// ===========================================================================
// MediaQueryExpNode hierarchy
// ===========================================================================

TEST(MediaQueryExpNodeTest, FeatureNodeSerialize) {
  auto f = MakeFeature(
      "width", MediaFeatureOperator::kGe,
      MediaFeatureValue::Dimension(600.0, MediaFeatureUnit::kPixels));
  auto node = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f));
  EXPECT_EQ(node->GetType(), MediaQueryExpNode::Type::kFeature);
  EXPECT_EQ(node->Serialize(), "(width >= 600px)");
}

TEST(MediaQueryExpNodeTest, FeatureNodeBooleanSerialize) {
  auto f = MakeFeature("hover", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Boolean());
  auto node = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f));
  EXPECT_EQ(node->Serialize(), "(hover)");
}

TEST(MediaQueryExpNodeTest, FeatureNodePlainSerialize) {
  auto f = MakeFeature("orientation", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Ident("portrait"));
  auto node = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f));
  EXPECT_EQ(node->Serialize(), "(orientation: portrait)");
}

TEST(MediaQueryExpNodeTest, FeatureNodeRangeSerialize) {
  auto f = MakeFeature(
      "width", MediaFeatureOperator::kGe,
      MediaFeatureValue::Dimension(100.0, MediaFeatureUnit::kPixels));
  f.SetRightBound(
      MediaFeatureOperator::kLt,
      MediaFeatureValue::Dimension(500.0, MediaFeatureUnit::kPixels));
  auto node = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f));
  EXPECT_EQ(node->Serialize(), "(100px <= width < 500px)");
}

TEST(MediaQueryExpNodeTest, NotNodeSerialize) {
  auto f = MakeFeature("hover", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Boolean());
  auto inner = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f));
  auto not_node = fml::MakeRefCounted<MediaQueryNotExpNode>(std::move(inner));
  EXPECT_EQ(not_node->GetType(), MediaQueryExpNode::Type::kNot);
  EXPECT_EQ(not_node->Serialize(), "not (hover)");
}

TEST(MediaQueryExpNodeTest, NestedNodeSerialize) {
  auto f = MakeFeature(
      "width", MediaFeatureOperator::kGe,
      MediaFeatureValue::Dimension(600.0, MediaFeatureUnit::kPixels));
  auto inner = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f));
  auto nested = fml::MakeRefCounted<MediaQueryNestedExpNode>(std::move(inner));
  EXPECT_EQ(nested->GetType(), MediaQueryExpNode::Type::kNested);
  EXPECT_EQ(nested->Serialize(), "((width >= 600px))");
}

TEST(MediaQueryExpNodeTest, AndNodeSerialize) {
  auto f1 = MakeFeature(
      "width", MediaFeatureOperator::kGe,
      MediaFeatureValue::Dimension(600.0, MediaFeatureUnit::kPixels));
  auto f2 = MakeFeature(
      "height", MediaFeatureOperator::kLe,
      MediaFeatureValue::Dimension(800.0, MediaFeatureUnit::kPixels));
  auto left = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f1));
  auto right = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f2));
  auto and_node = fml::MakeRefCounted<MediaQueryAndExpNode>(std::move(left),
                                                            std::move(right));
  EXPECT_EQ(and_node->GetType(), MediaQueryExpNode::Type::kAnd);
  EXPECT_EQ(and_node->Serialize(), "(width >= 600px) and (height <= 800px)");
}

TEST(MediaQueryExpNodeTest, OrNodeSerialize) {
  auto f1 = MakeFeature("orientation", MediaFeatureOperator::kNone,
                        MediaFeatureValue::Ident("portrait"));
  auto f2 = MakeFeature("orientation", MediaFeatureOperator::kNone,
                        MediaFeatureValue::Ident("landscape"));
  auto left = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f1));
  auto right = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f2));
  auto or_node = fml::MakeRefCounted<MediaQueryOrExpNode>(std::move(left),
                                                          std::move(right));
  EXPECT_EQ(or_node->GetType(), MediaQueryExpNode::Type::kOr);
  EXPECT_EQ(or_node->Serialize(),
            "(orientation: portrait) or (orientation: landscape)");
}

TEST(MediaQueryExpNodeTest, FeatureToFromLepusRoundtrip) {
  auto f = MakeFeature(
      "width", MediaFeatureOperator::kGe,
      MediaFeatureValue::Dimension(600.0, MediaFeatureUnit::kPixels));
  auto node = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f));
  auto lepus = node->ToLepus();
  auto restored = MediaQueryExpNode::FromLepus(lepus);
  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->GetType(), MediaQueryExpNode::Type::kFeature);
  EXPECT_EQ(restored->Serialize(), "(width >= 600px)");
}

TEST(MediaQueryExpNodeTest, NotToFromLepusRoundtrip) {
  auto f = MakeFeature("hover", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Boolean());
  auto inner = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f));
  auto not_node = fml::MakeRefCounted<MediaQueryNotExpNode>(std::move(inner));
  auto lepus = not_node->ToLepus();
  auto restored = MediaQueryExpNode::FromLepus(lepus);
  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->GetType(), MediaQueryExpNode::Type::kNot);
  EXPECT_EQ(restored->Serialize(), "not (hover)");
}

TEST(MediaQueryExpNodeTest, NestedToFromLepusRoundtrip) {
  auto f = MakeFeature(
      "height", MediaFeatureOperator::kLt,
      MediaFeatureValue::Dimension(1000.0, MediaFeatureUnit::kPixels));
  auto inner = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f));
  auto nested = fml::MakeRefCounted<MediaQueryNestedExpNode>(std::move(inner));
  auto lepus = nested->ToLepus();
  auto restored = MediaQueryExpNode::FromLepus(lepus);
  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->GetType(), MediaQueryExpNode::Type::kNested);
  EXPECT_EQ(restored->Serialize(), "((height < 1000px))");
}

TEST(MediaQueryExpNodeTest, AndToFromLepusRoundtrip) {
  auto f1 = MakeFeature(
      "width", MediaFeatureOperator::kGe,
      MediaFeatureValue::Dimension(600.0, MediaFeatureUnit::kPixels));
  auto f2 = MakeFeature(
      "width", MediaFeatureOperator::kLe,
      MediaFeatureValue::Dimension(1200.0, MediaFeatureUnit::kPixels));
  auto left = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f1));
  auto right = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f2));
  auto and_node = fml::MakeRefCounted<MediaQueryAndExpNode>(std::move(left),
                                                            std::move(right));
  auto lepus = and_node->ToLepus();
  auto restored = MediaQueryExpNode::FromLepus(lepus);
  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->GetType(), MediaQueryExpNode::Type::kAnd);
  EXPECT_EQ(restored->Serialize(), "(width >= 600px) and (width <= 1200px)");
}

TEST(MediaQueryExpNodeTest, OrToFromLepusRoundtrip) {
  auto f1 = MakeFeature("hover", MediaFeatureOperator::kNone,
                        MediaFeatureValue::Boolean());
  auto f2 = MakeFeature("pointer", MediaFeatureOperator::kNone,
                        MediaFeatureValue::Boolean());
  auto left = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f1));
  auto right = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f2));
  auto or_node = fml::MakeRefCounted<MediaQueryOrExpNode>(std::move(left),
                                                          std::move(right));
  auto lepus = or_node->ToLepus();
  auto restored = MediaQueryExpNode::FromLepus(lepus);
  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->GetType(), MediaQueryExpNode::Type::kOr);
  EXPECT_EQ(restored->Serialize(), "(hover) or (pointer)");
}

TEST(MediaQueryExpNodeTest, FromLepusInvalidInput) {
  lepus_value bad(42);
  auto result = MediaQueryExpNode::FromLepus(bad);
  EXPECT_EQ(result, nullptr);
}

TEST(MediaQueryExpNodeTest, FromLepusNotWithNonArrayPayload) {
  auto arr = lepus::CArray::Create();
  arr->emplace_back(static_cast<uint32_t>(MediaQueryExpNode::Type::kNot));
  arr->emplace_back(false);
  lepus_value truncated(std::move(arr));
  auto result = MediaQueryExpNode::FromLepus(truncated);
  EXPECT_EQ(result, nullptr);
}

// ===========================================================================
// MediaQuery
// ===========================================================================

TEST(MediaQueryTest, BareConditionSerialize) {
  auto f = MakeFeature(
      "width", MediaFeatureOperator::kGe,
      MediaFeatureValue::Dimension(600.0, MediaFeatureUnit::kPixels));
  auto cond = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f));
  auto query = fml::MakeRefCounted<MediaQuery>(MediaQueryRestrictor::kNone,
                                               std::string(), std::move(cond));
  EXPECT_EQ(query->Serialize(), "(width >= 600px)");
}

TEST(MediaQueryTest, MediaTypeOnly) {
  auto query = fml::MakeRefCounted<MediaQuery>(MediaQueryRestrictor::kNone,
                                               "screen", nullptr);
  EXPECT_EQ(query->Serialize(), "screen");
}

TEST(MediaQueryTest, NotMediaType) {
  auto query = fml::MakeRefCounted<MediaQuery>(MediaQueryRestrictor::kNot,
                                               "print", nullptr);
  EXPECT_EQ(query->Serialize(), "not print");
}

TEST(MediaQueryTest, OnlyMediaTypeWithCondition) {
  auto f = MakeFeature("orientation", MediaFeatureOperator::kNone,
                       MediaFeatureValue::Ident("landscape"));
  auto cond = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f));
  auto query = fml::MakeRefCounted<MediaQuery>(MediaQueryRestrictor::kOnly,
                                               "screen", std::move(cond));
  EXPECT_EQ(query->Serialize(), "only screen and (orientation: landscape)");
}

TEST(MediaQueryTest, ToFromLepusRoundtrip) {
  auto f = MakeFeature(
      "width", MediaFeatureOperator::kGe,
      MediaFeatureValue::Dimension(600.0, MediaFeatureUnit::kPixels));
  auto cond = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f));
  auto original = fml::MakeRefCounted<MediaQuery>(MediaQueryRestrictor::kNot,
                                                  "print", std::move(cond));
  auto lepus = original->ToLepus();
  auto restored = MediaQuery::FromLepus(lepus);
  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->Restrictor(), MediaQueryRestrictor::kNot);
  EXPECT_EQ(restored->MediaType(), "print");
  ASSERT_NE(restored->Condition(), nullptr);
  EXPECT_EQ(restored->Condition()->Serialize(), "(width >= 600px)");
}

TEST(MediaQueryTest, ToFromLepusNoCondition) {
  auto original = fml::MakeRefCounted<MediaQuery>(MediaQueryRestrictor::kNone,
                                                  "screen", nullptr);
  auto lepus = original->ToLepus();
  auto restored = MediaQuery::FromLepus(lepus);
  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->MediaType(), "screen");
  EXPECT_EQ(restored->Condition(), nullptr);
}

TEST(MediaQueryTest, FromLepusInvalidInput) {
  lepus_value bad(false);
  auto result = MediaQuery::FromLepus(bad);
  EXPECT_EQ(result, nullptr);
}

TEST(MediaQueryTest, FromLepusTruncatedCondition) {
  auto arr = lepus::CArray::Create();
  arr->emplace_back(static_cast<uint32_t>(MediaQueryRestrictor::kNone));
  arr->emplace_back(std::string("screen"));
  arr->emplace_back(true);
  lepus_value truncated(std::move(arr));
  auto result = MediaQuery::FromLepus(truncated);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->Restrictor(), MediaQueryRestrictor::kNot);
  EXPECT_EQ(result->MediaType(), "all");
  EXPECT_EQ(result->Condition(), nullptr);
}

// ===========================================================================
// MediaQuerySet
// ===========================================================================

TEST(MediaQuerySetTest, EmptySetSerialization) {
  auto set = fml::MakeRefCounted<MediaQuerySet>();
  EXPECT_TRUE(set->IsEmpty());
  EXPECT_EQ(set->Serialize(), "");
}

TEST(MediaQuerySetTest, AppendAndSerialize) {
  auto set = fml::MakeRefCounted<MediaQuerySet>();
  auto q1 = fml::MakeRefCounted<MediaQuery>(MediaQueryRestrictor::kNone,
                                            "screen", nullptr);
  auto q2 = fml::MakeRefCounted<MediaQuery>(MediaQueryRestrictor::kNone,
                                            "print", nullptr);
  set->Append(std::move(q1));
  set->Append(std::move(q2));
  EXPECT_FALSE(set->IsEmpty());
  EXPECT_EQ(set->Queries().size(), 2u);
  EXPECT_EQ(set->Serialize(), "screen, print");
}

TEST(MediaQuerySetTest, ToFromLepusRoundtrip) {
  auto set = fml::MakeRefCounted<MediaQuerySet>();
  auto f = MakeFeature(
      "width", MediaFeatureOperator::kGe,
      MediaFeatureValue::Dimension(600.0, MediaFeatureUnit::kPixels));
  auto cond = fml::MakeRefCounted<MediaQueryFeatureExpNode>(std::move(f));
  auto q = fml::MakeRefCounted<MediaQuery>(MediaQueryRestrictor::kNone,
                                           "screen", std::move(cond));
  set->Append(std::move(q));

  auto lepus = set->ToLepus();
  auto restored = MediaQuerySet::FromLepus(lepus);
  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->Queries().size(), 1u);
  EXPECT_EQ(restored->Serialize(), "screen and (width >= 600px)");
}

TEST(MediaQuerySetTest, FromLepusInvalidInput) {
  lepus_value bad(42);
  auto result = MediaQuerySet::FromLepus(bad);
  EXPECT_EQ(result, nullptr);
}

}  // namespace css
}  // namespace lynx
