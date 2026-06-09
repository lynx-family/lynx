// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/supports/supports_evaluator.h"

#include <utility>

#include "base/include/fml/memory/ref_counted.h"
#include "core/renderer/css/ng/supports/supports_condition.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace css {

namespace {

uint32_t Version(uint16_t major, uint16_t minor) {
  return SupportsEngineVersionNode::PackVersion(major, minor);
}

fml::RefPtr<const SupportsConditionNode> EngineVersion(uint16_t begin_major,
                                                       uint16_t begin_minor,
                                                       uint16_t end_major,
                                                       uint16_t end_minor) {
  return fml::MakeRefCounted<SupportsEngineVersionNode>(
      Version(begin_major, begin_minor), Version(end_major, end_minor));
}

fml::RefPtr<const SupportsConditionNode> Not(
    fml::RefPtr<const SupportsConditionNode> operand) {
  return fml::MakeRefCounted<SupportsUnaryNode>(
      SupportsConditionNode::Type::kNot, std::move(operand));
}

fml::RefPtr<const SupportsConditionNode> And(
    fml::RefPtr<const SupportsConditionNode> left,
    fml::RefPtr<const SupportsConditionNode> right) {
  return fml::MakeRefCounted<SupportsBinaryNode>(
      SupportsConditionNode::Type::kAnd, std::move(left), std::move(right));
}

fml::RefPtr<const SupportsConditionNode> Or(
    fml::RefPtr<const SupportsConditionNode> left,
    fml::RefPtr<const SupportsConditionNode> right) {
  return fml::MakeRefCounted<SupportsBinaryNode>(
      SupportsConditionNode::Type::kOr, std::move(left), std::move(right));
}

}  // namespace

TEST(SupportsEvaluatorTest, EngineVersionInRange) {
  SupportsEvaluator evaluator(Version(4, 0), {});
  EXPECT_TRUE(evaluator.Eval(EngineVersion(3, 0, 5, 0).get()));
}

TEST(SupportsEvaluatorTest, EngineVersionBeginInclusive) {
  SupportsEvaluator evaluator(Version(4, 0), {});
  EXPECT_TRUE(evaluator.Eval(EngineVersion(4, 0, 5, 0).get()));
}

TEST(SupportsEvaluatorTest, EngineVersionEndExclusive) {
  SupportsEvaluator evaluator(Version(4, 0), {});
  EXPECT_FALSE(evaluator.Eval(EngineVersion(3, 0, 4, 0).get()));
}

TEST(SupportsEvaluatorTest, EngineVersionMaxEndMatchesCurrentVersion) {
  SupportsEvaluator evaluator(Version(4, 0), {});
  auto node = fml::MakeRefCounted<SupportsEngineVersionNode>(
      Version(4, 0), SupportsEngineVersionNode::kMaxVersion);
  EXPECT_TRUE(evaluator.Eval(node.get()));
}

TEST(SupportsEvaluatorTest, NotInvertsEngineVersion) {
  SupportsEvaluator evaluator(Version(4, 0), {});
  EXPECT_TRUE(evaluator.Eval(Not(EngineVersion(5, 0, 6, 0)).get()));
}

TEST(SupportsEvaluatorTest, AndRequiresBothSides) {
  SupportsEvaluator evaluator(Version(4, 0), {});
  EXPECT_TRUE(evaluator.Eval(
      And(EngineVersion(3, 0, 5, 0), EngineVersion(4, 0, 4, 1)).get()));
  EXPECT_FALSE(evaluator.Eval(
      And(EngineVersion(3, 0, 5, 0), EngineVersion(5, 0, 6, 0)).get()));
}

TEST(SupportsEvaluatorTest, OrMatchesEitherSide) {
  SupportsEvaluator evaluator(Version(4, 0), {});
  EXPECT_TRUE(evaluator.Eval(
      Or(EngineVersion(5, 0, 6, 0), EngineVersion(3, 0, 5, 0)).get()));
  EXPECT_FALSE(evaluator.Eval(
      Or(EngineVersion(1, 0, 2, 0), EngineVersion(5, 0, 6, 0)).get()));
}

TEST(SupportsEvaluatorTest, DeclarationUsesUnitHandlerCapability) {
  SupportsEvaluator evaluator(Version(4, 0), {});
  EXPECT_TRUE(evaluator.Eval(
      fml::MakeRefCounted<SupportsDeclNode>("display", "flex", false).get()));
  EXPECT_TRUE(evaluator.Eval(
      fml::MakeRefCounted<SupportsDeclNode>("color", "red", true).get()));
  EXPECT_FALSE(evaluator.Eval(
      fml::MakeRefCounted<SupportsDeclNode>("display", "unknown", false)
          .get()));
  EXPECT_FALSE(evaluator.Eval(
      fml::MakeRefCounted<SupportsDeclNode>("unknown-property", "flex", false)
          .get()));
}

TEST(SupportsEvaluatorTest, UnsupportedNodesEvaluateFalse) {
  SupportsEvaluator evaluator(Version(4, 0), {});
  EXPECT_FALSE(evaluator.Eval(
      fml::MakeRefCounted<SupportsFunctionNode>("selector", ".foo").get()));
  EXPECT_FALSE(evaluator.Eval(
      fml::MakeRefCounted<SupportsGeneralEnclosedNode>("unknown").get()));
}

TEST(SupportsEvaluatorTest, NullNodeReturnsFalse) {
  SupportsEvaluator evaluator(Version(4, 0), {});
  EXPECT_FALSE(
      evaluator.Eval(static_cast<const SupportsConditionNode*>(nullptr)));
}

}  // namespace css
}  // namespace lynx
