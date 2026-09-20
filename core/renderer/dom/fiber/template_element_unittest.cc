// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/fiber/template_element.h"

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/dom/fiber/page_element.h"
#include "core/renderer/dom/fiber/tree_resolver.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/testing/fiber_element_test.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

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

  auto cloned_root = cloned->GetRoot();

  ASSERT_NE(cloned_root, nullptr);
  EXPECT_TRUE(cloned_root->is_raw_text());
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
