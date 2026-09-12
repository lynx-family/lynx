// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/fiber/template_element.h"

#include <future>
#include <memory>
#include <string>
#include <utility>

#include "core/public/pipeline_option.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/dom/fiber/page_element.h"
#include "core/renderer/dom/fiber/tree_resolver.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/testing/fiber_element_test.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/template_entry.h"
#include "core/renderer/utils/base/element_template_info.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/runtime/lepus/bytecode_generator.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/shell/runtime/mts/mts_runtime.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

namespace {
struct TemplateCallbackValues {
  std::shared_ptr<runtime::MTSRuntime> runtime;
  lepus::Value component_at_index;
  lepus::Value enqueue_component;
  lepus::Value component_at_indexes;
};

TemplateCallbackValues CreateTemplateCallbackValues(
    TemplateAssembler* template_assembler) {
  auto lepus_runtime = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  lepus_runtime->Initialize();
  lepus_runtime->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(
          static_cast<runtime::MTSRuntime::Delegate*>(template_assembler)));

  std::string js_source = R"(
    let componentAtIndex = () => 1;
    let enqueueComponent = () => {};
    let componentAtIndexes = () => {};
  )";
  lepus::BytecodeGenerator::GenerateBytecode(
      lepus_runtime->GetMTSContext(), js_source, lepus_runtime->GetSdkVersion(),
      "");
  lepus_runtime->Execute(nullptr);

  return TemplateCallbackValues{
      lepus_runtime, lepus_runtime->GetGlobalData("componentAtIndex"),
      lepus_runtime->GetGlobalData("enqueueComponent"),
      lepus_runtime->GetGlobalData("componentAtIndexes")};
}

TemplateCallbackValues CreateTemplateCallbackValuesReturningSign(
    TemplateAssembler* template_assembler, int32_t sign) {
  auto lepus_runtime = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  lepus_runtime->Initialize();
  lepus_runtime->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(
          static_cast<runtime::MTSRuntime::Delegate*>(template_assembler)));
  template_assembler->template_entries_[DEFAULT_ENTRY_NAME]->SetVm(
      lepus_runtime);

  std::string js_source =
      "let componentAtIndex = () => " + std::to_string(sign) + R"(;
       let enqueueComponent = () => {};
       let componentAtIndexes = () => {};
      )";
  lepus::BytecodeGenerator::GenerateBytecode(
      lepus_runtime->GetMTSContext(), js_source, lepus_runtime->GetSdkVersion(),
      "");
  lepus_runtime->Execute(nullptr);

  return TemplateCallbackValues{
      lepus_runtime, lepus_runtime->GetGlobalData("componentAtIndex"),
      lepus_runtime->GetGlobalData("enqueueComponent"),
      lepus_runtime->GetGlobalData("componentAtIndexes")};
}

TemplateCallbackValues CreateTemplateCallbackValuesRecordingEnqueue(
    TemplateAssembler* template_assembler, int32_t sign) {
  auto lepus_runtime = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  lepus_runtime->Initialize();
  lepus_runtime->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(
          static_cast<runtime::MTSRuntime::Delegate*>(template_assembler)));
  template_assembler->template_entries_[DEFAULT_ENTRY_NAME]->SetVm(
      lepus_runtime);

  std::string js_source =
      "let lastEnqueueSign = -1;"
      "let componentAtIndex = () => " +
      std::to_string(sign) + R"(;
       let enqueueComponent = (_list, _listID, sign) => {
         lastEnqueueSign = sign;
       };
       let componentAtIndexes = () => {};
      )";
  lepus::BytecodeGenerator::GenerateBytecode(
      lepus_runtime->GetMTSContext(), js_source, lepus_runtime->GetSdkVersion(),
      "");
  lepus_runtime->Execute(nullptr);

  return TemplateCallbackValues{
      lepus_runtime, lepus_runtime->GetGlobalData("componentAtIndex"),
      lepus_runtime->GetGlobalData("enqueueComponent"),
      lepus_runtime->GetGlobalData("componentAtIndexes")};
}

const lepus::Value* DatasetValue(const Element* element,
                                 const base::String& key) {
  auto it = element->data_model_->dataset().find(key);
  if (it == element->data_model_->dataset().end()) {
    return nullptr;
  }
  return &it->second;
}
}  // namespace

