// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include <cmath>
#include <memory>
#include <mutex>
#include <tuple>

#include "base/include/auto_reset.h"
#include "core/animation/css_transition_manager.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/base/threading/vsync_monitor.h"
#include "core/renderer/css/computed_css_style_css_text_helper.h"
#include "core/renderer/css/css_color.h"
#include "core/renderer/css/css_decoder.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/css/ng/parser/css_parser_token_range.h"
#include "core/renderer/css/ng/parser/css_tokenizer.h"
#include "core/renderer/css/ng/selector/css_parser_context.h"
#include "core/renderer/css/ng/selector/css_selector_parser.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/component_element.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/dom/fiber/for_element.h"
#include "core/renderer/dom/fiber/if_element.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/dom/fiber/none_element.h"
#include "core/renderer/dom/fiber/page_element.h"
#include "core/renderer/dom/fiber/raw_text_element.h"
#include "core/renderer/dom/fiber/scroll_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/tree_resolver.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/fiber/wrapper_element.h"
#include "core/renderer/dom/list_component_info.h"
#include "core/renderer/dom/testing/fiber_element_test.h"
#include "core/renderer/dom/testing/fiber_mock_painting_context.h"
#include "core/renderer/starlight/types/layout_attribute.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/renderer/ui_wrapper/common/testing/prop_bundle_mock.h"
#include "core/renderer/utils/test/text_utils_mock.h"
#include "core/runtime/lepus/bytecode_generator.h"
#include "core/runtime/lepus/js_object.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/services/event_report/event_tracker.h"
#include "core/shell/lynx_ui_operation_queue.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/shared_css_fragment.h"
#include "core/template_bundle/template_codec/generator/ttml_holder.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {
static std::unordered_map<std::string, uint32_t> kTestColorMap = {
    {"red", 4294901760},
    {"green", 4278222848},
    {"black", 4278190080},
};

TEST_P(FiberElementTest, CreateAndRemoveWrapperElement) {
  auto page = manager->CreateFiberPage("page", 11);
  auto element0 = manager->CreateFiberNode("view");
  auto element_before_black = manager->CreateFiberNode("view");
  auto element = manager->CreateFiberNode("view");
  auto text = manager->CreateFiberNode("text");
  auto wrapper = manager->CreateFiberWrapperElement();

  wrapper->InsertNode(element);
  wrapper->InsertNode(text);

  page->InsertNode(element0);
  page->InsertNode(element_before_black);
  page->InsertNode(wrapper);

  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), element0->impl_id(), -1));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(),
                                     element_before_black->impl_id(), -1));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), element->impl_id(), -1));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), text->impl_id(), -1));
  page->FlushActionsAsRoot();

  // Append after wrapper
  auto element_after_yellow = manager->CreateFiberNode("view");
  page->InsertNode(element_after_yellow);

  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(),
                                     element_after_yellow->impl_id(), -1));
  page->FlushActionsAsRoot();

  // remove Wrapper's sibling
  page->RemoveNode(element_before_black);
  page->RemoveNode(element_after_yellow);

  EXPECT_CALL(tasm_mediator, RemoveLayoutNode(page->impl_id(),
                                              element_before_black->impl_id()));
  EXPECT_CALL(tasm_mediator, RemoveLayoutNode(page->impl_id(),
                                              element_after_yellow->impl_id()));
  page->FlushActionsAsRoot();

  // Insert node to wrapper
  auto text1 = manager->CreateFiberNode("text");
  wrapper->InsertNode(text1);

  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), text1->impl_id(), -1));
  page->FlushActionsAsRoot();

  EXPECT_EQ(static_cast<int>(wrapper->children().size()), 3);

  // Remove node from wrapper
  wrapper->RemoveNode(text);

  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(page->impl_id(), text->impl_id()));
  page->FlushActionsAsRoot();
  EXPECT_EQ(static_cast<int>(wrapper->children().size()), 2);

  // remove wrapper
  page->RemoveNode(wrapper);

  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(page->impl_id(), element->impl_id()));
  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(page->impl_id(), text1->impl_id()));
  page->FlushActionsAsRoot();
}

TEST_P(FiberElementTest, RemoveWrapperElement) {
  auto page = manager->CreateFiberPage("page", 11);
  auto element0 = manager->CreateFiberNode("view");
  auto element_before_black = manager->CreateFiberNode("view");
  auto element = manager->CreateFiberNode("view");
  auto text = manager->CreateFiberNode("text");
  auto wrapper = manager->CreateFiberWrapperElement();
  wrapper->InsertNode(element);
  wrapper->InsertNode(text);

  page->InsertNode(element0);
  page->InsertNode(element_before_black);
  page->InsertNode(wrapper);

  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), element0->impl_id(), -1));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(),
                                     element_before_black->impl_id(), -1));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), element->impl_id(), -1));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), text->impl_id(), -1));
  page->FlushActionsAsRoot();

  // Append after wrapper
  auto element_after_yellow = manager->CreateFiberNode("view");
  page->InsertNode(element_after_yellow);

  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(),
                                     element_after_yellow->impl_id(), -1));
  page->FlushActionsAsRoot();

  // remove Wrapper's sibling
  page->RemoveNode(element_before_black);
  page->RemoveNode(element_after_yellow);

  EXPECT_CALL(tasm_mediator, RemoveLayoutNode(page->impl_id(),
                                              element_before_black->impl_id()));
  EXPECT_CALL(tasm_mediator, RemoveLayoutNode(page->impl_id(),
                                              element_after_yellow->impl_id()));
  page->FlushActionsAsRoot();

  // Insert node to wrapper
  auto text1 = manager->CreateFiberNode("text");
  wrapper->InsertNode(text1);

  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), text1->impl_id(), -1));
  page->FlushActionsAsRoot();

  EXPECT_EQ(static_cast<int>(wrapper->children().size()), 3);

  // remove wrapper
  page->RemoveNode(wrapper);

  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(page->impl_id(), element->impl_id()));
  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(page->impl_id(), text->impl_id()));
  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(page->impl_id(), text1->impl_id()));
  page->FlushActionsAsRoot();

  // Remove node from wrapper
  wrapper->RemoveNode(text);
  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(page->impl_id(), text->impl_id()));
  // do nothing, due to wrapper is detached form view tree
  page->FlushActionsAsRoot();
  EXPECT_EQ(static_cast<int>(wrapper->children().size()), 2);

  // insert wrapper again
  page->InsertNode(wrapper);
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), element->impl_id(), -1));
  // there might be a bug since text should be removed.
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), text->impl_id(), -1));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), text1->impl_id(), -1));
  page->FlushActionsAsRoot();
  EXPECT_EQ(static_cast<int>(page->children().size()), 2);
}

