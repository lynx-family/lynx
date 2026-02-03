// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public
#include <renderer/dom/fiber/text_element.h>

#include "core/renderer/dom/testing/fiber_element_test.h"
#include "core/renderer/dom/testing/fiber_mock_painting_context.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
namespace lynx {
namespace tasm {
namespace testing {

// position:fixed related
TEST_P(FiberElementTest, FiberElementUnderNewFixed) {
  manager->config_->layout_configs_.enable_fixed_new_ = true;
  auto page = manager->CreateFiberPage("page", 11);

  auto element0 = manager->CreateFiberNode("view");

  element0->SetStyle(CSSPropertyID::kPropertyIDBackground,
                     lepus::Value("green"));
  page->InsertNode(element0);

  // fixed
  auto fixed_element = manager->CreateFiberNode("view");
  fixed_element->SetStyle(CSSPropertyID::kPropertyIDBackground,
                          lepus::Value("red"));
  fixed_element->SetStyle(CSSPropertyID::kPropertyIDPosition,
                          lepus::Value("fixed"));
  element0->InsertNode(fixed_element);

  auto text = manager->CreateFiberText("text");
  fixed_element->InsertNode(text);

  auto element2 = manager->CreateFiberNode("view");
  element2->SetStyle(CSSPropertyID::kPropertyIDBackground,
                     lepus::Value("blue"));
  page->InsertNode(element2);

  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), element0->impl_id(), -1));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), element2->impl_id(), -1));

  // fixed element is treated as normal element under new fixed. Its layout node
  // should be attached to its parent.
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(element0->impl_id(),
                                     fixed_element->impl_id(), -1));

  EXPECT_CALL(tasm_mediator, InsertLayoutNodeBefore(fixed_element->impl_id(),
                                                    text->impl_id(), -1));
  page->FlushActionsAsRoot();

  // check painting node
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto* element0_painting_node =
      painting_context->node_map_.at(element0->impl_id()).get();
  auto* fixed_painting_node =
      painting_context->node_map_.at(fixed_element->impl_id()).get();
  auto* text_painting_node =
      painting_context->node_map_.at(text->impl_id()).get();
  auto* element2_painting_node =
      painting_context->node_map_.at(element2->impl_id()).get();
  auto page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 3);
  EXPECT_TRUE(page_children[0] == element0_painting_node);
  EXPECT_TRUE(page_children[1] == element2_painting_node);
  EXPECT_TRUE(page_children[2] == fixed_painting_node);
  EXPECT_TRUE(fixed_painting_node->children_[0] == text_painting_node);

  // remove fixed
  element0->RemoveNode(fixed_element);
  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(element0->impl_id(), fixed_element->impl_id()));
  element0->FlushActionsAsRoot();
  painting_context->Flush();

  // check painting node
  page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 2);
  EXPECT_TRUE(page_children[0] == element0_painting_node);
  EXPECT_TRUE(page_children[1] == element2_painting_node);

  EXPECT_TRUE(fixed_painting_node->children_[0] == text_painting_node);

  // re-insert fixed
  element0->InsertNode(fixed_element);
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(element0->impl_id(),
                                     fixed_element->impl_id(), -1));
  element0->FlushActionsAsRoot();
  painting_context->Flush();

  page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 3);
  EXPECT_TRUE(page_children[0] == element0_painting_node);
  EXPECT_TRUE(page_children[1] == element2_painting_node);
  EXPECT_TRUE(page_children[2] == fixed_painting_node);
  EXPECT_TRUE(fixed_painting_node->children_[0] == text_painting_node);

  // reset position:fixed style
  fixed_element->RemoveAllInlineStyles();
  EXPECT_TRUE(fixed_element->element_container_impl()->was_position_fixed_);

  fixed_element->FlushActionsAsRoot();
  painting_context->Flush();
  EXPECT_FALSE(fixed_element->element_container_impl()->was_position_fixed_);

  page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 2);
  EXPECT_TRUE(page_children[0] == element0_painting_node);
  EXPECT_TRUE(page_children[1] == element2_painting_node);
  EXPECT_TRUE(element0_painting_node->children_[0] == fixed_painting_node);
  EXPECT_TRUE(fixed_painting_node->children_[0] == text_painting_node);

  // re set position:fixed
  fixed_element->SetStyle(CSSPropertyID::kPropertyIDPosition,
                          lepus::Value("fixed"));
  fixed_element->FlushActionsAsRoot();
  painting_context->Flush();
  EXPECT_TRUE(fixed_element->element_container_impl()->was_position_fixed_);

  page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 3);
  EXPECT_TRUE(page_children[0] == element0_painting_node);
  EXPECT_TRUE(page_children[1] == element2_painting_node);
  EXPECT_TRUE(page_children[2] == fixed_painting_node);
  EXPECT_TRUE(fixed_painting_node->children_[0] == text_painting_node);
}

