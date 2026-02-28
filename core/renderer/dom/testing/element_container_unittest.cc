// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <iterator>

#define private public
#define protected public

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/element_container.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/fiber/wrapper_element.h"
#include "core/renderer/dom/fragment/fragment.h"
#include "core/renderer/tasm/config.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class ElementContainerTest : public ::testing::Test {
 public:
  ElementContainerTest() {}
  ~ElementContainerTest() override {}

  void SetUp() override {
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    manager = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<MockPaintingContext>(), tasm_mediator.get(),
        lynx_env_config);
    auto config = std::make_shared<PageConfig>();
    config->SetEnableZIndex(true);
    manager->SetConfig(config);
  }

  std::unique_ptr<lynx::tasm::ElementManager> manager;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator;
};

TEST_F(ElementContainerTest, Create) {
  auto element = manager->CreateFiberElement("view");
  auto element_container = std::make_unique<ElementContainer>(element.get());
  EXPECT_EQ(element_container->element(), element.get());

  auto child = manager->CreateFiberElement("view");
  child->CreateElementContainer(false);
  auto child_container = child->element_container_impl();
  EXPECT_EQ(child_container->element(), child.get());
}

TEST_F(ElementContainerTest, InsertAndDestroy) {
  auto element = manager->CreateFiberElement("view");
  element->CreateElementContainer(false);
  auto element_container = element->element_container_impl();

  auto child = manager->CreateFiberElement("view");
  element->AddChildAt(child, 0);
  EXPECT_EQ(child->parent(), element.get());

  child->CreateElementContainer(false);
  auto child_container = child->element_container_impl();
  child_container->InsertSelf();
  EXPECT_EQ(child_container->parent(), element_container);
  EXPECT_EQ(element_container->children().size(), static_cast<size_t>(1));

  child_container->RemoveSelf(true);
  EXPECT_EQ(child_container->parent(), nullptr);
  EXPECT_EQ(element_container->children().size(), static_cast<size_t>(0));
}

TEST_F(ElementContainerTest, Children) {
  auto element = manager->CreateFiberElement("view");
  element->CreateElementContainer(false);
  auto element_container = element->element_container_impl();

  auto child = manager->CreateFiberElement("view");
  element->AddChildAt(child, 0);
  child->CreateElementContainer(false);
  auto child_container = child->element_container_impl();
  child_container->InsertSelf();
  element_container->AddChild(child_container, 0);  // test insert again

  EXPECT_EQ(child_container->parent(), element_container);
  EXPECT_EQ(element_container->children().size(), static_cast<size_t>(1));
  child_container->RemoveSelf(false);

  EXPECT_EQ(element_container->children().size(), static_cast<size_t>(0));
}

TEST_F(ElementContainerTest, ZIndex) {
  auto page = manager->CreateFiberElement("page");
  manager->SetRoot(page.get());
  manager->SetRootOnLayout(page->impl_id());
  page->FlushProps();
  auto page_container = page->element_container_impl();
  ASSERT_TRUE(page->IsStackingContextNode());

  auto parent_element = manager->CreateFiberElement("view");
  page->AddChildAt(parent_element, 0);
  EXPECT_EQ(parent_element->parent(), page.get());

  parent_element->FlushProps();
  auto parent_container = parent_element->element_container_impl();
  parent_container->InsertSelf();
  EXPECT_EQ(parent_container->parent(), page_container);
  EXPECT_EQ(page_container->children().size(), static_cast<size_t>(1));

  auto child = manager->CreateFiberElement("view");
  parent_element->AddChildAt(child, 0);
  child->SetStyleInternal(CSSPropertyID::kPropertyIDZIndex,
                          tasm::CSSValue(1, CSSValuePattern::NUMBER));

  ASSERT_TRUE(child->IsStackingContextNode());
  child->FlushProps();
  ASSERT_TRUE(child->IsStackingContextNode());
  EXPECT_EQ(child->ZIndex(), static_cast<int>(1));

  auto child_container = child->element_container_impl();
  ASSERT_TRUE(child_container->IsStackingContextNode());
  child_container->InsertSelf();
  EXPECT_EQ(child_container->parent(), page_container);

  child->ResetStyle({CSSPropertyID::kPropertyIDZIndex});
  ASSERT_FALSE(child->IsStackingContextNode());
  ASSERT_FALSE(child_container->IsStackingContextNode());
  EXPECT_EQ(child->ZIndex(), static_cast<int>(0));
}

TEST_F(ElementContainerTest, ZIndexMove) {
  auto page = manager->CreateFiberElement("page");
  manager->SetRoot(page.get());
  manager->SetRootOnLayout(page->impl_id());
  page->FlushProps();
  auto page_container = page->element_container_impl();
  ASSERT_TRUE(page->IsStackingContextNode());

  auto parent_element = manager->CreateFiberElement("view");
  page->AddChildAt(parent_element, 0);
  EXPECT_EQ(parent_element->parent(), page.get());

  parent_element->FlushProps();
  auto parent_container = parent_element->element_container_impl();
  parent_container->InsertSelf();
  EXPECT_EQ(parent_container->parent(), page_container);
  EXPECT_EQ(page_container->children().size(), static_cast<size_t>(1));

  auto child = manager->CreateFiberElement("view");
  parent_element->AddChildAt(child, 0);
  child->SetStyleInternal(CSSPropertyID::kPropertyIDZIndex,
                          tasm::CSSValue(1, CSSValuePattern::NUMBER));

  ASSERT_TRUE(child->IsStackingContextNode());
  child->FlushProps();
  ASSERT_TRUE(child->IsStackingContextNode());
  EXPECT_EQ(child->ZIndex(), static_cast<int>(1));

  auto child_container = child->element_container_impl();
  ASSERT_TRUE(child_container->IsStackingContextNode());
  child_container->InsertSelf();
  EXPECT_EQ(child_container->parent(), page_container);
}