TEST_P(FiberElementTest, RemoveWrapperElementCase02) {
  auto page = manager->CreateFiberPage("page", 11);
  auto element0 = manager->CreateFiberView();
  auto element_before_black = manager->CreateFiberView();
  auto element = manager->CreateFiberView();
  auto text = manager->CreateFiberText("text");
  auto wrapper = manager->CreateFiberWrapperElement();
  auto first_wrapper_child = manager->CreateFiberView();
  first_wrapper_child->MarkCanBeLayoutOnly(false);
  first_wrapper_child->InsertNode(element);
  first_wrapper_child->InsertNode(text);

  wrapper->InsertNode(first_wrapper_child);

  page->InsertNode(element0);
  page->InsertNode(element_before_black);
  page->InsertNode(wrapper);

  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), element0->impl_id(), -1));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(),
                                     element_before_black->impl_id(), -1));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(),
                                     first_wrapper_child->impl_id(), -1));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(first_wrapper_child->impl_id(),
                                     element->impl_id(), -1));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(first_wrapper_child->impl_id(),
                                     text->impl_id(), -1));
  page->FlushActionsAsRoot();

  // remove wrapper
  page->RemoveNode(wrapper);

  EXPECT_CALL(tasm_mediator, RemoveLayoutNode(page->impl_id(),
                                              first_wrapper_child->impl_id()));
  page->FlushActionsAsRoot();
  // check dom tree
  EXPECT_TRUE(page->scoped_children_.size() == 2);
  EXPECT_TRUE(page->scoped_children_[0] == element0);
  EXPECT_TRUE(page->scoped_children_[1] == element_before_black);

  // check page painting node tree
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto page_painting_children = page_painting_node->children_;
  EXPECT_TRUE(page_painting_children.size() == 2);
  EXPECT_TRUE(page_painting_children[0]->id_ == element0->impl_id());
  EXPECT_TRUE(page_painting_children[1]->id_ ==
              element_before_black->impl_id());

  // check first_wrapper_child dom tree
  EXPECT_TRUE(first_wrapper_child->scoped_children_.size() == 2);
  EXPECT_TRUE(first_wrapper_child->scoped_children_[0] == element);
  EXPECT_TRUE(first_wrapper_child->scoped_children_[1] == text);

  // check first_wrapper_child painting tree
  auto* first_wrapper_child_painting_node =
      painting_context->node_map_.at(first_wrapper_child->impl_id()).get();
  auto first_wrapper_child_children =
      first_wrapper_child_painting_node->children_;
  EXPECT_EQ(static_cast<int>(first_wrapper_child_children.size()), 2);
  EXPECT_TRUE(first_wrapper_child_children[0]->id_ == element->impl_id());
  EXPECT_TRUE(first_wrapper_child_children[1]->id_ == text->impl_id());

  // re-insert the wrapper
  page->InsertNode(wrapper);
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(),
                                     first_wrapper_child->impl_id(), -1));
  page->FlushActionsAsRoot();
  painting_context->Flush();

  page_painting_children = page_painting_node->children_;
  EXPECT_TRUE(page_painting_children.size() == 3);
  EXPECT_TRUE(page_painting_children[0]->id_ == element0->impl_id());
  EXPECT_TRUE(page_painting_children[1]->id_ ==
              element_before_black->impl_id());
  EXPECT_TRUE(page_painting_children[2]->id_ == first_wrapper_child->impl_id());

  first_wrapper_child_children = first_wrapper_child_painting_node->children_;
  EXPECT_EQ(static_cast<int>(first_wrapper_child_children.size()), 2);
  EXPECT_TRUE(first_wrapper_child_children[0]->id_ == element->impl_id());
  EXPECT_TRUE(first_wrapper_child_children[1]->id_ == text->impl_id());
}

TEST_P(FiberElementTest, FiberElementCaseForWrapper) {
  auto page = manager->CreateFiberPage("page", 11);

  auto element0 = manager->CreateFiberNode("view");
  element0->SetAttribute("enable-layout", lepus::Value("false"));

  page->InsertNode(element0);  // page's first child:element0

  auto wrapper00 = manager->CreateFiberWrapperElement();
  element0->InsertNode(wrapper00);  // element0's first child:wrapper00

  auto element01 = manager->CreateFiberNode("view");
  element01->SetAttribute("enable-layout", lepus::Value("false"));

  auto element02 = manager->CreateFiberNode("view");
  element02->SetAttribute("enable-layout", lepus::Value("false"));

  element0->InsertNode(element01);  // element0's second child:element01
  element0->InsertNode(element02);  // element0's third child:element02

  auto wrapper000 = manager->CreateFiberWrapperElement();
  wrapper00->InsertNode(wrapper000);  // element00's first child:wrapper000

  auto element0000 = manager->CreateFiberNode("view");
  element0000->SetAttribute("enable-layout", lepus::Value("false"));

  auto element0001 = manager->CreateFiberNode("view");
  element0001->SetAttribute("enable-layout", lepus::Value("false"));

  wrapper000->InsertNode(element0000);  // wrapper000's first child:element0000
  wrapper000->InsertNode(element0001);  // wrapper000's first child:element0001

  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), element0->impl_id(), -1));
  EXPECT_CALL(tasm_mediator, InsertLayoutNodeBefore(element0->impl_id(),
                                                    element01->impl_id(), -1));
  EXPECT_CALL(tasm_mediator, InsertLayoutNodeBefore(element0->impl_id(),
                                                    element02->impl_id(), -1));
  EXPECT_CALL(tasm_mediator, InsertLayoutNodeBefore(element0->impl_id(),
                                                    element0000->impl_id(),
                                                    element01->impl_id()));
  EXPECT_CALL(tasm_mediator, InsertLayoutNodeBefore(element0->impl_id(),
                                                    element0001->impl_id(),
                                                    element01->impl_id()));
  page->FlushActionsAsRoot();

  // check element container tree
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto page_painting_children = page_painting_node->children_;
  EXPECT_TRUE(page_painting_children.size() == 1);
  EXPECT_TRUE(page_painting_children[0]->id_ == element0->impl_id());

  auto* element0_painting_node =
      painting_context->node_map_.at(element0->impl_id()).get();
  auto element0_painting_children = element0_painting_node->children_;
  EXPECT_TRUE(element0_painting_children.size() == 4);

  EXPECT_TRUE(element0_painting_children[0]->id_ == element0000->impl_id());
  EXPECT_TRUE(element0_painting_children[1]->id_ == element0001->impl_id());
  EXPECT_TRUE(element0_painting_children[2]->id_ == element01->impl_id());
  EXPECT_TRUE(element0_painting_children[3]->id_ == element02->impl_id());
}

TEST_P(FiberElementTest, FiberElementCaseForWrapper02) {
  auto page = manager->CreateFiberPage("page", 11);

  auto element0 = manager->CreateFiberNode("view");
  element0->SetAttribute("enable-layout", lepus::Value("false"));
  page->InsertNode(element0);  // page's first child:element0

  // insert element00 to element0
  auto element00 = manager->CreateFiberNode("view");
  element00->SetAttribute("enable-layout", lepus::Value("false"));
  element0->InsertNode(element00);
  // insert wrapper00 to element0
  auto wrapper00 = manager->CreateFiberWrapperElement();
  element0->InsertNode(wrapper00);

  // insert element01 to element0
  auto element01 = manager->CreateFiberNode("view");
  element01->SetAttribute("enable-layout", lepus::Value("false"));
  element0->InsertNode(element01);

  // insert wrapper000 to wrapper00
  auto wrapper000 = manager->CreateFiberWrapperElement();
  wrapper00->InsertNode(wrapper000);

  // insert wrapper001 to wrapper00
  auto wrapper001 = manager->CreateFiberWrapperElement();
  wrapper00->InsertNode(wrapper001);

  // insert element0000 to wrapper000
  auto element0000 = manager->CreateFiberNode("view");
  element0000->SetAttribute("enable-layout", lepus::Value("false"));
  wrapper000->InsertNode(element0000);

  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), element0->impl_id(), -1));
  EXPECT_CALL(tasm_mediator, InsertLayoutNodeBefore(element0->impl_id(),
                                                    element00->impl_id(), -1));
  EXPECT_CALL(tasm_mediator, InsertLayoutNodeBefore(element0->impl_id(),
                                                    element01->impl_id(), -1));
  EXPECT_CALL(tasm_mediator, InsertLayoutNodeBefore(element0->impl_id(),
                                                    element0000->impl_id(),
                                                    element01->impl_id()));
  page->FlushActionsAsRoot();

  // check element container tree
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto page_painting_children = page_painting_node->children_;
  EXPECT_TRUE(page_painting_children.size() == 1);
  EXPECT_TRUE(page_painting_children[0]->id_ == element0->impl_id());

  auto* element0_painting_node =
      painting_context->node_map_.at(element0->impl_id()).get();
  auto element0_painting_children = element0_painting_node->children_;
  EXPECT_TRUE(element0_painting_children.size() == 3);

  EXPECT_TRUE(element0_painting_children[0]->id_ == element00->impl_id());
  EXPECT_TRUE(element0_painting_children[1]->id_ == element0000->impl_id());
  EXPECT_TRUE(element0_painting_children[2]->id_ == element01->impl_id());
}

