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
  auto page = manager->CreateFiberPage("page", 11);
  page->enable_new_fixed_ = true;

  auto element0 = manager->CreateFiberNode("view");
  element0->enable_new_fixed_ = true;

  element0->SetStyle(CSSPropertyID::kPropertyIDBackground,
                     lepus::Value("green"));
  page->InsertNode(element0);

  // fixed
  auto fixed_element = manager->CreateFiberNode("view");
  fixed_element->enable_new_fixed_ = true;
  fixed_element->SetStyle(CSSPropertyID::kPropertyIDBackground,
                          lepus::Value("red"));
  fixed_element->SetStyle(CSSPropertyID::kPropertyIDPosition,
                          lepus::Value("fixed"));
  element0->InsertNode(fixed_element);

  auto text = manager->CreateFiberText("text");
  text->enable_new_fixed_ = true;
  fixed_element->InsertNode(text);

  auto element2 = manager->CreateFiberNode("view");
  element2->enable_new_fixed_ = true;
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
  auto painting_context =
      static_cast<FiberMockPaintingContext*>(page->painting_context()->impl());
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
  EXPECT_TRUE(fixed_element->element_container()->was_position_fixed_);

  fixed_element->FlushActionsAsRoot();
  painting_context->Flush();
  EXPECT_FALSE(fixed_element->element_container()->was_position_fixed_);

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
  EXPECT_TRUE(fixed_element->element_container()->was_position_fixed_);

  page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 3);
  EXPECT_TRUE(page_children[0] == element0_painting_node);
  EXPECT_TRUE(page_children[1] == element2_painting_node);
  EXPECT_TRUE(page_children[2] == fixed_painting_node);
  EXPECT_TRUE(fixed_painting_node->children_[0] == text_painting_node);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
