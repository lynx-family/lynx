// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/fiber/element_template_instance.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <type_traits>
#include <vector>

#include "core/public/pipeline_option.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/list_item_scheduler_adapter.h"
#include "core/renderer/dom/fiber/page_element.h"
#include "core/renderer/dom/fiber/tree_resolver.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/testing/fiber_element_test.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/template_entry.h"
#include "core/renderer/utils/base/element_template_info.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/runtime/lepus/bytecode_generator.h"
#include "core/shell/runtime/mts/mts_runtime.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

namespace {

class RecordingInspectorElementObserver final
    : public InspectorElementObserver {
 public:
  void OnDocumentUpdated() override {}
  void OnElementNodeAdded(Element* ptr) override {
    added_nodes.push_back(ptr);
    added_node_parents.push_back(ptr != nullptr ? ptr->parent() : nullptr);
  }
  void OnElementNodeRemoved(Element* ptr) override {
    removed_nodes.push_back(ptr);
    removed_node_parents.push_back(ptr != nullptr ? ptr->parent() : nullptr);
  }
  void OnCharacterDataModified(Element* ptr) override {}
  void OnElementDataModelSet(Element* ptr) override {}
  void OnElementManagerWillDestroy() override {}
  void OnCSSStyleSheetAdded(Element* ptr) override {}
  void OnComponentUselessUpdate(const std::string& component_name,
                                const lepus::Value& properties) override {}
  void OnSetNativeProps(Element* ptr, const std::string& name,
                        const std::string& value, bool is_style) override {}
  void OnCSSMediaQueryResultChanged() override {}

  std::map<lynx::devtool::DevToolFunction,
           std::function<void(const base::any&)>>
  GetDevToolFunction() override {
    auto noop = [](const base::any&) {};
    return {
        {lynx::devtool::DevToolFunction::InitForInspector, noop},
        {lynx::devtool::DevToolFunction::InitPlugForInspector, noop},
        {lynx::devtool::DevToolFunction::InitStyleValueElement, noop},
        {lynx::devtool::DevToolFunction::InitStyleRoot, noop},
        {lynx::devtool::DevToolFunction::SetDocElement, noop},
        {lynx::devtool::DevToolFunction::SetStyleValueElement, noop},
        {lynx::devtool::DevToolFunction::SetStyleRoot, noop},
    };
  }

  std::vector<Element*> added_nodes;
  std::vector<Element*> added_node_parents;
  std::vector<Element*> removed_nodes;
  std::vector<Element*> removed_node_parents;
};

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

TEST_P(
    ElementTemplateInstanceTest,
    ElementTemplateInitializationPreservesSlotShapesAndFiltersNonETChildren) {
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

  auto serialized = root->Serialize();
  EXPECT_TRUE(serialized.IsObject());

  auto serialized_child_slots = serialized.GetProperty("childSlots");
  EXPECT_TRUE(serialized_child_slots.IsArrayOrJSArray());
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
  root->SetAttributes(lepus::Value(attributes));

  auto serialized = root->Serialize();
  EXPECT_TRUE(serialized.IsObject());
  EXPECT_FALSE(serialized.GetProperty("templateKey").IsString());
  EXPECT_FALSE(serialized.Contains("attributeSlots"));
  EXPECT_EQ(serialized.GetProperty("tag").StdString(), "view");
  EXPECT_EQ(serialized.GetProperty("uid").Number(), 2);
  EXPECT_EQ(
      serialized.GetProperty("attributes").GetProperty("data-test").StdString(),
      "root_attr");

  auto serialized_slots = serialized.GetProperty("childSlots");
  EXPECT_TRUE(serialized_slots.IsArrayOrJSArray());
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
       MovingMaterializedChildToSparseMountPointHoleDetachesSource) {
  auto default_entry = std::make_shared<TemplateEntry>();
  default_entry->SetName(DEFAULT_ENTRY_NAME);
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = default_entry;

  auto template_info = std::make_shared<ElementTemplateInfo>();
  template_info->exist_ = true;
  template_info->key_ = "sparse_slot";
  auto root_info = ElementInfo();
  root_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  auto child_slot_info = ElementInfo();
  child_slot_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_SLOT;
  child_slot_info.slot_index_ = 2;
  root_info.children_.emplace_back(std::move(child_slot_info));
  template_info->elements_.emplace_back(std::move(root_info));
  default_entry->template_bundle_.element_template_infos_["sparse_slot"] =
      std::move(template_info);

  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTypedTag(base::String("raw-text"));
  auto source = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  source->SetTypedTag(base::String("view"));
  source->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());
  auto source_root = source->GetRoot();
  auto child_root = child->PeekMaterializedRoot();
  ASSERT_NE(source_root, nullptr);
  ASSERT_NE(child_root, nullptr);
  ASSERT_EQ(child_root->parent(), source_root.get());

  auto destination = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  destination->SetTASM(tasm.get());
  destination->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  destination->SetTemplateKey(base::String("sparse_slot"));
  ASSERT_NE(destination->GetRoot(), nullptr);

  destination->InsertNodeIntoChildSlot(1, lepus::Value(child), lepus::Value());

  EXPECT_EQ(child_root->parent(), nullptr);
  EXPECT_TRUE(source_root->children().empty());
  EXPECT_EQ(destination->Serialize()
                .GetProperty("childSlots")
                .GetProperty(1)
                .GetLength(),
            1);

  destination->RemoveNodeFromChildSlot(1, lepus::Value(child));

  EXPECT_EQ(child_root->parent(), nullptr);
  EXPECT_EQ(destination->Serialize()
                .GetProperty("childSlots")
                .GetProperty(1)
                .GetLength(),
            0);
}

TEST_P(ElementTemplateInstanceTest,
       MaterializedElementTemplatesBoundPartsAndTemplateScopeClones) {
  auto part = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  part->SetTypedTag(base::String("view"));
  part->SetUid(lepus::Value(3));

  auto inner = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  inner->SetTypedTag(base::String("view"));
  inner->SetUid(lepus::Value(4));
  inner->InsertNodeIntoChildSlot(0, lepus::Value(part), lepus::Value());

  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->SetUid(lepus::Value(0));
  page->InsertNodeIntoChildSlot(0, lepus::Value(inner), lepus::Value());

  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);
  auto inner_root = inner->PeekMaterializedRoot();
  auto part_root = part->PeekMaterializedRoot();
  ASSERT_NE(inner_root, nullptr);
  ASSERT_NE(part_root, nullptr);
  ASSERT_TRUE(page_root->IsTemplateElement());
  ASSERT_TRUE(inner_root->IsTemplateElement());
  ASSERT_TRUE(part_root->IsTemplateElement());
  part_root->MarkPartElement(base::String("inner-part"));

  auto page_parts = TreeResolver::GetTemplateParts(page_root);
  EXPECT_FALSE(page_parts->GetValueOrNull("inner-part").has_value());
  auto inner_parts = TreeResolver::GetTemplateParts(inner_root);
  auto inner_part = inner_parts->GetValueOrNull("inner-part");
  ASSERT_TRUE(inner_part.has_value());
  ASSERT_TRUE(inner_part->IsRefCounted());
  EXPECT_EQ(inner_part->RefCounted().get(), part_root.get());

  auto scope_clone = TreeResolver::CloneElements(
      page_root, tasm->style_sheet_manager(DEFAULT_ENTRY_NAME), false,
      TreeResolver::CloningDepth::kTemplateScope);
  ASSERT_NE(scope_clone, nullptr);
  EXPECT_TRUE(scope_clone->children().empty());
}