TEST_F(ElementContainerTest, StackingContextChanged) {
  auto page = manager->CreateFiberElement("page");
  manager->SetRoot(page.get());
  manager->SetRootOnLayout(page->impl_id());
  page->FlushProps();
  auto page_container = page->element_container_impl();
  ASSERT_TRUE(page->IsStackingContextNode());

  auto parent_element = manager->CreateFiberElement("view");
  page->AddChildAt(parent_element, 0);
  parent_element->SetStyleInternal(CSSPropertyID::kPropertyIDZIndex,
                                   tasm::CSSValue(1, CSSValuePattern::NUMBER));
  EXPECT_EQ(parent_element->parent(), page.get());

  parent_element->FlushProps();
  auto parent_container = parent_element->element_container_impl();
  parent_container->InsertSelf();
  EXPECT_EQ(parent_container->parent(), page_container);
  EXPECT_EQ(page_container->children().size(), static_cast<size_t>(1));

  // Add z-index children to child element
  auto child = manager->CreateFiberElement("view");
  parent_element->AddChildAt(child, 0);
  child->SetStyleInternal(CSSPropertyID::kPropertyIDZIndex,
                          tasm::CSSValue(1, CSSValuePattern::NUMBER));
  child->FlushProps();
  auto child_container = child->element_container_impl();
  ASSERT_TRUE(child_container->IsStackingContextNode());
  child_container->InsertSelf();
  EXPECT_EQ(child_container->parent(), parent_container);
}

TEST_F(ElementContainerTest, TransitionToNativeView) {
  auto page = manager->CreateFiberElement("page");
  manager->SetRoot(page.get());
  manager->SetRootOnLayout(page->impl_id());
  page->FlushProps();

  auto element = manager->CreateFiberElement("raw-text");
  element->FlushProps();
  ASSERT_TRUE(element->IsLayoutOnly());
  element->TransitionToNativeView();
  // Virtual element should never create native view.
  ASSERT_TRUE(element->IsLayoutOnly());

  element = manager->CreateFiberElement("view");
  element->SetStyleInternal(CSSPropertyID::kPropertyIDOverflow,
                            tasm::CSSValue(starlight::OverflowType::kVisible));
  element->FlushProps();
  ASSERT_TRUE(element->IsLayoutOnly());
  element->TransitionToNativeView();
  ASSERT_FALSE(element->IsLayoutOnly());
}

TEST_F(ElementContainerTest, FiberElementCase0) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberPage("page", 11);

  auto element0 = manager->CreateFiberNode("view");
  element0->SetAttribute("enable-layout", lepus::Value("false"));

  auto element_before_black = manager->CreateFiberNode("view");
  element_before_black->SetAttribute("enable-layout", lepus::Value("false"));

  auto text = manager->CreateFiberNode("text");

  // layout_only node
  auto ref = manager->CreateFiberWrapperElement();
  ref->InsertNode(text);

  auto element_after_yellow = manager->CreateFiberNode("view");
  element_after_yellow->SetAttribute("enable-layout", lepus::Value("false"));

  page->InsertNode(element0);
  page->InsertNode(element_before_black);
  page->InsertNode(ref);
  page->InsertNode(element_after_yellow);

  page->FlushActionsAsRoot();

  EXPECT_TRUE(ref->IsLayoutOnly());

  auto page_container = page->element_container_impl();
  auto page_container_children = page_container->children();

  EXPECT_TRUE(static_cast<int>(page_container_children.size()) == 5);
  EXPECT_TRUE(page_container->none_layout_only_children_size_ == 4);

  auto element0_container_index =
      ElementContainer::GetUIIndexForChildForFiber(page.get(), element0.get());
  auto element_before_black_index =
      ElementContainer::GetUIIndexForChildForFiber(page.get(),
                                                   element_before_black.get());
  auto ref_container_index =
      ElementContainer::GetUIIndexForChildForFiber(page.get(), ref.get());
  auto element_after_yellow_index =
      ElementContainer::GetUIIndexForChildForFiber(page.get(),
                                                   element_after_yellow.get());

  EXPECT_TRUE(element0_container_index == 0);
  EXPECT_TRUE(element_before_black_index == 1);
  EXPECT_TRUE(ref_container_index == 2);
  EXPECT_TRUE(element_after_yellow_index == 3);

  auto painting_context =
      static_cast<MockPaintingContext*>(manager->painting_context()->impl());

  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto page_painting_children = page_painting_node->children_;
  EXPECT_TRUE(page_painting_children.size() == 4);
  EXPECT_TRUE(page_painting_children[0]->id_ == element0->impl_id());
  EXPECT_TRUE(page_painting_children[1]->id_ ==
              element_before_black->impl_id());

  EXPECT_TRUE(page_painting_children[2]->id_ == text->impl_id());

  EXPECT_TRUE(page_painting_children[3]->id_ ==
              element_after_yellow->impl_id());
}