// Regression test for: [BugFix][Fiber] Reattach fixed elements via logical
// parent.
//
// When enableFixedNew is enabled, fixed elements are treated as normal elements
// in layout insertion. During reattach, the element's `render_parent_` can be
// temporarily null (e.g. after detaching/restoring layout nodes). The reattach
// insertion must use the element's logical parent (`parent_`) to ensure the
// fixed element is inserted back into the layout tree.
TEST_P(FiberElementTest,
       ReattachNewFixedUsesLogicalParentWhenRenderParentNull) {
  manager->config_->layout_configs_.enable_fixed_new_ = true;

  auto page = manager->CreateFiberPage("page", 11);
  auto parent = manager->CreateFiberNode("view");
  page->InsertNode(parent);

  // A fixed element that is logically under `parent`.
  auto fixed_element = manager->CreateFiberNode("view");
  fixed_element->SetStyle(CSSPropertyID::kPropertyIDPosition,
                          lepus::Value("fixed"));
  parent->InsertNode(fixed_element);

  // Flush once to build initial layout/painting state.
  page->FlushActionsAsRoot();
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  ASSERT_EQ(fixed_element->parent(), parent.get());
  ASSERT_EQ(fixed_element->render_parent(), parent.get());
  ASSERT_TRUE(fixed_element->attached_to_layout_parent_);

  // Detach the layout node from its render parent without changing the logical
  // DOM relationship, simulating cases where the element is removed from the
  // layout tree and later reattached.
  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(parent->impl_id(), fixed_element->impl_id()));
  parent->HandleRemoveChildAction(fixed_element.get());
  EXPECT_EQ(fixed_element->parent(), parent.get());
  EXPECT_EQ(fixed_element->render_parent(), nullptr);
  EXPECT_FALSE(fixed_element->attached_to_layout_parent_);
  ASSERT_EQ(parent->GetChildCount(), static_cast<size_t>(1));
  EXPECT_EQ(parent->GetChildAt(0), fixed_element.get());

  // Mark for reattach. The reattach insertion must not rely on render_parent_.
  fixed_element->MarkDirty(FiberElement::kDirtyReAttachContainer);

  // Reattach should insert layout node under its logical parent.
  EXPECT_CALL(
      tasm_mediator,
      InsertLayoutNodeBefore(parent->impl_id(), fixed_element->impl_id(), -1));
  fixed_element->FlushActionsAsRoot();

  // Ensure the element has been re-attached to the layout tree.
  EXPECT_EQ(fixed_element->render_parent(), parent.get());
  EXPECT_TRUE(fixed_element->attached_to_layout_parent_);
}

// Subtree move regression: when a subtree root (with fixed descendants) is
// removed and reattached, fixed nodes must reattach via their logical parent.
TEST_P(FiberElementTest,
       ReattachFixedDescendantAfterSubtreeMoveUsesLogicalParent) {
  manager->config_->layout_configs_.enable_fixed_new_ = true;

  auto page = manager->CreateFiberPage("page", 11);
  auto host_a = manager->CreateFiberNode("view");
  auto host_b = manager->CreateFiberNode("view");
  page->InsertNode(host_a);
  page->InsertNode(host_b);

  auto subtree_root = manager->CreateFiberNode("view");
  auto logical_parent = manager->CreateFiberNode("view");
  auto fixed_child = manager->CreateFiberNode("view");
  fixed_child->SetStyle(CSSPropertyID::kPropertyIDPosition,
                        lepus::Value("fixed"));

  host_a->InsertNode(subtree_root);
  subtree_root->InsertNode(logical_parent);
  logical_parent->InsertNode(fixed_child);

  page->FlushActionsAsRoot();
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* page_container = page->element_container_impl();
  auto* fixed_container = fixed_child->element_container_impl();
  ASSERT_TRUE(page_container->IsRootContainer());

  ASSERT_EQ(fixed_child->parent(), logical_parent.get());
  ASSERT_EQ(fixed_child->render_parent(), logical_parent.get());
  ASSERT_TRUE(fixed_child->attached_to_layout_parent_);
  ASSERT_EQ(fixed_container->parent(), page_container);

  ::testing::Mock::VerifyAndClearExpectations(&tasm_mediator);

  // Remove the subtree root from host_a.
  host_a->RemoveNode(subtree_root);
  page->FlushActionsAsRoot();
  painting_context->Flush();

  EXPECT_EQ(subtree_root->parent(), nullptr);
  EXPECT_EQ(fixed_child->parent(), logical_parent.get());
  EXPECT_EQ(fixed_child->render_parent(), nullptr);
  EXPECT_FALSE(fixed_child->attached_to_layout_parent_);
  EXPECT_TRUE(fixed_child->dirty_ & FiberElement::kDirtyReAttachContainer);

  ::testing::Mock::VerifyAndClearExpectations(&tasm_mediator);

  // Reattach the subtree root under host_b and ensure the fixed descendant is
  // reinserted via its logical parent.
  host_b->InsertNode(subtree_root);
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(::testing::_, ::testing::_, ::testing::_))
      .Times(::testing::AnyNumber());
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(logical_parent->impl_id(),
                                     fixed_child->impl_id(), -1));
  page->FlushActionsAsRoot();
  painting_context->Flush();

  EXPECT_EQ(subtree_root->parent(), host_b.get());
  EXPECT_EQ(fixed_child->parent(), logical_parent.get());
  EXPECT_EQ(fixed_child->render_parent(), logical_parent.get());
  EXPECT_TRUE(fixed_child->attached_to_layout_parent_);
  EXPECT_EQ(fixed_container->parent(), page_container);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
