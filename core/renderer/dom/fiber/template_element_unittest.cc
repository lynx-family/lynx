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
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