TEST_F(ElementContainerTest, FiberElementCase0_UnifiedBehavior) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableUnifyFixedBehavior(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberPage("page", 11);

  auto element0 = manager->CreateFiberNode("view");
  element0->SetAttribute("enable-layout", lepus::Value("false"));

  auto element_before_black = manager->CreateFiberNode("view");
  element_before_black->SetAttribute("enable-layout", lepus::Value("false"));

  auto text = manager->CreateFiberNode("text");

  // layout_only node
  auto ref = manager->CreateFiberWrapperElement();
  ref->InsertNode(text);

  auto element_after_yellow = manager->CreateFiberNode("view");
  element_after_yellow->SetAttribute("enable-layout", lepus::Value("false"));

  page->InsertNode(element0);
  page->InsertNode(element_before_black);
  page->InsertNode(ref);
  page->InsertNode(element_after_yellow);

  page->FlushActionsAsRoot();

  EXPECT_TRUE(ref->IsLayoutOnly());

  auto page_container = page->element_container_impl();
  auto page_container_children = page_container->children();

  EXPECT_TRUE(static_cast<int>(page_container_children.size()) == 5);
  EXPECT_TRUE(page_container->none_layout_only_children_size_ == 4);

  auto element0_container_index =
      ElementContainer::GetUIIndexForChildForFiber(page.get(), element0.get());
  auto element_before_black_index =
      ElementContainer::GetUIIndexForChildForFiber(page.get(),
                                                   element_before_black.get());
  auto ref_container_index =
      ElementContainer::GetUIIndexForChildForFiber(page.get(), ref.get());
  auto element_after_yellow_index =
      ElementContainer::GetUIIndexForChildForFiber(page.get(),
                                                   element_after_yellow.get());

  EXPECT_TRUE(element0_container_index == 0);
  EXPECT_TRUE(element_before_black_index == 1);
  EXPECT_TRUE(ref_container_index == 2);
  EXPECT_TRUE(element_after_yellow_index == 3);

  auto painting_context =
      static_cast<MockPaintingContext*>(manager->painting_context()->impl());

  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto page_painting_children = page_painting_node->children_;
  EXPECT_TRUE(page_painting_children.size() == 4);
  EXPECT_TRUE(page_painting_children[0]->id_ == element0->impl_id());
  EXPECT_TRUE(page_painting_children[1]->id_ ==
              element_before_black->impl_id());

  EXPECT_TRUE(page_painting_children[2]->id_ == text->impl_id());

  EXPECT_TRUE(page_painting_children[3]->id_ ==
              element_after_yellow->impl_id());
}

TEST_F(ElementContainerTest, FiberElementLayoutOnlyTransitionCase0) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberPage("page", 11);

  auto element0 = manager->CreateFiberView();
  element0->MarkCanBeLayoutOnly(false);

  auto element1 = manager->CreateFiberView();
  element1->computed_css_style()->SetOverflowDefaultVisible(true);
  element1->has_layout_only_props_ = true;

  page->InsertNode(element0);
  element0->InsertNode(element1);

  page->FlushActionsAsRoot();

  auto painting_context =
      static_cast<MockPaintingContext*>(manager->painting_context()->impl());

  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto page_painting_children = page_painting_node->children_;
  EXPECT_TRUE(page_painting_children.size() == 1);
  EXPECT_TRUE(page_painting_children[0]->id_ == element0->impl_id());

  auto* element0_painting_node =
      painting_context->node_map_.at(element0->impl_id()).get();
  auto element0_painting_children = element0_painting_node->children_;
  EXPECT_TRUE(element0_painting_children.size() == 0);

  EXPECT_TRUE(element1->IsLayoutOnly() == true);

  auto element1_it = painting_context->node_map_.find(element1->impl_id());

  EXPECT_TRUE(element1_it == painting_context->node_map_.end());

  // make element1 transition to non-layout only ,and insert child at the same
  // time
  element1->SetAttribute("enable-layout", lepus::Value("false"));
  auto text = manager->CreateFiberText("text");
  element1->InsertNode(text);
  page->FlushActionsAsRoot();

  EXPECT_TRUE(element1->IsLayoutOnly() == false);

  element0_painting_children = element0_painting_node->children_;
  EXPECT_TRUE(element0_painting_children.size() == 1);
  EXPECT_TRUE(element0_painting_children[0]->id_ == element1->impl_id());

  auto element1_painting_node =
      painting_context->node_map_.at(element1->impl_id()).get();
  auto element1_painting_children = element1_painting_node->children_;
  EXPECT_TRUE(element1_painting_children.size() == 1);

  EXPECT_TRUE(element1_painting_children.size() == 1);
  EXPECT_TRUE(element1_painting_children[0]->id_ == text->impl_id());
}

TEST_F(ElementContainerTest, FiberElementCase1) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberPage("page", 11);

  auto element0 = manager->CreateFiberNode("view");
  element0->SetAttribute("enable-layout", lepus::Value("false"));

  auto element = manager->CreateFiberNode("view");
  element->SetAttribute("enable-layout", lepus::Value("false"));

  // layout_only node
  auto ref = manager->CreateFiberWrapperElement();
  ref->InsertNode(element);

  auto element_before_black = manager->CreateFiberNode("view");
  element_before_black->SetAttribute("enable-layout", lepus::Value("false"));

  auto element_after_yellow = manager->CreateFiberNode("view");
  element_after_yellow->SetAttribute("enable-layout", lepus::Value("false"));

  page->InsertNode(element0);
  page->InsertNode(ref);
  page->InsertNode(element_after_yellow);

  page->InsertNodeBefore(element_before_black, ref);

  page->FlushActionsAsRoot();

  EXPECT_TRUE(ref->IsLayoutOnly());

  // Check the element container tree!
  auto* page_container = page->element_container_impl();

  auto page_container_children = page_container->children();

  EXPECT_TRUE(static_cast<int>(page_container_children.size()) == 5);
  EXPECT_TRUE(page_container->none_layout_only_children_size_ == 4);

  auto painting_context =
      static_cast<MockPaintingContext*>(manager->painting_context()->impl());

  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto page_painting_children = page_painting_node->children_;
  EXPECT_TRUE(page_painting_children.size() == 4);

  EXPECT_TRUE(page_painting_children[0]->id_ == element0->impl_id());
  EXPECT_TRUE(page_painting_children[1]->id_ ==
              element_before_black->impl_id());
  EXPECT_TRUE(page_painting_children[2]->id_ == element->impl_id());
  EXPECT_TRUE(page_painting_children[3]->id_ ==
              element_after_yellow->impl_id());

  auto text = manager->CreateFiberNode("text");
  ref->InsertNodeBefore(text, element);

  page->FlushActionsAsRoot();

  EXPECT_TRUE(page_container->none_layout_only_children_size_ == 5);

  page_painting_children = page_painting_node->children_;
  EXPECT_TRUE(page_painting_children.size() == 5);

  EXPECT_TRUE(page_painting_children[0]->id_ == element0->impl_id());
  EXPECT_TRUE(page_painting_children[1]->id_ ==
              element_before_black->impl_id());
  EXPECT_TRUE(page_painting_children[2]->id_ == text->impl_id());
  EXPECT_TRUE(page_painting_children[3]->id_ == element->impl_id());
  EXPECT_TRUE(page_painting_children[4]->id_ ==
              element_after_yellow->impl_id());
}