TEST_P(ElementTemplateInstanceTest,
       DetachedMaterializedElementTemplateUpdatesAndReattachesDirectly) {
  auto initial_attributes = lepus::Dictionary::Create();
  initial_attributes->SetValue(base::String("data-state"),
                               lepus::Value("initial"));
  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTypedTag(base::String("view"));
  child->SetUid(lepus::Value(1));
  child->SetAttributes(lepus::Value(initial_attributes));
  auto parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  parent->SetTypedTag(base::String("view"));
  parent->SetUid(lepus::Value(5));
  parent->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());
  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->SetUid(lepus::Value(0));
  page->InsertNodeIntoChildSlot(0, lepus::Value(parent), lepus::Value());

  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);
  auto parent_root = parent->PeekMaterializedRoot();
  auto child_root = child->PeekMaterializedRoot();
  ASSERT_NE(parent_root, nullptr);
  ASSERT_NE(child_root, nullptr);
  ASSERT_EQ(DatasetValue(child_root.get(), "state")->StdString(), "initial");

  parent->RemoveNodeFromChildSlot(0, lepus::Value(child));
  ASSERT_EQ(child_root->parent(), nullptr);

  auto updated_attributes = lepus::Dictionary::Create();
  updated_attributes->SetValue(base::String("data-state"),
                               lepus::Value("reattached"));
  child->SetAttributes(lepus::Value(updated_attributes));
  ASSERT_EQ(DatasetValue(child_root.get(), "state")->StdString(), "reattached");

  parent->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());

  ASSERT_EQ(child_root->parent(), parent_root.get());
  ASSERT_NE(DatasetValue(child_root.get(), "state"), nullptr);
  EXPECT_EQ(DatasetValue(child_root.get(), "state")->StdString(), "reattached");
}

TEST_P(ElementTemplateInstanceTest,
       TypedPageRemoveAndInsertExposeBothDirectMutations) {
  auto observer = std::make_shared<RecordingInspectorElementObserver>();
  manager->SetInspectorElementObserver(observer);
  manager->dom_tree_enabled_ = true;

  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTypedTag(base::String("view"));
  child->SetUid(lepus::Value(1));

  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->SetUid(lepus::Value(0));
  page->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());

  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);
  auto child_root = child->PeekMaterializedRoot();
  ASSERT_NE(child_root, nullptr);
  ASSERT_EQ(page_root->children().size(), 1u);
  EXPECT_EQ(page_root->children()[0].get(), child_root.get());

  observer->removed_nodes.clear();
  observer->removed_node_parents.clear();
  observer->added_nodes.clear();
  observer->added_node_parents.clear();

  page->RemoveNodeFromChildSlot(0, lepus::Value(child));
  page->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());

  ASSERT_EQ(page_root->children().size(), 1u);
  EXPECT_EQ(page_root->children()[0].get(), child_root.get());
  if (ENABLE_INSPECTOR) {
    ASSERT_EQ(observer->removed_nodes.size(), 1u);
    EXPECT_EQ(observer->removed_nodes[0], child_root.get());
    EXPECT_EQ(observer->removed_node_parents[0], page_root.get());
    ASSERT_EQ(observer->added_nodes.size(), 1u);
    EXPECT_EQ(observer->added_nodes[0], child_root.get());
    EXPECT_EQ(observer->added_node_parents[0], page_root.get());
  }
}