TEST_P(FiberElementTest, FiberElementFixedChangedBeforeWrapper) {
  auto page = manager->CreateFiberPage("page", 11);

  auto element0 = manager->CreateFiberNode("view");
  element0->SetStyle(CSSPropertyID::kPropertyIDBackground,
                     lepus::Value("green"));
  page->InsertNode(element0);

  auto text = manager->CreateFiberText("text");
  element0->InsertNode(text);

  // fixed
  auto element1_fixed = manager->CreateFiberNode("view");
  element1_fixed->SetStyle(CSSPropertyID::kPropertyIDBackground,
                           lepus::Value("red"));
  element1_fixed->SetStyle(CSSPropertyID::kPropertyIDPosition,
                           lepus::Value("fixed"));
  element0->InsertNode(element1_fixed);

  auto element_end = manager->CreateFiberNode("view");
  element_end->SetStyle(CSSPropertyID::kPropertyIDBackground,
                        lepus::Value("grey"));
  element0->InsertNode(element_end);

  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), element0->impl_id(), -1))
      .Times(::testing::AtLeast(1));
  EXPECT_CALL(
      tasm_mediator,
      InsertLayoutNodeBefore(page->impl_id(), element1_fixed->impl_id(), -1))
      .Times(::testing::AtLeast(1));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(element0->impl_id(), text->impl_id(), -1))
      .Times(::testing::AtLeast(1));
  EXPECT_CALL(tasm_mediator, InsertLayoutNodeBefore(element0->impl_id(),
                                                    element_end->impl_id(), -1))
      .Times(::testing::AtLeast(1));

  page->FlushActionsAsRoot();

  // check painting node
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto* element0_painting_node =
      painting_context->node_map_.at(element0->impl_id()).get();
  auto* text_painting_node =
      painting_context->node_map_.at(text->impl_id()).get();
  auto* element1_fixed_painting_node =
      painting_context->node_map_.at(element1_fixed->impl_id()).get();
  auto* element_end_painting_node =
      painting_context->node_map_.at(element_end->impl_id()).get();

  auto page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 2);
  EXPECT_TRUE(page_children[0] == element0_painting_node);
  EXPECT_TRUE(page_children[1] == element1_fixed_painting_node);
  EXPECT_TRUE(element0_painting_node->children_[0] == text_painting_node);
  EXPECT_TRUE(element0_painting_node->children_[1] ==
              element_end_painting_node);

  EXPECT_TRUE(element0->next_render_sibling_ == element1_fixed.get());
  EXPECT_TRUE(text->next_render_sibling_ == element_end.get());

  ::testing::Mock::VerifyAndClearExpectations(&tasm_mediator);

  element0->RemoveNode(text);

  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(element0->impl_id(), text->impl_id()))
      .Times(1);

  page->FlushActionsAsRoot();

  painting_context->Flush();
  page_children = page_painting_node->children_;

  EXPECT_TRUE(page_children.size() == 2);
  EXPECT_TRUE(element0_painting_node->children_[0] ==
              element_end_painting_node);

  // insert before fixed node!
  element0->InsertNodeBefore(text, element1_fixed);

  ::testing::Mock::VerifyAndClearExpectations(&tasm_mediator);

  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(element0->impl_id(), text->impl_id(),
                                     element_end->impl_id()))
      .Times(1);
  page->FlushActionsAsRoot();
  painting_context->Flush();

  page_children = page_painting_node->children_;

  EXPECT_TRUE(page_children.size() == 2);
  EXPECT_TRUE(element0_painting_node->children_[0] == text_painting_node);
  EXPECT_TRUE(element0_painting_node->children_[1] ==
              element_end_painting_node);
  ::testing::Mock::VerifyAndClearExpectations(&tasm_mediator);

  // fixed ref_node next_sibling is null
  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(element0->impl_id(), element_end->impl_id()))
      .Times(1);

  element0->RemoveNode(element_end);
  page->FlushActionsAsRoot();
  painting_context->Flush();

  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(element0->impl_id(), text->impl_id()))
      .Times(1);
  element0->RemoveNode(text);
  element0->InsertNodeBefore(text, element1_fixed);
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(element0->impl_id(), text->impl_id(), -1))
      .Times(1);
  page->FlushActionsAsRoot();
  painting_context->Flush();

  page_children = page_painting_node->children_;

  EXPECT_TRUE(page_children.size() == 2);
  EXPECT_TRUE(element0_painting_node->children_[0] == text_painting_node);
  ::testing::Mock::VerifyAndClearExpectations(&tasm_mediator);

  // fixed ref_node next_sibling is wrapper
  auto wrapper = manager->CreateFiberWrapperElement();
  wrapper->InsertNode(element_end);
  element0->InsertNodeBefore(wrapper, nullptr);

  EXPECT_CALL(tasm_mediator, InsertLayoutNodeBefore(element0->impl_id(),
                                                    element_end->impl_id(), -1))
      .Times(1);

  page->FlushActionsAsRoot();
  painting_context->Flush();

  page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 2);
  EXPECT_TRUE(element0_painting_node->children_[0] == text_painting_node);
  EXPECT_TRUE(element0_painting_node->children_[1] ==
              element_end_painting_node);
  ::testing::Mock::VerifyAndClearExpectations(&tasm_mediator);

  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(element0->impl_id(), text->impl_id()))
      .Times(1);
  element0->RemoveNode(text);
  element0->InsertNodeBefore(text, element1_fixed);
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(element0->impl_id(), text->impl_id(),
                                     element_end->impl_id()))
      .Times(1);
  page->FlushActionsAsRoot();
  painting_context->Flush();
  EXPECT_TRUE(element0_painting_node->children_[0] == text_painting_node);
  EXPECT_TRUE(element0_painting_node->children_[1] ==
              element_end_painting_node);
  ::testing::Mock::VerifyAndClearExpectations(&tasm_mediator);

  // fixed ref_node next_sibling is another fixed
  auto element2_fixed = manager->CreateFiberNode("view");
  element2_fixed->SetStyle(CSSPropertyID::kPropertyIDBackground,
                           lepus::Value("pink"));
  element2_fixed->SetStyle(CSSPropertyID::kPropertyIDPosition,
                           lepus::Value("fixed"));
  element0->InsertNodeBefore(element2_fixed, wrapper);
  {
    // now fixed always insert to dom parent then HandleSelfFixed to page!
    // FIXME(linxs:) Workaround: to be removed if new fixed support
    EXPECT_CALL(tasm_mediator, InsertLayoutNodeBefore(element0->impl_id(),
                                                      element2_fixed->impl_id(),
                                                      element_end->impl_id()))
        .Times(::testing::AtLeast(1));
  }

  EXPECT_CALL(
      tasm_mediator,
      InsertLayoutNodeBefore(page->impl_id(), element2_fixed->impl_id(), -1))
      .Times(::testing::AtLeast(1));

  page->FlushActionsAsRoot();
  painting_context->Flush();
  page_children = page_painting_node->children_;
  auto* element2_fixed_painting_node =
      painting_context->node_map_.at(element2_fixed->impl_id()).get();
  EXPECT_TRUE(page_children.size() == 3);
  EXPECT_TRUE(page_children[0] == element0_painting_node);
  EXPECT_TRUE(page_children[1] == element1_fixed_painting_node);
  EXPECT_TRUE(page_children[2] == element2_fixed_painting_node);
  EXPECT_TRUE(element0_painting_node->children_[0] == text_painting_node);
  EXPECT_TRUE(element0_painting_node->children_[1] ==
              element_end_painting_node);
  ::testing::Mock::VerifyAndClearExpectations(&tasm_mediator);

  // ref fixed && fixed change
  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(element0->impl_id(), text->impl_id()))
      .Times(::testing::AtLeast(1));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(element0->impl_id(), text->impl_id(),
                                     element_end->impl_id()))
      .Times(::testing::AtLeast(1));

  // element2 fixed change
  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(page->impl_id(), element1_fixed->impl_id()))
      .Times(::testing::AtLeast(1));
  EXPECT_CALL(tasm_mediator, InsertLayoutNodeBefore(element0->impl_id(),
                                                    element1_fixed->impl_id(),
                                                    element_end->impl_id()))
      .Times(::testing::AtLeast(1));

  element0->RemoveNode(text);
  element0->InsertNodeBefore(text, element1_fixed);

  // ref fixed-> non-fixed
  element1_fixed->SetStyle(CSSPropertyID::kPropertyIDPosition,
                           lepus::Value("relative"));

  page->FlushActionsAsRoot();
  painting_context->Flush();

  page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 2);
  EXPECT_TRUE(page_children[0] == element0_painting_node);
  EXPECT_TRUE(page_children[1] == element2_fixed_painting_node);

  EXPECT_TRUE(element0_painting_node->children_.size() == 3);
  EXPECT_TRUE(element0_painting_node->children_[0] == text_painting_node);
  EXPECT_TRUE(element0_painting_node->children_[1] ==
              element1_fixed_painting_node);
  EXPECT_TRUE(element0_painting_node->children_[2] ==
              element_end_painting_node);

  EXPECT_TRUE(element0->next_render_sibling_ == element2_fixed.get());
  EXPECT_TRUE(text->next_render_sibling_ == element1_fixed.get());
  EXPECT_TRUE(element1_fixed->next_render_sibling_ == wrapper.get());
}