TEST_F(ElementContainerTest, FiberElementCase2) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberPage("page", 11);

  auto element0 = manager->CreateFiberNode("view");
  element0->SetAttribute("enable-layout", lepus::Value("false"));

  auto element_before_black = manager->CreateFiberNode("view");
  element_before_black->SetAttribute("enable-layout", lepus::Value("false"));

  auto element = manager->CreateFiberNode("view");
  element->SetAttribute("enable-layout", lepus::Value("false"));

  auto text = manager->CreateFiberNode("text");

  // layout_only node
  auto ref = manager->CreateFiberWrapperElement();
  ref->InsertNode(element);
  ref->InsertNode(text);

  auto element_after_yellow = manager->CreateFiberNode("view");
  element_after_yellow->SetAttribute("enable-layout", lepus::Value("false"));

  page->InsertNode(element0);
  page->InsertNode(element_after_yellow);

  page->InsertNodeBefore(element_before_black, element_after_yellow);
  page->InsertNodeBefore(ref, element_before_black);

  page->RemoveNode(element_after_yellow);

  page->FlushActionsAsRoot();

  EXPECT_TRUE(ref->IsLayoutOnly());

  // Check the element container tree!
  auto* page_container = page->element_container_impl();

  auto page_container_children = page_container->children();

  EXPECT_TRUE(static_cast<int>(page_container_children.size()) == 5);
  EXPECT_TRUE(page_container->none_layout_only_children_size_ == 4);

  auto painting_context =
      static_cast<MockPaintingContext*>(manager->painting_context()->impl());

  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto page_painting_children = page_painting_node->children_;
  EXPECT_TRUE(page_painting_children.size() == 4);

  EXPECT_TRUE(page_painting_children[0]->id_ == element0->impl_id());
  EXPECT_TRUE(page_painting_children[1]->id_ == element->impl_id());
  EXPECT_TRUE(page_painting_children[2]->id_ == text->impl_id());
  EXPECT_TRUE(page_painting_children[3]->id_ ==
              element_before_black->impl_id());
}

TEST_F(ElementContainerTest, FiberElementUpdateLayoutForFixed) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberPage("page", 11);

  auto element0 = manager->CreateFiberView();
  element0->SetStyle(CSSPropertyID::kPropertyIDWidth, lepus::Value("200px"));
  element0->SetStyle(CSSPropertyID::kPropertyIDHeight, lepus::Value("200px"));
  element0->SetStyle(CSSPropertyID::kPropertyIDTop, lepus::Value("100px"));
  element0->SetStyle(CSSPropertyID::kPropertyIDPosition,
                     lepus::Value("absolute"));
  element0->computed_css_style()->SetOverflowDefaultVisible(true);

  auto element_fixed = manager->CreateFiberView();
  element_fixed->SetAttribute("enable-layout", lepus::Value("false"));
  element_fixed->SetStyle(CSSPropertyID::kPropertyIDPosition,
                          lepus::Value("fixed"));

  page->InsertNode(element0);
  element0->InsertNode(element_fixed);

  page->FlushActionsAsRoot();

  EXPECT_TRUE(element0->IsLayoutOnly());

  // mock UpdateLayout to element!
  page->UpdateLayout(0, 0, kWidth, kHeight, {0}, {0}, {0}, nullptr, 0);
  element0->UpdateLayout(100, 100, 200, 200, {0}, {0}, {0}, nullptr, 0);
  element_fixed->UpdateLayout(0, 0, 200, 200, {0}, {0}, {0}, nullptr, 0);

  // mock to dispatch updateLayout to painting node!
  page->element_container_impl()->UpdateLayout(page->left(), page->top());

  auto painting_context =
      static_cast<MockPaintingContext*>(manager->painting_context()->impl());

  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto page_painting_children = page_painting_node->children_;
  EXPECT_TRUE(page_painting_children.size() == 1);

  EXPECT_TRUE(page_painting_children[0]->id_ == element_fixed->impl_id());

  auto element_fixed_painting_node =
      painting_context->node_map_.at(element_fixed->impl_id()).get();
  EXPECT_TRUE(element_fixed_painting_node->frame_.left_ == 0);
  EXPECT_TRUE(element_fixed_painting_node->frame_.top_ == 0);
  EXPECT_TRUE(element_fixed_painting_node->frame_.width_ == 200);
  EXPECT_TRUE(element_fixed_painting_node->frame_.height_ == 200);
}

TEST_F(ElementContainerTest, FiberElementUpdateLayoutWithException) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberPage("page", 11);

  auto element0 = manager->CreateFiberView();
  element0->SetStyle(CSSPropertyID::kPropertyIDWidth, lepus::Value("200px"));
  element0->SetStyle(CSSPropertyID::kPropertyIDHeight, lepus::Value("200px"));
  element0->SetStyle(CSSPropertyID::kPropertyIDTop, lepus::Value("100px"));
  element0->SetStyle(CSSPropertyID::kPropertyIDPosition,
                     lepus::Value("absolute"));
  element0->computed_css_style()->SetOverflowDefaultVisible(true);
  page->InsertNode(element0);

  page->FlushActionsAsRoot();

  auto element_no_flush = manager->CreateFiberView();
  page->InsertNode(element_no_flush);
  EXPECT_EQ(element0->next_render_sibling_, nullptr);
  element0->next_render_sibling_ = element_no_flush.get();

  EXPECT_TRUE(element0->IsLayoutOnly());

  // mock UpdateLayout to element!
  page->UpdateLayout(0, 0, kWidth, kHeight, {0}, {0}, {0}, nullptr, 0);
  element0->UpdateLayout(100, 100, 200, 200, {0}, {0}, {0}, nullptr, 0);

  // mock to dispatch updateLayout to painting node!
  page->element_container_impl()->UpdateLayout(page->left(), page->top());

  auto painting_context =
      static_cast<MockPaintingContext*>(manager->painting_context()->impl());

  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto page_painting_children = page_painting_node->children_;
}

