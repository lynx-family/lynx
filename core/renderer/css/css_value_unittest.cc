// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/css/css_value.h"

#include <gtest/gtest.h>

#include "base/include/value/table.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/parser/css_string_parser.h"

namespace lynx {
namespace tasm {

class CSSValueSubstitutionTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Setup common test data
  }
  CSSParserConfigs configs_;
};

class CSSValueToVarReferenceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Setup common test data for ToVarReference tests
  }
  CSSParserConfigs configs_;
};

TEST(CSSValueTest, DefaultConstruct) {
  CSSValue v;
  EXPECT_TRUE(v.type_ == lynx_value_null);
  EXPECT_TRUE(v.IsEmpty());
  EXPECT_TRUE(v.GetValueType() == CSSValueType::DEFAULT);
}

TEST(CSSValueTest, ConstructFromEnum) {
  enum TempEnum { e1, e2, e3 = 1000, e4 = 9999 };
  CSSValue v1(e1);
  CSSValue v3(e3);
  CSSValue v4(e4);
  EXPECT_TRUE(v1.IsEnum());
  EXPECT_TRUE(v3.IsEnum());
  EXPECT_TRUE(v4.IsEnum());
  EXPECT_TRUE(v1.GetEnum<TempEnum>() == e1);
  EXPECT_TRUE(v3.GetEnum<TempEnum>() == e3);
  EXPECT_TRUE(v4.GetEnum<TempEnum>() == e4);
}

TEST(CSSValueTest, ConstructFromNumber) {
  {
    bool b = true;
    CSSValue v(b);
    EXPECT_TRUE(v.IsBoolean());
    EXPECT_TRUE(v.GetBool());
    EXPECT_TRUE(v.GetValue().IsBool());
    EXPECT_TRUE(v.GetValue().Bool());
  }

  {
    double d = 3.14;
    CSSValue v(d, CSSValuePattern::PX);
    EXPECT_TRUE(v.GetPattern() == CSSValuePattern::PX);
    EXPECT_TRUE(v.IsPx());
    EXPECT_TRUE(v.GetNumber() == d);
    EXPECT_TRUE(v.GetValue().IsNumber());
    EXPECT_TRUE(v.GetValue().IsDouble());
    EXPECT_TRUE(v.GetValue().Number() == d);
  }

  {
    int32_t i = 99;
    CSSValue v(i, CSSValuePattern::PERCENT);
    EXPECT_TRUE(v.GetPattern() == CSSValuePattern::PERCENT);
    EXPECT_TRUE(v.IsPercent());
    EXPECT_TRUE(v.GetNumber() == (double)i);
    EXPECT_TRUE(v.GetValue().IsNumber());
    EXPECT_TRUE(v.GetValue().IsInt32());
    EXPECT_TRUE(v.GetValue().Number() == (double)i);
  }

  {
    uint32_t i = 99;
    CSSValue v(i, CSSValuePattern::PERCENT);
    EXPECT_TRUE(v.GetPattern() == CSSValuePattern::PERCENT);
    EXPECT_TRUE(v.IsPercent());
    EXPECT_TRUE(v.GetNumber() == (double)i);
    EXPECT_TRUE(v.GetValue().IsNumber());
    EXPECT_TRUE(v.GetValue().IsUInt32());
    EXPECT_TRUE(v.GetValue().Number() == (double)i);
  }

  {
    int64_t i = 99;
    CSSValue v(i, CSSValuePattern::PERCENT);
    EXPECT_TRUE(v.GetPattern() == CSSValuePattern::PERCENT);
    EXPECT_TRUE(v.IsPercent());
    EXPECT_TRUE(v.GetNumber() == (double)i);
    EXPECT_TRUE(v.GetValue().IsNumber());
    EXPECT_TRUE(v.GetValue().IsInt64());
    EXPECT_TRUE(v.GetValue().Number() == (double)i);
  }

  {
    uint64_t i = 99;
    CSSValue v(i, CSSValuePattern::PERCENT);
    EXPECT_TRUE(v.GetPattern() == CSSValuePattern::PERCENT);
    EXPECT_TRUE(v.IsPercent());
    EXPECT_TRUE(v.GetNumber() == (double)i);
    EXPECT_TRUE(v.GetValue().IsNumber());
    EXPECT_TRUE(v.GetValue().IsUInt64());
    EXPECT_TRUE(v.GetValue().Number() == (double)i);
  }
}