TEST_P(FiberElementTest, SerializeTemplateElementRecursively) {
  auto child = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  child->SetTemplateKey(base::String("child_template"));
  child->SetBundleUrl(base::String("child_bundle.js"));

  auto child_attribute_slots = lepus::CArray::Create();
  child_attribute_slots->emplace_back(lepus::Value(true));
  child->SetAttributeSlots(lepus::Value(std::move(child_attribute_slots)));

  auto child_element_slots = lepus::CArray::Create();
  child_element_slots->emplace_back(lepus::Value(lepus::CArray::Create()));
  child->SetElementSlots(lepus::Value(std::move(child_element_slots)));

  auto root = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  root->SetTemplateKey(base::String("root_template"));
  root->SetBundleUrl(base::String("root_bundle.js"));

  auto root_attribute_slots = lepus::CArray::Create();
  root_attribute_slots->emplace_back(lepus::Value("slot_0"));
  root_attribute_slots->emplace_back(lepus::Value(42));
  root->SetAttributeSlots(lepus::Value(std::move(root_attribute_slots)));

  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(child));
  auto root_element_slots = lepus::CArray::Create();
  root_element_slots->emplace_back(lepus::Value(std::move(slot_children)));
  root->SetElementSlots(lepus::Value(std::move(root_element_slots)));

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

  auto serialized_element_slots = serialized.GetProperty("elementSlots");
  EXPECT_TRUE(serialized_element_slots.IsArrayOrJSArray());
  ASSERT_EQ(serialized_element_slots.GetLength(), 1);
  auto serialized_slot_children = serialized_element_slots.GetProperty(0);
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
  EXPECT_TRUE(serialized_child.GetProperty("elementSlots").IsArrayOrJSArray());
}

TEST_P(FiberElementTest, SerializeTemplateElementSkipsInvalidSlotChildren) {
  auto root = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  root->SetTemplateKey(base::String("root_template"));
  root->SetBundleUrl(base::String("root_bundle.js"));

  auto invalid_slot_children = lepus::CArray::Create();
  invalid_slot_children->emplace_back(lepus::Value(manager->CreateFiberView()));
  invalid_slot_children->emplace_back(lepus::Value(1));

  auto root_element_slots = lepus::CArray::Create();
  root_element_slots->emplace_back(
      lepus::Value(std::move(invalid_slot_children)));
  root_element_slots->emplace_back(lepus::Value("invalid_slot_shape"));
  root->SetElementSlots(lepus::Value(std::move(root_element_slots)));

  auto serialized = root->Serialize();
  EXPECT_TRUE(serialized.IsObject());

  auto serialized_element_slots = serialized.GetProperty("elementSlots");
  EXPECT_TRUE(serialized_element_slots.IsArrayOrJSArray());
  ASSERT_EQ(serialized_element_slots.GetLength(), 2);
  EXPECT_TRUE(serialized_element_slots.GetProperty(0).IsArrayOrJSArray());
  EXPECT_EQ(serialized_element_slots.GetProperty(0).GetLength(), 0);
  EXPECT_EQ(serialized_element_slots.GetProperty(1).StdString(),
            "invalid_slot_shape");
}

TEST_P(FiberElementTest, SerializeTypedTemplateElement) {
  auto child = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  child->SetTypedTag(base::String("raw-text"));
  child->SetUid(lepus::Value("child_uid"));

  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(child));
  auto element_slots = lepus::CArray::Create();
  element_slots->emplace_back(lepus::Value(slot_children));

  auto root = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  root->SetElementSlots(lepus::Value(element_slots));
  root->SetUid(lepus::Value("root_uid"));
  root->SetTypedTag(base::String("view"));
  auto attributes = lepus::Dictionary::Create();
  attributes->SetValue(base::String("data-test"), lepus::Value("root_attr"));
  root->SetRootAttributes(lepus::Value(attributes));

  auto serialized = root->Serialize();
  EXPECT_TRUE(serialized.IsObject());
  EXPECT_FALSE(serialized.GetProperty("templateKey").IsString());
  EXPECT_EQ(serialized.GetProperty("tag").StdString(), "view");
  EXPECT_EQ(serialized.GetProperty("uid").StdString(), "root_uid");
  EXPECT_EQ(
      serialized.GetProperty("attributes").GetProperty("data-test").StdString(),
      "root_attr");

  auto serialized_slots = serialized.GetProperty("elementSlots");
  EXPECT_TRUE(serialized_slots.IsArrayOrJSArray());
  ASSERT_EQ(serialized_slots.GetLength(), 1);
  auto serialized_child = serialized_slots.GetProperty(0).GetProperty(0);
  EXPECT_EQ(serialized_child.GetProperty("tag").StdString(), "raw-text");
  EXPECT_EQ(serialized_child.GetProperty("uid").StdString(), "child_uid");
}