TEST_F(ElementContainerTest, InsertFixedNew) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFixedNew(true);
  config->SetEnableZIndex(true);
  manager->SetConfig(config);
  auto page = manager->CreateFiberElement("page");
  manager->SetRoot(page.get());
  manager->SetRootOnLayout(page->impl_id());
  page->FlushProps();
  auto page_container = page->element_container_impl();
  ASSERT_TRUE(page->IsStackingContextNode());

  auto parent_element = manager->CreateFiberElement("view");
  page->AddChildAt(parent_element, 0);
  EXPECT_EQ(parent_element->parent(), page.get());

  parent_element->FlushProps();
  auto parent_container = parent_element->element_container_impl();
  parent_container->InsertSelf();
  EXPECT_EQ(parent_container->parent(), page_container);
  EXPECT_EQ(page_container->children().size(), static_cast<size_t>(1));

  auto fixed_child = manager->CreateFiberElement("view");
  parent_element->AddChildAt(fixed_child, 0);
  fixed_child->SetStyleInternal(CSSPropertyID::kPropertyIDPosition,
                                tasm::CSSValue(2, CSSValuePattern::NUMBER));

  ASSERT_TRUE(fixed_child->IsStackingContextNode());
  fixed_child->FlushProps();
  ASSERT_TRUE(fixed_child->IsStackingContextNode());
  EXPECT_TRUE(fixed_child->IsNewFixed());

  auto fixed_child_container = fixed_child->element_container_impl();
  ASSERT_TRUE(fixed_child_container->IsStackingContextNode());
  fixed_child_container->InsertSelf();
  EXPECT_EQ(fixed_child_container->parent(), page_container);

  fixed_child->ResetStyle({CSSPropertyID::kPropertyIDPosition});
  ASSERT_FALSE(fixed_child->IsStackingContextNode());
  ASSERT_FALSE(fixed_child_container->IsStackingContextNode());
  EXPECT_FALSE(fixed_child->IsNewFixed());
  fixed_child_container->StyleChanged();

  // Attach to parent container
  EXPECT_EQ(fixed_child_container->parent(), parent_container);
  EXPECT_EQ(manager->fixed_node_list_.size(), 0);
  {
    auto child_sibling = manager->CreateFiberElement("view");
    parent_element->AddChildAt(child_sibling, 1);
    child_sibling->SetStyleInternal(CSSPropertyID::kPropertyIDPosition,
                                    tasm::CSSValue(2, CSSValuePattern::NUMBER));
    child_sibling->FlushProps();

    auto child_sibling_container = child_sibling->element_container_impl();
    child_sibling_container->InsertSelf();
    EXPECT_EQ(child_sibling_container->parent(), page_container);
    // 0 parent_container 1 out child_container

    auto l_front = manager->fixed_node_list_.begin();
    std::advance(l_front, 0);
    EXPECT_EQ(*l_front, child_sibling_container);
    auto pipeline_options = std::make_shared<PipelineOptions>();
    manager->OnPatchFinish(pipeline_options);

    EXPECT_EQ(page_container->children().size(), static_cast<size_t>(2));

    fixed_child->SetStyleInternal(CSSPropertyID::kPropertyIDPosition,
                                  tasm::CSSValue(2, CSSValuePattern::NUMBER));
    fixed_child->FlushProps();
    fixed_child_container->StyleChanged();
    // fixed node should follow the order in the element tree.
    l_front = manager->fixed_node_list_.begin();
    std::advance(l_front, 0);
    EXPECT_EQ(*l_front, fixed_child_container);
    EXPECT_EQ(page_container->children().size(), static_cast<size_t>(3));
  }
}

TEST_F(ElementContainerTest, ReplaceNegativeZIndexChildren) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableZIndex(true);
  manager->SetConfig(config);
  auto page = manager->CreateFiberPage("page", 11);
  manager->SetRoot(page.get());
  manager->SetRootOnLayout(page->impl_id());
  page->FlushProps();
  auto page_container = page->element_container_impl();
  ASSERT_TRUE(page->IsStackingContextNode());

  // parent0
  auto parent0_element = manager->CreateFiberView();
  parent0_element->SetStyleInternal(
      CSSPropertyID::kPropertyIDZIndex,
      tasm::CSSValue(-1, CSSValuePattern::NUMBER));
  page->InsertNode(parent0_element);

  // parent1
  auto parent1_element = manager->CreateFiberView();
  parent1_element->SetStyleInternal(
      CSSPropertyID::kPropertyIDZIndex,
      tasm::CSSValue(-1, CSSValuePattern::NUMBER));
  page->InsertNode(parent1_element);

  page->FlushActionsAsRoot();
  auto pipeline_options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(pipeline_options);

  auto parent0_container = parent0_element->element_container_impl();
  EXPECT_EQ(parent0_container->parent(), page_container);
  EXPECT_EQ(parent0_container->children().size(), static_cast<size_t>(0));

  EXPECT_EQ(page_container->children().size(), static_cast<size_t>(2));

  auto painting_context =
      static_cast<MockPaintingContext*>(manager->painting_context()->impl());

  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto page_painting_children = page_painting_node->children_;
  EXPECT_TRUE(page_painting_children.size() == 2);

  EXPECT_TRUE(page_painting_children[0]->id_ == parent0_element->impl_id());
  EXPECT_TRUE(page_painting_children[1]->id_ == parent1_element->impl_id());

  // parent2
  auto parent2_element = manager->CreateFiberView();
  parent2_element->SetStyleInternal(
      CSSPropertyID::kPropertyIDZIndex,
      tasm::CSSValue(-1, CSSValuePattern::NUMBER));
  page->InsertNode(parent2_element);

  page->FlushActionsAsRoot();
  manager->OnPatchFinish(pipeline_options);

  page_painting_children = page_painting_node->children_;
  EXPECT_TRUE(page_painting_children.size() == 3);

  EXPECT_TRUE(page_painting_children[0]->id_ == parent0_element->impl_id());
  EXPECT_TRUE(page_painting_children[1]->id_ == parent1_element->impl_id());
  EXPECT_TRUE(page_painting_children[2]->id_ == parent2_element->impl_id());

  // update: page:[{z-index:-1},{z-index:-1},{z-index:-1}] -> insert z-index:-1
  // element & z-index:0 element parent3: z-index:-1 node
  auto parent3_element = manager->CreateFiberView();
  parent3_element->SetStyleInternal(
      CSSPropertyID::kPropertyIDZIndex,
      tasm::CSSValue(-1, CSSValuePattern::NUMBER));
  page->InsertNode(parent3_element);

  // parent4: z-index:0 node
  auto parent4_element = manager->CreateFiberView();
  parent4_element->SetStyleInternal(CSSPropertyID::kPropertyIDZIndex,
                                    tasm::CSSValue(0, CSSValuePattern::NUMBER));
  page->InsertNode(parent4_element);

  page->FlushActionsAsRoot();
  manager->OnPatchFinish(pipeline_options);

  page_painting_children = page_painting_node->children_;
  EXPECT_TRUE(page_painting_children.size() == 5);
  EXPECT_TRUE(page_painting_children[0]->id_ == parent0_element->impl_id());
  EXPECT_TRUE(page_painting_children[1]->id_ == parent1_element->impl_id());
  EXPECT_TRUE(page_painting_children[2]->id_ == parent2_element->impl_id());

  EXPECT_TRUE(page_painting_children[3]->id_ == parent3_element->impl_id());
  EXPECT_TRUE(page_painting_children[4]->id_ == parent4_element->impl_id());
}