TEST_P(ElementTemplateInstanceTest,
       TypedElementTemplateAppliesAttributesAsSpread) {
  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->SetTypedTag(base::String("view"));

  auto initial_attributes = lepus::Dictionary::Create();
  initial_attributes->SetValue(base::String("data-test"),
                               lepus::Value("before"));
  initial_attributes->SetValue(base::String("data-stale"),
                               lepus::Value("stale"));
  root->SetAttributes(lepus::Value(initial_attributes));
  EXPECT_EQ(root->PeekMaterializedRoot(), nullptr);

  auto updated_attributes = lepus::Dictionary::Create();
  updated_attributes->SetValue(base::String("data-test"),
                               lepus::Value("after"));
  updated_attributes->SetValue(base::String("data-added"),
                               lepus::Value("added"));
  updated_attributes->SetValue(base::String("bindtap"), lepus::Value("onTap"));
  root->SetAttributes(lepus::Value(updated_attributes));
  EXPECT_EQ(root->PeekMaterializedRoot(), nullptr);

  auto serialized_before_resolve = root->Serialize();
  EXPECT_EQ(serialized_before_resolve.GetProperty("attributes")
                .GetProperty("data-test")
                .StdString(),
            "after");
  EXPECT_EQ(serialized_before_resolve.GetProperty("attributes")
                .GetProperty("data-added")
                .StdString(),
            "added");
  EXPECT_FALSE(serialized_before_resolve.GetProperty("attributes")
                   .Contains(base::String("data-stale")));

  auto resolved = root->GetRoot();
  ASSERT_NE(resolved, nullptr);
  auto* test_data = DatasetValue(resolved.get(), "test");
  ASSERT_NE(test_data, nullptr);
  EXPECT_EQ(test_data->StdString(), "after");
  auto* added_data = DatasetValue(resolved.get(), "added");
  ASSERT_NE(added_data, nullptr);
  EXPECT_EQ(added_data->StdString(), "added");
  EXPECT_EQ(DatasetValue(resolved.get(), "stale"), nullptr);
  EXPECT_EQ(resolved->data_model_->attributes().count("data-test"), 0u);
  EXPECT_EQ(resolved->event_map().count("tap"), 1u);

  auto reset_attributes = lepus::Dictionary::Create();
  reset_attributes->SetValue(base::String("data-added"),
                             lepus::Value("updated"));
  root->SetAttributeSlot(0, lepus::Value(reset_attributes));

  EXPECT_EQ(DatasetValue(resolved.get(), "test"), nullptr);
  added_data = DatasetValue(resolved.get(), "added");
  ASSERT_NE(added_data, nullptr);
  EXPECT_EQ(added_data->StdString(), "updated");
  EXPECT_EQ(resolved->event_map().count("tap"), 0u);

  root->SetAttributes(lepus::Value(lepus::Dictionary::Create()));
  EXPECT_EQ(DatasetValue(resolved.get(), "added"), nullptr);
  EXPECT_TRUE(root->Serialize().GetProperty("attributes").IsEmpty());

  root->SetAttributes(lepus::Value(updated_attributes));
  root->SetAttributes(lepus::Value());
  EXPECT_EQ(DatasetValue(resolved.get(), "test"), nullptr);
  EXPECT_EQ(DatasetValue(resolved.get(), "added"), nullptr);
  EXPECT_EQ(resolved->event_map().count("tap"), 0u);
  EXPECT_FALSE(root->Serialize().Contains("attributes"));
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
  typed->SetAttributes(lepus::Value(attributes));
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

  auto slot_info = ElementInfo();
  slot_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_SLOT;
  slot_info.slot_index_ = 0;
  root_info.children_.emplace_back(std::move(slot_info));

  auto sentinel_info = ElementInfo();
  sentinel_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  sentinel_info.attrs_[base::String("id")] = lepus::Value("sentinel");
  root_info.children_.emplace_back(std::move(sentinel_info));

  template_info->elements_.emplace_back(std::move(root_info));
  default_entry->template_bundle_.element_template_infos_["root_template"] =
      std::move(template_info);

  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTypedTag(base::String("raw-text"));
  child->SetUid(lepus::Value(1));

  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(child));
  auto child_slots = lepus::CArray::Create();
  child_slots->emplace_back(lepus::Value(slot_children));

  auto attribute_slots = lepus::CArray::Create();
  attribute_slots->emplace_back(lepus::Value("compiled-value"));

  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->SetTASM(tasm.get());
  root->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  root->SetTemplateKey(base::String("root_template"));
  root->SetAttributeSlots(lepus::Value(attribute_slots));
  root->InitializeChildSlots(lepus::Value(child_slots));

  EXPECT_EQ(root->PeekMaterializedRoot(), nullptr);

  auto resolved = root->GetRoot();
  ASSERT_NE(resolved, nullptr);
  EXPECT_TRUE(resolved->is_view());
  EXPECT_TRUE(resolved->IsTemplateElement());
  auto* test_data = DatasetValue(resolved.get(), "test");
  ASSERT_NE(test_data, nullptr);
  EXPECT_EQ(test_data->StdString(), "compiled-value");

  auto child_root = child->PeekMaterializedRoot();
  ASSERT_NE(child_root, nullptr);
  ASSERT_EQ(resolved->children().size(), 2u);
  EXPECT_EQ(resolved->children()[0].get(), child_root.get());
  EXPECT_TRUE(child_root->is_raw_text());
  EXPECT_TRUE(child_root->IsTemplateElement());
  auto* sentinel = static_cast<Element*>(resolved->children()[1].get());
  ASSERT_NE(sentinel, nullptr);
  EXPECT_TRUE(sentinel->is_view());

  auto serialized = root->Serialize();
  EXPECT_EQ(serialized.GetProperty("childSlots")
                .GetProperty(0)
                .GetProperty(0)
                .GetProperty("uid")
                .Number(),
            1);
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateMaterializedUpdatesApplyDirectly) {
  auto page = manager->CreateFiberPage("page", 0);
  manager->SetFiberPageElement(page);

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

  auto slot_info = ElementInfo();
  slot_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_SLOT;
  slot_info.slot_index_ = 0;
  root_info.children_.emplace_back(std::move(slot_info));

  auto sentinel_info = ElementInfo();
  sentinel_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  sentinel_info.attrs_[base::String("id")] = lepus::Value("sentinel");
  root_info.children_.emplace_back(std::move(sentinel_info));

  template_info->elements_.emplace_back(std::move(root_info));
  default_entry->template_bundle_.element_template_infos_["root_template"] =
      std::move(template_info);

  auto first = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  first->SetTypedTag(base::String("raw-text"));
  first->SetUid(lepus::Value(9));

  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(first));
  auto child_slots = lepus::CArray::Create();
  child_slots->emplace_back(lepus::Value(slot_children));

  auto attribute_slots = lepus::CArray::Create();
  attribute_slots->emplace_back(lepus::Value("initial-value"));

  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->SetTASM(tasm.get());
  root->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  root->SetTemplateKey(base::String("root_template"));
  root->SetAttributeSlots(lepus::Value(attribute_slots));
  root->InitializeChildSlots(lepus::Value(child_slots));

  auto resolved = root->GetRoot();
  ASSERT_NE(resolved, nullptr);
  auto first_root = first->PeekMaterializedRoot();
  ASSERT_NE(first_root, nullptr);
  ASSERT_EQ(resolved->children().size(), 2u);
  EXPECT_EQ(resolved->children()[0].get(), first_root.get());
  auto* test_data = DatasetValue(resolved.get(), "test");
  ASSERT_NE(test_data, nullptr);
  EXPECT_EQ(test_data->StdString(), "initial-value");

  auto second = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  second->SetTypedTag(base::String("raw-text"));
  second->SetUid(lepus::Value(10));
  auto third = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  third->SetTypedTag(base::String("raw-text"));
  third->SetUid(lepus::Value(11));
  auto fourth = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  fourth->SetTypedTag(base::String("raw-text"));
  fourth->SetUid(lepus::Value(12));

  root->SetAttributeSlot(0, lepus::Value("updated-value"));
  root->InsertNodeIntoChildSlot(0, lepus::Value(second), lepus::Value(first));
  root->InsertNodeIntoChildSlot(0, lepus::Value(third), lepus::Value(first));
  root->RemoveNodeFromChildSlot(0, lepus::Value(first));
  root->RemoveNodeFromChildSlot(0, lepus::Value(second));
  root->InsertNodeIntoChildSlot(0, lepus::Value(fourth), lepus::Value(third));

  EXPECT_FALSE(root->HasPendingChildMounts());
  test_data = DatasetValue(resolved.get(), "test");
  ASSERT_NE(test_data, nullptr);
  EXPECT_EQ(test_data->StdString(), "updated-value");
  auto fourth_root = fourth->PeekMaterializedRoot();
  ASSERT_NE(fourth_root, nullptr);
  auto third_root = third->PeekMaterializedRoot();
  ASSERT_NE(third_root, nullptr);
  ASSERT_EQ(resolved->children().size(), 3u);
  EXPECT_EQ(resolved->children()[0].get(), fourth_root.get());
  EXPECT_EQ(resolved->children()[1].get(), third_root.get());
  EXPECT_TRUE(fourth_root->is_raw_text());
  EXPECT_TRUE(fourth_root->IsTemplateElement());
  EXPECT_TRUE(third_root->is_raw_text());
  EXPECT_TRUE(third_root->IsTemplateElement());
  EXPECT_EQ(first_root->parent(), nullptr);
  auto second_root = second->PeekMaterializedRoot();
  ASSERT_NE(second_root, nullptr);
  EXPECT_EQ(second_root->parent(), nullptr);

  auto serialized = root->Serialize();
  auto serialized_slot_children =
      serialized.GetProperty("childSlots").GetProperty(0);
  ASSERT_TRUE(serialized_slot_children.IsArrayOrJSArray());
  ASSERT_EQ(serialized_slot_children.GetLength(), 2);
  EXPECT_EQ(serialized_slot_children.GetProperty(0).GetProperty("uid").Number(),
            12);
  EXPECT_EQ(serialized_slot_children.GetProperty(1).GetProperty("uid").Number(),
            11);
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

  auto serialized = root->Serialize();
  auto serialized_slots = serialized.GetProperty("childSlots");
  ASSERT_TRUE(serialized_slots.IsArrayOrJSArray());
  auto first_serialized_slot = serialized_slots.GetProperty(0);
  ASSERT_TRUE(first_serialized_slot.IsArrayOrJSArray());
  ASSERT_EQ(first_serialized_slot.GetLength(), 1);
  EXPECT_EQ(first_serialized_slot.GetProperty(0)
                .GetProperty("templateKey")
                .StdString(),
            "other_template");

  auto second_serialized_slot = serialized_slots.GetProperty(1);
  ASSERT_TRUE(second_serialized_slot.IsArrayOrJSArray());
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
  auto first_slot = first_parent_slots.GetProperty(0);
  auto second_slot = first_parent_slots.GetProperty(1);
  ASSERT_TRUE(first_slot.IsArrayOrJSArray());
  EXPECT_EQ(first_slot.GetLength(), 0);
  ASSERT_TRUE(second_slot.IsArrayOrJSArray());
  ASSERT_EQ(second_slot.GetLength(), 1);
  EXPECT_EQ(second_slot.GetProperty(0).GetProperty("templateKey").StdString(),
            "child_template");

  second_parent->InsertNodeIntoChildSlot(0, lepus::Value(child),
                                         lepus::Value());

  second_slot =
      first_parent->Serialize().GetProperty("childSlots").GetProperty(1);
  auto moved_slot =
      second_parent->Serialize().GetProperty("childSlots").GetProperty(0);
  ASSERT_TRUE(second_slot.IsArrayOrJSArray());
  EXPECT_EQ(second_slot.GetLength(), 0);
  ASSERT_TRUE(moved_slot.IsArrayOrJSArray());
  ASSERT_EQ(moved_slot.GetLength(), 1);
  EXPECT_EQ(moved_slot.GetProperty(0).GetProperty("templateKey").StdString(),
            "child_template");
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateSameSlotMovesConvergeBeforeAndAfterMaterialization) {
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
  EXPECT_EQ(parent->PeekMaterializedRoot(), nullptr);

  auto serialized_slot =
      parent->Serialize().GetProperty("childSlots").GetProperty(0);
  ASSERT_EQ(serialized_slot.GetLength(), 3);
  EXPECT_EQ(serialized_slot.GetProperty(0).GetProperty("uid").Number(), 11);
  EXPECT_EQ(serialized_slot.GetProperty(1).GetProperty("uid").Number(), 10);
  EXPECT_EQ(serialized_slot.GetProperty(2).GetProperty("uid").Number(), 9);

  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->SetUid(lepus::Value(0));
  page->InsertNodeIntoChildSlot(0, lepus::Value(parent), lepus::Value());

  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);
  auto parent_root = parent->PeekMaterializedRoot();
  auto first_root = first->PeekMaterializedRoot();
  auto second_root = second->PeekMaterializedRoot();
  auto third_root = third->PeekMaterializedRoot();
  ASSERT_NE(parent_root, nullptr);
  ASSERT_NE(first_root, nullptr);
  ASSERT_NE(second_root, nullptr);
  ASSERT_NE(third_root, nullptr);
  ASSERT_EQ(parent_root->children().size(), 3u);
  EXPECT_EQ(parent_root->children()[0].get(), third_root.get());
  EXPECT_EQ(parent_root->children()[1].get(), second_root.get());
  EXPECT_EQ(parent_root->children()[2].get(), first_root.get());

  parent->InsertNodeIntoChildSlot(0, lepus::Value(first), lepus::Value(third));
  parent->InsertNodeIntoChildSlot(0, lepus::Value(second), lepus::Value(first));

  ASSERT_EQ(parent_root->children().size(), 3u);
  EXPECT_EQ(parent_root->children()[0].get(), second_root.get());
  EXPECT_EQ(parent_root->children()[1].get(), first_root.get());
  EXPECT_EQ(parent_root->children()[2].get(), third_root.get());

  serialized_slot =
      parent->Serialize().GetProperty("childSlots").GetProperty(0);
  ASSERT_EQ(serialized_slot.GetLength(), 3);
  EXPECT_EQ(serialized_slot.GetProperty(0).GetProperty("uid").Number(), 10);
  EXPECT_EQ(serialized_slot.GetProperty(1).GetProperty("uid").Number(), 9);
  EXPECT_EQ(serialized_slot.GetProperty(2).GetProperty("uid").Number(), 11);

  auto before_self_reference = parent->Serialize();
  parent->InsertNodeIntoChildSlot(0, lepus::Value(first), lepus::Value(first));
  EXPECT_FALSE(parent->HasPendingChildMounts());
  EXPECT_EQ(parent->Serialize(), before_self_reference);
  ASSERT_EQ(parent_root->children().size(), 3u);
  EXPECT_EQ(parent_root->children()[0].get(), second_root.get());
  EXPECT_EQ(parent_root->children()[1].get(), first_root.get());
  EXPECT_EQ(parent_root->children()[2].get(), third_root.get());
}