TEST_P(FiberElementTest, PageTemplateElementSlotsPrepareChildrenRecursively) {
  auto compiled_child =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  compiled_child->SetTemplateKey(base::String("compiled_child"));

  auto typed_parent =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  typed_parent->SetTypedTag(base::String("list"));
  auto typed_parent_slot_children = lepus::CArray::Create();
  typed_parent_slot_children->emplace_back(lepus::Value(compiled_child));
  auto typed_parent_slots = lepus::CArray::Create();
  typed_parent_slots->emplace_back(lepus::Value(typed_parent_slot_children));
  typed_parent->SetElementSlots(lepus::Value(typed_parent_slots));

  EXPECT_FALSE(typed_parent->IsInTemplateTree());
  EXPECT_FALSE(compiled_child->IsInTemplateTree());
  EXPECT_EQ(compiled_child->async_create_task_, nullptr);

  auto page = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  page->SetTypedTag(base::String("page"));
  auto page_slot_children = lepus::CArray::Create();
  page_slot_children->emplace_back(lepus::Value(typed_parent));
  auto page_slots = lepus::CArray::Create();
  page_slots->emplace_back(lepus::Value(page_slot_children));
  page->SetElementSlots(lepus::Value(page_slots));

  EXPECT_TRUE(page->IsInTemplateTree());
  EXPECT_TRUE(typed_parent->IsInTemplateTree());
  EXPECT_TRUE(compiled_child->IsInTemplateTree());
  EXPECT_EQ(page->async_create_task_, nullptr);
  EXPECT_EQ(typed_parent->async_create_task_, nullptr);
  EXPECT_NE(compiled_child->async_create_task_, nullptr);
}

TEST_P(FiberElementTest, NonPageTemplateElementSlotsDoNotPrepareBeforeTree) {
  auto compiled_child =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  compiled_child->SetTemplateKey(base::String("compiled_child"));

  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(compiled_child));
  auto element_slots = lepus::CArray::Create();
  element_slots->emplace_back(lepus::Value(slot_children));

  auto typed_parent =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  typed_parent->SetTypedTag(base::String("list"));
  typed_parent->SetElementSlots(lepus::Value(element_slots));

  EXPECT_FALSE(typed_parent->IsInTemplateTree());
  EXPECT_FALSE(compiled_child->IsInTemplateTree());
  EXPECT_EQ(compiled_child->async_create_task_, nullptr);
}

TEST_P(FiberElementTest, InsertElementSlotChildMarksChildInTreeBeforeResolve) {
  auto grandchild =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  grandchild->SetTemplateKey(base::String("grandchild_template"));

  auto child = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  child->SetTypedTag(base::String("list"));
  auto child_slot_children = lepus::CArray::Create();
  child_slot_children->emplace_back(lepus::Value(grandchild));
  auto child_slots = lepus::CArray::Create();
  child_slots->emplace_back(lepus::Value(child_slot_children));
  child->SetElementSlots(lepus::Value(child_slots));

  auto parent = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  parent->SetTypedTag(base::String("page"));
  parent->SetElementSlots(lepus::Value(lepus::CArray::Create()));
  ASSERT_TRUE(parent->IsInTemplateTree());

  parent->InsertElementSlotChild(0, child, nullptr);

  EXPECT_EQ(parent->result_, nullptr);
  ASSERT_EQ(parent->pending_operations_.size(), 1u);
  EXPECT_TRUE(child->IsInTemplateTree());
  EXPECT_TRUE(grandchild->IsInTemplateTree());
  EXPECT_EQ(child->async_create_task_, nullptr);
  EXPECT_NE(grandchild->async_create_task_, nullptr);

  parent->RemoveElementSlotChild(0, child);
  EXPECT_TRUE(child->IsInTemplateTree());
  EXPECT_TRUE(grandchild->IsInTemplateTree());
}

TEST_P(FiberElementTest, TypedTemplateElementAppliesRootAttributesAsSpread) {
  auto root = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
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
  root->SetRootAttributes(lepus::Value(reset_attributes));

  EXPECT_EQ(DatasetValue(resolved.get(), "test"), nullptr);
  added_data = DatasetValue(resolved.get(), "added");
  ASSERT_NE(added_data, nullptr);
  EXPECT_EQ(added_data->StdString(), "updated");
  EXPECT_EQ(resolved->event_map().count("tap"), 0u);

  root->SetRootAttributes(lepus::Value(lepus::Dictionary::Create()));
  EXPECT_EQ(DatasetValue(resolved.get(), "added"), nullptr);
  EXPECT_TRUE(root->Serialize().GetProperty("attributes").IsEmpty());
}

TEST_P(FiberElementTest, TypedTemplateElementResolvesListRootAndSlotChildren) {
  auto slot_child = manager->CreateFiberView();

  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(slot_child));
  auto element_slots = lepus::CArray::Create();
  element_slots->emplace_back(lepus::Value(slot_children));

  auto root = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  root->SetTASM(tasm.get());
  root->SetElementSlots(lepus::Value(element_slots));
  root->SetTypedTag(base::String("list"));

  EXPECT_EQ(root->result_, nullptr);
  EXPECT_EQ(root->async_create_task_, nullptr);
  auto resolved = root->GetRoot();

  ASSERT_NE(resolved, nullptr);
  EXPECT_TRUE(resolved->is_list());
  ASSERT_EQ(resolved->children().size(), 1u);
  EXPECT_EQ(resolved->children()[0].get(), slot_child.get());
  EXPECT_TRUE(slot_child->is_list_item());
}

