// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/element_template_instance.h"

#include <cmath>
#include <limits>
#include <type_traits>

#include "core/renderer/dom/fiber/view_element.h"
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

TEST_P(ElementTemplateInstanceTest, SerializeElementTemplateRecursively) {
  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTemplateKey(base::String("child_template"));
  child->SetBundleUrl(base::String("child_bundle.js"));

  auto child_attribute_slots = lepus::CArray::Create();
  child_attribute_slots->emplace_back(lepus::Value(true));
  child->SetAttributeSlots(lepus::Value(std::move(child_attribute_slots)));

  auto child_child_slots = lepus::CArray::Create();
  child_child_slots->emplace_back(lepus::Value(lepus::CArray::Create()));
  child->InitializeChildSlots(lepus::Value(std::move(child_child_slots)));

  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->SetTemplateKey(base::String("root_template"));
  root->SetBundleUrl(base::String("root_bundle.js"));

  auto root_attribute_slots = lepus::CArray::Create();
  root_attribute_slots->emplace_back(lepus::Value("slot_0"));
  root_attribute_slots->emplace_back(lepus::Value(42));
  root->SetAttributeSlots(lepus::Value(std::move(root_attribute_slots)));

  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(child));
  auto root_child_slots = lepus::CArray::Create();
  root_child_slots->emplace_back(lepus::Value(std::move(slot_children)));
  root->InitializeChildSlots(lepus::Value(std::move(root_child_slots)));

  auto serialized = root->Serialize();
  EXPECT_TRUE(serialized.IsObject());
  EXPECT_EQ(serialized.GetProperty("templateKey").StdString(), "root_template");
  EXPECT_EQ(serialized.GetProperty("bundleUrl").StdString(), "root_bundle.js");
  EXPECT_FALSE(serialized.GetProperty("kind").IsString());

  auto serialized_attribute_slots = serialized.GetProperty("attributeSlots");
  EXPECT_TRUE(serialized_attribute_slots.IsArrayOrJSArray());
  ASSERT_EQ(serialized_attribute_slots.GetLength(), 2);
  EXPECT_EQ(serialized_attribute_slots.GetProperty(0).StdString(), "slot_0");
  EXPECT_EQ(serialized_attribute_slots.GetProperty(1).Number(), 42);

  auto serialized_child_slots = serialized.GetProperty("childSlots");
  EXPECT_TRUE(serialized_child_slots.IsArrayOrJSArray());
  ASSERT_EQ(serialized_child_slots.GetLength(), 1);
  auto serialized_slot_children = serialized_child_slots.GetProperty(0);
  EXPECT_TRUE(serialized_slot_children.IsArrayOrJSArray());
  ASSERT_EQ(serialized_slot_children.GetLength(), 1);

  auto serialized_child = serialized_slot_children.GetProperty(0);
  EXPECT_TRUE(serialized_child.IsObject());
  EXPECT_EQ(serialized_child.GetProperty("templateKey").StdString(),
            "child_template");
  EXPECT_EQ(serialized_child.GetProperty("bundleUrl").StdString(),
            "child_bundle.js");
  EXPECT_FALSE(serialized_child.GetProperty("kind").IsString());
  EXPECT_TRUE(
      serialized_child.GetProperty("attributeSlots").IsArrayOrJSArray());
  EXPECT_EQ(
      serialized_child.GetProperty("attributeSlots").GetProperty(0).Bool(),
      true);
  EXPECT_TRUE(serialized_child.GetProperty("childSlots").IsArrayOrJSArray());
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateInitializationOwnsCallerSlotArrays) {
  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTypedTag(base::String("raw-text"));
  child->SetUid(lepus::Value(7));

  auto first_slot_children = lepus::CArray::Create();
  first_slot_children->emplace_back(lepus::Value(child));
  auto second_slot_children = lepus::CArray::Create();
  second_slot_children->emplace_back(lepus::Value(child));
  auto child_slots = lepus::CArray::Create();
  child_slots->emplace_back(lepus::Value(first_slot_children));
  child_slots->emplace_back(lepus::Value(second_slot_children));

  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->InitializeChildSlots(lepus::Value(child_slots));

  ASSERT_TRUE(first_slot_children->Erase(0, first_slot_children->size()));
  ASSERT_TRUE(second_slot_children->Erase(0, second_slot_children->size()));
  ASSERT_TRUE(child_slots->Erase(0, child_slots->size()));

  auto serialized_slots = root->Serialize().GetProperty("childSlots");
  ASSERT_TRUE(serialized_slots.IsArrayOrJSArray());
  ASSERT_EQ(serialized_slots.GetLength(), 2);
  ASSERT_TRUE(serialized_slots.GetProperty(0).IsArrayOrJSArray());
  EXPECT_EQ(serialized_slots.GetProperty(0).GetLength(), 0);
  ASSERT_TRUE(serialized_slots.GetProperty(1).IsArrayOrJSArray());
  ASSERT_EQ(serialized_slots.GetProperty(1).GetLength(), 1);
  EXPECT_EQ(serialized_slots.GetProperty(1)
                .GetProperty(0)
                .GetProperty("uid")
                .Number(),
            7);
}

