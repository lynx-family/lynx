// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/parser/supports_condition_parser.h"

#include <string>

#include "base/include/value/base_value.h"
#include "core/renderer/css/ng/supports/supports_condition.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace css {

namespace {

// Helper: parse and assert non-null
fml::RefPtr<const SupportsConditionNode> ParseOk(const std::string& text) {
  auto node = SupportsConditionParser::Parse(text);
  EXPECT_NE(node, nullptr) << "Failed to parse: " << text;
  return node;
}

// Helper: parse and assert null (invalid input)
void ParseFail(const std::string& text) {
  auto node = SupportsConditionParser::Parse(text);
  EXPECT_EQ(node, nullptr) << "Expected parse failure for: " << text;
}

}  // namespace

// ===========================================================================
// Empty / whitespace / invalid
// ===========================================================================

TEST(SupportsConditionParserTest, EmptyInput) { ParseFail(""); }

TEST(SupportsConditionParserTest, WhitespaceOnly) { ParseFail("   \t  "); }

// ===========================================================================
// Parse failure: boundary cases
// ===========================================================================

TEST(SupportsConditionParserTest, TrailingGarbage) {
  ParseFail("(display: flex) garbage");
}

TEST(SupportsConditionParserTest, NotWithoutParensOrFunction) {
  ParseFail("not display: flex");
}

TEST(SupportsConditionParserTest, DoubleNotFails) {
  // "not not (a: 1)" is invalid — not expects <supports-in-parens>
  ParseFail("not not (a: 1)");
}

// ===========================================================================
// Declaration: (property: value)
// ===========================================================================