TEST_P(FiberElementTest, TypedTemplateElementListRootUsesPageComponentScope) {
  auto page = manager->CreateFiberPage("page", 11);

  auto root = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  root->SetTASM(tasm.get());
  root->SetTypedTag(base::String("list"));

  auto resolved = root->GetRoot();
  ASSERT_NE(resolved, nullptr);
  EXPECT_TRUE(resolved->is_list());
  EXPECT_EQ(resolved->GetParentComponentUniqueIdForFiber(), page->impl_id());

  page->InsertNode(root);
  page->PrepareChildren();

  ASSERT_EQ(page->children().size(), 1u);
  EXPECT_EQ(page->children()[0].get(), resolved.get());
  EXPECT_EQ(resolved->parent(), page.get());
  EXPECT_EQ(resolved->GetParentComponentElement(), page.get());
  EXPECT_EQ(resolved->GetCSSID(), 11);
}

TEST_P(FiberElementTest, TypedTemplateElementListRootAppliesCallbacks) {
  auto callbacks = CreateTemplateCallbackValues(tasm.get());
  ASSERT_TRUE(callbacks.component_at_index.IsCallable());
  ASSERT_TRUE(callbacks.enqueue_component.IsCallable());
  ASSERT_TRUE(callbacks.component_at_indexes.IsCallable());

  auto attributes = lepus::Dictionary::Create();
  attributes->SetValue(base::String("component-at-index"),
                       callbacks.component_at_index);
  attributes->SetValue(base::String("enqueue-component"),
                       callbacks.enqueue_component);
  attributes->SetValue(base::String("component-at-indexes"),
                       callbacks.component_at_indexes);
  auto snapshot_payload = lepus::Dictionary::Create();
  snapshot_payload->SetValue(base::String("value"), lepus::Value("before"));
  attributes->SetValue(base::String("data-payload"),
                       lepus::Value(snapshot_payload));
  auto converted_attributes = lepus::Value(attributes).ToLepusValue();
  ASSERT_TRUE(
      converted_attributes.GetProperty("component-at-index").IsCallable());

  auto root = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  root->SetTASM(tasm.get());
  root->SetTypedTag(base::String("list"));
  root->SetRootAttributes(converted_attributes);
  snapshot_payload->SetValue(base::String("value"), lepus::Value("after"));

  EXPECT_EQ(root->Serialize()
                .GetProperty("attributes")
                .GetProperty("data-payload")
                .GetProperty("value")
                .StdString(),
            "before");

  auto resolved = root->GetRoot();
  ASSERT_NE(resolved, nullptr);
  ASSERT_TRUE(resolved->is_list());
  auto* list = static_cast<ListElement*>(resolved.get());
  EXPECT_EQ(list->tasm_, tasm.get());
  EXPECT_TRUE(list->component_at_index_.IsCallable());
  EXPECT_TRUE(list->enqueue_component_.IsCallable());
  EXPECT_TRUE(list->component_at_indexes_.IsCallable());
  EXPECT_EQ(list->data_model_->attributes().count("component-at-index"), 0u);
  EXPECT_EQ(list->data_model_->attributes().count("enqueue-component"), 0u);
  EXPECT_EQ(list->data_model_->attributes().count("component-at-indexes"), 0u);

  auto updated_attributes = lepus::Dictionary::Create();
  updated_attributes->SetValue(base::String("component-at-index"),
                               callbacks.component_at_indexes);
  root->SetRootAttributes(lepus::Value(updated_attributes));

  EXPECT_TRUE(list->component_at_index_.IsCallable());
  EXPECT_FALSE(list->enqueue_component_.IsCallable());
  EXPECT_FALSE(list->component_at_indexes_.IsCallable());

  root->SetRootAttributes(lepus::Value(lepus::Dictionary::Create()));
  EXPECT_FALSE(list->component_at_index_.IsCallable());
}

TEST_P(FiberElementTest, CloneTypedTemplateElementCreatesRoot) {
  auto root = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  root->SetTypedTag(base::String("raw-text"));

  auto cloned_element = TreeResolver::CloneElements(
      root, tasm->style_sheet_manager(DEFAULT_ENTRY_NAME), false,
      TreeResolver::CloningDepth::kSingle);

  ASSERT_NE(cloned_element, nullptr);
  ASSERT_TRUE(cloned_element->is_template());
  auto* cloned = static_cast<TemplateElement*>(cloned_element.get());
  EXPECT_EQ(cloned->async_create_task_, nullptr);

  auto cloned_root = cloned->GetRoot();

  ASSERT_NE(cloned_root, nullptr);
  EXPECT_TRUE(cloned_root->is_raw_text());
  EXPECT_EQ(cloned->async_create_task_, nullptr);
}