TEST_P(
    ElementTemplateInstanceTest,
    ElementTemplateInitializationMovesOwnershipAndClearsParentOnDestruction) {
  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTemplateKey(base::String("child_template"));
  auto first_parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  first_parent->SetTemplateKey(base::String("first_parent"));

  auto first_children = lepus::CArray::Create();
  first_children->emplace_back(lepus::Value(child));
  auto first_slots = lepus::CArray::Create();
  first_slots->emplace_back(lepus::Value(first_children));
  first_parent->InitializeChildSlots(lepus::Value(first_slots));
  EXPECT_EQ(first_parent->Serialize()
                .GetProperty("childSlots")
                .GetProperty(0)
                .GetProperty(0)
                .GetProperty("templateKey")
                .StdString(),
            "child_template");

  auto second_parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  second_parent->SetTemplateKey(base::String("second_parent"));
  auto second_children = lepus::CArray::Create();
  second_children->emplace_back(lepus::Value(child));
  auto second_slots = lepus::CArray::Create();
  second_slots->emplace_back(lepus::Value(second_children));
  second_parent->InitializeChildSlots(lepus::Value(second_slots));

  auto detached_slot =
      first_parent->Serialize().GetProperty("childSlots").GetProperty(0);
  ASSERT_TRUE(detached_slot.IsArrayOrJSArray());
  EXPECT_EQ(detached_slot.GetLength(), 0);
  EXPECT_EQ(second_parent->Serialize()
                .GetProperty("childSlots")
                .GetProperty(0)
                .GetProperty(0)
                .GetProperty("templateKey")
                .StdString(),
            "child_template");

  second_parent = nullptr;
  auto third_parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  third_parent->SetTemplateKey(base::String("third_parent"));
  third_parent->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());
  EXPECT_EQ(third_parent->Serialize()
                .GetProperty("childSlots")
                .GetProperty(0)
                .GetProperty(0)
                .GetProperty("templateKey")
                .StdString(),
            "child_template");
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateInitializationLeavesDuplicateChildAtLastPosition) {
  auto moving_child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  moving_child->SetTemplateKey(base::String("moving_child"));
  auto first_parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  first_parent->SetTemplateKey(base::String("first_parent"));
  auto second_parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  second_parent->SetTemplateKey(base::String("second_parent"));

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
  auto second_first_slot = second_slots.GetProperty(0);
  auto second_last_slot = second_slots.GetProperty(1);
  ASSERT_TRUE(first_slot.IsArrayOrJSArray());
  ASSERT_TRUE(second_first_slot.IsArrayOrJSArray());
  ASSERT_TRUE(second_last_slot.IsArrayOrJSArray());
  EXPECT_EQ(first_slot.GetLength(), 0);
  EXPECT_EQ(second_first_slot.GetLength(), 0);
  ASSERT_EQ(second_last_slot.GetLength(), 1);
  EXPECT_EQ(
      second_last_slot.GetProperty(0).GetProperty("templateKey").StdString(),
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
  target->SetTemplateKey(base::String("target"));
  auto proposed_children = lepus::CArray::Create();
  proposed_children->emplace_back(lepus::Value(nested_parent));
  proposed_children->emplace_back(lepus::Value(nested_child));
  auto proposed_slots = lepus::CArray::Create();
  proposed_slots->emplace_back(lepus::Value(std::move(proposed_children)));

  target->InitializeChildSlots(lepus::Value(proposed_slots));
  auto target_children =
      target->Serialize().GetProperty("childSlots").GetProperty(0);
  ASSERT_TRUE(target_children.IsArrayOrJSArray());
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
       ElementTemplateAdjacentChildSlotsPreserveDirectOrder) {
  auto default_entry = std::make_shared<TemplateEntry>();
  default_entry->SetName(DEFAULT_ENTRY_NAME);
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = default_entry;

  auto template_info = std::make_shared<ElementTemplateInfo>();
  template_info->exist_ = true;
  template_info->key_ = "adjacent_slots";
  auto root_info = ElementInfo();
  root_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  auto first_slot_info = ElementInfo();
  first_slot_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_SLOT;
  first_slot_info.slot_index_ = 0;
  root_info.children_.emplace_back(std::move(first_slot_info));
  auto second_slot_info = ElementInfo();
  second_slot_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_SLOT;
  second_slot_info.slot_index_ = 1;
  root_info.children_.emplace_back(std::move(second_slot_info));
  auto sentinel_info = ElementInfo();
  sentinel_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  root_info.children_.emplace_back(std::move(sentinel_info));
  template_info->elements_.emplace_back(std::move(root_info));
  default_entry->template_bundle_.element_template_infos_["adjacent_slots"] =
      std::move(template_info);

  auto first = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  first->SetTypedTag(base::String("view"));
  auto second = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  second->SetTypedTag(base::String("view"));
  auto third = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  third->SetTypedTag(base::String("view"));
  auto first_slot_children = lepus::CArray::Create();
  first_slot_children->emplace_back(lepus::Value(first));
  auto second_slot_children = lepus::CArray::Create();
  second_slot_children->emplace_back(lepus::Value(second));
  auto child_slots = lepus::CArray::Create();
  child_slots->emplace_back(lepus::Value(first_slot_children));
  child_slots->emplace_back(lepus::Value(second_slot_children));

  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->SetTASM(tasm.get());
  root->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  root->SetTemplateKey(base::String("adjacent_slots"));
  root->InitializeChildSlots(lepus::Value(child_slots));
  auto resolved = root->GetRoot();
  auto first_root = first->PeekMaterializedRoot();
  auto second_root = second->PeekMaterializedRoot();
  ASSERT_NE(resolved, nullptr);
  ASSERT_NE(first_root, nullptr);
  ASSERT_NE(second_root, nullptr);
  ASSERT_EQ(resolved->children().size(), 3u);
  EXPECT_EQ(resolved->children()[0].get(), first_root.get());
  EXPECT_EQ(resolved->children()[1].get(), second_root.get());

  root->InsertNodeIntoChildSlot(0, lepus::Value(third), lepus::Value());
  auto third_root = third->PeekMaterializedRoot();
  ASSERT_NE(third_root, nullptr);
  ASSERT_EQ(resolved->children().size(), 4u);
  EXPECT_EQ(resolved->children()[0].get(), first_root.get());
  EXPECT_EQ(resolved->children()[1].get(), third_root.get());
  EXPECT_EQ(resolved->children()[2].get(), second_root.get());

  root->InsertNodeIntoChildSlot(0, lepus::Value(second), lepus::Value(third));
  ASSERT_EQ(resolved->children().size(), 4u);
  EXPECT_EQ(resolved->children()[0].get(), first_root.get());
  EXPECT_EQ(resolved->children()[1].get(), second_root.get());
  EXPECT_EQ(resolved->children()[2].get(), third_root.get());
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateCrossSlotSwapAppliesInCallOrder) {
  auto page = manager->CreateFiberPage("page", 0);
  manager->SetFiberPageElement(page);

  auto observer = std::make_shared<RecordingInspectorElementObserver>();
  manager->SetInspectorElementObserver(observer);
  manager->dom_tree_enabled_ = true;

  auto default_entry = std::make_shared<TemplateEntry>();
  default_entry->SetName(DEFAULT_ENTRY_NAME);
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = default_entry;

  auto template_info = std::make_shared<ElementTemplateInfo>();
  template_info->exist_ = true;
  template_info->key_ = "root_template";

  auto root_info = ElementInfo();
  root_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  auto first_slot_info = ElementInfo();
  first_slot_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_SLOT;
  first_slot_info.slot_index_ = 0;
  root_info.children_.emplace_back(std::move(first_slot_info));
  auto first_sentinel_info = ElementInfo();
  first_sentinel_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  first_sentinel_info.attrs_[base::String("id")] =
      lepus::Value("first-sentinel");
  root_info.children_.emplace_back(std::move(first_sentinel_info));
  auto second_slot_info = ElementInfo();
  second_slot_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_SLOT;
  second_slot_info.slot_index_ = 1;
  root_info.children_.emplace_back(std::move(second_slot_info));
  auto second_sentinel_info = ElementInfo();
  second_sentinel_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  second_sentinel_info.attrs_[base::String("id")] =
      lepus::Value("second-sentinel");
  root_info.children_.emplace_back(std::move(second_sentinel_info));

  template_info->elements_.emplace_back(std::move(root_info));
  default_entry->template_bundle_.element_template_infos_["root_template"] =
      std::move(template_info);

  auto first = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  first->SetTypedTag(base::String("raw-text"));
  first->SetUid(lepus::Value(9));
  auto second = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  second->SetTypedTag(base::String("raw-text"));
  second->SetUid(lepus::Value(10));

  auto first_slot_children = lepus::CArray::Create();
  first_slot_children->emplace_back(lepus::Value(first));
  auto second_slot_children = lepus::CArray::Create();
  second_slot_children->emplace_back(lepus::Value(second));
  auto child_slots = lepus::CArray::Create();
  child_slots->emplace_back(lepus::Value(first_slot_children));
  child_slots->emplace_back(lepus::Value(second_slot_children));

  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->SetTASM(tasm.get());
  root->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  root->SetTemplateKey(base::String("root_template"));
  root->InitializeChildSlots(lepus::Value(child_slots));

  auto resolved = root->GetRoot();
  ASSERT_NE(resolved, nullptr);
  auto first_root = first->PeekMaterializedRoot();
  auto second_root = second->PeekMaterializedRoot();
  ASSERT_NE(first_root, nullptr);
  ASSERT_NE(second_root, nullptr);
  ASSERT_EQ(resolved->children().size(), 4u);
  EXPECT_EQ(resolved->children()[0].get(), first_root.get());
  EXPECT_EQ(resolved->children()[2].get(), second_root.get());

  observer->removed_nodes.clear();
  observer->removed_node_parents.clear();
  observer->added_nodes.clear();
  observer->added_node_parents.clear();

  root->InsertNodeIntoChildSlot(0, lepus::Value(second), lepus::Value());
  root->InsertNodeIntoChildSlot(1, lepus::Value(first), lepus::Value());

  ASSERT_EQ(resolved->children().size(), 4u);
  EXPECT_EQ(resolved->children()[0].get(), second_root.get());
  EXPECT_EQ(resolved->children()[2].get(), first_root.get());

  auto serialized_slots = root->Serialize().GetProperty("childSlots");
  ASSERT_EQ(serialized_slots.GetLength(), 2);
  EXPECT_EQ(serialized_slots.GetProperty(0)
                .GetProperty(0)
                .GetProperty("uid")
                .Number(),
            10);
  EXPECT_EQ(serialized_slots.GetProperty(1)
                .GetProperty(0)
                .GetProperty("uid")
                .Number(),
            9);

  if (ENABLE_INSPECTOR) {
    EXPECT_EQ(std::count(observer->removed_nodes.begin(),
                         observer->removed_nodes.end(), first_root.get()),
              1);
    EXPECT_EQ(std::count(observer->removed_nodes.begin(),
                         observer->removed_nodes.end(), second_root.get()),
              1);
    EXPECT_EQ(std::count(observer->added_nodes.begin(),
                         observer->added_nodes.end(), first_root.get()),
              1);
    EXPECT_EQ(std::count(observer->added_nodes.begin(),
                         observer->added_nodes.end(), second_root.get()),
              1);
    for (auto* parent_snapshot : observer->removed_node_parents) {
      EXPECT_EQ(parent_snapshot, resolved.get());
    }
    for (auto* parent_snapshot : observer->added_node_parents) {
      EXPECT_EQ(parent_snapshot, resolved.get());
    }
  }

  observer->removed_nodes.clear();
  observer->removed_node_parents.clear();
  observer->added_nodes.clear();
  observer->added_node_parents.clear();

  // Swap back with the inverse sequence of direct moves.
  root->InsertNodeIntoChildSlot(1, lepus::Value(second), lepus::Value());
  root->InsertNodeIntoChildSlot(0, lepus::Value(first), lepus::Value());

  ASSERT_EQ(resolved->children().size(), 4u);
  EXPECT_EQ(resolved->children()[0].get(), first_root.get());
  EXPECT_EQ(resolved->children()[2].get(), second_root.get());
  serialized_slots = root->Serialize().GetProperty("childSlots");
  EXPECT_EQ(serialized_slots.GetProperty(0)
                .GetProperty(0)
                .GetProperty("uid")
                .Number(),
            9);
  EXPECT_EQ(serialized_slots.GetProperty(1)
                .GetProperty(0)
                .GetProperty("uid")
                .Number(),
            10);
  if (ENABLE_INSPECTOR) {
    EXPECT_EQ(std::count(observer->removed_nodes.begin(),
                         observer->removed_nodes.end(), first_root.get()),
              1);
    EXPECT_EQ(std::count(observer->removed_nodes.begin(),
                         observer->removed_nodes.end(), second_root.get()),
              1);
    EXPECT_EQ(std::count(observer->added_nodes.begin(),
                         observer->added_nodes.end(), first_root.get()),
              1);
    EXPECT_EQ(std::count(observer->added_nodes.begin(),
                         observer->added_nodes.end(), second_root.get()),
              1);
    for (auto* parent_snapshot : observer->removed_node_parents) {
      EXPECT_EQ(parent_snapshot, resolved.get());
    }
    for (auto* parent_snapshot : observer->added_node_parents) {
      EXPECT_EQ(parent_snapshot, resolved.get());
    }
  }
}

TEST_P(ElementTemplateInstanceTest,
       RemovingAndMovingChildrenPreservesPendingCompiledMounts) {
  auto pending_child = CreateCompiledSpreadInstance();
  pending_child->SetAttributeSlot(0, lepus::Value(lepus::Dictionary::Create()));
  auto removed_child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  removed_child->SetTypedTag(base::String("view"));
  auto moved_child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  moved_child->SetTypedTag(base::String("view"));
  auto source = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  source->SetTypedTag(base::String("view"));
  source->InsertNodeIntoChildSlot(0, lepus::Value(pending_child),
                                  lepus::Value());
  source->InsertNodeIntoChildSlot(0, lepus::Value(removed_child),
                                  lepus::Value());
  source->InsertNodeIntoChildSlot(0, lepus::Value(moved_child), lepus::Value());
  auto destination = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  destination->SetTypedTag(base::String("view"));
  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->InsertNodeIntoChildSlot(0, lepus::Value(source), lepus::Value());
  page->InsertNodeIntoChildSlot(0, lepus::Value(destination), lepus::Value());
  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);
  auto source_root = source->PeekMaterializedRoot();
  auto destination_root = destination->PeekMaterializedRoot();
  ASSERT_NE(source_root, nullptr);
  ASSERT_NE(destination_root, nullptr);
  ASSERT_EQ(pending_child->PeekMaterializedRoot(), nullptr);

  source->RemoveNodeFromChildSlot(0, lepus::Value(removed_child));
  destination->InsertNodeIntoChildSlot(0, lepus::Value(moved_child),
                                       lepus::Value());
  manager->DrainPendingElementTemplateChildMounts(destination_root.get());
  EXPECT_EQ(pending_child->PeekMaterializedRoot(), nullptr);
  EXPECT_TRUE(source_root->children().empty());
  ASSERT_EQ(destination_root->children().size(), 1u);
  EXPECT_EQ(destination_root->children()[0].get(),
            moved_child->PeekMaterializedRoot().get());
  EXPECT_EQ(removed_child->PeekMaterializedRoot()->parent(), nullptr);

  manager->DrainPendingElementTemplateChildMounts(source_root.get());
  ASSERT_NE(pending_child->PeekMaterializedRoot(), nullptr);
  ASSERT_EQ(source_root->children().size(), 1u);
  EXPECT_EQ(source_root->children()[0].get(),
            pending_child->PeekMaterializedRoot().get());
  EXPECT_FALSE(source->HasPendingChildMounts());
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateCrossParentMoveNotifiesAfterDirectAttributeUpdate) {
  auto observer = std::make_shared<RecordingInspectorElementObserver>();
  manager->SetInspectorElementObserver(observer);
  manager->dom_tree_enabled_ = true;

  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTypedTag(base::String("raw-text"));
  child->SetUid(lepus::Value(1));

  auto source = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  source->SetTypedTag(base::String("view"));
  source->SetUid(lepus::Value(7));
  source->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());

  auto destination = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  destination->SetTypedTag(base::String("view"));
  destination->SetUid(lepus::Value(8));

  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->SetUid(lepus::Value(0));
  page->InsertNodeIntoChildSlot(0, lepus::Value(source), lepus::Value());
  page->InsertNodeIntoChildSlot(0, lepus::Value(destination), lepus::Value());

  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);
  auto source_root = source->PeekMaterializedRoot();
  auto destination_root = destination->PeekMaterializedRoot();
  auto child_root = child->PeekMaterializedRoot();
  ASSERT_NE(source_root, nullptr);
  ASSERT_NE(destination_root, nullptr);
  ASSERT_NE(child_root, nullptr);
  ASSERT_EQ(source_root->children().size(), 1u);
  EXPECT_EQ(source_root->children()[0].get(), child_root.get());
  EXPECT_TRUE(destination_root->children().empty());

  observer->removed_nodes.clear();
  observer->removed_node_parents.clear();
  observer->added_nodes.clear();
  observer->added_node_parents.clear();

  auto destination_attributes = lepus::Dictionary::Create();
  destination_attributes->SetValue(base::String("data-state"),
                                   lepus::Value("updated-first"));
  destination->SetAttributes(lepus::Value(destination_attributes));
  ASSERT_NE(DatasetValue(destination_root.get(), "state"), nullptr);
  EXPECT_EQ(DatasetValue(destination_root.get(), "state")->StdString(),
            "updated-first");
  destination->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());

  EXPECT_TRUE(source_root->children().empty());
  ASSERT_EQ(destination_root->children().size(), 1u);
  EXPECT_EQ(destination_root->children()[0].get(), child_root.get());
  EXPECT_EQ(child_root->parent(), destination_root.get());
  if (ENABLE_INSPECTOR) {
    ASSERT_EQ(observer->removed_nodes.size(), 1u);
    EXPECT_EQ(observer->removed_nodes[0], child_root.get());
    EXPECT_EQ(observer->removed_node_parents[0], source_root.get());
    ASSERT_EQ(observer->added_nodes.size(), 1u);
    EXPECT_EQ(observer->added_nodes[0], child_root.get());
    EXPECT_EQ(observer->added_node_parents[0], destination_root.get());
  }

  EXPECT_EQ(
      source->Serialize().GetProperty("childSlots").GetProperty(0).GetLength(),
      0);
  EXPECT_EQ(destination->Serialize()
                .GetProperty("childSlots")
                .GetProperty(0)
                .GetProperty(0)
                .GetProperty("uid")
                .Number(),
            1);
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateMaterializesStaticEventListeners) {
  manager->config_->SetEnableEventHandleRefactor(true);
  manager->SetConfig(manager->config_);
  tasm->page_config_ = manager->config_;

  auto default_entry = std::make_shared<TemplateEntry>();
  default_entry->SetName(DEFAULT_ENTRY_NAME);
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = default_entry;

  auto template_info = std::make_shared<ElementTemplateInfo>();
  template_info->exist_ = true;
  template_info->key_ = "root_template";

  auto target_info = ElementInfo();
  target_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  target_info.attributes_ =
      std::make_shared<const TemplateAttributes>(TemplateAttributes{
          Attribute{ATTRIBUTE_BINDING_TYPE_STATIC, base::String("bindtap"),
                    lepus::Value("onStaticTap"), 0}});
  template_info->elements_.emplace_back(std::move(target_info));
  default_entry->template_bundle_.element_template_infos_["root_template"] =
      std::move(template_info);

  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->SetTASM(tasm.get());
  root->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  root->SetTemplateKey(base::String("root_template"));

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
       CompiledAttributeSlotsUpdatedAfterPrepareUseLatestState) {
  auto instance = CreateCompiledSpreadInstance();
  auto initial_slot = lepus::Dictionary::Create();
  initial_slot->SetValue(base::String("data-slot-stale"), lepus::Value("old"));
  instance->SetAttributeSlot(0, lepus::Value(initial_slot));
  // Finish detached preparation before updating the logical slot snapshot.
  instance->RequestMaterializationRecursively();
  instance->create_element_tree_task_->Run();

  auto latest_slot = lepus::Dictionary::Create();
  latest_slot->SetValue(base::String("data-slot-current"), lepus::Value("new"));
  instance->SetAttributeSlot(0, lepus::Value(latest_slot));

  auto root = instance->GetRoot();
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(DatasetValue(root.get(), "slot-stale"), nullptr);
  ASSERT_NE(DatasetValue(root.get(), "slot-current"), nullptr);
  EXPECT_EQ(DatasetValue(root.get(), "slot-current")->StdString(), "new");
}