TEST_P(FiberElementTest, ImageTest0) {
  // create image and insert it to wrapper
  auto image = manager->CreateFiberImage("image");
  image->ConvertToInlineElement();

  EXPECT_EQ(image->tag_, "inline-image");
  EXPECT_FALSE(image->TendToFlatten());
}

TEST_P(FiberElementTest, ImageTest1) {
  // create image and insert it to wrapper
  auto image = manager->CreateFiberImage("x-image");
  image->ConvertToInlineElement();

  EXPECT_EQ(image->tag_, "x-inline-image");
  EXPECT_FALSE(image->TendToFlatten());
}

TEST_P(FiberElementTest, InlineElementTest0) {
  auto page = manager->CreateFiberPage("page", 11);

  // create text and insert it to page
  auto text = manager->CreateFiberText("text");
  text->MarkCanBeLayoutOnly(false);
  page->InsertNode(text);

  // create wrapper and insert it to text
  auto wrapper = manager->CreateFiberWrapperElement();
  wrapper->MarkCanBeLayoutOnly(false);
  text->InsertNode(wrapper);

  // create image and insert it to wrapper
  auto image = manager->CreateFiberImage("image");
  image->MarkCanBeLayoutOnly(false);
  wrapper->InsertNode(image);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  EXPECT_FALSE(text->is_inline_element());
  EXPECT_TRUE(text->TendToFlatten());

  EXPECT_TRUE(wrapper->is_inline_element());
  EXPECT_FALSE(wrapper->TendToFlatten());

  EXPECT_TRUE(image->is_inline_element());
  EXPECT_FALSE(image->TendToFlatten());

  EXPECT_TRUE(HasCaptureSignWithTag(page->impl_id(), "page"));
  EXPECT_TRUE(HasCaptureSignWithTag(text->impl_id(), "text"));
  EXPECT_TRUE(HasCaptureSignWithTag(image->impl_id(), "inline-image"));
}

TEST_P(FiberElementTest, InlineElementTest0_0) {
  int32_t captured_element_memory_size = 0;
  manager->vm_update_outer_obj_size_callback_ =
      [&captured_element_memory_size](int32_t size) {
        captured_element_memory_size = size;
      };
  auto page = manager->CreateFiberPage("page", 11);

  // create text and insert it to page
  auto text = manager->CreateFiberText("text");
  text->MarkCanBeLayoutOnly(false);

  // create wrapper and insert it to text
  auto wrapper = manager->CreateFiberWrapperElement();
  wrapper->MarkCanBeLayoutOnly(false);

  // create image and insert it to wrapper
  auto image = manager->CreateFiberImage("image");
  image->MarkCanBeLayoutOnly(false);

  wrapper->InsertNode(image);
  text->InsertNode(wrapper);
  page->InsertNode(text);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  EXPECT_TRUE(manager->total_memory_ > 0);
  EXPECT_EQ(captured_element_memory_size, manager->total_memory_);

  EXPECT_FALSE(text->is_inline_element());
  EXPECT_TRUE(text->TendToFlatten());

  EXPECT_TRUE(wrapper->is_inline_element());
  EXPECT_FALSE(wrapper->TendToFlatten());

  EXPECT_TRUE(image->is_inline_element());
  EXPECT_FALSE(image->TendToFlatten());

  EXPECT_TRUE(HasCaptureSignWithTag(page->impl_id(), "page"));
  EXPECT_TRUE(HasCaptureSignWithTag(text->impl_id(), "text"));
  EXPECT_TRUE(HasCaptureSignWithTag(image->impl_id(), "inline-image"));
}

TEST_P(FiberElementTest, InlineElementTest1) {
  auto page = manager->CreateFiberPage("page", 11);

  // create text and insert it to page
  auto text = manager->CreateFiberText("text");
  text->MarkCanBeLayoutOnly(false);
  page->InsertNode(text);

  // create wrapper and insert it to text
  auto wrapper = manager->CreateFiberWrapperElement();
  wrapper->MarkCanBeLayoutOnly(false);
  text->InsertNode(wrapper);

  // create wrapper1 and insert it to text
  auto wrapper1 = manager->CreateFiberWrapperElement();
  wrapper1->MarkCanBeLayoutOnly(false);
  wrapper->InsertNode(wrapper1);

  // create image and insert it to wrapper1
  auto image = manager->CreateFiberImage("image");
  image->MarkCanBeLayoutOnly(false);
  wrapper1->InsertNode(image);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  EXPECT_FALSE(text->is_inline_element());
  EXPECT_TRUE(text->TendToFlatten());

  EXPECT_TRUE(wrapper->is_inline_element());
  EXPECT_FALSE(wrapper->TendToFlatten());

  EXPECT_TRUE(wrapper1->is_inline_element());
  EXPECT_FALSE(wrapper1->TendToFlatten());

  EXPECT_TRUE(image->is_inline_element());
  EXPECT_FALSE(image->TendToFlatten());

  EXPECT_TRUE(HasCaptureSignWithTag(page->impl_id(), "page"));
  EXPECT_TRUE(HasCaptureSignWithTag(text->impl_id(), "text"));
  EXPECT_TRUE(HasCaptureSignWithTag(image->impl_id(), "inline-image"));
}