TEST_F(ElementContainerTest, OldFixedZIndexSwitchCase) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableZIndex(true);
  manager->SetConfig(config);
  auto page = manager->CreateFiberPage("page", 11);
  manager->SetRoot(page.get());
  manager->SetRootOnLayout(page->impl_id());
  page->FlushProps();
  auto page_container = page->element_container_impl();
  ASSERT_TRUE(page->IsStackingContextNode());

  // parent0
  auto parent0_element = manager->CreateFiberView();
  parent0_element->SetStyle(CSSPropertyID::kPropertyIDZIndex, lepus::Value(1));
  parent0_element->SetStyle(CSSPropertyID::kPropertyIDPosition,
                            lepus::Value("fixed"));
  page->InsertNode(parent0_element);

  // parent1
  auto parent1_element = manager->CreateFiberView();
  parent1_element->SetStyle(CSSPropertyID::kPropertyIDOpacity,
                            lepus::Value(0.9));
  page->InsertNode(parent1_element);

  // parent 1-0: fixed+zIndex:100
  auto parent1_0_element = manager->CreateFiberView();
  parent1_0_element->SetStyle(CSSPropertyID::kPropertyIDZIndex,
                              lepus::Value(100));
  parent1_0_element->SetStyle(CSSPropertyID::kPropertyIDPosition,
                              lepus::Value("fixed"));
  parent1_element->InsertNode(parent1_0_element);

  // normal
  auto parent1_1_element = manager->CreateFiberView();
  parent1_1_element->SetStyle(CSSPropertyID::kPropertyIDBackground,
                              lepus::Value("red"));
  parent1_element->InsertNode(parent1_1_element);

  // z-index:2
  auto parent1_2_element = manager->CreateFiberView();
  parent1_2_element->SetStyle(CSSPropertyID::kPropertyIDZIndex,
                              lepus::Value(2));
  parent1_element->InsertNode(parent1_2_element);

  page->FlushActionsAsRoot();

  auto pipeline_options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(pipeline_options);

  EXPECT_EQ(page_container->children().size(), static_cast<size_t>(3));

  auto painting_context =
      static_cast<MockPaintingContext*>(manager->painting_context()->impl());

  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto page_painting_children = page_painting_node->children_;
  EXPECT_TRUE(page_painting_children.size() == 3);

  EXPECT_TRUE(page_painting_children[0]->id_ == parent1_element->impl_id());
  EXPECT_TRUE(page_painting_children[1]->id_ ==
              parent0_element->impl_id());  // fixed&zIndex:1
  EXPECT_TRUE(page_painting_children[2]->id_ ==
              parent1_0_element->impl_id());  // fixed&zIndex:100

  // change parent1_0_element from z-index:100 to z-index:0
  parent1_0_element->SetStyle(CSSPropertyID::kPropertyIDZIndex,
                              lepus::Value(0));
  page->FlushActionsAsRoot();

  pipeline_options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(pipeline_options);

  EXPECT_TRUE(page_painting_children.size() == 3);
  EXPECT_TRUE(parent1_0_element->element_container_impl()->parent() ==
              page_container);
}

TEST_F(ElementContainerTest, StackingContextDirtyChangeCase) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableZIndex(true);
  manager->SetConfig(config);
  auto page = manager->CreateFiberPage("page", 11);
  manager->SetRoot(page.get());
  manager->SetRootOnLayout(page->impl_id());
  page->FlushProps();

  // parent0 stackingContext
  auto parent0_element = manager->CreateFiberView();
  parent0_element->SetStyle(CSSPropertyID::kPropertyIDBackground,
                            lepus::Value("red"));

  page->InsertNode(parent0_element);

  page->FlushActionsAsRoot();

  auto pipeline_options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(pipeline_options);

  EXPECT_FALSE(parent0_element->IsStackingContextNode());

  // mock 'transition' or 'animation delay' to directly setComputeStyle which
  // may affect the IsStackingContextNode()
  CSSValue opacity_value = tasm::CSSValue(0.5, CSSValuePattern::NUMBER);
  parent0_element->computed_css_style()->SetValue(
      CSSPropertyID::kPropertyIDOpacity, opacity_value);
  EXPECT_TRUE(parent0_element->IsStackingContextNode());

  // inset z-index
  auto element_0 = manager->CreateFiberView();
  element_0->SetStyle(CSSPropertyID::kPropertyIDZIndex, lepus::Value(100));
  parent0_element->InsertNode(element_0);

  page->FlushActionsAsRoot();
  manager->OnPatchFinish(pipeline_options);

  EXPECT_TRUE(element_0->element_container_impl()->parent() ==
              parent0_element->element_container_impl());

  parent0_element->RemoveNode(element_0);
  page->RemoveNode(parent0_element);
  element_0 = nullptr;
  parent0_element = nullptr;
  page->FlushActionsAsRoot();
  manager->OnPatchFinish(pipeline_options);

  EXPECT_TRUE(page->element_container_impl()->children().size() == 0);
}