TEST_P(ElementTemplateInstanceTest, CompiledSpreadUpdatesPreserveCallbacks) {
  auto lepus_runtime = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  lepus_runtime->Initialize();
  lepus_runtime->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  lepus::BytecodeGenerator::GenerateBytecode(
      lepus_runtime->GetMTSContext(),
      "let initialCallback = () => 1; let updatedCallback = () => 2;",
      lepus_runtime->GetSdkVersion(), "");
  ASSERT_TRUE(lepus_runtime->Execute(nullptr));
  auto initial_callback = lepus_runtime->GetGlobalData("initialCallback");
  auto updated_callback = lepus_runtime->GetGlobalData("updatedCallback");
  ASSERT_TRUE(initial_callback.IsCallable());
  ASSERT_TRUE(updated_callback.IsCallable());

  for (bool materialize_before_update : {false, true}) {
    SCOPED_TRACE(materialize_before_update);
    auto instance = CreateCompiledSpreadInstance();
    auto initial = lepus::Dictionary::Create();
    initial->SetValue(base::String("main-thread:bindtap"), initial_callback);
    auto slots = lepus::CArray::Create();
    slots->emplace_back(lepus::Value(initial));
    instance->SetAttributeSlots(lepus::Value(slots));
    if (materialize_before_update) {
      ASSERT_NE(instance->GetRoot(), nullptr);
    } else {
      instance->RequestMaterializationRecursively();
      instance->create_element_tree_task_->Run();
    }

    auto updated = lepus::Dictionary::Create();
    updated->SetValue(base::String("main-thread:bindtap"), updated_callback);
    instance->SetAttributeSlot(0, lepus::Value(updated));

    auto stored_callback = instance->Serialize()
                               .GetProperty("attributeSlots")
                               .GetProperty(0)
                               .GetProperty("main-thread:bindtap");
    ASSERT_TRUE(stored_callback.IsCallable());
    EXPECT_TRUE(stored_callback.IsEqual(updated_callback));

    auto root = instance->GetRoot();
    ASSERT_NE(root, nullptr);
    auto event = root->event_map().find("tap");
    ASSERT_NE(event, root->event_map().end());
    EXPECT_FALSE(event->second->is_js_event());
    auto handler = event->second->lepus_function();
    ASSERT_TRUE(handler.IsCallable());
    EXPECT_TRUE(handler.IsEqual(updated_callback));
    EXPECT_EQ(lepus_runtime->CallClosure(handler).Number(), 2);
  }
}

