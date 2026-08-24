// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/fiber/element_template_instance.h"

#include <cmath>
#include <limits>
#include <memory>
#include <type_traits>

#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/testing/fiber_element_test.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/template_entry.h"
#include "core/renderer/utils/base/element_template_info.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

namespace {
const lepus::Value* DatasetValue(const Element* element,
                                 const base::String& key) {
  auto it = element->data_model_->dataset().find(key);
  if (it == element->data_model_->dataset().end()) {
    return nullptr;
  }
  return &it->second;
}
}  // namespace

class ElementTemplateInstanceTest : public FiberElementTest {
 protected:
  fml::RefPtr<ElementTemplateInstance> CreateCompiledSpreadInstance() {
    auto entry = std::make_shared<TemplateEntry>();
    entry->SetName(DEFAULT_ENTRY_NAME);
    tasm->template_entries_[DEFAULT_ENTRY_NAME] = entry;

    auto info = std::make_shared<ElementTemplateInfo>();
    info->exist_ = true;
    info->key_ = "spread_template";
    auto root_info = ElementInfo();
    root_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
    root_info.attributes_ =
        std::make_shared<const TemplateAttributes>(TemplateAttributes{
            Attribute{ATTRIBUTE_BINDING_TYPE_SPREAD, base::String("spread"),
                      lepus::Value(), 0}});
    info->elements_.emplace_back(std::move(root_info));
    entry->template_bundle_.element_template_infos_["spread_template"] = info;

    auto instance = fml::AdoptRef<ElementTemplateInstance>(
        new ElementTemplateInstance(manager));
    instance->SetTASM(tasm.get());
    instance->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
    instance->SetTemplateKey(base::String("spread_template"));
    return instance;
  }
};

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
       ElementTemplateEntryPointsUseBaselineStoragePolicy) {
  auto nested = lepus::Dictionary::Create();
  nested->SetValue(base::String("value"), lepus::Value("before"));

  auto attributes = lepus::Dictionary::Create();
  attributes->SetValue(base::String("payload"), lepus::Value(nested));
  auto options = lepus::Dictionary::Create();
  options->SetValue(base::String("payload"), lepus::Value(nested));
  auto attribute_slots = lepus::CArray::Create();
  attribute_slots->emplace_back(lepus::Value(nested));

  auto typed = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  typed->SetTypedTag(base::String("view"));
  typed->SetRootAttributes(lepus::Value(attributes));
  typed->SetOptions(lepus::Value(options));

  auto compiled = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  compiled->SetTemplateKey(base::String("template"));
  compiled->SetAttributeSlots(lepus::Value(attribute_slots));
  compiled->SetAttributeSlot(1, lepus::Value(nested));

  nested->SetValue(base::String("value"), lepus::Value("after"));
  attributes->SetValue(base::String("late"), lepus::Value(true));
  options->SetValue(base::String("late"), lepus::Value(true));
  attribute_slots->set(0, lepus::Value("after"));

  auto typed_serialized = typed->Serialize();
  EXPECT_EQ(typed_serialized.GetProperty("attributes")
                .GetProperty("payload")
                .GetProperty("value")
                .StdString(),
            "before");
  EXPECT_FALSE(typed_serialized.GetProperty("attributes").Contains("late"));
  EXPECT_EQ(typed_serialized.GetProperty("options")
                .GetProperty("payload")
                .GetProperty("value")
                .StdString(),
            "after");
  EXPECT_TRUE(typed_serialized.GetProperty("options").Contains("late"));

  auto serialized_slots = compiled->Serialize().GetProperty("attributeSlots");
  EXPECT_EQ(serialized_slots.GetProperty(0).GetProperty("value").StdString(),
            "before");
  EXPECT_EQ(serialized_slots.GetProperty(1).GetProperty("value").StdString(),
            "before");
}

