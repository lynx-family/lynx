// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/css_value.h"

#include <gtest/gtest.h>

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

TEST_F(CSSValueSubstitutionTest, SimpleVariableSubstitution) {
  CustomPropertiesMap variables;
  variables.insert_or_assign("--color", CSSValue(lepus::Value("red")));

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
  variables.insert_or_assign("--primary", CSSValue(lepus::Value("red")));

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
  variables.insert_or_assign("--a", CSSValue(lepus::Value("red")));

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

  variables.insert_or_assign("--a6", CSSValue(lepus::Value("red")));

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
  variables.insert_or_assign("--color", CSSValue(lepus::Value("red")));
  variables.insert_or_assign("--size", CSSValue(lepus::Value("16px")));

  lepus::Value variable =
      lepus::Value("color: var(--color); font-size: var(--size)");
  CSSStringParser parser = CSSStringParser::FromLepusString(variable, configs_);
  CSSValue css_value = parser.ParseVariable();

  std::string result = CSSValue::Substitution(css_value, variables);
  EXPECT_EQ(result, "color: red; font-size: 16px");
}

TEST_F(CSSValueSubstitutionTest, NonVariableValue) {
  CustomPropertiesMap variables;

  CSSValue css_value(lepus::Value("red"));
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
  variables.insert_or_assign("--valid", CSSValue(lepus::Value("blue")));

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
  variables.insert_or_assign("--valid", CSSValue(lepus::Value("blue")));

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
  variables.insert_or_assign("--valid", CSSValue(lepus::Value("blue")));

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
  variables.insert_or_assign("--valid", CSSValue(lepus::Value("green")));

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

  EXPECT_EQ(custom_properties["--a"].AsString(), "blue");
  EXPECT_EQ(custom_properties["--b"].AsString(), "blue");
  EXPECT_EQ(custom_properties["--c"].AsString(), "blue");
  EXPECT_EQ(custom_properties["--d"].AsString(), "   blue");
}

}  // namespace tasm
}  // namespace lynx