TEST_P(ElementTemplateInstanceTest,
       SerializeElementTemplatePreservesSlotShapesAndFiltersNonETChildren) {
  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->SetTemplateKey(base::String("root_template"));
  root->SetBundleUrl(base::String("root_bundle.js"));

  auto invalid_slot_children = lepus::CArray::Create();
  invalid_slot_children->emplace_back(lepus::Value(manager->CreateFiberView()));
  invalid_slot_children->emplace_back(lepus::Value(1));

  auto root_child_slots = lepus::CArray::Create();
  root_child_slots->emplace_back(
      lepus::Value(std::move(invalid_slot_children)));
  root_child_slots->emplace_back(lepus::Value("invalid_slot_shape"));
  root->InitializeChildSlots(lepus::Value(std::move(root_child_slots)));

  auto serialized_child_slots = root->Serialize().GetProperty("childSlots");
  ASSERT_TRUE(serialized_child_slots.IsArrayOrJSArray());
  ASSERT_EQ(serialized_child_slots.GetLength(), 2);
  EXPECT_TRUE(serialized_child_slots.GetProperty(0).IsArrayOrJSArray());
  EXPECT_EQ(serialized_child_slots.GetProperty(0).GetLength(), 0);
  EXPECT_EQ(serialized_child_slots.GetProperty(1).StdString(),
            "invalid_slot_shape");
}