TEST_P(ElementTemplateInstanceTest,
       TypedElementTemplateAppliesRootAttributesAsSpread) {
  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->SetTypedTag(base::String("view"));

  auto initial_attributes = lepus::Dictionary::Create();
  initial_attributes->SetValue(base::String("data-test"),
                               lepus::Value("before"));
  initial_attributes->SetValue(base::String("data-stale"),
                               lepus::Value("stale"));
  root->SetRootAttributes(lepus::Value(initial_attributes));
  EXPECT_EQ(root->result_, nullptr);

  auto updated_attributes = lepus::Dictionary::Create();
  updated_attributes->SetValue(base::String("data-test"),
                               lepus::Value("after"));
  updated_attributes->SetValue(base::String("data-added"),
                               lepus::Value("added"));
  updated_attributes->SetValue(base::String("bindtap"), lepus::Value("onTap"));
  root->SetRootAttributes(lepus::Value(updated_attributes));
  EXPECT_EQ(root->result_, nullptr);

  auto resolved = root->GetRoot();
  ASSERT_NE(resolved, nullptr);
  ASSERT_NE(DatasetValue(resolved.get(), "test"), nullptr);
  EXPECT_EQ(DatasetValue(resolved.get(), "test")->StdString(), "after");
  ASSERT_NE(DatasetValue(resolved.get(), "added"), nullptr);
  EXPECT_EQ(DatasetValue(resolved.get(), "added")->StdString(), "added");
  EXPECT_EQ(DatasetValue(resolved.get(), "stale"), nullptr);
  EXPECT_EQ(resolved->event_map().count("tap"), 1u);

  auto reset_attributes = lepus::Dictionary::Create();
  reset_attributes->SetValue(base::String("data-added"),
                             lepus::Value("updated"));
  root->SetRootAttributes(lepus::Value(reset_attributes));

  EXPECT_EQ(DatasetValue(resolved.get(), "test"), nullptr);
  ASSERT_NE(DatasetValue(resolved.get(), "added"), nullptr);
  EXPECT_EQ(DatasetValue(resolved.get(), "added")->StdString(), "updated");
  EXPECT_EQ(resolved->event_map().count("tap"), 0u);

  root->SetRootAttributes(lepus::Value(lepus::Dictionary::Create()));
  EXPECT_EQ(DatasetValue(resolved.get(), "added"), nullptr);
  EXPECT_TRUE(root->Serialize().GetProperty("attributes").IsEmpty());
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

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateInstanceMaterializesCompiledRoot) {
  auto default_entry = std::make_shared<TemplateEntry>();
  default_entry->SetName(DEFAULT_ENTRY_NAME);
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = default_entry;

  auto template_info = std::make_shared<ElementTemplateInfo>();
  template_info->exist_ = true;
  template_info->key_ = "root_template";

  auto root_info = ElementInfo();
  root_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  root_info.attributes_ =
      std::make_shared<const TemplateAttributes>(TemplateAttributes{
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("data-test"),
                    lepus::Value(), 0}});
  auto sentinel_info = ElementInfo();
  sentinel_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  sentinel_info.attrs_[base::String("id")] = lepus::Value("sentinel");
  root_info.children_.emplace_back(std::move(sentinel_info));
  template_info->elements_.emplace_back(std::move(root_info));
  default_entry->template_bundle_.element_template_infos_["root_template"] =
      std::move(template_info);

  auto attribute_slots = lepus::CArray::Create();
  attribute_slots->emplace_back(lepus::Value("compiled-value"));

  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->SetTASM(tasm.get());
  root->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  root->SetTemplateKey(base::String("root_template"));
  root->SetAttributeSlots(lepus::Value(attribute_slots));

  EXPECT_EQ(root->PeekMaterializedRoot(), nullptr);

  auto resolved = root->GetRoot();
  ASSERT_NE(resolved, nullptr);
  EXPECT_TRUE(resolved->is_view());
  EXPECT_TRUE(resolved->IsTemplateElement());
  auto* test_data = DatasetValue(resolved.get(), "test");
  ASSERT_NE(test_data, nullptr);
  EXPECT_EQ(test_data->StdString(), "compiled-value");
  ASSERT_EQ(resolved->children().size(), 1u);
  auto* sentinel = static_cast<Element*>(resolved->children()[0].get());
  ASSERT_NE(sentinel, nullptr);
  EXPECT_TRUE(sentinel->is_view());
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateStaticEventsSyncAfterAttach) {
  manager->config_->SetEnableEventHandleRefactor(true);
  manager->SetConfig(manager->config_);
  tasm->page_config_ = manager->config_;

  auto default_entry = std::make_shared<TemplateEntry>();
  default_entry->SetName(DEFAULT_ENTRY_NAME);
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = default_entry;

  auto template_info = std::make_shared<ElementTemplateInfo>();
  template_info->exist_ = true;
  template_info->key_ = "event_template";

  auto target_info = ElementInfo();
  target_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  target_info.attributes_ =
      std::make_shared<const TemplateAttributes>(TemplateAttributes{
          Attribute{ATTRIBUTE_BINDING_TYPE_STATIC, base::String("bindtap"),
                    lepus::Value("onStaticTap"), 0}});
  template_info->elements_.emplace_back(std::move(target_info));
  default_entry->template_bundle_.element_template_infos_["event_template"] =
      std::move(template_info);

  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->SetTASM(tasm.get());
  root->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  root->SetTemplateKey(base::String("event_template"));

  auto resolved = root->GetRoot();

  ASSERT_NE(resolved, nullptr);
  EXPECT_EQ(resolved->element_manager(), manager);
  auto* listeners = resolved->GetEventListenerMap()->Find("tap");
  ASSERT_NE(listeners, nullptr);
  ASSERT_EQ(listeners->size(), 1u);
  EXPECT_FALSE(listeners->front()->GetOptions().IsCapture());
  EXPECT_FALSE(listeners->front()->GetOptions().IsCatch());
}