TEST_P(ElementTemplateInstanceTest,
       MaterializedCompiledSpreadClearsRemovedKeys) {
  auto instance = CreateCompiledSpreadInstance();
  auto initial = lepus::Dictionary::Create();
  initial->SetValue(base::String("data-stale"), lepus::Value("old"));
  initial->SetValue(base::String("data-current"), lepus::Value("before"));
  instance->SetAttributeSlot(0, lepus::Value(initial));

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

  instance->SetAttributeSlot(0, lepus::Value(lepus::Dictionary::Create()));
  EXPECT_EQ(DatasetValue(root.get(), "current"), nullptr);
}

TEST_P(ElementTemplateInstanceTest,
       PageFlushMountsCompiledSubtreeWithLatestAttributes) {
  manager->config_->SetEnableEventHandleRefactor(true);
  manager->SetConfig(manager->config_);
  tasm->page_config_ = manager->config_;

  auto default_entry = std::make_shared<TemplateEntry>();
  default_entry->SetName(DEFAULT_ENTRY_NAME);
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = default_entry;

  auto template_info = std::make_shared<ElementTemplateInfo>();
  template_info->exist_ = true;
  template_info->key_ = "prepared_child";
  auto root_info = ElementInfo();
  root_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  root_info.attributes_ =
      std::make_shared<const TemplateAttributes>(TemplateAttributes{
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("data-test"),
                    lepus::Value(), 0},
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("bindtap"),
                    lepus::Value(), 1},
          Attribute{ATTRIBUTE_BINDING_TYPE_SPREAD, base::String("spread"),
                    lepus::Value(), 2}});
  auto child_slot_info = ElementInfo();
  child_slot_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_SLOT;
  child_slot_info.slot_index_ = 0;
  root_info.children_.emplace_back(std::move(child_slot_info));
  template_info->elements_.emplace_back(std::move(root_info));
  default_entry->template_bundle_.element_template_infos_["prepared_child"] =
      std::move(template_info);

  auto grandchild_template_info = std::make_shared<ElementTemplateInfo>();
  grandchild_template_info->exist_ = true;
  grandchild_template_info->key_ = "prepared_grandchild";
  auto grandchild_root_info = ElementInfo();
  grandchild_root_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  grandchild_template_info->elements_.emplace_back(
      std::move(grandchild_root_info));
  default_entry->template_bundle_
      .element_template_infos_["prepared_grandchild"] =
      std::move(grandchild_template_info);

  auto grandchild = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  grandchild->SetTASM(tasm.get());
  grandchild->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  grandchild->SetTemplateKey(base::String("prepared_grandchild"));

  auto initial_attribute_slots = lepus::CArray::Create();
  initial_attribute_slots->emplace_back(lepus::Value("before"));
  initial_attribute_slots->emplace_back(lepus::Value("onBeforeTap"));
  auto initial_spread = lepus::Dictionary::Create();
  initial_spread->SetValue("data-old", lepus::Value("old-value"));
  initial_attribute_slots->emplace_back(lepus::Value(initial_spread));
  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTASM(tasm.get());
  child->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  child->SetTemplateKey(base::String("prepared_child"));
  child->SetAttributeSlots(lepus::Value(initial_attribute_slots));
  auto child_slot_children = lepus::CArray::Create();
  child_slot_children->emplace_back(lepus::Value(grandchild));
  auto child_slots = lepus::CArray::Create();
  child_slots->emplace_back(lepus::Value(child_slot_children));
  child->InitializeChildSlots(lepus::Value(child_slots));

  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->SetUid(lepus::Value(0));
  page->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());

  EXPECT_EQ(child->PeekMaterializedRoot(), nullptr);
  EXPECT_EQ(grandchild->PeekMaterializedRoot(), nullptr);

  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);
  EXPECT_TRUE(page_root->children().empty());
  EXPECT_EQ(child->PeekMaterializedRoot(), nullptr);
  EXPECT_EQ(grandchild->PeekMaterializedRoot(), nullptr);

  child->SetAttributeSlot(0, lepus::Value("after"));
  child->SetAttributeSlot(1, lepus::Value("onAfterTap"));
  auto final_spread = lepus::Dictionary::Create();
  final_spread->SetValue("data-new", lepus::Value("new-value"));
  child->SetAttributeSlot(2, lepus::Value(final_spread));
  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page_root.get());

  auto child_root = child->PeekMaterializedRoot();
  ASSERT_NE(child_root, nullptr);
  auto grandchild_root = grandchild->PeekMaterializedRoot();
  ASSERT_NE(grandchild_root, nullptr);
  ASSERT_EQ(page_root->children().size(), 1u);
  EXPECT_EQ(page_root->children()[0].get(), child_root.get());
  ASSERT_EQ(child_root->children().size(), 1u);
  EXPECT_EQ(child_root->children()[0].get(), grandchild_root.get());
  auto* test_data = DatasetValue(child_root.get(), "test");
  ASSERT_NE(test_data, nullptr);
  EXPECT_EQ(test_data->StdString(), "after");
  EXPECT_EQ(DatasetValue(child_root.get(), "old"), nullptr);
  auto* new_data = DatasetValue(child_root.get(), "new");
  ASSERT_NE(new_data, nullptr);
  EXPECT_EQ(new_data->StdString(), "new-value");
  auto event_iter = child_root->event_map().find("tap");
  ASSERT_NE(event_iter, child_root->event_map().end());
  EXPECT_EQ(event_iter->second->function(), "onAfterTap");
  auto* listeners = child_root->GetEventListenerMap()->Find("tap");
  ASSERT_NE(listeners, nullptr);
  EXPECT_EQ(listeners->size(), 1u);
}