TEST_P(FiberElementTest,
       ListItemTemplateElementGraphCacheTransfersMaterializedTree) {
  auto parent = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  parent->SetTemplateKey(base::String("parent_template"));
  parent->result_ = manager->CreateFiberView();

  auto parent_slot_parent = manager->CreateFiberView();
  auto parent_sentinel = manager->CreateFiberView();
  parent->element_slot_targets_.push_back(
      ElementSlotMountPoint{parent_slot_parent, parent_sentinel});

  auto old_item = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  old_item->SetTemplateKey(base::String("item_template"));
  old_item->SetBundleUrl(base::String("bundle.js"));
  old_item->MarkAsListItem();
  auto old_item_root = manager->CreateFiberView();
  old_item->result_ = old_item_root;

  auto old_child = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  old_child->SetTemplateKey(base::String("child_template"));
  old_child->SetBundleUrl(base::String("bundle.js"));
  auto old_child_root = manager->CreateFiberView();
  old_child->result_ = old_child_root;

  auto old_stale_child =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  old_stale_child->SetTemplateKey(base::String("stale_template"));
  old_stale_child->SetBundleUrl(base::String("bundle.js"));
  auto old_stale_root = manager->CreateFiberView();
  old_stale_child->result_ = old_stale_root;

  auto item_slot_parent = manager->CreateFiberView();
  auto item_sentinel = manager->CreateFiberView();
  item_slot_parent->InsertNode(old_child_root);
  item_slot_parent->InsertNode(old_stale_root);
  item_slot_parent->InsertNode(item_sentinel);
  old_item->element_slot_targets_.push_back(
      ElementSlotMountPoint{item_slot_parent, item_sentinel});

  auto old_item_slot_children = lepus::CArray::Create();
  old_item_slot_children->emplace_back(lepus::Value(old_child));
  old_item_slot_children->emplace_back(lepus::Value(old_stale_child));
  auto old_item_slots = lepus::CArray::Create();
  old_item_slots->emplace_back(lepus::Value(old_item_slot_children));
  old_item->SetElementSlots(lepus::Value(old_item_slots));

  parent_slot_parent->InsertNode(old_item_root);
  parent_slot_parent->InsertNode(parent_sentinel);
  auto parent_slot_children = lepus::CArray::Create();
  parent_slot_children->emplace_back(lepus::Value(old_item));
  auto parent_slots = lepus::CArray::Create();
  parent_slots->emplace_back(lepus::Value(parent_slot_children));
  parent->SetElementSlots(lepus::Value(parent_slots));

  parent->RemoveElementSlotChild(0, old_item);
  EXPECT_EQ(old_item_root->parent(), nullptr);

  auto new_item = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  new_item->SetTemplateKey(base::String("item_template"));
  new_item->SetBundleUrl(base::String("bundle.js"));
  new_item->MarkAsListItem();

  auto new_child = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  new_child->SetTemplateKey(base::String("child_template"));
  new_child->SetBundleUrl(base::String("bundle.js"));

  auto new_item_slot_children = lepus::CArray::Create();
  new_item_slot_children->emplace_back(lepus::Value(new_child));
  auto new_item_slots = lepus::CArray::Create();
  new_item_slots->emplace_back(lepus::Value(new_item_slot_children));
  new_item->SetElementSlots(lepus::Value(new_item_slots));

  auto resolved = new_item->GetRoot();

  ASSERT_NE(resolved, nullptr);
  EXPECT_EQ(resolved.get(), old_item_root.get());
  EXPECT_EQ(new_item->result_.get(), old_item_root.get());
  EXPECT_EQ(old_item->result_, nullptr);
  EXPECT_EQ(new_child->result_.get(), old_child_root.get());
  EXPECT_EQ(old_child->result_, nullptr);
  EXPECT_EQ(old_stale_child->result_, nullptr);
  EXPECT_EQ(old_stale_root->parent(), nullptr);
  ASSERT_EQ(item_slot_parent->children().size(), 2u);
  EXPECT_EQ(item_slot_parent->children()[0].get(), old_child_root.get());
  EXPECT_EQ(item_slot_parent->children()[1].get(), item_sentinel.get());
}