TEST_P(ElementTemplateInstanceTest,
       CompiledAttributesUpdatedAfterPrepareUseLatestState) {
  auto instance = CreateCompiledSpreadInstance();
  auto initial_slot = lepus::Dictionary::Create();
  initial_slot->SetValue(base::String("data-slot-stale"), lepus::Value("old"));
  instance->SetAttributeSlot(0, lepus::Value(initial_slot));
  auto initial_root = lepus::Dictionary::Create();
  initial_root->SetValue(base::String("data-root-stale"), lepus::Value("old"));
  instance->SetRootAttributes(lepus::Value(initial_root));

  // Finish detached preparation before updating either logical snapshot.
  instance->RequestMaterializationRecursively();
  instance->create_element_tree_task_->Run();

  auto latest_slot = lepus::Dictionary::Create();
  latest_slot->SetValue(base::String("data-slot-current"), lepus::Value("new"));
  instance->SetAttributeSlot(0, lepus::Value(latest_slot));
  auto latest_root = lepus::Dictionary::Create();
  latest_root->SetValue(base::String("data-root-current"), lepus::Value("new"));
  instance->SetRootAttributes(lepus::Value(latest_root));

  auto root = instance->GetRoot();
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(DatasetValue(root.get(), "slot-stale"), nullptr);
  EXPECT_EQ(DatasetValue(root.get(), "root-stale"), nullptr);
  ASSERT_NE(DatasetValue(root.get(), "slot-current"), nullptr);
  ASSERT_NE(DatasetValue(root.get(), "root-current"), nullptr);
  EXPECT_EQ(DatasetValue(root.get(), "slot-current")->StdString(), "new");
  EXPECT_EQ(DatasetValue(root.get(), "root-current")->StdString(), "new");
}

TEST_P(ElementTemplateInstanceTest,
       MaterializedCompiledSpreadClearsRemovedKeys) {
  auto instance = CreateCompiledSpreadInstance();
  auto initial = lepus::Dictionary::Create();
  initial->SetValue(base::String("data-stale"), lepus::Value("old"));
  initial->SetValue(base::String("data-current"), lepus::Value("before"));
  instance->SetAttributeSlot(0, lepus::Value(initial));
  auto root_attributes = lepus::Dictionary::Create();
  root_attributes->SetValue(base::String("data-root"), lepus::Value("root"));
  instance->SetRootAttributes(lepus::Value(root_attributes));

  auto root = instance->GetRoot();
  ASSERT_NE(root, nullptr);
  ASSERT_NE(DatasetValue(root.get(), "stale"), nullptr);
  ASSERT_NE(DatasetValue(root.get(), "current"), nullptr);
  EXPECT_EQ(DatasetValue(root.get(), "current")->StdString(), "before");

  auto updated = lepus::Dictionary::Create();
  updated->SetValue(base::String("data-current"), lepus::Value("after"));
  instance->SetAttributeSlot(0, lepus::Value(updated));

  EXPECT_EQ(DatasetValue(root.get(), "stale"), nullptr);
  ASSERT_NE(DatasetValue(root.get(), "current"), nullptr);
  EXPECT_EQ(DatasetValue(root.get(), "current")->StdString(), "after");
  ASSERT_NE(DatasetValue(root.get(), "root"), nullptr);
  EXPECT_EQ(DatasetValue(root.get(), "root")->StdString(), "root");

  instance->SetAttributeSlot(0, lepus::Value(lepus::Dictionary::Create()));
  EXPECT_EQ(DatasetValue(root.get(), "current"), nullptr);
  EXPECT_NE(DatasetValue(root.get(), "root"), nullptr);
}

INSTANTIATE_TEST_SUITE_P(ElementTemplateInstanceTestModule,
                         ElementTemplateInstanceTest,
                         ::testing::ValuesIn(fiber_element_generation_params));

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