TEST_P(ElementTemplateInstanceTest, SerializeTypedElementTemplate) {
  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTypedTag(base::String("raw-text"));
  child->SetUid(lepus::Value(1));

  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(child));
  auto child_slots = lepus::CArray::Create();
  child_slots->emplace_back(lepus::Value(slot_children));

  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->InitializeChildSlots(lepus::Value(child_slots));
  root->SetUid(lepus::Value(2));
  root->SetTypedTag(base::String("view"));
  auto attributes = lepus::Dictionary::Create();
  attributes->SetValue(base::String("data-test"), lepus::Value("root_attr"));
  root->SetRootAttributes(lepus::Value(attributes));

  auto serialized = root->Serialize();
  EXPECT_TRUE(serialized.IsObject());
  EXPECT_FALSE(serialized.GetProperty("templateKey").IsString());
  EXPECT_EQ(serialized.GetProperty("tag").StdString(), "view");
  EXPECT_EQ(serialized.GetProperty("uid").Number(), 2);
  EXPECT_EQ(
      serialized.GetProperty("attributes").GetProperty("data-test").StdString(),
      "root_attr");

  auto serialized_slots = serialized.GetProperty("childSlots");
  ASSERT_TRUE(serialized_slots.IsArrayOrJSArray());
  ASSERT_EQ(serialized_slots.GetLength(), 1);
  auto serialized_child = serialized_slots.GetProperty(0).GetProperty(0);
  EXPECT_EQ(serialized_child.GetProperty("tag").StdString(), "raw-text");
  EXPECT_EQ(serialized_child.GetProperty("uid").Number(), 1);
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

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateInsertNodeIntoChildSlotKeepsOtherSlots) {
  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->SetTemplateKey(base::String("root_template"));

  auto child_in_other_slot = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child_in_other_slot->SetTemplateKey(base::String("other_template"));
  auto child_to_insert = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child_to_insert->SetTemplateKey(base::String("inserted_template"));
  auto ref_node = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  ref_node->SetTemplateKey(base::String("ref_template"));

  auto first_slot_children = lepus::CArray::Create();
  first_slot_children->emplace_back(lepus::Value(child_in_other_slot));
  auto second_slot_children = lepus::CArray::Create();
  second_slot_children->emplace_back(lepus::Value(ref_node));
  auto child_slots = lepus::CArray::Create();
  child_slots->emplace_back(lepus::Value(first_slot_children));
  child_slots->emplace_back(lepus::Value(second_slot_children));
  root->InitializeChildSlots(lepus::Value(child_slots));

  root->InsertNodeIntoChildSlot(1, lepus::Value(child_to_insert),
                                lepus::Value(ref_node));

  auto serialized_slots = root->Serialize().GetProperty("childSlots");
  auto first_serialized_slot = serialized_slots.GetProperty(0);
  ASSERT_EQ(first_serialized_slot.GetLength(), 1);
  EXPECT_EQ(first_serialized_slot.GetProperty(0)
                .GetProperty("templateKey")
                .StdString(),
            "other_template");

  auto second_serialized_slot = serialized_slots.GetProperty(1);
  ASSERT_EQ(second_serialized_slot.GetLength(), 2);
  EXPECT_EQ(second_serialized_slot.GetProperty(0)
                .GetProperty("templateKey")
                .StdString(),
            "inserted_template");
  EXPECT_EQ(second_serialized_slot.GetProperty(1)
                .GetProperty("templateKey")
                .StdString(),
            "ref_template");
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateInsertMovesAcrossSlotsAndParents) {
  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTemplateKey(base::String("child_template"));
  auto first_parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  first_parent->SetTemplateKey(base::String("first_parent"));
  auto second_parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  second_parent->SetTemplateKey(base::String("second_parent"));

  first_parent->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());
  first_parent->InsertNodeIntoChildSlot(1, lepus::Value(child), lepus::Value());

  auto first_parent_slots = first_parent->Serialize().GetProperty("childSlots");
  EXPECT_EQ(first_parent_slots.GetProperty(0).GetLength(), 0);
  ASSERT_EQ(first_parent_slots.GetProperty(1).GetLength(), 1);
  EXPECT_EQ(first_parent_slots.GetProperty(1)
                .GetProperty(0)
                .GetProperty("templateKey")
                .StdString(),
            "child_template");

  second_parent->InsertNodeIntoChildSlot(0, lepus::Value(child),
                                         lepus::Value());

  EXPECT_EQ(first_parent->Serialize()
                .GetProperty("childSlots")
                .GetProperty(1)
                .GetLength(),
            0);
  auto moved_slot =
      second_parent->Serialize().GetProperty("childSlots").GetProperty(0);
  ASSERT_EQ(moved_slot.GetLength(), 1);
  EXPECT_EQ(moved_slot.GetProperty(0).GetProperty("templateKey").StdString(),
            "child_template");

  first_parent->RemoveNodeFromChildSlot(0, lepus::Value(child));
  first_parent->RemoveNodeFromChildSlot(1, lepus::Value(child));
  second_parent->RemoveNodeFromChildSlot(0, lepus::Value(child));
}

TEST_P(ElementTemplateInstanceTest, ElementTemplateSameSlotMovesLogically) {
  auto first = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  first->SetTypedTag(base::String("raw-text"));
  first->SetUid(lepus::Value(9));
  auto second = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  second->SetTypedTag(base::String("raw-text"));
  second->SetUid(lepus::Value(10));
  auto third = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  third->SetTypedTag(base::String("raw-text"));
  third->SetUid(lepus::Value(11));

  auto parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  parent->SetTypedTag(base::String("view"));
  parent->SetUid(lepus::Value(5));
  parent->InsertNodeIntoChildSlot(0, lepus::Value(first), lepus::Value());
  parent->InsertNodeIntoChildSlot(0, lepus::Value(second), lepus::Value());
  parent->InsertNodeIntoChildSlot(0, lepus::Value(third), lepus::Value());

  parent->InsertNodeIntoChildSlot(0, lepus::Value(third), lepus::Value(first));
  parent->InsertNodeIntoChildSlot(0, lepus::Value(first), lepus::Value());

  auto serialized_slot =
      parent->Serialize().GetProperty("childSlots").GetProperty(0);
  ASSERT_EQ(serialized_slot.GetLength(), 3);
  EXPECT_EQ(serialized_slot.GetProperty(0).GetProperty("uid").Number(), 11);
  EXPECT_EQ(serialized_slot.GetProperty(1).GetProperty("uid").Number(), 10);
  EXPECT_EQ(serialized_slot.GetProperty(2).GetProperty("uid").Number(), 9);

  auto before_self_reference = parent->Serialize();
  parent->InsertNodeIntoChildSlot(0, lepus::Value(first), lepus::Value(first));
  EXPECT_EQ(parent->Serialize(), before_self_reference);
}