TEST_P(ElementTemplateInstanceTest,
       DetachedPendingCompiledChildMountWakesOnReattach) {
  auto child = CreateCompiledSpreadInstance();

  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->SetUid(lepus::Value(0));
  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);

  auto parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  parent->SetTypedTag(base::String("view"));
  auto parent_root = parent->GetRoot();
  ASSERT_NE(parent_root, nullptr);
  parent->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());

  ASSERT_TRUE(parent->HasPendingChildMounts());
  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page_root.get());

  EXPECT_TRUE(parent->HasPendingChildMounts());
  EXPECT_EQ(child->PeekMaterializedRoot(), nullptr);

  page->InsertNodeIntoChildSlot(0, lepus::Value(parent), lepus::Value());
  EXPECT_EQ(parent_root->parent(), page_root.get());

  options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page_root.get());

  auto child_root = child->PeekMaterializedRoot();
  ASSERT_NE(child_root, nullptr);
  EXPECT_EQ(child_root->parent(), parent_root.get());
  EXPECT_FALSE(parent->HasPendingChildMounts());
}

TEST_P(ElementTemplateInstanceTest,
       PendingCompiledChildMoveUsesFinalParentAndScopedDrain) {
  auto child = CreateCompiledSpreadInstance();

  auto source = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  source->SetTypedTag(base::String("view"));
  auto destination = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  destination->SetTypedTag(base::String("view"));
  auto sibling = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  sibling->SetTypedTag(base::String("view"));
  destination->InsertNodeIntoChildSlot(0, lepus::Value(sibling),
                                       lepus::Value());

  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->SetUid(lepus::Value(0));
  page->InsertNodeIntoChildSlot(0, lepus::Value(source), lepus::Value());
  page->InsertNodeIntoChildSlot(0, lepus::Value(destination), lepus::Value());
  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);
  auto source_root = source->PeekMaterializedRoot();
  auto destination_root = destination->PeekMaterializedRoot();
  auto sibling_root = sibling->PeekMaterializedRoot();
  ASSERT_NE(source_root, nullptr);
  ASSERT_NE(destination_root, nullptr);
  ASSERT_NE(sibling_root, nullptr);

  page_root->FlushActionsAsRoot();

  source->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());
  destination->InsertNodeIntoChildSlot(0, lepus::Value(child),
                                       lepus::Value(sibling));

  ASSERT_TRUE(source->HasPendingChildMounts());
  ASSERT_TRUE(destination->HasPendingChildMounts());
  source_root->FlushActionsAsRoot();
  EXPECT_FALSE(source->HasPendingChildMounts());
  EXPECT_TRUE(destination->HasPendingChildMounts());
  EXPECT_EQ(child->PeekMaterializedRoot(), nullptr);

  destination_root->FlushActionsAsRoot();

  auto child_root = child->PeekMaterializedRoot();
  ASSERT_NE(child_root, nullptr);
  ASSERT_EQ(destination_root->children().size(), 2u);
  EXPECT_EQ(destination_root->children()[0].get(), child_root.get());
  EXPECT_EQ(destination_root->children()[1].get(), sibling_root.get());
  EXPECT_EQ(child_root->parent(), destination_root.get());
  EXPECT_TRUE(source_root->children().empty());
  EXPECT_FALSE(destination->HasPendingChildMounts());
}