TEST_F(CSSValueSubstitutionTest, SimpleVariableSubstitution) {
  CustomPropertiesMap variables;
  variables.insert_or_assign("--color", CSSValue::MakePlainString("red"));

  lepus::Value variable = lepus::Value("var(--color)");
  CSSStringParser parser = CSSStringParser::FromLepusString(variable, configs_);

  CSSValue css_value = parser.ParseVariable();

  std::string result = CSSValue::Substitution(css_value, variables);
  EXPECT_EQ(result, "red");
}

TEST_F(CSSValueSubstitutionTest, VariableWithFallback) {
  CustomPropertiesMap variables;
  // --primary is not defined, should use fallback

  lepus::Value variable = lepus::Value("var(--primary, blue)");
  CSSStringParser parser = CSSStringParser::FromLepusString(variable, configs_);
  CSSValue css_value = parser.ParseVariable();

  std::string result = CSSValue::Substitution(css_value, variables);
  EXPECT_EQ(result, " blue");
}

TEST_F(CSSValueSubstitutionTest, NestedVariableReferences) {
  CustomPropertiesMap variables;
  variables.insert_or_assign("--primary", CSSValue::MakePlainString("red"));

  {
    lepus::Value variable = lepus::Value("var(--primary)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue var_primary = parser.ParseVariable();

    variables.insert_or_assign("--secondary", var_primary);
  }

  lepus::Value variable = lepus::Value("var(--secondary)");
  CSSStringParser parser = CSSStringParser::FromLepusString(variable, configs_);
  CSSValue css_value = parser.ParseVariable();

  std::string result = CSSValue::Substitution(css_value, variables);
  EXPECT_EQ(result, "red");
}

TEST_F(CSSValueSubstitutionTest, DeepNestedVariableReferences) {
  CustomPropertiesMap variables;
  variables.insert_or_assign("--a", CSSValue::MakePlainString("red"));

  {
    lepus::Value variable = lepus::Value("var(--a)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_a = parser.ParseVariable();
    variables.insert_or_assign("--b", ref_to_a);
  }

  {
    lepus::Value variable = lepus::Value("var(--b)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_b = parser.ParseVariable();
    variables.insert_or_assign("--c", ref_to_b);
  }

  {
    lepus::Value variable = lepus::Value("var(--c)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_c = parser.ParseVariable();
    variables.insert_or_assign("--d", ref_to_c);
  }

  lepus::Value variable = lepus::Value("var(--d)");
  CSSStringParser parser = CSSStringParser::FromLepusString(variable, configs_);
  CSSValue css_value = parser.ParseVariable();

  std::string result = CSSValue::Substitution(css_value, variables);
  EXPECT_EQ(result, "red");
}

TEST_F(CSSValueSubstitutionTest, CycleDetection) {
  CustomPropertiesMap variables;

  {
    lepus::Value variable = lepus::Value("var(--a)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_a = parser.ParseVariable();
    variables.insert_or_assign("--b", ref_to_a);
  }

  {
    lepus::Value variable = lepus::Value("var(--b)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_b = parser.ParseVariable();
    variables.insert_or_assign("--a", ref_to_b);
  }

  lepus::Value variable = lepus::Value("var(--a)");
  CSSStringParser parser = CSSStringParser::FromLepusString(variable, configs_);
  CSSValue css_value = parser.ParseVariable();

  std::string result = CSSValue::Substitution(css_value, variables);
  EXPECT_EQ(result, "");  // Should return empty due to cycle
}

TEST_F(CSSValueSubstitutionTest, DepthLimit) {
  CustomPropertiesMap variables;

  {
    lepus::Value variable = lepus::Value("var(--a2)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_a2 = parser.ParseVariable();
    variables.insert_or_assign("--a1", ref_to_a2);
  }

  {
    lepus::Value variable = lepus::Value("var(--a3)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_a3 = parser.ParseVariable();
    variables.insert_or_assign("--a2", ref_to_a3);
  }

  {
    lepus::Value variable = lepus::Value("var(--a4)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_a4 = parser.ParseVariable();
    variables.insert_or_assign("--a3", ref_to_a4);
  }

  {
    lepus::Value variable = lepus::Value("var(--a5)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_a5 = parser.ParseVariable();
    variables.insert_or_assign("--a4", ref_to_a5);
  }

  {
    lepus::Value variable = lepus::Value("var(--a6)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_a6 = parser.ParseVariable();
    variables.insert_or_assign("--a5", ref_to_a6);
  }

  variables.insert_or_assign("--a6", CSSValue::MakePlainString("red"));

  lepus::Value variable = lepus::Value("var(--a1)");
  CSSStringParser parser = CSSStringParser::FromLepusString(variable, configs_);
  CSSValue css_value = parser.ParseVariable();

  // Test with depth limit 3 (should hit limit)
  std::string result = CSSValue::Substitution(css_value, variables, 3);
  EXPECT_EQ(result, "");  // Should return empty due to depth limit

  // Test with sufficient depth
  result = CSSValue::Substitution(css_value, variables, 10);
  EXPECT_EQ(result, "red");
}

TEST_F(CSSValueSubstitutionTest, MultipleVariablesInOneString) {
  CustomPropertiesMap variables;
  variables.insert_or_assign("--color", CSSValue::MakePlainString("red"));
  variables.insert_or_assign("--size", CSSValue::MakePlainString("16px"));

  lepus::Value variable =
      lepus::Value("color: var(--color); font-size: var(--size)");
  CSSStringParser parser = CSSStringParser::FromLepusString(variable, configs_);
  CSSValue css_value = parser.ParseVariable();

  std::string result = CSSValue::Substitution(css_value, variables);
  EXPECT_EQ(result, "color: red; font-size: 16px");
}

TEST_F(CSSValueSubstitutionTest, NonVariableValue) {
  CustomPropertiesMap variables;

  auto css_value = CSSValue::MakePlainString("red");
  std::string result = CSSValue::Substitution(css_value, variables);
  EXPECT_EQ(result, "red");
}

TEST_F(CSSValueSubstitutionTest, EmptyVariableMap) {
  CustomPropertiesMap variables;

  lepus::Value variable = lepus::Value("var(--undefined)");
  CSSStringParser parser = CSSStringParser::FromLepusString(variable, configs_);
  CSSValue css_value = parser.ParseVariable();

  std::string result = CSSValue::Substitution(css_value, variables);
  EXPECT_EQ(result, "");  // Should return empty for undefined variable
}

TEST_F(CSSValueSubstitutionTest, MultiVariableCycleDetection) {
  CustomPropertiesMap variables;

  // Create a cycle: --a -> --b -> --c -> --a
  {
    lepus::Value variable = lepus::Value("var(--b)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_b = parser.ParseVariable();
    variables.insert_or_assign("--a", ref_to_b);
  }

  {
    lepus::Value variable = lepus::Value("var(--c)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_c = parser.ParseVariable();
    variables.insert_or_assign("--b", ref_to_c);
  }

  {
    lepus::Value variable = lepus::Value("var(--a)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_a = parser.ParseVariable();
    variables.insert_or_assign("--c", ref_to_a);
  }

  lepus::Value variable = lepus::Value("var(--a)");
  CSSStringParser parser = CSSStringParser::FromLepusString(variable, configs_);
  CSSValue css_value = parser.ParseVariable();

  std::string result = CSSValue::Substitution(css_value, variables);
  EXPECT_EQ(result, "");  // Should return empty due to cycle
}

TEST_F(CSSValueSubstitutionTest, SelfReferencingVariable) {
  CustomPropertiesMap variables;

  {
    lepus::Value variable = lepus::Value("var(--self)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_self = parser.ParseVariable();
    variables.insert_or_assign("--self", ref_to_self);
  }

  lepus::Value variable = lepus::Value("var(--self)");
  CSSStringParser parser = CSSStringParser::FromLepusString(variable, configs_);
  CSSValue css_value = parser.ParseVariable();

  std::string result = CSSValue::Substitution(css_value, variables);
  EXPECT_EQ(result, "");  // Should return empty due to self-reference
}

TEST_F(CSSValueSubstitutionTest, MultipleVariablesWithCycle) {
  CustomPropertiesMap variables;

  // Create variables with a cycle
  variables.insert_or_assign("--valid", CSSValue::MakePlainString("blue"));

  {
    lepus::Value variable = lepus::Value("var(--cycle1)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_cycle1 = parser.ParseVariable();
    variables.insert_or_assign("--cycle2", ref_to_cycle1);
  }

  {
    lepus::Value variable = lepus::Value("var(--cycle2)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_cycle2 = parser.ParseVariable();
    variables.insert_or_assign("--cycle1", ref_to_cycle2);
  }

  // Test multiple variables in one string where one has a cycle
  lepus::Value variable =
      lepus::Value("color: var(--valid); border: var(--cycle1)");
  CSSStringParser parser = CSSStringParser::FromLepusString(variable, configs_);
  CSSValue css_value = parser.ParseVariable();

  std::string result = CSSValue::Substitution(css_value, variables);
  EXPECT_EQ(result, "color: blue; border: ");  // --cycle1 should resolve to
                                               // empty due to cycle
}

TEST_F(CSSValueSubstitutionTest, ComplexCycleWithFallback) {
  CustomPropertiesMap variables;

  // Create a cycle with fallback values
  {
    lepus::Value variable = lepus::Value("var(--cyclic, fallback)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_cyclic = parser.ParseVariable();
    variables.insert_or_assign("--cyclic", ref_to_cyclic);
  }

  lepus::Value variable = lepus::Value("var(--cyclic)");
  CSSStringParser parser = CSSStringParser::FromLepusString(variable, configs_);
  CSSValue css_value = parser.ParseVariable();

  std::string result = CSSValue::Substitution(css_value, variables);
  EXPECT_EQ(result, "");  // Should return empty due to cycle, not fallback
}

TEST_F(CSSValueSubstitutionTest, CrossReferenceCycleDetection) {
  CustomPropertiesMap variables;

  // Create variables that reference each other in a complex way
  {
    lepus::Value variable = lepus::Value("var(--x) var(--y)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue complex_ref = parser.ParseVariable();
    variables.insert_or_assign("--z", complex_ref);
  }

  {
    lepus::Value variable = lepus::Value("var(--z)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_z = parser.ParseVariable();
    variables.insert_or_assign("--x", ref_to_z);
  }

  {
    lepus::Value variable = lepus::Value("var(--x)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_x = parser.ParseVariable();
    variables.insert_or_assign("--y", ref_to_x);
  }

  lepus::Value variable = lepus::Value("var(--x)");
  CSSStringParser parser = CSSStringParser::FromLepusString(variable, configs_);
  CSSValue css_value = parser.ParseVariable();

  std::string result = CSSValue::Substitution(css_value, variables);
  EXPECT_EQ(result, "");  // Should detect cycle across multiple references
}

TEST_F(CSSValueSubstitutionTest, CycleWithFallbackCorrectBehavior) {
  CustomPropertiesMap variables;

  // Create a 3-element cycle: --a -> --b -> --c -> --a
  {
    lepus::Value variable = lepus::Value("var(--b, fallback-b)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_b = parser.ParseVariable();
    variables.insert_or_assign("--a", ref_to_b);
  }

  {
    lepus::Value variable = lepus::Value("var(--c, fallback-c)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_c = parser.ParseVariable();
    variables.insert_or_assign("--b", ref_to_c);
  }

  {
    lepus::Value variable = lepus::Value("var(--a, fallback-a)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_a = parser.ParseVariable();
    variables.insert_or_assign("--c", ref_to_a);
  }

  // Test that variables in the cycle return empty (undefined) instead of
  // fallback
  lepus::Value variable_a = lepus::Value("var(--a)");
  CSSStringParser parser_a =
      CSSStringParser::FromLepusString(variable_a, configs_);
  CSSValue css_value_a = parser_a.ParseVariable();
  std::string result_a = CSSValue::Substitution(css_value_a, variables);
  EXPECT_EQ(result_a, "");  // Should return empty due to cycle, not fallback

  lepus::Value variable_b = lepus::Value("var(--b)");
  CSSStringParser parser_b =
      CSSStringParser::FromLepusString(variable_b, configs_);
  CSSValue css_value_b = parser_b.ParseVariable();
  std::string result_b = CSSValue::Substitution(css_value_b, variables);
  EXPECT_EQ(result_b, "");  // Should return empty due to cycle, not fallback

  lepus::Value variable_c = lepus::Value("var(--c)");
  CSSStringParser parser_c =
      CSSStringParser::FromLepusString(variable_c, configs_);
  CSSValue css_value_c = parser_c.ParseVariable();
  std::string result_c = CSSValue::Substitution(css_value_c, variables);
  EXPECT_EQ(result_c, "");  // Should return empty due to cycle, not fallback

  // Test that non-cyclic variables can use fallback correctly
  variables.insert_or_assign("--valid", CSSValue::MakePlainString("blue"));

  lepus::Value variable_d = lepus::Value("var(--nonexistent, fallback-value)");
  CSSStringParser parser_d =
      CSSStringParser::FromLepusString(variable_d, configs_);
  CSSValue css_value_d = parser_d.ParseVariable();
  std::string result_d = CSSValue::Substitution(css_value_d, variables);
  EXPECT_EQ(result_d,
            " fallback-value");  // Should use fallback for non-cyclic undefined
}

TEST_F(CSSValueSubstitutionTest, NonCycleFallbackBehavior) {
  CustomPropertiesMap variables;
  variables.insert_or_assign("--valid", CSSValue::MakePlainString("blue"));

  // Test that variables not in cycles can use fallback correctly
  lepus::Value variable = lepus::Value("var(--undefined, fallback)");
  CSSStringParser parser = CSSStringParser::FromLepusString(variable, configs_);
  CSSValue css_value = parser.ParseVariable();

  std::string result = CSSValue::Substitution(css_value, variables);
  EXPECT_EQ(
      result,
      " fallback");  // Should use fallback for non-cyclic undefined variable
}

TEST_F(CSSValueSubstitutionTest, MixedCycleAndValidVariables) {
  CustomPropertiesMap variables;
  variables.insert_or_assign("--valid", CSSValue::MakePlainString("green"));

  // Create a cycle
  {
    lepus::Value variable = lepus::Value("var(--cyclic)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_cyclic = parser.ParseVariable();
    variables.insert_or_assign("--a", ref_to_cyclic);
  }

  {
    lepus::Value variable = lepus::Value("var(--a)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_a = parser.ParseVariable();
    variables.insert_or_assign("--cyclic", ref_to_a);
  }

  // Test mixed usage: valid variable + cyclic variable
  lepus::Value variable =
      lepus::Value("color: var(--valid); border: var(--cyclic, fallback)");
  CSSStringParser parser = CSSStringParser::FromLepusString(variable, configs_);
  CSSValue css_value = parser.ParseVariable();

  std::string result = CSSValue::Substitution(css_value, variables);
  EXPECT_EQ(result,
            "color: green; border:  fallback");  // --cyclic is on circle, but
                                                 // caller can use fallback
}

TEST_F(CSSValueSubstitutionTest, MixedCycleAndValidVariables2) {
  CustomPropertiesMap variables;
  /**
   *
   *  background: var(--cyclic-a, blue);
   *  --cyclic: var(--cyclic-b, red);
   *  --cyclic-a: var(--cyclic-b, yellow);
   *  --cyclic-b: var(--cyclic, pink);
   * */

  // Create a cycle
  {
    lepus::Value variable = lepus::Value("var(--cyclic-b, red)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_cyclic = parser.ParseVariable();
    variables.insert_or_assign("--cyclic", ref_to_cyclic);
  }

  {
    lepus::Value variable = lepus::Value("var(--cyclic-b, yellow)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_a = parser.ParseVariable();
    variables.insert_or_assign("--cyclic-a", ref_to_a);
  }

  {
    lepus::Value variable = lepus::Value("var(--cyclic, pink)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_b = parser.ParseVariable();
    variables.insert_or_assign("--cyclic-b", ref_to_b);
  }

  // Test mixed usage: valid variable + cyclic variable
  lepus::Value variable = lepus::Value("var(--cyclic-a, blue)");
  CSSStringParser parser = CSSStringParser::FromLepusString(variable, configs_);
  CSSValue css_value = parser.ParseVariable();

  std::string result = CSSValue::Substitution(css_value, variables);
  EXPECT_EQ(result, " yellow");
}

TEST_F(CSSValueSubstitutionTest, SubstitutionConsumeProperty1) {
  CustomPropertiesMap custom_properties;

  {
    lepus::Value variable = lepus::Value("var(--b, red)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_cyclic = parser.ParseVariable();
    custom_properties.insert_or_assign("--a", ref_to_cyclic);
  }

  {
    lepus::Value variable = lepus::Value("var(--c, yellow)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_a = parser.ParseVariable();
    custom_properties.insert_or_assign("--b", ref_to_a);
  }

  {
    lepus::Value variable = lepus::Value("blue");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_b = parser.ParseVariable();
    custom_properties.insert_or_assign("--c", ref_to_b);
  }

  CSSVariableMap result_css_related_variables{};
  CSSVariableMap target_css_related_variables{};
  target_css_related_variables.insert_or_assign("--d", "");
  target_css_related_variables.insert_or_assign("--a", "var(--b, red)");
  target_css_related_variables.insert_or_assign("--b", "var(--c, yellow)");
  target_css_related_variables.insert_or_assign("--c", "blue");

  lepus::Value variable = lepus::Value("var(--d, var(--a))");
  CSSStringParser parser = CSSStringParser::FromLepusString(variable, configs_);
  CSSValue css_value = parser.ParseVariable();

  CSSValue::HandleCustomPropertyFunc handle_func =
      [&result_css_related_variables](const base::String& name,
                                      const base::String& value) {
        result_css_related_variables.insert_or_assign(name, value);
      };

  std::string value_str =
      CSSValue::Substitution(css_value, custom_properties, 10, handle_func);
  EXPECT_EQ(value_str, " blue");
  EXPECT_EQ(result_css_related_variables, target_css_related_variables);
}

TEST_F(CSSValueSubstitutionTest, SubstitutionNestedVariable) {
  CustomPropertiesMap custom_properties;
  {
    lepus::Value variable = lepus::Value("var(--b, red)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_cyclic = parser.ParseVariable();
    custom_properties.insert_or_assign("--a", ref_to_cyclic);
  }
  {
    lepus::Value variable = lepus::Value("var(--c, yellow)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_a = parser.ParseVariable();
    custom_properties.insert_or_assign("--b", ref_to_a);
  }
  {
    lepus::Value variable = lepus::Value("blue");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_b = parser.ParseVariable();
    custom_properties.insert_or_assign("--c", ref_to_b);
  }

  lepus::Value variable = lepus::Value(
      "var(--d, var(--invalid-name, var(--invalid-name2, var(--a))))");
  CSSStringParser parser = CSSStringParser::FromLepusString(variable, configs_);
  CSSValue css_value = parser.ParseVariable();
  std::string result =
      CSSValue::Substitution(css_value, custom_properties, 10, nullptr);
  EXPECT_EQ(result, "   blue");
}

TEST_F(CSSValueSubstitutionTest, SubstituteAll) {
  CustomPropertiesMap custom_properties;
  {
    lepus::Value variable = lepus::Value("var(--b, red)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_cyclic = parser.ParseVariable();
    custom_properties.insert_or_assign("--a", ref_to_cyclic);
    EXPECT_TRUE(ref_to_cyclic.NeedsVariableResolution());
    EXPECT_TRUE(custom_properties["--a"].NeedsVariableResolution());
  }
  {
    lepus::Value variable = lepus::Value("var(--c, yellow)");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_a = parser.ParseVariable();
    custom_properties.insert_or_assign("--b", ref_to_a);
  }
  {
    lepus::Value variable = lepus::Value("blue");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue ref_to_b = parser.ParseVariable();
    custom_properties.insert_or_assign("--c", ref_to_b);
  }
  {
    lepus::Value variable = lepus::Value(
        "var(--d, var(--invalid-name, var(--invalid-name2, var(--a))))");
    CSSStringParser parser =
        CSSStringParser::FromLepusString(variable, configs_);
    CSSValue css_value = parser.ParseVariable();
    custom_properties.insert_or_assign("--d", css_value);
  }

  CSSValue::SubstituteAll(custom_properties, 10, nullptr);

  EXPECT_EQ(custom_properties["--a"].AsStdString(), "blue");
  EXPECT_EQ(custom_properties["--b"].AsStdString(), "blue");
  EXPECT_EQ(custom_properties["--c"].AsStdString(), "blue");
  EXPECT_EQ(custom_properties["--d"].AsStdString(), "   blue");
}

// Tests for ToVarReference function using Substitution method
TEST_F(CSSValueToVarReferenceTest, SimpleVariableToVarReference) {
  // Test simple {{--variable}} format conversion
  CustomPropertiesMap variables;
  variables.insert_or_assign("--color", CSSValue::MakePlainString("red"));

  CSSValue css_value("{{--color}}", CSSValuePattern::STRING,
                     CSSValueType::VARIABLE);
  css_value.SetDefaultValue("blue");

  // Convert to VarReference
  EXPECT_TRUE(css_value.ToVarReference());
  EXPECT_TRUE(css_value.NeedsVariableResolution());
  EXPECT_TRUE(css_value.IsVariable());

  // Test that substitution works with the converted VarReference
  std::string result = CSSValue::Substitution(css_value, variables);
  EXPECT_EQ(result, "red");  // Should resolve to the variable value
}

TEST_F(CSSValueToVarReferenceTest, MultipleVariablesToVarReference) {
  // Test multiple variables in one value: {{--color}} {{--size}}
  CustomPropertiesMap variables;
  variables.insert_or_assign("--color", CSSValue::MakePlainString("red"));
  variables.insert_or_assign("--size", CSSValue::MakePlainString("16px"));

  CSSValue css_value("{{--color}} {{--size}}", CSSValuePattern::STRING,
                     CSSValueType::VARIABLE);

  EXPECT_TRUE(css_value.ToVarReference());
  EXPECT_TRUE(css_value.NeedsVariableResolution());

  // Test that substitution works with multiple variables
  std::string result = CSSValue::Substitution(css_value, variables);
  EXPECT_EQ(result, "red 16px");  // Should resolve both variables
}

TEST_F(CSSValueToVarReferenceTest, VariableWithFallbackMapToVarReference) {
  // Test variable with fallback from default value map
  CustomPropertiesMap variables;
  // --primary is not defined, should use fallback

  // Create a CSS value with default value map
  lepus::Value default_map(lepus::Dictionary::Create());
  default_map.Table()->SetValue("--primary", lepus::Value("red"));

  CSSValue css_value("{{--primary}}", CSSValuePattern::STRING,
                     CSSValueType::VARIABLE);
  css_value.SetDefaultValueMap(default_map);

  EXPECT_TRUE(css_value.ToVarReference());

  // Test that substitution works with fallback from default map
  std::string result = CSSValue::Substitution(css_value, variables);
  EXPECT_EQ(result, "red");  // Should use fallback from default map
}

TEST_F(CSSValueToVarReferenceTest, NoVariableConversionForNonVariable) {
  // Test that non-variable CSS values are not converted
  auto css_value = CSSValue::MakePlainString("red");

  EXPECT_FALSE(css_value.ToVarReference());
  EXPECT_FALSE(css_value.NeedsVariableResolution());
  EXPECT_FALSE(css_value.IsVariable());
}

TEST_F(CSSValueToVarReferenceTest, NoDoubleConversion) {
  // Test that already converted variables are not converted again
  CustomPropertiesMap variables;
  variables.insert_or_assign("--color", CSSValue::MakePlainString("red"));

  CSSValue css_value("{{--color}}", CSSValuePattern::STRING,
                     CSSValueType::VARIABLE);

  // First conversion
  EXPECT_TRUE(css_value.ToVarReference());
  EXPECT_TRUE(css_value.NeedsVariableResolution());

  // Second conversion should fail
  EXPECT_FALSE(css_value.ToVarReference());

  // Verify substitution still works after attempted double conversion
  std::string result = CSSValue::Substitution(css_value, variables);
  EXPECT_EQ(result, "red");
}

TEST_F(CSSValueToVarReferenceTest, EmptyVariableName) {
  // Test edge case with empty variable name: {{}}
  CustomPropertiesMap variables;

  CSSValue css_value("{{}}", CSSValuePattern::STRING, CSSValueType::VARIABLE);

  EXPECT_TRUE(css_value.ToVarReference());

  // Test substitution with empty variable name
  std::string result = CSSValue::Substitution(css_value, variables);
  EXPECT_EQ(result, "");  // Should return empty for undefined variable
}

TEST_F(CSSValueToVarReferenceTest, ComplexVariableWithCalc) {
  // Test complex variable usage: calc({{--size}} * 2)
  CustomPropertiesMap variables;
  variables.insert_or_assign("--size", CSSValue::MakePlainString("10px"));

  CSSValue css_value("calc({{--size}} * 2)", CSSValuePattern::STRING,
                     CSSValueType::VARIABLE);

  EXPECT_TRUE(css_value.ToVarReference());

  // Test that substitution works with complex expressions
  std::string result = CSSValue::Substitution(css_value, variables);
  EXPECT_EQ(result,
            "calc(10px * 2)");  // Should resolve variable in calc expression
}

TEST_F(CSSValueToVarReferenceTest, VariableWithSimpleFallback) {
  // Test variable with simple fallback value
  CustomPropertiesMap variables;
  // --undefined-color is not in variables, should use fallback

  CSSValue css_value("{{--undefined-color}}", CSSValuePattern::STRING,
                     CSSValueType::VARIABLE);
  css_value.SetDefaultValue("black");

  EXPECT_TRUE(css_value.ToVarReference());

  // Test that substitution works with simple fallback
  std::string result = CSSValue::Substitution(css_value, variables);
  EXPECT_EQ(result, "black");  // Should use the fallback value
}

}  // namespace tasm
}  // namespace lynx