TEST(SupportsConditionParserTest, SimpleDeclaration) {
  auto node = ParseOk("(display: flex)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kDeclaration);
  auto* decl = static_cast<const SupportsDeclNode*>(node.get());
  EXPECT_EQ(decl->Property(), "display");
  EXPECT_EQ(decl->Value(), "flex");
  EXPECT_FALSE(decl->Important());
}

TEST(SupportsConditionParserTest, DeclarationWithImportant) {
  auto node = ParseOk("(color: red !important)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kDeclaration);
  auto* decl = static_cast<const SupportsDeclNode*>(node.get());
  EXPECT_EQ(decl->Property(), "color");
  EXPECT_EQ(decl->Value(), "red");
  EXPECT_TRUE(decl->Important());
}

TEST(SupportsConditionParserTest, DeclarationWithComplexValue) {
  auto node = ParseOk("(background: linear-gradient(red, blue))");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kDeclaration);
  auto* decl = static_cast<const SupportsDeclNode*>(node.get());
  EXPECT_EQ(decl->Property(), "background");
  EXPECT_NE(decl->Value().find("linear-gradient"), std::string::npos);
}

TEST(SupportsConditionParserTest, DeclarationNoColon) {
  // "display flex" has no colon → general-enclosed
  auto node = ParseOk("(display flex)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kGeneralEnclosed);
}

TEST(SupportsConditionParserTest, DeclarationEmptyValue) {
  // "display:" with no value → general-enclosed
  auto node = ParseOk("(display:)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kGeneralEnclosed);
}

TEST(SupportsConditionParserTest, DeclarationCustomProperty) {
  auto node = ParseOk("(--my-var: 42px)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kDeclaration);
  auto* decl = static_cast<const SupportsDeclNode*>(node.get());
  EXPECT_EQ(decl->Property(), "--my-var");
  EXPECT_EQ(decl->Value(), "42px");
}

// ===========================================================================
// NOT
// ===========================================================================

TEST(SupportsConditionParserTest, Not) {
  auto node = ParseOk("not (display: grid)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kNot);
  auto* not_node = static_cast<const SupportsUnaryNode*>(node.get());
  ASSERT_NE(not_node->Operand(), nullptr);
  EXPECT_EQ(not_node->Operand()->GetType(),
            SupportsConditionNode::Type::kDeclaration);
}

// ===========================================================================
// AND
// ===========================================================================

TEST(SupportsConditionParserTest, And) {
  auto node = ParseOk("(display: flex) and (color: red)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kAnd);
  auto* and_node = static_cast<const SupportsBinaryNode*>(node.get());
  EXPECT_NE(and_node->Left(), nullptr);
  EXPECT_NE(and_node->Right(), nullptr);
}

TEST(SupportsConditionParserTest, MultipleAnd) {
  auto node = ParseOk("(a: 1) and (b: 2) and (c: 3)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kAnd);
  // Left-associative: ((a and b) and c)
  auto* outer = static_cast<const SupportsBinaryNode*>(node.get());
  EXPECT_EQ(outer->Left()->GetType(), SupportsConditionNode::Type::kAnd);
  EXPECT_EQ(outer->Right()->GetType(),
            SupportsConditionNode::Type::kDeclaration);
}

// ===========================================================================
// OR
// ===========================================================================

TEST(SupportsConditionParserTest, Or) {
  auto node = ParseOk("(display: flex) or (display: grid)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kOr);
}

TEST(SupportsConditionParserTest, MultipleOr) {
  auto node = ParseOk("(a: 1) or (b: 2) or (c: 3)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kOr);
  // Left-associative: ((a or b) or c)
  auto* outer = static_cast<const SupportsBinaryNode*>(node.get());
  EXPECT_EQ(outer->Left()->GetType(), SupportsConditionNode::Type::kOr);
  EXPECT_EQ(outer->Right()->GetType(),
            SupportsConditionNode::Type::kDeclaration);
}

// ===========================================================================
// Mixed and/or is invalid without parentheses
// ===========================================================================

TEST(SupportsConditionParserTest, MixedAndOrFails) {
  ParseFail("(a: 1) and (b: 2) or (c: 3)");
}

// ===========================================================================
// Nested conditions
// ===========================================================================

TEST(SupportsConditionParserTest, NestedCondition) {
  auto node = ParseOk("((display: flex) or (display: grid))");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kNested);
  auto* nested = static_cast<const SupportsUnaryNode*>(node.get());
  EXPECT_EQ(nested->Operand()->GetType(), SupportsConditionNode::Type::kOr);
}

TEST(SupportsConditionParserTest, NotWithNested) {
  auto node = ParseOk("not ((display: flex) and (color: red))");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kNot);
  auto* not_node = static_cast<const SupportsUnaryNode*>(node.get());
  EXPECT_EQ(not_node->Operand()->GetType(),
            SupportsConditionNode::Type::kNested);
}

TEST(SupportsConditionParserTest, DeepNesting) {
  // (((display: flex))) → nested(nested(decl))
  auto node = ParseOk("(((display: flex)))");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kNested);
  auto* outer = static_cast<const SupportsUnaryNode*>(node.get());
  EXPECT_EQ(outer->Operand()->GetType(), SupportsConditionNode::Type::kNested);
}

TEST(SupportsConditionParserTest, EmptyParens) {
  // () → general-enclosed
  auto node = ParseOk("()");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kGeneralEnclosed);
}

// ===========================================================================
// selector() function → SupportsFunctionNode
// ===========================================================================

TEST(SupportsConditionParserTest, SelectorFunction) {
  auto node = ParseOk("selector(:is(.a, .b))");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kFunction);
  auto* fn = static_cast<const SupportsFunctionNode*>(node.get());
  EXPECT_EQ(fn->Name(), "selector");
  EXPECT_NE(fn->RawArgs().find(":is"), std::string::npos);
}

TEST(SupportsConditionParserTest, SelectorSimple) {
  auto node = ParseOk("selector(div > p)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kFunction);
  auto* fn = static_cast<const SupportsFunctionNode*>(node.get());
  EXPECT_EQ(fn->Name(), "selector");
  EXPECT_NE(fn->RawArgs().find("div"), std::string::npos);
}

// ===========================================================================
// font-tech() function → SupportsFunctionNode
// ===========================================================================

TEST(SupportsConditionParserTest, FontTech) {
  auto node = ParseOk("font-tech(color-svg)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kFunction);
  auto* fn = static_cast<const SupportsFunctionNode*>(node.get());
  EXPECT_EQ(fn->Name(), "font-tech");
  EXPECT_EQ(fn->RawArgs(), "color-svg");
}

TEST(SupportsConditionParserTest, FontTechVariations) {
  auto node = ParseOk("font-tech(variations)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kFunction);
  auto* fn = static_cast<const SupportsFunctionNode*>(node.get());
  EXPECT_EQ(fn->Name(), "font-tech");
  EXPECT_EQ(fn->RawArgs(), "variations");
}

// ===========================================================================
// font-format() function → SupportsFunctionNode
// ===========================================================================

TEST(SupportsConditionParserTest, FontFormat) {
  auto node = ParseOk("font-format(woff2)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kFunction);
  auto* fn = static_cast<const SupportsFunctionNode*>(node.get());
  EXPECT_EQ(fn->Name(), "font-format");
  EXPECT_EQ(fn->RawArgs(), "woff2");
}

TEST(SupportsConditionParserTest, FontFormatOpentype) {
  auto node = ParseOk("font-format(opentype)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kFunction);
  auto* fn = static_cast<const SupportsFunctionNode*>(node.get());
  EXPECT_EQ(fn->Name(), "font-format");
  EXPECT_EQ(fn->RawArgs(), "opentype");
}

// ===========================================================================
// -x-engine-version() — Lynx extension
// ===========================================================================

TEST(SupportsConditionParserTest, EngineVersionRange) {
  auto node = ParseOk("-x-engine-version(3.0, 4.0)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kEngineVersion);
  auto* ev = static_cast<const SupportsEngineVersionNode*>(node.get());
  EXPECT_EQ(ev->Begin(), SupportsEngineVersionNode::PackVersion(3, 0));
  EXPECT_EQ(ev->End(), SupportsEngineVersionNode::PackVersion(4, 0));
}

TEST(SupportsConditionParserTest, EngineVersionUnboundedEnd) {
  auto node = ParseOk("-x-engine-version(3.0, *)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kEngineVersion);
  auto* ev = static_cast<const SupportsEngineVersionNode*>(node.get());
  EXPECT_EQ(ev->Begin(), SupportsEngineVersionNode::PackVersion(3, 0));
  EXPECT_EQ(ev->End(), SupportsEngineVersionNode::kMaxVersion);
}

TEST(SupportsConditionParserTest, EngineVersionSingleArg) {
  auto node = ParseOk("-x-engine-version(3.0)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kEngineVersion);
  auto* ev = static_cast<const SupportsEngineVersionNode*>(node.get());
  EXPECT_EQ(ev->Begin(), SupportsEngineVersionNode::PackVersion(3, 0));
  EXPECT_EQ(ev->End(), SupportsEngineVersionNode::kMaxVersion);
}

TEST(SupportsConditionParserTest, EngineVersionZero) {
  // 0.0 is a valid version and must not collide with the invalid sentinel.
  auto node = ParseOk("-x-engine-version(0.0)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kEngineVersion);
  auto* ev = static_cast<const SupportsEngineVersionNode*>(node.get());
  EXPECT_EQ(ev->Begin(), SupportsEngineVersionNode::PackVersion(0, 0));
  EXPECT_EQ(ev->End(), SupportsEngineVersionNode::kMaxVersion);
}

TEST(SupportsConditionParserTest, EngineVersionZeroRange) {
  auto node = ParseOk("-x-engine-version(0.0, 0.0)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kEngineVersion);
  auto* ev = static_cast<const SupportsEngineVersionNode*>(node.get());
  EXPECT_EQ(ev->Begin(), SupportsEngineVersionNode::PackVersion(0, 0));
  EXPECT_EQ(ev->End(), SupportsEngineVersionNode::PackVersion(0, 0));
}

TEST(SupportsConditionParserTest, EngineVersionOverflowFallback) {
  // major/minor exceeding 0xFFFF must be rejected (no silent uint16 wrap).
  ParseFail("-x-engine-version(99999.0)");
}

TEST(SupportsConditionParserTest, EngineVersionMaxBoundary) {
  // 65535.65535 is the largest representable version (each segment == 0xFFFF).
  auto node = ParseOk("-x-engine-version(65535.65535)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kEngineVersion);
  auto* ev = static_cast<const SupportsEngineVersionNode*>(node.get());
  EXPECT_EQ(ev->Begin(), SupportsEngineVersionNode::PackVersion(65535, 65535));
}

TEST(SupportsConditionParserTest, EngineVersionMajorJustOverMaxFallback) {
  // 65536 == 0xFFFF + 1; would wrap to 0 in uint16. Must be rejected.
  ParseFail("-x-engine-version(65536.0)");
}

TEST(SupportsConditionParserTest, EngineVersionMinorJustOverMaxFallback) {
  ParseFail("-x-engine-version(0.65536)");
}

TEST(SupportsConditionParserTest, EngineVersionUint16WrapAliasFallback) {
  // 131072 == 2 * 65536; a naive uint16 accumulate would wrap back to 0 and
  // masquerade as a valid "0". The uint32 + early-check guard must reject it.
  ParseFail("-x-engine-version(131072.0)");
}

TEST(SupportsConditionParserTest, EngineVersionVeryLongDigitsFallback) {
  // Far beyond uint32 range; must be rejected before any uint32 wrap matters.
  ParseFail("-x-engine-version(4294967296.0)");
}

TEST(SupportsConditionParserTest, EngineVersionLeadingZeros) {
  // Leading zeros are valid digits; "003.007" parses as 3.7.
  auto node = ParseOk("-x-engine-version(003.007)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kEngineVersion);
  auto* ev = static_cast<const SupportsEngineVersionNode*>(node.get());
  EXPECT_EQ(ev->Begin(), SupportsEngineVersionNode::PackVersion(3, 7));
}

TEST(SupportsConditionParserTest, EngineVersionTrailingZeroMinor) {
  // "3.10" must parse as minor==10, NOT 1. Parsing from source text avoids the
  // double precision loss that would collapse "3.10" into 3.1.
  auto node = ParseOk("-x-engine-version(3.10)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kEngineVersion);
  auto* ev = static_cast<const SupportsEngineVersionNode*>(node.get());
  EXPECT_EQ(ev->Begin(), SupportsEngineVersionNode::PackVersion(3, 10));
}

TEST(SupportsConditionParserTest, EngineVersionDistinguishMinorOneVsTen) {
  // 3.1 and 3.10 are different versions and must pack to different values.
  auto one = ParseOk("-x-engine-version(3.1)");
  auto ten = ParseOk("-x-engine-version(3.10)");
  ASSERT_EQ(one->GetType(), SupportsConditionNode::Type::kEngineVersion);
  ASSERT_EQ(ten->GetType(), SupportsConditionNode::Type::kEngineVersion);
  auto* ev_one = static_cast<const SupportsEngineVersionNode*>(one.get());
  auto* ev_ten = static_cast<const SupportsEngineVersionNode*>(ten.get());
  EXPECT_EQ(ev_one->Begin(), SupportsEngineVersionNode::PackVersion(3, 1));
  EXPECT_EQ(ev_ten->Begin(), SupportsEngineVersionNode::PackVersion(3, 10));
  EXPECT_NE(ev_one->Begin(), ev_ten->Begin());
}

TEST(SupportsConditionParserTest, EngineVersionEndArgOverflowFallback) {
  // Overflow in the second (end) argument must also be rejected.
  ParseFail("-x-engine-version(3.0, 99999.0)");
}

TEST(SupportsConditionParserTest, EngineVersionWildcardBeginFallback) {
  // `*` is only valid as the end/max version marker.
  ParseFail("-x-engine-version(*)");
  ParseFail("-x-engine-version(*, 4.0)");
}

TEST(SupportsConditionParserTest, EngineVersionSignedFallback) {
  // Signed numbers like "+3.0" are not valid version literals.
  ParseFail("-x-engine-version(+3.0)");
}

TEST(SupportsConditionParserTest, EngineVersionExponentFallback) {
  // Exponent forms like "3e2" are not valid version literals.
  ParseFail("-x-engine-version(3e2)");
}

TEST(SupportsConditionParserTest, EngineVersionTrailingTokensFail) {
  // Trailing tokens after valid args must be rejected.
  ParseFail("-x-engine-version(3.0 garbage)");
  ParseFail("-x-engine-version(3.0, 4.0 extra)");
}

// ===========================================================================
// at-rule() → SupportsFunctionNode (raw_args string)
// ===========================================================================

TEST(SupportsConditionParserTest, AtRuleNameOnly) {
  auto node = ParseOk("at-rule(layer)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kFunction);
  auto* fn = static_cast<const SupportsFunctionNode*>(node.get());
  EXPECT_EQ(fn->Name(), "at-rule");
  EXPECT_EQ(fn->RawArgs(), "layer");
}

TEST(SupportsConditionParserTest, AtRuleWithPrelude) {
  auto node = ParseOk("at-rule(container; (width > 0px))");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kFunction);
  auto* fn = static_cast<const SupportsFunctionNode*>(node.get());
  EXPECT_EQ(fn->Name(), "at-rule");
  EXPECT_NE(fn->RawArgs().find("container"), std::string::npos);
  EXPECT_NE(fn->RawArgs().find("width"), std::string::npos);
}

// ===========================================================================
// Unknown functions → SupportsFunctionNode (generic)
// ===========================================================================

TEST(SupportsConditionParserTest, UnknownFunction) {
  auto node = ParseOk("future-feature(something)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kFunction);
  auto* fn = static_cast<const SupportsFunctionNode*>(node.get());
  EXPECT_EQ(fn->Name(), "future-feature");
  EXPECT_EQ(fn->RawArgs(), "something");
}

TEST(SupportsConditionParserTest, EmptyArgsFunction) {
  auto node = ParseOk("unknown-fn()");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kFunction);
  auto* fn = static_cast<const SupportsFunctionNode*>(node.get());
  EXPECT_EQ(fn->Name(), "unknown-fn");
  EXPECT_EQ(fn->RawArgs(), "");
}

TEST(SupportsConditionParserTest, EngineVersionBadArgsFallback) {
  // No number arg → ConsumeEngineVersion fails → whole parse fails.
  ParseFail("-x-engine-version()");
}

// ===========================================================================
// General-enclosed (forward-compat, always eval false)
// ===========================================================================

TEST(SupportsConditionParserTest, GeneralEnclosedNumber) {
  // (1 + 1) doesn't match condition or declaration → general-enclosed
  auto node = ParseOk("(1 + 1)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kGeneralEnclosed);
}

TEST(SupportsConditionParserTest, GeneralEnclosedInAndChain) {
  // The unknown part becomes general-enclosed; overall AST is still valid
  auto node = ParseOk("(display: flex) and (unknown stuff)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kAnd);
  auto* and_node = static_cast<const SupportsBinaryNode*>(node.get());
  EXPECT_EQ(and_node->Left()->GetType(),
            SupportsConditionNode::Type::kDeclaration);
  EXPECT_EQ(and_node->Right()->GetType(),
            SupportsConditionNode::Type::kGeneralEnclosed);
}

TEST(SupportsConditionParserTest, NotWithGeneralEnclosed) {
  // not (unknown stuff) → not(general-enclosed) → evaluates true
  auto node = ParseOk("not (unknown stuff)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kNot);
  auto* not_node = static_cast<const SupportsUnaryNode*>(node.get());
  EXPECT_EQ(not_node->Operand()->GetType(),
            SupportsConditionNode::Type::kGeneralEnclosed);
}

// ===========================================================================
// Complex combinations
// ===========================================================================

TEST(SupportsConditionParserTest, FunctionInAndChain) {
  auto node = ParseOk("(display: flex) and selector(:is(.a))");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kAnd);
  auto* and_node = static_cast<const SupportsBinaryNode*>(node.get());
  EXPECT_EQ(and_node->Left()->GetType(),
            SupportsConditionNode::Type::kDeclaration);
  EXPECT_EQ(and_node->Right()->GetType(),
            SupportsConditionNode::Type::kFunction);
}

TEST(SupportsConditionParserTest, NotWithFunction) {
  auto node = ParseOk("not font-tech(variations)");
  ASSERT_EQ(node->GetType(), SupportsConditionNode::Type::kNot);
  auto* not_node = static_cast<const SupportsUnaryNode*>(node.get());
  EXPECT_EQ(not_node->Operand()->GetType(),
            SupportsConditionNode::Type::kFunction);
}

// ===========================================================================
// Serialize round-trip
// ===========================================================================

TEST(SupportsConditionParserTest, SerializeDeclaration) {
  auto node = ParseOk("(display: flex)");
  EXPECT_EQ(node->Serialize(), "(display: flex)");
}

TEST(SupportsConditionParserTest, SerializeNot) {
  auto node = ParseOk("not (display: grid)");
  EXPECT_EQ(node->Serialize(), "not (display: grid)");
}

TEST(SupportsConditionParserTest, SerializeFunction) {
  auto node = ParseOk("font-tech(variations)");
  EXPECT_EQ(node->Serialize(), "font-tech(variations)");
}

TEST(SupportsConditionParserTest, SerializeEngineVersion) {
  auto node = ParseOk("-x-engine-version(3.0, 4.0)");
  EXPECT_EQ(node->Serialize(), "-x-engine-version(3.0, 4.0)");
}

TEST(SupportsConditionParserTest, SerializeSelector) {
  auto node = ParseOk("selector(div > p)");
  EXPECT_NE(node->Serialize().find("selector("), std::string::npos);
}

TEST(SupportsConditionParserTest, SerializeAnd) {
  auto node = ParseOk("(a: 1) and (b: 2)");
  EXPECT_EQ(node->Serialize(), "(a: 1) and (b: 2)");
}

TEST(SupportsConditionParserTest, SerializeOr) {
  auto node = ParseOk("(a: 1) or (b: 2)");
  EXPECT_EQ(node->Serialize(), "(a: 1) or (b: 2)");
}

TEST(SupportsConditionParserTest, SerializeImportant) {
  auto node = ParseOk("(color: red !important)");
  EXPECT_EQ(node->Serialize(), "(color: red !important)");
}

TEST(SupportsConditionParserTest, SerializeGeneralEnclosed) {
  auto node = ParseOk("(1 + 1)");
  // General-enclosed serializes its raw text
  EXPECT_FALSE(node->Serialize().empty());
}

// ===========================================================================
// ToLepus / FromLepus round-trip
// ===========================================================================

TEST(SupportsConditionParserTest, LepusRoundTripDeclaration) {
  auto node = ParseOk("(display: flex)");
  auto lepus = node->ToLepus();
  auto restored = SupportsConditionNode::FromLepus(lepus);
  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->GetType(), SupportsConditionNode::Type::kDeclaration);
  auto* decl = static_cast<const SupportsDeclNode*>(restored.get());
  EXPECT_EQ(decl->Property(), "display");
  EXPECT_EQ(decl->Value(), "flex");
}

TEST(SupportsConditionParserTest, LepusRoundTripEngineVersion) {
  auto node = ParseOk("-x-engine-version(3.0, *)");
  auto lepus = node->ToLepus();
  auto restored = SupportsConditionNode::FromLepus(lepus);
  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->GetType(), SupportsConditionNode::Type::kEngineVersion);
  auto* ev = static_cast<const SupportsEngineVersionNode*>(restored.get());
  EXPECT_EQ(ev->Begin(), SupportsEngineVersionNode::PackVersion(3, 0));
  EXPECT_EQ(ev->End(), SupportsEngineVersionNode::kMaxVersion);
}

TEST(SupportsConditionParserTest, LepusRoundTripComplex) {
  auto node = ParseOk("(display: flex) and (not (color: red))");
  auto lepus = node->ToLepus();
  auto restored = SupportsConditionNode::FromLepus(lepus);
  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->GetType(), SupportsConditionNode::Type::kAnd);
}

TEST(SupportsConditionParserTest, LepusRoundTripGeneralEnclosed) {
  auto node = ParseOk("(1 + 1)");
  auto lepus = node->ToLepus();
  auto restored = SupportsConditionNode::FromLepus(lepus);
  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->GetType(), SupportsConditionNode::Type::kGeneralEnclosed);
}

TEST(SupportsConditionParserTest, LepusRoundTripFunction) {
  auto node = ParseOk("selector(div > p)");
  auto lepus = node->ToLepus();
  auto restored = SupportsConditionNode::FromLepus(lepus);
  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->GetType(), SupportsConditionNode::Type::kFunction);
  auto* fn = static_cast<const SupportsFunctionNode*>(restored.get());
  EXPECT_EQ(fn->Name(), "selector");
  EXPECT_NE(fn->RawArgs().find("div"), std::string::npos);
}

TEST(SupportsConditionParserTest, LepusRoundTripOr) {
  auto node = ParseOk("(a: 1) or (b: 2)");
  auto lepus = node->ToLepus();
  auto restored = SupportsConditionNode::FromLepus(lepus);
  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->GetType(), SupportsConditionNode::Type::kOr);
}

}  // namespace css
}  // namespace lynx