TEST_P(FiberElementTest, InlineElementTest1_0) {
  auto page = manager->CreateFiberPage("page", 11);

  // create text and insert it to page
  auto text = manager->CreateFiberText("text");
  text->MarkCanBeLayoutOnly(false);

  // create wrapper and insert it to text
  auto wrapper = manager->CreateFiberWrapperElement();
  wrapper->MarkCanBeLayoutOnly(false);

  // create wrapper1 and insert it to text
  auto wrapper1 = manager->CreateFiberWrapperElement();
  wrapper1->MarkCanBeLayoutOnly(false);

  // create image and insert it to wrapper1
  auto image = manager->CreateFiberImage("image");
  image->MarkCanBeLayoutOnly(false);

  wrapper1->InsertNode(image);
  wrapper->InsertNode(wrapper1);
  text->InsertNode(wrapper);
  page->InsertNode(text);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  EXPECT_FALSE(text->is_inline_element());
  EXPECT_TRUE(text->TendToFlatten());

  EXPECT_TRUE(wrapper->is_inline_element());
  EXPECT_FALSE(wrapper->TendToFlatten());

  EXPECT_TRUE(wrapper1->is_inline_element());
  EXPECT_FALSE(wrapper1->TendToFlatten());

  EXPECT_TRUE(image->is_inline_element());
  EXPECT_FALSE(image->TendToFlatten());

  EXPECT_TRUE(HasCaptureSignWithTag(page->impl_id(), "page"));
  EXPECT_TRUE(HasCaptureSignWithTag(text->impl_id(), "text"));
  EXPECT_TRUE(HasCaptureSignWithTag(image->impl_id(), "inline-image"));
}

TEST_P(FiberElementTest, InlineElementTestLayoutOnly1_0) {
  auto page = manager->CreateFiberPage("page", 11);

  // create text and insert it to page
  auto text = manager->CreateFiberText("text");

  // create wrapper and insert it to text
  auto wrapper = manager->CreateFiberWrapperElement();

  // create wrapper1 and insert it to text
  auto wrapper1 = manager->CreateFiberWrapperElement();

  // create image and insert it to wrapper1
  auto image = manager->CreateFiberImage("image");

  wrapper1->InsertNode(image);
  wrapper->InsertNode(wrapper1);
  text->InsertNode(wrapper);
  page->InsertNode(text);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  EXPECT_FALSE(text->is_inline_element());
  EXPECT_FALSE(text->IsLayoutOnly());
  EXPECT_TRUE(text->TendToFlatten());

  EXPECT_TRUE(wrapper->is_inline_element());
  EXPECT_TRUE(wrapper->IsLayoutOnly());
  EXPECT_FALSE(wrapper->TendToFlatten());

  EXPECT_TRUE(wrapper1->is_inline_element());
  EXPECT_TRUE(wrapper1->IsLayoutOnly());
  EXPECT_FALSE(wrapper1->TendToFlatten());

  EXPECT_TRUE(image->is_inline_element());
  EXPECT_TRUE(image->IsLayoutOnly());
  EXPECT_FALSE(image->TendToFlatten());
}

TEST_P(FiberElementTest, InlineElementTest2) {
  auto page = manager->CreateFiberPage("page", 11);

  // create text and insert it to page
  auto text = manager->CreateFiberText("text");
  text->MarkCanBeLayoutOnly(false);
  page->InsertNode(text);

  // create wrapper and insert it to text
  auto wrapper = manager->CreateFiberWrapperElement();
  wrapper->MarkCanBeLayoutOnly(false);
  text->InsertNode(wrapper);

  // create text1 and insert it to wrapper
  auto text1 = manager->CreateFiberText("text");
  text1->MarkCanBeLayoutOnly(false);
  wrapper->InsertNode(text1);

  // create wrapper1 and insert it to text1
  auto wrapper1 = manager->CreateFiberWrapperElement();
  wrapper1->MarkCanBeLayoutOnly(false);
  text1->InsertNode(wrapper1);

  // create image and insert it to wrapper1
  auto image = manager->CreateFiberImage("image");
  image->MarkCanBeLayoutOnly(false);
  wrapper1->InsertNode(image);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  EXPECT_FALSE(text->is_inline_element());
  EXPECT_TRUE(text->TendToFlatten());

  EXPECT_TRUE(text1->is_inline_element());
  EXPECT_FALSE(text1->TendToFlatten());

  EXPECT_TRUE(wrapper->is_inline_element());
  EXPECT_FALSE(wrapper->TendToFlatten());

  EXPECT_TRUE(wrapper1->is_inline_element());
  EXPECT_FALSE(wrapper1->TendToFlatten());

  EXPECT_TRUE(image->is_inline_element());
  EXPECT_FALSE(image->TendToFlatten());

  EXPECT_TRUE(HasCaptureSignWithTag(page->impl_id(), "page"));
  EXPECT_TRUE(HasCaptureSignWithTag(text->impl_id(), "text"));
  EXPECT_TRUE(HasCaptureSignWithTag(text1->impl_id(), "inline-text"));
  EXPECT_TRUE(HasCaptureSignWithTag(image->impl_id(), "inline-image"));
}

TEST_P(FiberElementTest, InlineElementTest2_1) {
  auto page = manager->CreateFiberPage("page", 11);

  // create text and insert it to page
  auto text = manager->CreateFiberText("text");
  text->MarkCanBeLayoutOnly(false);
  page->InsertNode(text);

  // create wrapper and insert it to text
  auto wrapper = manager->CreateFiberWrapperElement();
  wrapper->MarkCanBeLayoutOnly(false);
  text->InsertNode(wrapper);

  // create text1 and insert it to wrapper
  auto text1 = manager->CreateFiberText("text");
  text1->MarkCanBeLayoutOnly(false);
  text1->layout_node_type_ = LayoutNodeType::CUSTOM;
  wrapper->InsertNode(text1);

  // create wrapper1 and insert it to text1
  auto wrapper1 = manager->CreateFiberWrapperElement();
  wrapper1->MarkCanBeLayoutOnly(false);
  text1->InsertNode(wrapper1);

  // create image and insert it to wrapper1
  auto image = manager->CreateFiberImage("image");
  image->MarkCanBeLayoutOnly(false);
  image->layout_node_type_ = LayoutNodeType::CUSTOM;
  wrapper1->InsertNode(image);

  // create list and insert it to wrapper1
  lepus::Value component_at_index(10);
  lepus::Value enqueue_component;
  lepus::Value component_at_indexes;
  auto list = manager->CreateFiberList(nullptr, "list", component_at_index,
                                       enqueue_component, component_at_indexes);
  list = fml::AdoptRef<ListElement>(
      new ListElement(*static_cast<ListElement*>(list.get()), true));
  list->AttachToElementManager(
      manager, tasm->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME), false);
  list->SetAttribute("custom-list-name", lepus::Value("list-container"));
  wrapper1->InsertNode(list);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  auto* mock_painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->platform_impl_.get());
  mock_painting_context->Flush();

  EXPECT_FALSE(text->is_inline_element());
  EXPECT_TRUE(text->TendToFlatten());

  EXPECT_TRUE(text1->is_inline_element());
  EXPECT_FALSE(text1->TendToFlatten());

  EXPECT_TRUE(wrapper->is_inline_element());
  EXPECT_FALSE(wrapper->TendToFlatten());

  EXPECT_TRUE(wrapper1->is_inline_element());
  EXPECT_FALSE(wrapper1->TendToFlatten());

  EXPECT_TRUE(image->is_inline_element());
  EXPECT_FALSE(image->TendToFlatten());

  EXPECT_TRUE(HasCaptureSignWithTag(page->impl_id(), "page"));
  EXPECT_TRUE(HasCaptureSignWithTag(text->impl_id(), "text"));
  EXPECT_TRUE(HasCaptureSignWithTag(text1->impl_id(), "inline-text"));
  EXPECT_TRUE(HasCaptureSignWithTag(image->impl_id(), "inline-image"));
  EXPECT_TRUE(HasCaptureSignWithTag(list->impl_id(), "list"));

  EXPECT_TRUE(HasCapturePlatformNodeTag(page->impl_id(), "page"));
  EXPECT_TRUE(HasCapturePlatformNodeTag(text->impl_id(), "text"));
  EXPECT_TRUE(HasCapturePlatformNodeTag(text1->impl_id(), "inline-text"));
  EXPECT_TRUE(HasCapturePlatformNodeTag(image->impl_id(), "inline-image"));
  EXPECT_TRUE(HasCapturePlatformNodeTag(list->impl_id(), "list-container"));
}