TEST_P(ElementTemplateInstanceTest, FlushSkipsRemovedPendingCompiledChild) {
  auto child = CreateCompiledSpreadInstance();
  auto page_root = manager->CreateFiberPage("page", 0);
  auto parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  parent->SetTypedTag(base::String("view"));
  auto parent_root = parent->GetRoot();
  ASSERT_NE(parent_root, nullptr);
  page_root->InsertNode(parent_root);
  page_root->FlushActionsAsRoot();
  parent->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());
  ASSERT_TRUE(parent->HasPendingChildMounts());

  parent->RemoveNodeFromChildSlot(0, lepus::Value(child));
  parent_root->FlushActionsAsRoot();

  EXPECT_TRUE(parent_root->children().empty());
  EXPECT_EQ(child->PeekMaterializedRoot(), nullptr);
  EXPECT_FALSE(parent->HasPendingChildMounts());
}

TEST_P(ElementTemplateInstanceTest, FlushDoesNotRetainPendingTemplateOwner) {
  auto child = CreateCompiledSpreadInstance();
  auto page_root = manager->CreateFiberPage("page", 0);
  auto parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  parent->SetTypedTag(base::String("view"));
  auto parent_root = parent->GetRoot();
  ASSERT_NE(parent_root, nullptr);
  page_root->InsertNode(parent_root);
  page_root->FlushActionsAsRoot();
  parent->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());
  ASSERT_TRUE(parent->HasPendingChildMounts());

  auto weak_parent = parent->WeakFromThis();
  parent = nullptr;
  EXPECT_FALSE(weak_parent);
  parent_root->FlushActionsAsRoot();

  EXPECT_TRUE(parent_root->children().empty());
  EXPECT_EQ(child->PeekMaterializedRoot(), nullptr);
}

TEST_P(ElementTemplateInstanceTest, ListSchedulerFlushMountsCompiledChild) {
  auto page_root = manager->CreateFiberPage("page", 0);
  for (auto strategy :
       {list::BatchRenderStrategy::kAsyncResolveProperty,
        list::BatchRenderStrategy::kAsyncResolvePropertyAndElementTree}) {
    SCOPED_TRACE(static_cast<int>(strategy));
    auto child = CreateCompiledSpreadInstance();
    auto parent = fml::AdoptRef<ElementTemplateInstance>(
        new ElementTemplateInstance(manager));
    parent->SetTypedTag(base::String("view"));
    auto parent_root = parent->GetRoot();
    ASSERT_NE(parent_root, nullptr);
    page_root->InsertNode(parent_root);
    page_root->FlushActionsAsRoot();
    parent_root->CreateListItemScheduler(strategy, true);
    parent_root->RecursivelyMarkRenderRootElement(parent_root.get());
    parent->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());
    ASSERT_EQ(child->PeekMaterializedRoot(), nullptr);

    auto* scheduler = parent_root->GetSchedulerAdapter();
    auto& tasks = manager->ParallelResolveTreeTasks();
    scheduler->ResolveElementTree(tasks);
    while (!tasks.empty()) {
      tasks.front()->Run();
      tasks.front()->GetFuture().get()();
      tasks.pop_front();
    }

    auto child_root = child->PeekMaterializedRoot();
    ASSERT_NE(child_root, nullptr);
    ASSERT_EQ(parent_root->children().size(), 1u);
    EXPECT_EQ(parent_root->children()[0].get(), child_root.get());
    EXPECT_EQ(child_root->parent(), parent_root.get());
    EXPECT_FALSE(parent->HasPendingChildMounts());
  }
}

INSTANTIATE_TEST_SUITE_P(ElementTemplateInstanceTestModule,
                         ElementTemplateInstanceTest,
                         ::testing::ValuesIn(fiber_element_generation_params));

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
