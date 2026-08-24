// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/element_template_instance.h"

#include <cmath>
#include <limits>
#include <type_traits>

#include "core/renderer/dom/testing/fiber_element_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

class ElementTemplateInstanceTest : public FiberElementTest {};

TEST_P(ElementTemplateInstanceTest, UsesIndependentLepusRefType) {
  auto instance = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));

  EXPECT_EQ(instance->GetRefType(), lepus::RefType::kElementTemplate);
  static_assert(!std::is_base_of_v<Element, ElementTemplateInstance>);
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateUidPreservesRuntimeNumberValues) {
  auto instance = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  instance->SetTypedTag(base::String("view"));

  instance->SetUid(lepus::Value(1.5));
  EXPECT_EQ(instance->Serialize().GetProperty("uid").Number(), 1.5);

  instance->SetUid(lepus::Value(std::numeric_limits<double>::quiet_NaN()));
  EXPECT_TRUE(std::isnan(instance->Serialize().GetProperty("uid").Number()));
}

INSTANTIATE_TEST_SUITE_P(ElementTemplateInstanceTestModule,
                         ElementTemplateInstanceTest,
                         ::testing::ValuesIn(fiber_element_generation_params));

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