TEST_P(FiberElementTest, InlineElementTestLayoutOnly2) {
  auto page = manager->CreateFiberPage("page", 11);

  // create text and insert it to page
  auto text = manager->CreateFiberText("text");
  page->InsertNode(text);

  // create wrapper and insert it to text
  auto wrapper = manager->CreateFiberWrapperElement();
  text->InsertNode(wrapper);

  // create text1 and insert it to wrapper
  auto text1 = manager->CreateFiberText("text");
  wrapper->InsertNode(text1);

  // create wrapper1 and insert it to text1
  auto wrapper1 = manager->CreateFiberWrapperElement();
  text1->InsertNode(wrapper1);

  // create image and insert it to wrapper1
  auto image = manager->CreateFiberImage("image");
  wrapper1->InsertNode(image);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  EXPECT_FALSE(text->is_inline_element());
  EXPECT_FALSE(text->IsLayoutOnly());
  EXPECT_TRUE(text->TendToFlatten());

  EXPECT_TRUE(text1->is_inline_element());
  EXPECT_TRUE(text1->IsLayoutOnly());
  EXPECT_FALSE(text1->TendToFlatten());

  EXPECT_TRUE(wrapper->is_inline_element());
  EXPECT_TRUE(wrapper->IsLayoutOnly());
  EXPECT_FALSE(wrapper->TendToFlatten());

  EXPECT_TRUE(wrapper1->is_inline_element());
  EXPECT_TRUE(wrapper1->IsLayoutOnly());
  EXPECT_FALSE(wrapper1->TendToFlatten());

  EXPECT_TRUE(image->is_inline_element());
  EXPECT_TRUE(image->IsLayoutOnly());
  EXPECT_FALSE(image->TendToFlatten());
}

TEST_P(FiberElementTest, InlineElementTest2_0) {
  auto page = manager->CreateFiberPage("page", 11);

  // create text and insert it to page
  auto text = manager->CreateFiberText("text");
  text->MarkCanBeLayoutOnly(false);

  // create wrapper and insert it to text
  auto wrapper = manager->CreateFiberWrapperElement();
  wrapper->MarkCanBeLayoutOnly(false);

  // create text1 and insert it to wrapper
  auto text1 = manager->CreateFiberText("text");
  text1->MarkCanBeLayoutOnly(false);

  // create wrapper1 and insert it to text1
  auto wrapper1 = manager->CreateFiberWrapperElement();
  wrapper1->MarkCanBeLayoutOnly(false);

  // create image and insert it to wrapper1
  auto image = manager->CreateFiberImage("image");
  image->MarkCanBeLayoutOnly(false);
  wrapper1->InsertNode(image);
  text1->InsertNode(wrapper1);
  wrapper->InsertNode(text1);
  text->InsertNode(wrapper);
  page->InsertNode(text);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  EXPECT_FALSE(text->is_inline_element());
  EXPECT_TRUE(text->TendToFlatten());

  EXPECT_TRUE(text1->is_inline_element());
  EXPECT_FALSE(text1->TendToFlatten());

  EXPECT_TRUE(wrapper->is_inline_element());
  EXPECT_FALSE(wrapper->TendToFlatten());

  EXPECT_TRUE(wrapper1->is_inline_element());
  EXPECT_FALSE(wrapper1->TendToFlatten());

  EXPECT_TRUE(image->is_inline_element());
  EXPECT_FALSE(image->TendToFlatten());

  EXPECT_TRUE(HasCaptureSignWithTag(page->impl_id(), "page"));
  EXPECT_TRUE(HasCaptureSignWithTag(text->impl_id(), "text"));
  EXPECT_TRUE(HasCaptureSignWithTag(text1->impl_id(), "inline-text"));
  EXPECT_TRUE(HasCaptureSignWithTag(image->impl_id(), "inline-image"));
}

TEST_P(FiberElementTest, InlineElementTestLayoutOnly2_0) {
  auto page = manager->CreateFiberPage("page", 11);

  // create text and insert it to page
  auto text = manager->CreateFiberText("text");
  text->MarkCanBeLayoutOnly(false);

  // create wrapper and insert it to text
  auto wrapper = manager->CreateFiberWrapperElement();
  wrapper->MarkCanBeLayoutOnly(false);

  // create text1 and insert it to wrapper
  auto text1 = manager->CreateFiberText("text");
  text1->MarkCanBeLayoutOnly(false);

  // create wrapper1 and insert it to text1
  auto wrapper1 = manager->CreateFiberWrapperElement();
  wrapper1->MarkCanBeLayoutOnly(false);

  // create image and insert it to wrapper1
  auto image = manager->CreateFiberImage("image");
  image->MarkCanBeLayoutOnly(false);
  wrapper1->InsertNode(image);
  text1->InsertNode(wrapper1);
  wrapper->InsertNode(text1);
  text->InsertNode(wrapper);
  page->InsertNode(text);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  EXPECT_FALSE(text->is_inline_element());
  EXPECT_FALSE(text->IsLayoutOnly());
  EXPECT_TRUE(text->TendToFlatten());

  EXPECT_TRUE(text1->is_inline_element());
  EXPECT_TRUE(text1->IsLayoutOnly());
  EXPECT_FALSE(text1->TendToFlatten());

  EXPECT_TRUE(wrapper->is_inline_element());
  EXPECT_TRUE(wrapper->IsLayoutOnly());
  EXPECT_FALSE(wrapper->TendToFlatten());

  EXPECT_TRUE(wrapper1->is_inline_element());
  EXPECT_TRUE(wrapper1->IsLayoutOnly());
  EXPECT_FALSE(wrapper1->TendToFlatten());

  EXPECT_TRUE(image->is_inline_element());
  EXPECT_TRUE(image->IsLayoutOnly());
  EXPECT_FALSE(image->TendToFlatten());
}

TEST_P(FiberElementTest, InlineElementTest3) {
  auto page = manager->CreateFiberPage("page", 11);

  // create text and insert it to page
  auto text = manager->CreateFiberText("text");
  text->MarkCanBeLayoutOnly(false);
  page->InsertNode(text);

  // create wrapper and insert it to text
  auto wrapper = manager->CreateFiberWrapperElement();
  wrapper->MarkCanBeLayoutOnly(false);
  text->InsertNode(wrapper);

  // create truncation and insert it to wrapper
  auto truncation = manager->CreateFiberNode("truncation");
  truncation->MarkCanBeLayoutOnly(false);
  wrapper->InsertNode(truncation);

  // create inline-text and insert it to truncation
  auto inline_text = manager->CreateFiberText("inline-text");
  inline_text->MarkCanBeLayoutOnly(false);
  truncation->InsertNode(inline_text);

  // create inline-text and insert it to truncation
  auto text1 = manager->CreateFiberText("text");
  text1->MarkCanBeLayoutOnly(false);
  truncation->InsertNode(text1);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  EXPECT_FALSE(text->is_inline_element());
  EXPECT_TRUE(text->TendToFlatten());

  EXPECT_TRUE(truncation->is_inline_element());
  EXPECT_FALSE(truncation->TendToFlatten());

  EXPECT_TRUE(inline_text->is_inline_element());
  EXPECT_FALSE(inline_text->TendToFlatten());

  EXPECT_TRUE(text1->is_inline_element());
  EXPECT_FALSE(text1->TendToFlatten());

  EXPECT_TRUE(HasCaptureSignWithTag(page->impl_id(), "page"));
  EXPECT_TRUE(HasCaptureSignWithTag(truncation->impl_id(), "truncation"));
  EXPECT_TRUE(HasCaptureSignWithTag(text->impl_id(), "text"));
  EXPECT_TRUE(HasCaptureSignWithTag(text1->impl_id(), "inline-text"));
}