TEST_P(FiberElementTest,
       ListItemTemplateElementGraphCacheOwnerReclaimAppliesPendingOps) {
  auto parent = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  parent->SetTemplateKey(base::String("parent_template"));
  parent->result_ = manager->CreateFiberView();

  auto parent_slot_parent = manager->CreateFiberView();
  auto parent_sentinel = manager->CreateFiberView();
  parent_slot_parent->InsertNode(parent_sentinel);
  parent->element_slot_targets_.push_back(
      ElementSlotMountPoint{parent_slot_parent, parent_sentinel});

  auto item = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  item->SetTemplateKey(base::String("item_template"));
  item->SetBundleUrl(base::String("bundle.js"));
  item->MarkAsListItem();
  item->result_ = manager->CreateFiberView();

  auto attribute_slots = lepus::CArray::Create();
  attribute_slots->emplace_back(lepus::Value("old_value"));
  item->SetAttributeSlots(lepus::Value(attribute_slots));

  auto target = manager->CreateFiberView();
  auto template_attributes =
      std::make_shared<const TemplateAttributes>(TemplateAttributes{
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("data-test"),
                    lepus::Value(), 0}});
  target->SetTemplateAttributes(template_attributes);
  target->AddDataset("test", lepus::Value("old_value"));
  item->attribute_slot_targets_.push_back(target);
  item->result_->InsertNode(target);

  auto child = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  child->SetTemplateKey(base::String("child_template"));
  child->SetBundleUrl(base::String("bundle.js"));
  child->result_ = manager->CreateFiberView();

  auto child_attribute_slots = lepus::CArray::Create();
  child_attribute_slots->emplace_back(lepus::Value("old_child_value"));
  child->SetAttributeSlots(lepus::Value(child_attribute_slots));

  auto child_target = manager->CreateFiberView();
  child_target->SetTemplateAttributes(template_attributes);
  child_target->AddDataset("test", lepus::Value("old_child_value"));
  child->attribute_slot_targets_.push_back(child_target);
  child->result_->InsertNode(child_target);

  auto item_slot_children = lepus::CArray::Create();
  item_slot_children->emplace_back(lepus::Value(child));
  auto item_slots = lepus::CArray::Create();
  item_slots->emplace_back(lepus::Value(item_slot_children));
  item->SetElementSlots(lepus::Value(item_slots));

  parent_slot_parent->InsertNodeBefore(item->result_, parent_sentinel);
  auto parent_slot_children = lepus::CArray::Create();
  parent_slot_children->emplace_back(lepus::Value(item));
  auto parent_slots = lepus::CArray::Create();
  parent_slots->emplace_back(lepus::Value(parent_slot_children));
  parent->SetElementSlots(lepus::Value(parent_slots));

  parent->RemoveElementSlotChild(0, item);
  ASSERT_TRUE(item->IsInTemplateCache());
  ASSERT_TRUE(child->IsInTemplateCache());
  EXPECT_EQ(item->result_->parent(), nullptr);

  item->SetAttributeSlot(0, lepus::Value("new_value"));
  child->SetAttributeSlot(0, lepus::Value("new_child_value"));
  auto* test_data_before_reclaim = DatasetValue(target.get(), "test");
  ASSERT_NE(test_data_before_reclaim, nullptr);
  EXPECT_EQ(test_data_before_reclaim->StdString(), "old_value");
  auto* child_test_data_before_reclaim =
      DatasetValue(child_target.get(), "test");
  ASSERT_NE(child_test_data_before_reclaim, nullptr);
  EXPECT_EQ(child_test_data_before_reclaim->StdString(), "old_child_value");
  ASSERT_EQ(item->pending_operations_.size(), 1u);
  ASSERT_EQ(child->pending_operations_.size(), 1u);

  parent->InsertElementSlotChild(0, item, nullptr);

  EXPECT_FALSE(item->IsInTemplateCache());
  EXPECT_FALSE(child->IsInTemplateCache());
  EXPECT_TRUE(item->pending_operations_.empty());
  EXPECT_TRUE(child->pending_operations_.empty());
  auto* test_data_after_reclaim = DatasetValue(target.get(), "test");
  ASSERT_NE(test_data_after_reclaim, nullptr);
  EXPECT_EQ(test_data_after_reclaim->StdString(), "new_value");
  auto* child_test_data_after_reclaim =
      DatasetValue(child_target.get(), "test");
  ASSERT_NE(child_test_data_after_reclaim, nullptr);
  EXPECT_EQ(child_test_data_after_reclaim->StdString(), "new_child_value");
  ASSERT_EQ(parent_slot_parent->children().size(), 2u);
  EXPECT_EQ(parent_slot_parent->children()[0].get(), item->result_.get());
  EXPECT_EQ(parent_slot_parent->children()[1].get(), parent_sentinel.get());
}