TEST_P(
    ElementTemplateInstanceTest,
    ElementTemplateInitializationMovesOwnershipAndClearsParentOnDestruction) {
  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTemplateKey(base::String("child_template"));
  auto first_parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));

  auto first_children = lepus::CArray::Create();
  first_children->emplace_back(lepus::Value(child));
  auto first_slots = lepus::CArray::Create();
  first_slots->emplace_back(lepus::Value(first_children));
  first_parent->InitializeChildSlots(lepus::Value(first_slots));

  auto second_parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  auto second_children = lepus::CArray::Create();
  second_children->emplace_back(lepus::Value(child));
  auto second_slots = lepus::CArray::Create();
  second_slots->emplace_back(lepus::Value(second_children));
  second_parent->InitializeChildSlots(lepus::Value(second_slots));

  EXPECT_EQ(first_parent->Serialize()
                .GetProperty("childSlots")
                .GetProperty(0)
                .GetLength(),
            0);
  ASSERT_EQ(second_parent->Serialize()
                .GetProperty("childSlots")
                .GetProperty(0)
                .GetLength(),
            1);

  second_parent = nullptr;
  auto third_parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  third_parent->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());
  ASSERT_EQ(third_parent->Serialize()
                .GetProperty("childSlots")
                .GetProperty(0)
                .GetLength(),
            1);
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateInitializationLeavesDuplicateChildAtLastPosition) {
  auto moving_child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  moving_child->SetTemplateKey(base::String("moving_child"));
  auto first_parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  auto second_parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));

  auto first_children = lepus::CArray::Create();
  first_children->emplace_back(lepus::Value(moving_child));
  auto first_slots = lepus::CArray::Create();
  first_slots->emplace_back(lepus::Value(first_children));
  first_parent->InitializeChildSlots(lepus::Value(first_slots));

  auto duplicate_children = lepus::CArray::Create();
  duplicate_children->emplace_back(lepus::Value(moving_child));
  auto duplicate_children_other_slot = lepus::CArray::Create();
  duplicate_children_other_slot->emplace_back(lepus::Value(moving_child));
  auto duplicate_slots = lepus::CArray::Create();
  duplicate_slots->emplace_back(lepus::Value(duplicate_children));
  duplicate_slots->emplace_back(lepus::Value(duplicate_children_other_slot));

  second_parent->InitializeChildSlots(lepus::Value(duplicate_slots));
  auto first_slot =
      first_parent->Serialize().GetProperty("childSlots").GetProperty(0);
  auto second_slots = second_parent->Serialize().GetProperty("childSlots");
  EXPECT_EQ(first_slot.GetLength(), 0);
  EXPECT_EQ(second_slots.GetProperty(0).GetLength(), 0);
  ASSERT_EQ(second_slots.GetProperty(1).GetLength(), 1);
  EXPECT_EQ(second_slots.GetProperty(1)
                .GetProperty(0)
                .GetProperty("templateKey")
                .StdString(),
            "moving_child");
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateInitializationAppliesNestedOwnershipMovesInOrder) {
  auto nested_child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  nested_child->SetTemplateKey(base::String("nested_child"));
  auto nested_parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  nested_parent->SetTemplateKey(base::String("nested_parent"));
  nested_parent->InsertNodeIntoChildSlot(0, lepus::Value(nested_child),
                                         lepus::Value());

  auto target = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  auto proposed_children = lepus::CArray::Create();
  proposed_children->emplace_back(lepus::Value(nested_parent));
  proposed_children->emplace_back(lepus::Value(nested_child));
  auto proposed_slots = lepus::CArray::Create();
  proposed_slots->emplace_back(lepus::Value(std::move(proposed_children)));

  target->InitializeChildSlots(lepus::Value(proposed_slots));
  auto target_children =
      target->Serialize().GetProperty("childSlots").GetProperty(0);
  ASSERT_EQ(target_children.GetLength(), 2);
  EXPECT_EQ(
      target_children.GetProperty(0).GetProperty("templateKey").StdString(),
      "nested_parent");
  EXPECT_EQ(
      target_children.GetProperty(1).GetProperty("templateKey").StdString(),
      "nested_child");
  EXPECT_EQ(nested_parent->Serialize()
                .GetProperty("childSlots")
                .GetProperty(0)
                .GetLength(),
            0);
}

INSTANTIATE_TEST_SUITE_P(ElementTemplateInstanceTestModule,
                         ElementTemplateInstanceTest,
                         ::testing::ValuesIn(fiber_element_generation_params));

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