TEST_P(FiberElementTest, InlineElementTestLayoutOnly3) {
  auto page = manager->CreateFiberPage("page", 11);

  // create text and insert it to page
  auto text = manager->CreateFiberText("text");
  page->InsertNode(text);

  // create wrapper and insert it to text
  auto wrapper = manager->CreateFiberWrapperElement();
  text->InsertNode(wrapper);

  // create truncation and insert it to wrapper
  auto truncation = manager->CreateFiberNode("truncation");
  wrapper->InsertNode(truncation);

  // create inline-text and insert it to truncation
  auto inline_text = manager->CreateFiberText("inline-text");
  truncation->InsertNode(inline_text);

  // create inline-text and insert it to truncation
  auto text1 = manager->CreateFiberText("text");
  truncation->InsertNode(text1);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  EXPECT_FALSE(text->is_inline_element());
  EXPECT_FALSE(text->IsLayoutOnly());
  EXPECT_TRUE(text->TendToFlatten());

  EXPECT_TRUE(truncation->is_inline_element());
  EXPECT_FALSE(truncation->IsLayoutOnly());
  EXPECT_FALSE(truncation->TendToFlatten());

  EXPECT_TRUE(inline_text->is_inline_element());
  EXPECT_TRUE(inline_text->IsLayoutOnly());
  EXPECT_FALSE(inline_text->TendToFlatten());

  EXPECT_TRUE(text1->is_inline_element());
  EXPECT_TRUE(text1->IsLayoutOnly());
  EXPECT_FALSE(text1->TendToFlatten());
}

TEST_P(FiberElementTest, InlineElementTest3_0) {
  auto page = manager->CreateFiberPage("page", 11);

  // create text and insert it to page
  auto text = manager->CreateFiberText("text");
  text->MarkCanBeLayoutOnly(false);

  // create wrapper and insert it to text
  auto wrapper = manager->CreateFiberWrapperElement();
  wrapper->MarkCanBeLayoutOnly(false);

  // create image and insert it to wrapper
  auto truncation = manager->CreateFiberNode("truncation");
  truncation->MarkCanBeLayoutOnly(false);

  // create inline-text and insert it to page
  auto inline_text = manager->CreateFiberText("inline-text");
  inline_text->MarkCanBeLayoutOnly(false);
  truncation->InsertNode(inline_text);
  wrapper->InsertNode(truncation);
  text->InsertNode(wrapper);
  page->InsertNode(text);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  EXPECT_FALSE(text->is_inline_element());
  EXPECT_TRUE(text->TendToFlatten());

  EXPECT_TRUE(truncation->is_inline_element());
  EXPECT_FALSE(truncation->TendToFlatten());

  EXPECT_TRUE(inline_text->is_inline_element());
  EXPECT_FALSE(inline_text->TendToFlatten());

  EXPECT_TRUE(HasCaptureSignWithTag(page->impl_id(), "page"));
  EXPECT_TRUE(HasCaptureSignWithTag(truncation->impl_id(), "truncation"));
  EXPECT_TRUE(HasCaptureSignWithTag(text->impl_id(), "text"));
  EXPECT_TRUE(HasCaptureSignWithTag(inline_text->impl_id(), "inline-text"));
}

TEST_P(FiberElementTest, InlineElementTestLayoutOnly3_0) {
  auto page = manager->CreateFiberPage("page", 11);

  // create text and insert it to page
  auto text = manager->CreateFiberText("text");

  // create wrapper and insert it to text
  auto wrapper = manager->CreateFiberWrapperElement();

  // create image and insert it to wrapper
  auto truncation = manager->CreateFiberNode("truncation");

  // create inline-text and insert it to page
  auto inline_text = manager->CreateFiberText("inline-text");
  truncation->InsertNode(inline_text);
  wrapper->InsertNode(truncation);
  text->InsertNode(wrapper);
  page->InsertNode(text);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  EXPECT_FALSE(text->is_inline_element());
  EXPECT_FALSE(text->IsLayoutOnly());

  EXPECT_TRUE(truncation->is_inline_element());
  EXPECT_FALSE(truncation->IsLayoutOnly());

  EXPECT_TRUE(inline_text->is_inline_element());
  EXPECT_TRUE(inline_text->IsLayoutOnly());
}

TEST_P(FiberElementTest, InlineElementTest4) {
  auto page = manager->CreateFiberPage("page", 11);

  // create text and insert it to page
  auto text = manager->CreateFiberText("text");
  text->MarkCanBeLayoutOnly(false);
  page->InsertNode(text);

  // create view and insert it to text
  auto view = manager->CreateFiberView();
  view->MarkCanBeLayoutOnly(false);
  text->InsertNode(view);

  // create wrapper1 and insert it to view
  auto wrapper1 = manager->CreateFiberWrapperElement();
  wrapper1->MarkCanBeLayoutOnly(false);
  view->InsertNode(wrapper1);

  // create image and insert it to wrapper1
  auto image = manager->CreateFiberImage("image");
  image->MarkCanBeLayoutOnly(false);
  wrapper1->InsertNode(image);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  EXPECT_FALSE(text->is_inline_element());
  EXPECT_TRUE(text->TendToFlatten());

  EXPECT_TRUE(view->is_inline_element());
  EXPECT_FALSE(view->TendToFlatten());

  EXPECT_FALSE(wrapper1->is_inline_element());
  EXPECT_TRUE(wrapper1->TendToFlatten());

  EXPECT_FALSE(image->is_inline_element());
  EXPECT_TRUE(image->TendToFlatten());

  EXPECT_TRUE(HasCaptureSignWithTag(page->impl_id(), "page"));
  EXPECT_TRUE(HasCaptureSignWithTag(view->impl_id(), "view"));
  EXPECT_TRUE(HasCaptureSignWithTag(text->impl_id(), "text"));
  EXPECT_TRUE(HasCaptureSignWithTag(image->impl_id(), "image"));
}

TEST_P(FiberElementTest, InlineElementTestLayoutOnly4) {
  auto page = manager->CreateFiberPage("page", 11);

  // create text and insert it to page
  auto text = manager->CreateFiberText("text");
  page->InsertNode(text);

  // create view and insert it to text
  auto view = manager->CreateFiberView();
  text->InsertNode(view);

  // create wrapper1 and insert it to view
  auto wrapper1 = manager->CreateFiberWrapperElement();
  view->InsertNode(wrapper1);

  // create image and insert it to wrapper1
  auto image = manager->CreateFiberImage("image");
  wrapper1->InsertNode(image);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  EXPECT_FALSE(text->is_inline_element());
  EXPECT_FALSE(text->IsLayoutOnly());

  EXPECT_TRUE(view->is_inline_element());
  EXPECT_FALSE(view->IsLayoutOnly());

  EXPECT_FALSE(wrapper1->is_inline_element());
  EXPECT_TRUE(wrapper1->IsLayoutOnly());

  EXPECT_FALSE(image->is_inline_element());
  EXPECT_FALSE(image->IsLayoutOnly());
}