TEST_F(ElementContainerTest, FragmentMarkNeedRedraw) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->page_options_.embedded_mode_ = static_cast<EmbeddedMode>(
      static_cast<int32_t>(manager->page_options_.embedded_mode_) |
      static_cast<int32_t>(EmbeddedMode::FRAGMENT_LAYER_RENDER));
  manager->SetConfig(config);

  auto element = manager->CreateFiberElement("view");
  auto fragment = element->fragment_impl();
  EXPECT_FALSE(fragment->NeedRedraw());

  element->CreateElementContainer(false);
  EXPECT_TRUE(fragment->NeedRedraw());

  fragment->ResetDirtyState(BaseElementContainer::kNeedRedraw);
  fragment->UpdateLayout(starlight::LayoutResultForRendering());
  EXPECT_TRUE(fragment->NeedRedraw());

  fragment->ResetDirtyState(BaseElementContainer::kNeedRedraw);
  EXPECT_FALSE(fragment->NeedRedraw());

  // auto child_element = manager->CreateFiberElement("view");
  // child_element->CreateElementContainer(false);
  // auto child_fragment = child_element->fragment_impl();
  // fragment->AddChild(child_fragment, 0);
  // EXPECT_TRUE(fragment->NeedRedraw());

  fragment->ResetDirtyState(BaseElementContainer::kNeedRedraw);

  // child_fragment->RemoveSelf(false);
  // EXPECT_TRUE(fragment->NeedRedraw());

  fragment->ResetDirtyState(BaseElementContainer::kNeedRedraw);
  fragment->CreatePaintingNode(false, nullptr);
  EXPECT_TRUE(fragment->NeedRedraw());

  fragment->ResetDirtyState(BaseElementContainer::kNeedRedraw);
  fragment->UpdatePaintingNode(false, nullptr);
  EXPECT_TRUE(fragment->NeedRedraw());
}

TEST_F(ElementContainerTest, TestIsRootContainer) {
  auto config = std::make_shared<PageConfig>();
  manager->SetConfig(config);

  auto element = manager->CreateFiberElement("view");
  auto container = element->element_container();
  EXPECT_FALSE(container->IsRootContainer());

  element = manager->CreateFiberElement("page");
  container = element->element_container();
  EXPECT_TRUE(container->IsRootContainer());
}

TEST_F(ElementContainerTest, TestMarkDirty) {
  auto config = std::make_shared<PageConfig>();
  manager->SetConfig(config);

  auto element = manager->CreateFiberElement("view");
  auto container = element->element_container();
  EXPECT_FALSE(container->IsRootContainer());

  container->MarkDirtyState(BaseElementContainer::kNeedSortZChild);
  EXPECT_TRUE(container->NeedSortZChild());
  EXPECT_TRUE(container->has_z_child());

  container->ResetDirtyState(BaseElementContainer::kNeedSortZChild);
  EXPECT_FALSE(container->NeedSortZChild());
  EXPECT_TRUE(container->has_z_child());

  container->MarkDirtyState(BaseElementContainer::kNeedSortFixedChild);
  EXPECT_FALSE(container->NeedSortFixedChild());
  EXPECT_FALSE(container->has_fixed_child());

  container->MarkDirtyState(BaseElementContainer::kNeedRedraw);
  EXPECT_TRUE(container->NeedRedraw());
}

TEST_F(ElementContainerTest, TestMarkDirty0) {
  auto config = std::make_shared<PageConfig>();
  manager->SetConfig(config);

  auto element = manager->CreateFiberElement("page");
  auto container = element->element_container();
  EXPECT_TRUE(container->IsRootContainer());

  container->MarkDirtyState(BaseElementContainer::kNeedSortZChild);
  EXPECT_TRUE(container->NeedSortZChild());
  EXPECT_TRUE(container->has_z_child());

  container->ResetDirtyState(BaseElementContainer::kNeedSortZChild);
  EXPECT_FALSE(container->NeedSortZChild());
  EXPECT_TRUE(container->has_z_child());

  container->MarkDirtyState(BaseElementContainer::kNeedSortFixedChild);
  EXPECT_TRUE(container->NeedSortFixedChild());
  EXPECT_TRUE(container->has_fixed_child());

  container->ResetDirtyState(BaseElementContainer::kNeedSortFixedChild);
  EXPECT_FALSE(container->NeedSortFixedChild());
  EXPECT_TRUE(container->has_fixed_child());

  container->MarkDirtyState(BaseElementContainer::kNeedRedraw);
  EXPECT_TRUE(container->NeedRedraw());
}