TEST_P(FiberElementTest, ElementTemplateStaticEventsSyncAfterAttach) {
  manager->config_->SetEnableEventHandleRefactor(true);
  manager->SetConfig(manager->config_);
  tasm->page_config_ = manager->config_;

  auto default_entry = std::make_shared<TemplateEntry>();
  default_entry->SetName(DEFAULT_ENTRY_NAME);
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = default_entry;

  ElementTemplateInfo template_info;
  template_info.exist_ = true;
  template_info.key_ = "root_template";

  auto target_info = ElementInfo();
  target_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  target_info.attributes_ =
      std::make_shared<const TemplateAttributes>(TemplateAttributes{
          Attribute{ATTRIBUTE_BINDING_TYPE_STATIC, base::String("bindtap"),
                    lepus::Value("onStaticTap"), 0}});
  template_info.elements_.emplace_back(std::move(target_info));

  auto generated =
      TreeResolver::GenerateElementsFromTemplateInfo(template_info);
  auto* target = generated.result_.get();
  ASSERT_NE(target, nullptr);
  ASSERT_EQ(generated.static_event_targets_.size(), 1u);
  EXPECT_EQ(generated.static_event_targets_[0].get(), target);
  ASSERT_NE(target->event_map().find("tap"), target->event_map().end());
  EXPECT_EQ(target->GetEventListenerMap()->Find("tap"), nullptr);

  auto root = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  root->entry_ = default_entry.get();
  std::promise<GeneratedElementsResult> promise;
  auto future = promise.get_future();
  root->async_create_task_ =
      fml::MakeRefCounted<base::OnceTask<GeneratedElementsResult>>(
          [generated = std::move(generated),
           promise = std::move(promise)]() mutable {
            promise.set_value(std::move(generated));
          },
          std::move(future));

  auto resolved = root->GetRoot();

  EXPECT_EQ(resolved.get(), target);
  EXPECT_EQ(target->element_manager(), manager);
  auto* listeners = target->GetEventListenerMap()->Find("tap");
  ASSERT_NE(listeners, nullptr);
  ASSERT_EQ(listeners->size(), 1u);
  EXPECT_FALSE(listeners->front()->GetOptions().IsCapture());
  EXPECT_FALSE(listeners->front()->GetOptions().IsCatch());
}

TEST_P(FiberElementTest,
       ElementTemplateInitialRootAttributesSplitAfterPrepare) {
  manager->config_->SetEnableEventHandleRefactor(true);
  manager->SetConfig(manager->config_);
  tasm->page_config_ = manager->config_;

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
                    lepus::Value(), 0},
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("bindtap"),
                    lepus::Value(), 1}});
  template_info->elements_.emplace_back(std::move(root_info));
  default_entry->template_bundle_.element_template_infos_["root_template"] =
      std::move(template_info);

  auto attribute_slots = lepus::CArray::Create();
  attribute_slots->emplace_back(lepus::Value("compiled-value"));
  attribute_slots->emplace_back(lepus::Value("onTap"));

  auto old_root_attributes = lepus::Dictionary::Create();
  old_root_attributes->SetValue("data-root", lepus::Value("old-root"));
  old_root_attributes->SetValue("data-old", lepus::Value("old-value"));
  old_root_attributes->SetValue("bindfocus", lepus::Value("onOldFocus"));

  auto root = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  root->entry_ = default_entry.get();
  root->SetTemplateKey(base::String("root_template"));
  root->SetAttributeSlots(lepus::Value(attribute_slots));
  root->SetRootAttributes(lepus::Value(old_root_attributes));
  root->PrepareAsyncCreateElementTree();

  auto new_root_attributes = lepus::Dictionary::Create();
  new_root_attributes->SetValue("data-root", lepus::Value("new-root"));
  new_root_attributes->SetValue("bindfocus", lepus::Value("onNewFocus"));
  root->SetRootAttributes(lepus::Value(new_root_attributes));

  auto resolved = root->GetRoot();
  ASSERT_NE(resolved, nullptr);

  auto* compiled_data = DatasetValue(resolved.get(), "test");
  ASSERT_NE(compiled_data, nullptr);
  EXPECT_EQ(compiled_data->StdString(), "compiled-value");
  auto* root_data = DatasetValue(resolved.get(), "root");
  ASSERT_NE(root_data, nullptr);
  EXPECT_EQ(root_data->StdString(), "new-root");
  EXPECT_EQ(resolved->data_model_->dataset().count("old"), 0u);

  auto* tap_listeners = resolved->GetEventListenerMap()->Find("tap");
  ASSERT_NE(tap_listeners, nullptr);
  ASSERT_EQ(tap_listeners->size(), 1u);
  auto* focus_listeners = resolved->GetEventListenerMap()->Find("focus");
  ASSERT_NE(focus_listeners, nullptr);
  ASSERT_EQ(focus_listeners->size(), 1u);
  auto focus_iter = resolved->event_map().find("focus");
  ASSERT_NE(focus_iter, resolved->event_map().end());
  EXPECT_EQ(focus_iter->second->function(), "onNewFocus");

  root->SetAttributeSlot(0, lepus::Value("updated-compiled-value"));
  compiled_data = DatasetValue(resolved.get(), "test");
  ASSERT_NE(compiled_data, nullptr);
  EXPECT_EQ(compiled_data->StdString(), "updated-compiled-value");
}