TEST_P(FiberElementTest, InlineElementTest4_0) {
  auto page = manager->CreateFiberPage("page", 11);

  // create text and insert it to page
  auto text = manager->CreateFiberText("text");
  text->MarkCanBeLayoutOnly(false);

  // create view and insert it to text
  auto view = manager->CreateFiberView();
  view->MarkCanBeLayoutOnly(false);

  // create wrapper1 and insert it to view
  auto wrapper1 = manager->CreateFiberWrapperElement();
  wrapper1->MarkCanBeLayoutOnly(false);

  // create image and insert it to wrapper1
  auto image = manager->CreateFiberImage("image");
  image->MarkCanBeLayoutOnly(false);

  wrapper1->InsertNode(image);
  view->InsertNode(wrapper1);
  text->InsertNode(view);
  page->InsertNode(text);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  EXPECT_FALSE(text->is_inline_element());
  EXPECT_TRUE(text->TendToFlatten());

  EXPECT_TRUE(view->is_inline_element());
  EXPECT_FALSE(view->TendToFlatten());

  EXPECT_FALSE(wrapper1->is_inline_element());
  EXPECT_TRUE(wrapper1->TendToFlatten());

  EXPECT_FALSE(image->is_inline_element());
  EXPECT_TRUE(image->TendToFlatten());

  EXPECT_TRUE(HasCaptureSignWithTag(page->impl_id(), "page"));
  EXPECT_TRUE(HasCaptureSignWithTag(view->impl_id(), "view"));
  EXPECT_TRUE(HasCaptureSignWithTag(text->impl_id(), "text"));
  EXPECT_TRUE(HasCaptureSignWithTag(image->impl_id(), "image"));
}

TEST_P(FiberElementTest, InlineElementTestLayoutOnly4_0) {
  auto page = manager->CreateFiberPage("page", 11);

  // create text and insert it to page
  auto text = manager->CreateFiberText("text");

  // create view and insert it to text
  auto view = manager->CreateFiberView();

  // create wrapper1 and insert it to view
  auto wrapper1 = manager->CreateFiberWrapperElement();

  // create image and insert it to wrapper1
  auto image = manager->CreateFiberImage("image");

  wrapper1->InsertNode(image);
  view->InsertNode(wrapper1);
  text->InsertNode(view);
  page->InsertNode(text);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());

  EXPECT_FALSE(text->is_inline_element());
  EXPECT_FALSE(text->IsLayoutOnly());

  EXPECT_TRUE(view->is_inline_element());
  EXPECT_FALSE(view->IsLayoutOnly());

  EXPECT_FALSE(wrapper1->is_inline_element());
  EXPECT_TRUE(wrapper1->IsLayoutOnly());

  EXPECT_FALSE(image->is_inline_element());
  EXPECT_FALSE(image->IsLayoutOnly());
}

TEST_P(FiberElementTest, TestFlushActionsOnWrapper) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableZIndex(true);
  config->SetEnableCSSInheritance(true);
  manager->SetConfig(config);

  // normal case
  auto page = manager->CreateFiberPage("page", 11);
  auto element0 = manager->CreateFiberView();
  page->InsertNode(element0);
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), element0->impl_id(), -1));

  auto element1 = manager->CreateFiberView();
  element0->InsertNode(element1);
  EXPECT_CALL(tasm_mediator, InsertLayoutNodeBefore(element0->impl_id(),
                                                    element1->impl_id(), -1));

  auto child = manager->CreateFiberView();
  child->SetStyle(CSSPropertyID::kPropertyIDPosition, lepus::Value("fixed"));

  element1->InsertNode(child);
  // diffrent input params will execute diffrent time, there might be a bug.
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), child->impl_id(), -1))
      .Times(::testing::AtLeast(1));
  page->FlushActionsAsRoot();

  // remove child style
  element1->RemoveNode(child);
  // diffrent input params will execute diffrent time, there might be a bug.
  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(page->impl_id(), child->impl_id()))
      .Times(::testing::AtLeast(1));
  child->RemoveAllInlineStyles();

  auto wrapper = manager->CreateFiberWrapperElement();
  wrapper->InsertNode(child);
  element1->InsertNode(wrapper);
  EXPECT_CALL(tasm_mediator, InsertLayoutNodeBefore(element1->impl_id(),
                                                    child->impl_id(), -1));

  // a bad case, element1&wrapper is not flushed, do nothing
  child->FlushActionsAsRoot();

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto* element0_painting_node =
      painting_context->node_map_.at(element0->impl_id()).get();

  auto* element1_painting_node =
      painting_context->node_map_.at(element1->impl_id()).get();

  auto* child_painting_node =
      painting_context->node_map_.at(child->impl_id()).get();

  auto page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 1);
  EXPECT_TRUE(page_children[0] == element0_painting_node);

  auto element0_children = element0_painting_node->children_;
  EXPECT_TRUE(element0_children.size() == 1);
  EXPECT_TRUE(element0_children[0] == element1_painting_node);

  auto element1_children = element1_painting_node->children_;
  EXPECT_TRUE(element1_children.size() == 1);
  EXPECT_TRUE(element1_children[0] == child_painting_node);
}

TEST_P(FiberElementTest, InlineViewTest) {
  // TODO: integrate with parameterized test later
  auto params = current_parameter_;
  auto thread_strategy = std::get<1>(params);
  if (thread_strategy == 0) {
    manager->SetThreadStrategy(base::ThreadStrategyForRendering::ALL_ON_UI);
  } else {
    manager->SetThreadStrategy(base::ThreadStrategyForRendering::MULTI_THREADS);
  }

  // page
  auto page = manager->CreateFiberPage("page", 11);

  base::String tag("text");
  auto text = manager->CreateFiberText(tag);
  auto wrapper = manager->CreateFiberWrapperElement();
  auto view = manager->CreateFiberView();

  page->InsertNode(text);
  text->InsertNode(wrapper);
  wrapper->InsertNode(view);

  page->FlushActionsAsRoot();

  EXPECT_TRUE(view->is_inline_element());
  EXPECT_FALSE(view->TendToFlatten());

  EXPECT_TRUE(HasCaptureSignWithInlineParentContainer(page->impl_id(), false));
  EXPECT_TRUE(HasCaptureSignWithInlineParentContainer(text->impl_id(), false));
}

TEST_P(FiberElementTest, ConvertToInlineForView) {
  auto parent_element = manager->CreateFiberView();
  auto child_element = manager->CreateFiberView();
  parent_element->InsertNode(child_element);

  EXPECT_FALSE(parent_element->is_inline_element());
  EXPECT_FALSE(child_element->is_inline_element());

  parent_element->ConvertToInlineElement();

  EXPECT_TRUE(parent_element->is_inline_element());
  EXPECT_FALSE(child_element->is_inline_element());
}

TEST_P(FiberElementTest, ConvertToInlineForComponent) {
  base::String component_id("comp-1");
  int32_t css_id = 101;
  base::String entry_name("TestEntry");
  base::String component_name("TestComponent");
  base::String path("/test/path");
  auto component_element = manager->CreateFiberComponent(
      component_id, css_id, entry_name, component_name, path);

  auto child_element = manager->CreateFiberView();
  component_element->InsertNode(child_element);

  EXPECT_FALSE(component_element->is_inline_element());
  EXPECT_FALSE(child_element->is_inline_element());

  component_element->ConvertToInlineElement();

  EXPECT_TRUE(component_element->is_inline_element());
  EXPECT_FALSE(child_element->is_inline_element());
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