TEST_F(ElementContainerTest, CalcUIIndexForFixedIncludesNegativeZChildren) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFixedNew(true);
  config->SetEnableZIndex(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberElement("page");
  manager->SetRoot(page.get());
  manager->SetRootOnLayout(page->impl_id());
  page->FlushProps();
  auto page_container = page->element_container_impl();
  ASSERT_TRUE(page->IsStackingContextNode());

  auto negative_z_child = manager->CreateFiberElement("view");
  page->AddChildAt(negative_z_child, 0);
  negative_z_child->SetStyleInternal(
      CSSPropertyID::kPropertyIDZIndex,
      tasm::CSSValue(-1, CSSValuePattern::NUMBER));
  negative_z_child->FlushProps();
  auto negative_z_container = negative_z_child->element_container_impl();
  negative_z_container->InsertSelf();
  EXPECT_EQ(negative_z_container->parent(), page_container);

  auto fixed_child = manager->CreateFiberElement("view");
  page->AddChildAt(fixed_child, 1);
  fixed_child->SetStyleInternal(CSSPropertyID::kPropertyIDPosition,
                                tasm::CSSValue(2, CSSValuePattern::NUMBER));
  fixed_child->FlushProps();
  ASSERT_TRUE(fixed_child->IsNewFixed());

  int index = 0;
  page_container->CalcUIIndexForFixedNew(fixed_child->element_container_impl(),
                                         index);
  EXPECT_EQ(index,
            static_cast<int>(page_container->negative_z_children_.size()) + 0);
}

TEST_F(ElementContainerTest, RemoveChildFixedWithZIndex) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFixedNew(true);
  config->SetEnableZIndex(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberElement("page");
  manager->SetRoot(page.get());
  manager->SetRootOnLayout(page->impl_id());
  page->FlushProps();
  auto page_container = page->element_container_impl();

  // Test that fixed elements with non-zero z-index are NOT added to
  // fixed_node_list
  auto fixed_child = manager->CreateFiberElement("view");
  page->AddChildAt(fixed_child, 0);
  fixed_child->SetStyleInternal(CSSPropertyID::kPropertyIDPosition,
                                tasm::CSSValue(2, CSSValuePattern::NUMBER));
  fixed_child->SetStyleInternal(CSSPropertyID::kPropertyIDZIndex,
                                tasm::CSSValue(5, CSSValuePattern::NUMBER));
  fixed_child->FlushProps();
  auto fixed_container = fixed_child->element_container_impl();
  fixed_container->InsertSelf();
  auto pipeline_options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(pipeline_options);
  EXPECT_EQ(manager->fixed_node_list_.size(), static_cast<size_t>(0));

  // Test that fixed elements with zero z-index ARE added to fixed_node_list
  auto fixed_child_z0 = manager->CreateFiberElement("view");
  page->AddChildAt(fixed_child_z0, 1);
  fixed_child_z0->SetStyleInternal(CSSPropertyID::kPropertyIDPosition,
                                   tasm::CSSValue(2, CSSValuePattern::NUMBER));
  fixed_child_z0->FlushProps();
  auto fixed_container_z0 = fixed_child_z0->element_container_impl();
  fixed_container_z0->InsertSelf();
  manager->OnPatchFinish(pipeline_options);
  EXPECT_EQ(manager->fixed_node_list_.size(), static_cast<size_t>(1));

  page_container->RemoveChild(fixed_container_z0);
  EXPECT_EQ(manager->fixed_node_list_.size(), static_cast<size_t>(0));
}

TEST_F(ElementContainerTest, MoveZChildrenRecursivelySkipsFixedElements) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFixedNew(true);
  config->SetEnableZIndex(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberElement("page");
  manager->SetRoot(page.get());
  manager->SetRootOnLayout(page->impl_id());
  page->FlushProps();
  auto page_container = page->element_container_impl();

  auto z_child = manager->CreateFiberElement("view");
  page->AddChildAt(z_child, 0);
  z_child->SetStyleInternal(CSSPropertyID::kPropertyIDZIndex,
                            tasm::CSSValue(1, CSSValuePattern::NUMBER));
  z_child->FlushProps();
  auto z_container = z_child->element_container_impl();
  z_container->InsertSelf();
  EXPECT_EQ(z_container->parent(), page_container);

  auto fixed_child = manager->CreateFiberElement("view");
  page->AddChildAt(fixed_child, 1);
  fixed_child->SetStyleInternal(CSSPropertyID::kPropertyIDPosition,
                                tasm::CSSValue(2, CSSValuePattern::NUMBER));
  fixed_child->SetStyleInternal(CSSPropertyID::kPropertyIDZIndex,
                                tasm::CSSValue(2, CSSValuePattern::NUMBER));
  fixed_child->FlushProps();
  auto fixed_container = fixed_child->element_container_impl();
  fixed_container->InsertSelf();
  EXPECT_EQ(fixed_container->parent(), page_container);

  auto new_parent = manager->CreateFiberElement("view");
  auto new_parent_container = new_parent->element_container_impl();

  page_container->MoveZChildrenRecursively(page.get(), new_parent_container);

  EXPECT_EQ(z_container->parent(), new_parent_container);
  EXPECT_EQ(fixed_container->parent(), page_container);
}

TEST_F(ElementContainerTest,
       CalcUIIndexForFixedUnifiedIncludesNegativeZChildren) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableUnifyFixedBehavior(true);
  config->SetEnableZIndex(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberElement("page");
  manager->SetRoot(page.get());
  manager->SetRootOnLayout(page->impl_id());
  page->FlushProps();
  auto page_container = page->element_container_impl();

  auto negative_z_child = manager->CreateFiberElement("view");
  page->AddChildAt(negative_z_child, 0);
  negative_z_child->SetStyleInternal(
      CSSPropertyID::kPropertyIDZIndex,
      tasm::CSSValue(-1, CSSValuePattern::NUMBER));
  negative_z_child->FlushProps();
  auto negative_z_container = negative_z_child->element_container_impl();
  negative_z_container->InsertSelf();

  auto fixed_child = manager->CreateFiberElement("view");
  fixed_child->SetStyleInternal(CSSPropertyID::kPropertyIDPosition,
                                tasm::CSSValue(2, CSSValuePattern::NUMBER));
  page->AddChildAt(fixed_child, 1);
  fixed_child->FlushProps();
  ASSERT_TRUE(fixed_child->IsFixedUnified());

  int index = 0;
  page_container->CalcUIIndexForFixedUnified(
      fixed_child->element_container_impl(), index);
  EXPECT_EQ(index,
            static_cast<int>(page_container->negative_z_children_.size()) + 0);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