TEST_P(FiberElementTest, ComponentAtIndexReturnsResolvedTemplateElementRootId) {
  auto template_item =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  template_item->SetTypedTag(base::String("view"));
  auto template_root = template_item->GetRoot();
  ASSERT_NE(template_root, nullptr);

  auto callbacks = CreateTemplateCallbackValuesReturningSign(
      tasm.get(), template_item->impl_id());
  auto target = manager->CreateFiberList(
      tasm.get(), base::String("list"), callbacks.component_at_index,
      callbacks.enqueue_component, callbacks.component_at_indexes);
  auto page = manager->CreateFiberPage("page", 11);
  manager->SetFiberPageElement(page);
  page->InsertNode(target);

  EXPECT_EQ(target->ComponentAtIndex(0, 1, false), template_root->impl_id());
}

TEST_P(FiberElementTest, EnqueueComponentMapsResolvedTemplateRootIdToShellId) {
  auto template_item =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  template_item->SetTypedTag(base::String("view"));
  auto template_root = template_item->GetRoot();
  ASSERT_NE(template_root, nullptr);

  auto callbacks = CreateTemplateCallbackValuesRecordingEnqueue(
      tasm.get(), template_item->impl_id());
  auto target = manager->CreateFiberList(
      tasm.get(), base::String("list"), callbacks.component_at_index,
      callbacks.enqueue_component, callbacks.component_at_indexes);
  auto page = manager->CreateFiberPage("page", 11);
  manager->SetFiberPageElement(page);
  page->InsertNode(target);

  ASSERT_EQ(target->ComponentAtIndex(0, 1, false), template_root->impl_id());
  target->EnqueueComponent(template_root->impl_id());

  EXPECT_EQ(callbacks.runtime->GetGlobalData("lastEnqueueSign").Number(),
            template_item->impl_id());
}

TEST_P(FiberElementTest,
       ComponentAtIndexKeepsUnresolvedTemplateElementShellId) {
  auto template_item =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  template_item->SetTypedTag(base::String("view"));
  ASSERT_EQ(template_item->result_, nullptr);

  auto callbacks = CreateTemplateCallbackValuesReturningSign(
      tasm.get(), template_item->impl_id());
  auto target = manager->CreateFiberList(
      tasm.get(), base::String("list"), callbacks.component_at_index,
      callbacks.enqueue_component, callbacks.component_at_indexes);
  auto page = manager->CreateFiberPage("page", 11);
  manager->SetFiberPageElement(page);
  page->InsertNode(target);

  EXPECT_EQ(target->ComponentAtIndex(0, 1, false), template_item->impl_id());
  EXPECT_EQ(template_item->result_, nullptr);
}

TEST_P(FiberElementTest,
       OnPatchFinishNormalizesTemplateListItemIdsAfterResolve) {
  auto page = manager->CreateFiberPage("page", 11);
  manager->SetFiberPageElement(page);

  auto template_item =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  template_item->SetTypedTag(base::String("view"));
  auto template_root = template_item->GetRoot();
  ASSERT_NE(template_root, nullptr);

  auto normal_item = manager->CreateFiberView();
  auto missing_id = template_root->impl_id() + normal_item->impl_id() + 1000;

  auto options = std::make_shared<PipelineOptions>();
  options->trigger_layout_ = false;
  options->list_comp_id_ = template_item->impl_id();
  options->list_item_ids_ = {template_item->impl_id(), normal_item->impl_id(),
                             missing_id};

  manager->OnPatchFinish(options, page.get());

  EXPECT_EQ(options->list_comp_id_, template_root->impl_id());
  ASSERT_EQ(options->list_item_ids_.size(), 3u);
  EXPECT_EQ(options->list_item_ids_[0], template_root->impl_id());
  EXPECT_EQ(options->list_item_ids_[1], normal_item->impl_id());
  EXPECT_EQ(options->list_item_ids_[2], missing_id);
}

TEST_P(FiberElementTest,
       OnPatchFinishKeepsUnresolvedTemplateListItemIdsUnchanged) {
  auto page = manager->CreateFiberPage("page", 11);
  manager->SetFiberPageElement(page);

  auto template_item =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  template_item->SetTypedTag(base::String("view"));
  ASSERT_EQ(template_item->result_, nullptr);

  auto options = std::make_shared<PipelineOptions>();
  options->trigger_layout_ = false;
  options->list_comp_id_ = template_item->impl_id();
  options->list_item_ids_ = {template_item->impl_id()};

  manager->OnPatchFinish(options, page.get());

  EXPECT_EQ(options->list_comp_id_, template_item->impl_id());
  ASSERT_EQ(options->list_item_ids_.size(), 1u);
  EXPECT_EQ(options->list_item_ids_[0], template_item->impl_id());
  EXPECT_EQ(template_item->result_, nullptr);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
