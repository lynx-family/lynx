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

TEST_P(FiberElementTest, TestUpdateLayoutNodeByBundle00) {
  if (enable_batch_layout_operation) {
    GTEST_SKIP();
  }
  auto view = manager->CreateFiberPage("0", 0);
  view->InitLayoutBundle();
  EXPECT_NE(view->layout_bundle_, nullptr);
  view->UpdateLayoutNodeByBundle();
  EXPECT_EQ(view->layout_bundle_, nullptr);

  view->parallel_flush_ = true;
  view->InitLayoutBundle();
  EXPECT_NE(view->layout_bundle_, nullptr);
  view->UpdateLayoutNodeByBundle();
  EXPECT_EQ(view->layout_bundle_, nullptr);
  EXPECT_EQ(!view->parallel_reduce_tasks_->empty(),
            manager->GetParallelWithSyncLayout());
  view->parallel_reduce_tasks_->clear();
}

TEST_P(FiberElementTest, TestUpdateLayoutNodeByBundle01) {
  if (!enable_batch_layout_operation) {
    GTEST_SKIP();
  }
  auto view = manager->CreateFiberPage("0", 0);
  view->InitLayoutBundle();
  EXPECT_NE(view->layout_bundle_, nullptr);
  view->UpdateLayoutNodeByBundle();
  EXPECT_EQ(view->layout_bundle_, nullptr);

  view->parallel_flush_ = true;
  view->InitLayoutBundle();
  EXPECT_NE(view->layout_bundle_, nullptr);
  view->UpdateLayoutNodeByBundle();
  EXPECT_EQ(view->layout_bundle_, nullptr);
  EXPECT_EQ(!(manager->element_context_task_queue_->task_queue_.Empty()),
            manager->GetParallelWithSyncLayout());
  EXPECT_TRUE(view->parallel_reduce_tasks_->empty());
  manager->element_context_task_queue_->task_queue_.ReversePopAll();
}

TEST_P(FiberElementTest, TestMarkLayoutDirty) {
  manager->page_options_.embedded_mode_ = static_cast<EmbeddedMode>(
      static_cast<int32_t>(manager->page_options_.embedded_mode_) |
      static_cast<int32_t>(EmbeddedMode::LAYOUT_IN_ELEMENT));

  auto page = manager->CreateFiberPage("page", 11);

  auto parent = manager->CreateFiberNode("view");
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 0);
  page->InsertNode(parent);

  auto element = manager->CreateFiberNode("view");
  element->SetStyleInternal(CSSPropertyID::kPropertyIDOverflow,
                            tasm::CSSValue::MakePlainString("visible"));
  parent->InsertNode(element);

  auto element0 = manager->CreateFiberNode("view");
  element->InsertNode(element0);

  page->FlushActionsAsRoot();

  EXPECT_TRUE(page->sl_node_ != nullptr);
  EXPECT_TRUE(page->sl_node_->is_dirty_);
  EXPECT_TRUE(parent->sl_node_ != nullptr);
  EXPECT_TRUE(parent->sl_node_->is_dirty_);
  EXPECT_TRUE(element->sl_node_ != nullptr);
  EXPECT_TRUE(element->sl_node_->is_dirty_);
  EXPECT_TRUE(element0->sl_node_ != nullptr);
  EXPECT_TRUE(element0->sl_node_->is_dirty_);

  page->Layout(std::make_shared<PipelineOptions>());

  EXPECT_FALSE(page->sl_node_->is_dirty_);
  EXPECT_FALSE(parent->sl_node_->is_dirty_);
  EXPECT_FALSE(element->sl_node_->is_dirty_);
  EXPECT_FALSE(element0->sl_node_->is_dirty_);

  element0->RequestLayout();
  EXPECT_TRUE(page->sl_node_->is_dirty_);
  EXPECT_TRUE(parent->sl_node_->is_dirty_);
  EXPECT_TRUE(element->sl_node_->is_dirty_);
  EXPECT_TRUE(element0->sl_node_->is_dirty_);
}

TEST_P(FiberElementTest, LayoutInElement_RemoveVirtualChildResetsAttachedFlag) {
  manager->page_options_.embedded_mode_ = static_cast<EmbeddedMode>(
      static_cast<int32_t>(manager->page_options_.embedded_mode_) |
      static_cast<int32_t>(EmbeddedMode::LAYOUT_IN_ELEMENT));

  auto page = manager->CreateFiberPage("page", 11);
  auto virtual_child = manager->CreateFiberNode("inline-text");
  page->InsertNode(virtual_child);

  page->FlushActionsAsRoot();
  EXPECT_TRUE(virtual_child->is_virtual_);
  EXPECT_TRUE(virtual_child->attached_to_layout_parent_);

  page->RemoveNode(virtual_child);
  page->FlushActionsAsRoot();
  EXPECT_FALSE(virtual_child->attached_to_layout_parent_);
}

TEST_P(FiberElementTest, InsertNode) {
  auto parent = manager->CreateFiberNode("view");
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 0);

  auto element = manager->CreateFiberNode("view");
  element->SetStyleInternal(CSSPropertyID::kPropertyIDOverflow,
                            tasm::CSSValue::MakePlainString("visible"));
  parent->InsertNode(element);

  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 1);
  EXPECT_EQ(parent->GetChildAt(0), element.get());
}

TEST_P(FiberElementTest, InsertNodeBefore) {
  auto parent = manager->CreateFiberNode("view");
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 0);

  auto element = manager->CreateFiberNode("view");
  element->SetStyleInternal(CSSPropertyID::kPropertyIDOverflow,
                            tasm::CSSValue::MakePlainString("visible"));
  parent->InsertNode(element);
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 1);
  auto second_element = manager->CreateFiberNode("view");

  parent->InsertNodeBefore(second_element, element);
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 2);

  EXPECT_EQ(parent->GetChildAt(0), second_element.get());
  EXPECT_EQ(parent->GetChildAt(1), element.get());
}

TEST_P(FiberElementTest, RemoveNodeAndFlush) {
  auto page = manager->CreateFiberPage("page", 11);
  auto parent = manager->CreateFiberNode("view");
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 0);

  // insert parent to page
  page->InsertNode(parent);

  // insert first_element to parent
  auto first_element = manager->CreateFiberNode("view");
  first_element->SetStyleInternal(CSSPropertyID::kPropertyIDOverflow,
                                  tasm::CSSValue::MakePlainString("visible"));
  parent->InsertNode(first_element);
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 1);

  // insert second element before first element to parent
  auto second_element = manager->CreateFiberNode("view");
  parent->InsertNodeBefore(second_element, first_element);
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 2);

  EXPECT_EQ(parent->GetChildAt(0), second_element.get());
  EXPECT_EQ(parent->GetChildAt(1), first_element.get());

  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), parent->impl_id(), -1));
  EXPECT_CALL(
      tasm_mediator,
      InsertLayoutNodeBefore(parent->impl_id(), second_element->impl_id(), -1));
  EXPECT_CALL(
      tasm_mediator,
      InsertLayoutNodeBefore(parent->impl_id(), first_element->impl_id(), -1));
  page->FlushActionsAsRoot();

  // remove the second element
  parent->RemoveNode(second_element);

  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 1);
  EXPECT_EQ(parent->GetChildAt(0), first_element.get());

  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(parent->impl_id(), second_element->impl_id()));
  page->FlushActionsAsRoot();
}

TEST_P(FiberElementTest, RemoveElement) {
  auto page = manager->CreateFiberPage("page", 11);
  auto element0 = manager->CreateFiberNode("view");
  auto element_before_black = manager->CreateFiberNode("view");
  auto element = manager->CreateFiberNode("view");
  auto text = manager->CreateFiberNode("text");
  auto ref = manager->CreateFiberNode("view");

  ref->InsertNode(element);
  ref->InsertNode(text);

  page->InsertNode(element0);
  page->InsertNode(element_before_black);
  page->InsertNode(ref);

  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), element0->impl_id(), -1));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(),
                                     element_before_black->impl_id(), -1));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), ref->impl_id(), -1));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(ref->impl_id(), element->impl_id(), -1));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(ref->impl_id(), text->impl_id(), -1));
  page->FlushActionsAsRoot();

  // Append after ref
  auto element_after_yellow = manager->CreateFiberNode("view");

  page->InsertNode(element_after_yellow);

  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(),
                                     element_after_yellow->impl_id(), -1));
  page->FlushActionsAsRoot();

  // remove ref's sibling
  page->RemoveNode(element_before_black);
  page->RemoveNode(element_after_yellow);
  EXPECT_CALL(tasm_mediator, RemoveLayoutNode(page->impl_id(),
                                              element_before_black->impl_id()));
  EXPECT_CALL(tasm_mediator, RemoveLayoutNode(page->impl_id(),
                                              element_after_yellow->impl_id()));
  page->FlushActionsAsRoot();

  // Insert node to ref
  auto text1 = manager->CreateFiberNode("text");
  ref->InsertNode(text1);
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(ref->impl_id(), text1->impl_id(), -1));
  page->FlushActionsAsRoot();
  EXPECT_EQ(static_cast<int>(ref->children().size()), 3);

  // remove ref
  page->RemoveNode(ref);
  EXPECT_CALL(tasm_mediator, RemoveLayoutNode(page->impl_id(), ref->impl_id()));
  page->FlushActionsAsRoot();

  // Remove node from ref
  ref->RemoveNode(text);
  EXPECT_CALL(tasm_mediator, RemoveLayoutNode(ref->impl_id(), text->impl_id()));
  EXPECT_EQ(static_cast<int>(ref->children().size()), 2);

  // do nothing for ref's layout node children, due to ref is detached form
  // view tree, but the remove action is still in ref
  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(ref->impl_id(), element->impl_id()))
      .Times(0);
  page->FlushActionsAsRoot();

  // insert ref again
  page->InsertNode(ref);
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), ref->impl_id(), -1));
  page->FlushActionsAsRoot();
  EXPECT_EQ(static_cast<int>(page->children().size()), 2);
}

TEST_P(FiberElementTest, TestLayoutNodeAPI) {
  auto page = manager->CreateFiberPage("page", 11);
  auto element0 = manager->CreateFiberNode("view");
  auto element = manager->CreateFiberNode("view");
  auto text = manager->CreateFiberNode("text");
  auto ref = manager->CreateFiberWrapperElement();

  ref->InsertNode(element);
  ref->InsertNode(text);

  page->InsertNode(element0);
  page->InsertNode(ref);

  page->RemoveNode(ref);
  page->InsertNode(ref);

  auto element_end = manager->CreateFiberNode("view");
  page->InsertNode(element_end);

  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), element0->impl_id(), -1));
  EXPECT_CALL(tasm_mediator, InsertLayoutNodeBefore(
                                 page->impl_id(), element_end->impl_id(), -1));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), element->impl_id(),
                                     element_end->impl_id()));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), text->impl_id(),
                                     element_end->impl_id()));

  page->FlushActionsAsRoot();
}

TEST_P(FiberElementTest, ZIndexFlushUnExpectedFiberCase) {
  auto page = manager->CreateFiberPage("page", 11);

  auto parent_element_0 = manager->CreateFiberView();
  page->InsertNode(parent_element_0);

  auto parent_element_1 = manager->CreateFiberView();
  parent_element_0->InsertNode(parent_element_1);

  auto parent_element = manager->CreateFiberView();
  parent_element_1->InsertNode(parent_element);

  page->FlushActionsAsRoot();

  auto child = manager->CreateFiberView();
  parent_element->InsertNode(child);
  child->SetStyle(CSSPropertyID::kPropertyIDZIndex, lepus::Value(1));

  page->RemoveNode(parent_element_0);

  child->FlushActionsAsRoot();

  // child flush will be blocked!
  EXPECT_TRUE(child->dirty_ != 0);
}

TEST_P(FiberElementTest, ZIndexFlushUnExpectedFiberCase1) {
  auto page = manager->CreateFiberPage("page", 11);

  auto parent_element_0 = manager->CreateFiberView();
  page->InsertNode(parent_element_0);

  auto parent_element_1 = manager->CreateFiberView();
  parent_element_0->InsertNode(parent_element_1);

  auto parent_element = manager->CreateFiberView();
  parent_element_1->InsertNode(parent_element);

  auto z_index_change_node = manager->CreateFiberView();
  parent_element->InsertNode(z_index_change_node);

  page->FlushActionsAsRoot();

  z_index_change_node->SetStyle(CSSPropertyID::kPropertyIDZIndex,
                                lepus::Value(1));

  for (int i = 0; i < 100; ++i) {
    auto new_z_index = manager->CreateFiberView();
    new_z_index->SetStyle(CSSPropertyID::kPropertyIDZIndex, lepus::Value(1));
    z_index_change_node->InsertNode(new_z_index);
  }
  page->FlushActionsAsRoot();
}

TEST_P(FiberElementTest, TestOverflowAndLayoutOnly) {
  // styles for fiber_element
  //  constructor css fragment
  StyleMap indexAttributes;
  CSSParserConfigs configs;
  auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);

  CSSParserTokenMap indexTokensMap;
  // class .test
  {
    auto id = CSSPropertyID::kPropertyIDOpacity;
    auto impl = lepus::Value(0.3);
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".test";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  // class .test01
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDWidth;
    auto impl = lepus::Value("20px");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".test01";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap fontfaces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokensMap, keyframes, fontfaces);

  // parent
  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  EXPECT_FALSE(page->is_layout_only_);

  // child
  auto fiber_element = manager->CreateFiberView();
  fiber_element->parent_component_element_ = page.get();
  page->InsertNode(fiber_element);
  fiber_element->SetClass("test01");
  // force the element to overflow hidden
  fiber_element->computed_css_style()->SetOverflowDefaultVisible(false);

  page->FlushActionsAsRoot();

  EXPECT_FALSE(fiber_element->is_layout_only_);

  // child0
  auto fiber_element_0 = manager->CreateFiberView();
  fiber_element_0->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_0);
  fiber_element_0->SetClass("test01");
  // force the element to overflow visible
  fiber_element_0->computed_css_style()->SetOverflowDefaultVisible(true);

  page->FlushActionsAsRoot();

  EXPECT_TRUE(fiber_element_0->is_layout_only_);

  // child1
  auto fiber_element_1 = manager->CreateFiberView();
  fiber_element_1->parent_component_element_ = page.get();
  fiber_element_1->SetClass("test01");
  // force the element to overflow visible
  fiber_element_1->computed_css_style()->SetOverflowDefaultVisible(true);

  auto scroll_view = manager->CreateFiberScrollView("scroll-view");
  scroll_view->InsertNode(fiber_element_1);
  page->InsertNode(scroll_view);

  page->FlushActionsAsRoot();
  EXPECT_FALSE(fiber_element_1->is_layout_only_);

  // child2 component
  base::String component_id("21");
  int32_t css_id = 100;
  base::String entry_name("__Card__");
  base::String component_name("TestComp");
  base::String path("/index/components/TestComp");

  auto comp = manager->CreateFiberComponent(component_id, css_id, entry_name,
                                            component_name, path);

  comp->SetClass("test01");
  // force the element to overflow visible
  comp->computed_css_style()->SetOverflowDefaultVisible(true);

  lepus::Value component_at_index(10);
  lepus::Value enqueue_component;
  lepus::Value component_at_indexes;

  auto list = manager->CreateFiberList(nullptr, "list", component_at_index,
                                       enqueue_component, component_at_indexes);
  list->InsertNode(comp);
  list->SetAttribute("column-count", lepus::Value(2));
  page->InsertNode(list);

  page->FlushActionsAsRoot();

  EXPECT_FALSE(comp->is_layout_only_);
  EXPECT_TRUE(HasCaptureSignWithLayoutAttribute(
      list->impl_id(), starlight::LayoutAttribute::kColumnCount));
  EXPECT_TRUE(HasCaptureSignWithLayoutAttribute(
      list->impl_id(), starlight::LayoutAttribute::kScroll));
}

TEST_P(FiberElementTest, TestIsLayoutOnlyUpdate) {
  // styles for fiber_element
  //  constructor css fragment
  StyleMap indexAttributes;
  CSSParserConfigs configs;
  auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);

  CSSParserTokenMap indexTokensMap;
  // class .test
  {
    auto id = CSSPropertyID::kPropertyIDOpacity;
    auto impl = lepus::Value(0.3);
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".test";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  // class .test01
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);

    std::string key = ".test01";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap fontfaces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokensMap, keyframes, fontfaces);

  // parent
  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  EXPECT_FALSE(page->is_layout_only_);

  // child0
  auto fiber_element_0 = manager->CreateFiberView();
  fiber_element_0->parent_component_element_ = page.get();

  page->InsertNode(fiber_element_0);
  fiber_element_0->SetClass("test01");
  // force the element to overflow visible
  fiber_element_0->computed_css_style()->SetOverflowDefaultVisible(true);

  // child1
  auto fiber_element_1 = manager->CreateFiberView();
  fiber_element_1->parent_component_element_ = page.get();

  page->InsertNode(fiber_element_1);
  fiber_element_1->SetClass("test01");
  // force the element to overflow visible
  fiber_element_1->computed_css_style()->SetOverflowDefaultVisible(true);

  page->FlushActionsAsRoot();
  EXPECT_TRUE(fiber_element_0->is_layout_only_);
  EXPECT_TRUE(fiber_element_1->is_layout_only_);

  fiber_element_0->SetStyle(kPropertyIDBackground, lepus::Value("black"));
  fiber_element_1->SetStyle(kPropertyIDBackground, lepus::Value("black"));

  // child2
  auto fiber_element_2 = manager->CreateFiberView();
  fiber_element_2->parent_component_element_ = page.get();

  page->InsertNode(fiber_element_2);
  page->FlushActionsAsRoot();
  EXPECT_FALSE(fiber_element_0->is_layout_only_);
  EXPECT_FALSE(fiber_element_1->is_layout_only_);
}

TEST_P(FiberElementTest, TestZIndexRemovedRelated) {
  // styles for fiber_element
  //  constructor css fragment
  StyleMap indexAttributes;
  CSSParserConfigs configs;
  auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);

  CSSParserTokenMap indexTokensMap;
  // class .test
  {
    auto id = CSSPropertyID::kPropertyIDZIndex;
    auto impl = lepus::Value(2);
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".test";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap fontfaces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokensMap, keyframes, fontfaces);

  // parent
  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // child
  auto fiber_element = manager->CreateFiberView();
  fiber_element->parent_component_element_ = page.get();
  page->InsertNode(fiber_element);
  // force the element to overflow hidden
  fiber_element->computed_css_style()->SetOverflowDefaultVisible(false);

  // child0
  auto fiber_element_0 = manager->CreateFiberView();
  fiber_element_0->parent_component_element_ = page.get();
  fiber_element->InsertNode(fiber_element_0);
  // force the element to overflow visible
  fiber_element_0->computed_css_style()->SetOverflowDefaultVisible(false);

  // child1
  auto fiber_element_1 = manager->CreateFiberView();
  fiber_element_1->parent_component_element_ = page.get();
  fiber_element_1->SetClass("test");
  // force the element to overflow visible
  fiber_element_1->computed_css_style()->SetOverflowDefaultVisible(false);
  fiber_element_0->InsertNode(fiber_element_1);

  page->FlushActionsAsRoot();

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();

  EXPECT_TRUE(page_painting_node->children_.size() == 2);

  // first child is child
  auto* child_painting_node =
      painting_context->node_map_.at(fiber_element->impl_id()).get();
  EXPECT_TRUE(page_painting_node->children_[0] == child_painting_node);

  // second child is zIndex
  auto* child1_painting_node =
      painting_context->node_map_.at(fiber_element_1->impl_id()).get();
  EXPECT_TRUE(page_painting_node->children_[1] == child1_painting_node);

  auto* child0_painting_node =
      painting_context->node_map_.at(fiber_element_0->impl_id()).get();
  EXPECT_TRUE(child_painting_node->children_[0] == child0_painting_node);

  // Remove child0 from child
  fiber_element->RemoveNode(fiber_element_0);
  page->FlushActionsAsRoot();
  painting_context->Flush();

  EXPECT_TRUE(page_painting_node->children_.size() == 1);
  EXPECT_TRUE(page_painting_node->children_[0] == child_painting_node);

  // re insert child0 to child
  fiber_element->InsertNode(fiber_element_0);
  page->FlushActionsAsRoot();
  painting_context->Flush();

  EXPECT_TRUE(page_painting_node->children_.size() == 2);
  EXPECT_TRUE(page_painting_node->children_[0] == child_painting_node);

  EXPECT_TRUE(page_painting_node->children_[1] == child1_painting_node);

  EXPECT_TRUE(child_painting_node->children_[0] == child0_painting_node);
}

TEST_P(FiberElementTest, RequireFlush) {
  auto page = manager->CreateFiberPage("10", 11);
  page->SetIdSelector("page");

  // element0 tree
  auto element0 = manager->CreateFiberView();
  page->InsertNode(element0);

  auto element00 = manager->CreateFiberView();
  element0->InsertNode(element00);

  auto element000 = manager->CreateFiberView();
  element00->InsertNode(element000);

  // element 1
  auto element1 = manager->CreateFiberView();
  page->InsertNode(element1);

  auto element10 = manager->CreateFiberView();
  element1->InsertNode(element10);

  EXPECT_TRUE(page->flush_required_ == true);
  EXPECT_TRUE(element0->flush_required_ == true);
  EXPECT_TRUE(element00->flush_required_ == true);
  EXPECT_TRUE(element000->flush_required_ == true);
  EXPECT_TRUE(element1->flush_required_ == true);
  EXPECT_TRUE(element10->flush_required_ == true);

  page->FlushActionsAsRoot();

  EXPECT_TRUE(page->flush_required_ == false);
  EXPECT_TRUE(element0->flush_required_ == false);
  EXPECT_TRUE(element00->flush_required_ == false);
  EXPECT_TRUE(element000->flush_required_ == false);
  EXPECT_TRUE(element1->flush_required_ == false);
  EXPECT_TRUE(element10->flush_required_ == false);

  element00->SetAttribute("enable-layout", lepus::Value("false"));
  element10->dirty_ |= FiberElement::kDirtyAttr;  // just hardcode for testing

  EXPECT_TRUE(page->flush_required_ == true);
  EXPECT_TRUE(element0->flush_required_ == true);
  EXPECT_TRUE(element00->flush_required_ == true);
  EXPECT_TRUE(element000->flush_required_ == false);
  EXPECT_TRUE(element1->flush_required_ == false);
  EXPECT_TRUE(element10->flush_required_ == false);

  page->FlushActionsAsRoot();
  EXPECT_TRUE(page->flush_required_ == false);
  EXPECT_TRUE(element0->flush_required_ == false);
  EXPECT_TRUE(element00->flush_required_ == false);
  EXPECT_TRUE(element000->flush_required_ == false);
  EXPECT_TRUE(element1->flush_required_ == false);
  EXPECT_TRUE(element10->flush_required_ == false);
  EXPECT_TRUE((element10->dirty_ & FiberElement::kDirtyAttr) != 0);
}

TEST_P(FiberElementTest, FlushActionsAsRootCase01) {
  auto page = manager->CreateFiberPage("page", 11);

  auto element0 = manager->CreateFiberView();
  page->InsertNode(element0);

  auto wrapper = manager->CreateFiberWrapperElement();
  element0->InsertNode(wrapper);

  auto element = manager->CreateFiberView();

  wrapper->InsertNode(element);

  auto text = manager->CreateFiberText("text");
  element->InsertNode(text);

  page->FlushActionsAsRoot();

  // check painting node
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto* element0_painting_node =
      painting_context->node_map_.at(element0->impl_id()).get();
  auto* element_painting_node =
      painting_context->node_map_.at(element->impl_id()).get();
  auto* text_painting_node =
      painting_context->node_map_.at(text->impl_id()).get();

  auto page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children[0] == element0_painting_node);

  EXPECT_TRUE(element0_painting_node->children_[0] == element_painting_node);
  EXPECT_TRUE(element_painting_node->children_[0] == text_painting_node);

  element->SetStyle(CSSPropertyID::kPropertyIDVisibility,
                    lepus::Value("hidden"));
  element->FlushActionsAsRoot();
  painting_context->Flush();

  EXPECT_TRUE(element_painting_node->props_.size() == 1);
  std::string visibility("visibility");
  EXPECT_TRUE(element_painting_node->props_.at(visibility) == lepus::Value(0));
}

TEST_P(FiberElementTest, RemoveIntergenerationalChild) {
  //===test fixed element =====//
  // normal case
  auto page = manager->CreateFiberPage("page", 11);
  auto element0 = manager->CreateFiberView();
  element0->MarkCanBeLayoutOnly(false);
  page->InsertNode(element0);
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), element0->impl_id(), -1));

  auto element1 = manager->CreateFiberView();
  element1->MarkCanBeLayoutOnly(false);
  element0->InsertNode(element1);
  EXPECT_CALL(tasm_mediator, InsertLayoutNodeBefore(element0->impl_id(),
                                                    element1->impl_id(), -1));

  auto wrapper = manager->CreateFiberWrapperElement();

  auto child = manager->CreateFiberView();
  child->MarkCanBeLayoutOnly(false);
  child->SetStyle(CSSPropertyID::kPropertyIDPosition, lepus::Value("fixed"));
  wrapper->InsertNode(child);
  element1->InsertNode(wrapper);
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), child->impl_id(), -1));

  page->FlushActionsAsRoot();

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto* element0_painting_node =
      painting_context->node_map_.at(element0->impl_id()).get();

  auto* child_painting_node =
      painting_context->node_map_.at(child->impl_id()).get();
  auto page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 2);
  EXPECT_TRUE(page_children[0] == element0_painting_node);
  EXPECT_TRUE(page_children[1] == child_painting_node);

  // remove element1 from page, take care,the fixed child should also be
  // removed
  page->RemoveNode(element0);
  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(page->impl_id(), element0->impl_id()));
  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(page->impl_id(), child->impl_id()));
  page->FlushActionsAsRoot();
  painting_context->Flush();
  page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 0);

  // re-insert element0, the fixed child should be re-attached!
  page->InsertNode(element0);
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), element0->impl_id(), -1));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), child->impl_id(), -1));
  page->FlushActionsAsRoot();
  painting_context->Flush();

  page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 2);
  EXPECT_TRUE(page_children[0] == element0_painting_node);
  EXPECT_TRUE(page_children[1] == child_painting_node);

  page->RemoveNode(element0);
  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(page->impl_id(), element0->impl_id()));
  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(page->impl_id(), child->impl_id()));

  // test ZIndex child === //
  auto element00 = manager->CreateFiberView();
  element00->MarkCanBeLayoutOnly(false);
  element00->SetStyle(CSSPropertyID::kPropertyIDZIndex, lepus::Value("3"));
  page->InsertNode(element00);
  EXPECT_CALL(tasm_mediator, InsertLayoutNodeBefore(page->impl_id(),
                                                    element00->impl_id(), -1));

  auto element11 = manager->CreateFiberView();
  element11->MarkCanBeLayoutOnly(false);
  element00->InsertNode(element11);

  auto wrapper00 = manager->CreateFiberWrapperElement();
  EXPECT_CALL(tasm_mediator, InsertLayoutNodeBefore(element00->impl_id(),
                                                    element11->impl_id(), -1));

  auto child00 = manager->CreateFiberView();
  child00->MarkCanBeLayoutOnly(false);
  child00->SetStyle(CSSPropertyID::kPropertyIDZIndex, lepus::Value("99"));
  wrapper00->InsertNode(child00);
  element11->InsertNode(wrapper00);
  EXPECT_CALL(tasm_mediator, InsertLayoutNodeBefore(element11->impl_id(),
                                                    child00->impl_id(), -1));

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());
  painting_context->Flush();

  auto* element00_painting_node =
      painting_context->node_map_.at(element00->impl_id()).get();

  auto* element11_painting_node =
      painting_context->node_map_.at(element11->impl_id()).get();

  auto* child00_painting_node =
      painting_context->node_map_.at(child00->impl_id()).get();
  page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 1);
  EXPECT_TRUE(page_children[0] == element00_painting_node);

  auto element00_children = element00_painting_node->children_;
  EXPECT_TRUE(element00_children.size() == 2);
  EXPECT_TRUE(element00_children[0] == element11_painting_node);
  EXPECT_TRUE(element00_children[1] == child00_painting_node);

  EXPECT_TRUE(element11_painting_node->children_.size() == 0);

  element11->RemoveNode(wrapper00);
  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(element11->impl_id(), child00->impl_id()));
  manager->OnPatchFinish(options, element11.get());
  painting_context->Flush();

  EXPECT_TRUE(element00_painting_node->children_.size() == 1);
  EXPECT_TRUE(element00_painting_node->children_[0] == element11_painting_node);
  EXPECT_TRUE(child00_painting_node->parent_ == nullptr);

  // re-attach wrapper00
  element11->InsertNode(wrapper00);
  EXPECT_CALL(tasm_mediator, InsertLayoutNodeBefore(element11->impl_id(),
                                                    child00->impl_id(), -1));
  manager->OnPatchFinish(options, element11.get());
  painting_context->Flush();

  element00_children = element00_painting_node->children_;
  EXPECT_TRUE(element00_children.size() == 2);
  EXPECT_TRUE(element00_children[0] == element11_painting_node);
  EXPECT_TRUE(element00_children[1] == child00_painting_node);

  EXPECT_TRUE(element11_painting_node->children_.size() == 0);

  //=====insert fixed to wrapper-wrapped node ===/
  page->RemoveNode(element00);
  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(page->impl_id(), element00->impl_id()));
  auto element000 = manager->CreateFiberView();
  element000->MarkCanBeLayoutOnly(false);
  page->InsertNode(element000);
  EXPECT_CALL(tasm_mediator, InsertLayoutNodeBefore(page->impl_id(),
                                                    element000->impl_id(), -1));

  auto element222 = manager->CreateFiberView();
  element222->MarkCanBeLayoutOnly(false);
  element000->InsertNode(element222);
  EXPECT_CALL(tasm_mediator, InsertLayoutNodeBefore(element000->impl_id(),
                                                    element222->impl_id(), -1));

  auto wrapper11 = manager->CreateFiberWrapperElement();

  auto wrapper22 = manager->CreateFiberWrapperElement();
  wrapper11->InsertNode(wrapper22);

  element222->InsertNode(wrapper11);

  auto child22 = manager->CreateFiberView();
  child22->MarkCanBeLayoutOnly(false);
  child22->SetStyle(CSSPropertyID::kPropertyIDPosition, lepus::Value("fixed"));
  wrapper22->InsertNode(child22);
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), child22->impl_id(), -1));

  auto text = manager->CreateFiberText("text");
  child22->InsertNode(text);
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(child22->impl_id(), text->impl_id(), -1));

  manager->OnPatchFinish(options, page.get());
  painting_context->Flush();

  auto* element000_painting_node =
      painting_context->node_map_.at(element000->impl_id()).get();

  auto* text_painting_node =
      painting_context->node_map_.at(text->impl_id()).get();

  auto* child22_painting_node =
      painting_context->node_map_.at(child22->impl_id()).get();

  page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 2);
  EXPECT_TRUE(page_children[0] == element000_painting_node);
  EXPECT_TRUE(page_children[1] == child22_painting_node);
  EXPECT_TRUE(child22_painting_node->children_[0] == text_painting_node);

  wrapper11->RemoveNode(wrapper22);
  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(page->impl_id(), child22->impl_id()));
  manager->OnPatchFinish(options, wrapper11.get());
  painting_context->Flush();

  page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 1);
  EXPECT_TRUE(page_children[0] == element000_painting_node);

  wrapper11->InsertNode(wrapper22);
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), child22->impl_id(), -1));
  manager->OnPatchFinish(options, wrapper11.get());
  painting_context->Flush();

  page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 2);
  EXPECT_TRUE(page_children[0] == element000_painting_node);
  EXPECT_TRUE(page_children[1] == child22_painting_node);
}

TEST_P(FiberElementTest, DISABLED_RemoveIntergenerationalChild1) {
  //===test fixed element =====//
  // normal case
  auto page = manager->CreateFiberPage("page", 11);
  auto element0 = manager->CreateFiberView();
  element0->MarkCanBeLayoutOnly(false);
  page->InsertNode(element0);

  auto element1 = manager->CreateFiberView();
  element1->MarkCanBeLayoutOnly(false);
  element0->InsertNode(element1);

  auto wrapper = manager->CreateFiberWrapperElement();

  auto child0 = manager->CreateFiberView();
  child0->MarkCanBeLayoutOnly(false);
  wrapper->InsertNode(child0);
  element1->InsertNode(wrapper);

  auto child1 = manager->CreateFiberView();
  child1->MarkCanBeLayoutOnly(false);
  child0->InsertNode(child1);

  auto child2 = manager->CreateFiberView();
  child2->MarkCanBeLayoutOnly(false);
  child1->InsertNode(child2);

  auto child3 = manager->CreateFiberView();
  child3->MarkCanBeLayoutOnly(false);
  child2->InsertNode(child3);

  auto child = manager->CreateFiberView();
  child->MarkCanBeLayoutOnly(false);
  child->SetStyle(CSSPropertyID::kPropertyIDPosition, lepus::Value("fixed"));
  child3->InsertNode(child);

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());
  painting_context->Flush();
  manager->catalyzer_->UpdateLayoutRecursively();

  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  EXPECT_EQ(page_painting_node->children_.size(), 2);

  child2->RemoveNode(child3);
  wrapper->RemoveNode(child0);

  manager->OnPatchFinish(options, page.get());
  painting_context->Flush();
  manager->catalyzer_->UpdateLayoutRecursively();

  EXPECT_EQ(page_painting_node->children_.size(), 1);

  child0->InsertNode(child);
  wrapper->InsertNode(child0);

  manager->OnPatchFinish(options, page.get());
  painting_context->Flush();
  manager->catalyzer_->UpdateLayoutRecursively();
}

TEST_P(FiberElementTest, DISABLED_RemoveIntergenerationalChild2) {
  //===test fixed element =====//
  // normal case
  auto page = manager->CreateFiberPage("page", 11);
  auto element0 = manager->CreateFiberView();
  element0->MarkCanBeLayoutOnly(false);
  page->InsertNode(element0);

  auto element1 = manager->CreateFiberView();
  element1->MarkCanBeLayoutOnly(false);
  element0->InsertNode(element1);

  auto wrapper = manager->CreateFiberWrapperElement();

  auto child0 = manager->CreateFiberView();
  child0->MarkCanBeLayoutOnly(false);
  wrapper->InsertNode(child0);
  element1->InsertNode(wrapper);

  auto child1 = manager->CreateFiberView();
  child1->MarkCanBeLayoutOnly(false);
  child0->InsertNode(child1);

  auto child2 = manager->CreateFiberView();
  child2->MarkCanBeLayoutOnly(false);
  child1->InsertNode(child2);

  auto child3 = manager->CreateFiberView();
  child3->MarkCanBeLayoutOnly(false);
  child2->InsertNode(child3);

  auto child = manager->CreateFiberView();
  child->MarkCanBeLayoutOnly(false);
  child->SetStyle(CSSPropertyID::kPropertyIDPosition, lepus::Value("fixed"));
  child3->InsertNode(child);

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());
  painting_context->Flush();
  manager->catalyzer_->UpdateLayoutRecursively();

  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  EXPECT_EQ(page_painting_node->children_.size(), 2);

  child3->RemoveNode(child);
  wrapper->RemoveNode(child0);

  manager->OnPatchFinish(options, page.get());
  painting_context->Flush();
  manager->catalyzer_->UpdateLayoutRecursively();

  EXPECT_EQ(page_painting_node->children_.size(), 1);

  child0->InsertNode(child);
  wrapper->InsertNode(child0);

  manager->OnPatchFinish(options, page.get());
  painting_context->Flush();
  manager->catalyzer_->UpdateLayoutRecursively();
}

TEST_P(FiberElementTest, DISABLED_RemoveIntergenerationalChild3) {
  //===test fixed element =====//
  // normal case
  auto page = manager->CreateFiberPage("page", 11);
  auto element0 = manager->CreateFiberView();
  element0->MarkCanBeLayoutOnly(false);
  page->InsertNode(element0);

  auto element1 = manager->CreateFiberView();
  element1->MarkCanBeLayoutOnly(false);
  element0->InsertNode(element1);

  auto wrapper = manager->CreateFiberWrapperElement();

  auto child0 = manager->CreateFiberView();
  child0->MarkCanBeLayoutOnly(false);
  wrapper->InsertNode(child0);
  element1->InsertNode(wrapper);

  auto child1 = manager->CreateFiberView();
  child1->MarkCanBeLayoutOnly(false);
  child0->InsertNode(child1);

  auto child2 = manager->CreateFiberView();
  child2->MarkCanBeLayoutOnly(false);
  child1->InsertNode(child2);

  auto child3 = manager->CreateFiberView();
  child3->MarkCanBeLayoutOnly(false);
  child2->InsertNode(child3);

  auto child = manager->CreateFiberView();
  child->MarkCanBeLayoutOnly(false);
  child->SetStyle(CSSPropertyID::kPropertyIDZIndex, lepus::Value(999));
  child3->InsertNode(child);

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page.get());
  painting_context->Flush();
  manager->catalyzer_->UpdateLayoutRecursively();

  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  EXPECT_EQ(page_painting_node->children_.size(), 2);

  child3->RemoveNode(child);
  wrapper->RemoveNode(child0);

  manager->OnPatchFinish(options, page.get());
  painting_context->Flush();
  manager->catalyzer_->UpdateLayoutRecursively();

  EXPECT_EQ(page_painting_node->children_.size(), 1);

  child0->InsertNode(child);
  wrapper->InsertNode(child0);

  manager->OnPatchFinish(options, page.get());
  painting_context->Flush();
  manager->catalyzer_->UpdateLayoutRecursively();
}

TEST_P(FiberElementTest, ReInsertNodeTest) {
  auto page = manager->CreateFiberPage("page", 11);

  auto parent = manager->CreateFiberView();
  parent->MarkCanBeLayoutOnly(false);

  page->InsertNodeBefore(parent, nullptr);
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), parent->impl_id(), -1));

  auto element = manager->CreateFiberView();
  element->MarkCanBeLayoutOnly(false);

  parent->InsertNodeBefore(element, nullptr);
  EXPECT_CALL(tasm_mediator, InsertLayoutNodeBefore(parent->impl_id(),
                                                    element->impl_id(), -1));

  page->FlushActionsAsRoot();

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto* parent_painting_node =
      painting_context->node_map_.at(parent->impl_id()).get();

  EXPECT_TRUE(page_painting_node->children_[0] == parent_painting_node);

  auto* element_painting_node =
      painting_context->node_map_.at(element->impl_id()).get();

  EXPECT_TRUE(parent_painting_node->children_[0] == element_painting_node);

  auto parent1 = manager->CreateFiberView();
  parent1->MarkCanBeLayoutOnly(false);

  parent1->InsertNodeBefore(element, nullptr);
  EXPECT_CALL(tasm_mediator, InsertLayoutNodeBefore(parent1->impl_id(),
                                                    element->impl_id(), -1));
  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(parent->impl_id(), element->impl_id()));
  page->InsertNodeBefore(parent1, nullptr);
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), parent1->impl_id(), -1));
  page->FlushActionsAsRoot();
  painting_context->Flush();

  auto* parent1_painting_node =
      painting_context->node_map_.at(parent1->impl_id()).get();
  EXPECT_TRUE(parent_painting_node->children_.size() == 0);
  EXPECT_TRUE(parent1_painting_node->children_.size() == 1);
  EXPECT_TRUE(parent1_painting_node->children_[0] == element_painting_node);
}

TEST_P(FiberElementTest, TestFlushActionsFromSubTree) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableZIndex(true);
  config->SetEnableCSSInheritance(true);
  manager->SetConfig(config);

  // normal case
  auto page = manager->CreateFiberPage("page", 11);
  auto element0 = manager->CreateFiberView();
  page->InsertNode(element0);
  page->FlushActionsAsRoot();

  // element1 is not flushed ever
  auto element1 = manager->CreateFiberView();
  element0->InsertNode(element1);

  auto new_parent = manager->CreateFiberView();
  element1->SetStyle(CSSPropertyID::kPropertyIDBorder,
                     lepus::Value("1px solid red"));

  auto child = manager->CreateFiberView();
  child->MarkCanBeLayoutOnly(false);
  new_parent->InsertNode(child);

  element1->InsertNode(new_parent);

  // a bad case, element1 is not flushed
  new_parent->FlushActionsAsRoot();
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();

  auto* element0_painting_node =
      painting_context->node_map_.at(element0->impl_id()).get();
  auto* element1_painting_node =
      painting_context->node_map_.at(element1->impl_id()).get();

  auto* new_parent_painting_node =
      painting_context->node_map_.at(new_parent->impl_id()).get();

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
  EXPECT_TRUE(element1_children[0] == new_parent_painting_node);

  auto new_parent_children = new_parent_painting_node->children_;
  EXPECT_TRUE(new_parent_children[0] == child_painting_node);

  auto element1_props = element1_painting_node->props_;
  EXPECT_TRUE(element1_props.size() > 1);
}

TEST_P(FiberElementTest, ExtendedLayoutOnlyOpt) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableExtendedLayoutOpt(
      false);  // false can not make the opt invalid
  manager->SetConfig(config);

  // page
  auto page = manager->CreateFiberPage("page", 11);

  auto parent = manager->CreateFiberView();
  parent->computed_css_style()->SetOverflowDefaultVisible(true);

  auto child = manager->CreateFiberView();
  child->MarkCanBeLayoutOnly(false);

  page->InsertNode(parent);
  parent->InsertNode(child);

  page->FlushActionsAsRoot();

  // check element container tree
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto page_painting_children = page_painting_node->children_;
  EXPECT_TRUE(page_painting_children.size() == 1);
  EXPECT_TRUE(page_painting_children[0]->id_ == child->impl_id());

  page->RemoveNode(parent);
  page->FlushActionsAsRoot();
  painting_context->Flush();

  auto parent2 = manager->CreateFiberView();
  parent2->computed_css_style()->SetOverflowDefaultVisible(true);
  parent2->SetStyle(CSSPropertyID::kPropertyIDTextAlign,
                    lepus::Value("center"));
  parent2->SetStyle(CSSPropertyID::kPropertyIDDirection, lepus::Value("ltr"));

  auto child2 = manager->CreateFiberView();
  child2->MarkCanBeLayoutOnly(false);

  page->InsertNode(parent2);
  parent2->InsertNode(child2);

  page->FlushActionsAsRoot();
  painting_context->Flush();

  // check element container tree

  page_painting_children = page_painting_node->children_;
  EXPECT_TRUE(page_painting_children.size() == 1);
  EXPECT_TRUE(page_painting_children[0]->id_ == child2->impl_id());
}

TEST_P(FiberElementTest, VirtualParentTest) {
  fml::RefPtr<IfElement> if_element =
      fml::AdoptRef<IfElement>(new IfElement(manager, "if"));

  fml::RefPtr<ForElement> for_element =
      fml::AdoptRef<ForElement>(new ForElement(manager, "for"));

  auto view = manager->CreateFiberView();

  for_element->set_virtual_parent(if_element.get());
  view->set_virtual_parent(for_element.get());

  EXPECT_TRUE(for_element->virtual_parent()->impl_id() ==
              if_element->impl_id());
  EXPECT_TRUE(view->virtual_parent()->impl_id() == for_element->impl_id());
  EXPECT_TRUE(view->root_virtual_parent()->impl_id() == if_element->impl_id());
}

TEST_P(FiberElementTest, LayoutAPITest) {
  // page
  auto page = manager->CreateFiberPage("page", 11);
  page->PreparePropBundleIfNeed();

  page->InitLayoutBundle();
  page->MarkAsLayoutRoot();
  page->AttachLayoutNode(page->prop_bundle_);
  page->UpdateLayoutNodeProps(page->prop_bundle_);
  page->UpdateLayoutNodeStyle(CSSPropertyID::kPropertyIDWidth, CSSValue());
  page->ResetLayoutNodeStyle(CSSPropertyID::kPropertyIDWidth);
  page->UpdateLayoutNodeFontSize(100, 100);
  page->UpdateLayoutNodeAttribute(starlight::LayoutAttribute::kScroll,
                                  lepus::Value(true));

  page->FlushActionsAsRoot();
  EXPECT_TRUE(HasCaptureSignWithStyleKeyAndValuePattern(
      page->impl_id(), CSSPropertyID::kPropertyIDWidth, CSSValue()));
  EXPECT_TRUE(HasCaptureSignWithResetStyle(page->impl_id(),
                                           CSSPropertyID::kPropertyIDWidth));
  EXPECT_TRUE(HasCaptureSignWithTag(page->impl_id(), "page"));
  EXPECT_TRUE(HasCaptureSignWithFontSize(page->impl_id(), 100, 100, 1));
  EXPECT_TRUE(HasCaptureSignWithLayoutAttribute(
      page->impl_id(), starlight::LayoutAttribute::kScroll,
      lepus::Value(true)));
}

TEST_P(FiberElementTest, TestRemoveVirtualParentCase) {
  // page
  auto page = manager->CreateFiberPage("page", 11);

  // text
  auto text_element = manager->CreateFiberText("text");
  page->InsertNode(text_element);

  auto fiber_raw_text = manager->CreateFiberRawText();
  text_element->InsertNode(fiber_raw_text);

  // inline text(it's virtual)
  auto inline_text_element = manager->CreateFiberText("text");
  text_element->InsertNode(inline_text_element);

  // inline view
  auto inline_view_element = manager->CreateFiberView();
  inline_view_element->SetStyle(CSSPropertyID::kPropertyIDBorder,
                                lepus::Value("1px"));
  inline_text_element->InsertNode(inline_view_element);

  page->FlushActionsAsRoot();

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());

  painting_context->Flush();

  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();

  auto page_painting_children = page_painting_node->children_;
  EXPECT_TRUE(page_painting_children.size() == 1);
  EXPECT_TRUE(page_painting_children[0]->id_ == text_element->impl_id());

  auto* text_painting_node =
      painting_context->node_map_.at(text_element->impl_id()).get();
  EXPECT_TRUE(text_painting_node->children_.size() == 1);
  EXPECT_TRUE(text_painting_node->children_[0]->id_ ==
              inline_view_element->impl_id());

  text_element->RemoveNode(inline_text_element);
  page->FlushActionsAsRoot();
  painting_context->Flush();

  // check the inline-view is detached from text_element
  EXPECT_TRUE(text_painting_node->children_.size() == 0);
}

TEST_P(FiberElementTest, TestComponentElementSetAttributeToBeNotLayoutOnly) {
  base::String component_id("21");
  int32_t css_id = 100;
  base::String entry_name("TTTT");
  base::String component_name("TestComp");
  base::String path("/index/components/TestComp");

  auto comp = manager->CreateFiberComponent(component_id, css_id, entry_name,
                                            component_name, path);
  EXPECT_EQ(comp->ComponentStrId(), "21");
  EXPECT_TRUE(comp->AttrDirty());
  const auto& attr_map = comp->updated_attr_map();
  const auto& it = attr_map.find(BASE_STATIC_STRING(kComponentID));
  EXPECT_TRUE(attr_map.end() != it);
  EXPECT_EQ(it->second, lepus::Value(component_id));
  comp->SetAttribute("1234", lepus::Value("1234"));

  // Test has_layout_only_props_
  auto page = manager->CreateFiberPage("page", 11);
  page->InsertNode(comp);
  page->FlushActionsAsRoot();
  EXPECT_FALSE(comp->has_layout_only_props_);
}

TEST_P(FiberElementTest, TestCanBeLayoutOnly) {
  // create component
  base::String component_id("21");
  int32_t css_id = 100;
  base::String entry_name("__Card__");
  base::String component_name("TestComp");
  base::String path("/index/components/TestComp");

  auto comp = manager->CreateFiberComponent(component_id, css_id, entry_name,
                                            component_name, path);
  comp->computed_css_style()->SetOverflowDefaultVisible(true);
  // component can be layout only by default.
  EXPECT_TRUE(comp->CanBeLayoutOnly());

  // create view
  auto fiber_element = manager->CreateFiberView();
  fiber_element->computed_css_style()->SetOverflowDefaultVisible(true);

  // view can be layout only by default.
  EXPECT_TRUE(fiber_element->CanBeLayoutOnly());

  // With enable_extended_layout_only_opt_, "text-align,direction" shall not
  // make the layout only optimization invalid
  fiber_element->SetStyleInternal(CSSPropertyID::kPropertyIDDirection,
                                  tasm::CSSValue::MakePlainString("lynx-rtl"));
  fiber_element->SetStyleInternal(CSSPropertyID::kPropertyIDTextAlign,
                                  tasm::CSSValue::MakePlainString("center"));
  // view can be layout only by default.
  EXPECT_TRUE(fiber_element->CanBeLayoutOnly());
  // Other style will make layout only false.
  fiber_element->SetStyleInternal(CSSPropertyID::kPropertyIDOpacity,
                                  tasm::CSSValue(0.2, CSSValuePattern::NUMBER));
  EXPECT_FALSE(fiber_element->CanBeLayoutOnly());
}

TEST_P(FiberElementTest, TestRemovePaintingNodeIsMoveFlag) {
  // Create the page element as root
  auto page = manager->CreateFiberPage("page", 11);

  // Create container node as child of page element
  auto container = manager->CreateFiberView();
  container->parent_component_element_ = page.get();
  page->InsertNode(container);
  // Ensure container is not layout only
  container->computed_css_style()->SetOverflowDefaultVisible(false);

  // Create leaf element as child of container
  auto leaf = manager->CreateFiberView();
  leaf->parent_component_element_ = page.get();
  container->InsertNode(leaf);
  // Ensure leaf element is not layout only
  leaf->computed_css_style()->SetOverflowDefaultVisible(false);

  // Flush actions to build the tree
  page->FlushActionsAsRoot();

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  // Verify that the leaf element is not layout only
  EXPECT_FALSE(leaf->is_layout_only_);

  // Create leaf element as child of container
  auto second_leaf = manager->CreateFiberView();
  second_leaf->parent_component_element_ = page.get();
  // Ensure leaf element is not layout only
  second_leaf->computed_css_style()->SetOverflowDefaultVisible(false);

  base::Vector<fml::RefPtr<FiberElement>> inserted_elements{};
  base::Vector<fml::RefPtr<FiberElement>> removed_elements{};
  inserted_elements.emplace_back(second_leaf);
  inserted_elements.emplace_back(leaf);
  removed_elements.emplace_back(leaf);
  container->ReplaceElements(inserted_elements, removed_elements, nullptr);

  // Flush actions to build the tree
  page->FlushActionsAsRoot();

  painting_context->Flush();

  EXPECT_TRUE(second_leaf->IsAttached());
  EXPECT_TRUE(leaf->IsAttached());
  EXPECT_FALSE(painting_context->HasCapturedRemoveSign(leaf->impl_id()));
}

TEST_P(FiberElementTest, FiberElementRemovedFromPassesZIndexStatus) {
  manager->GetLynxEnvConfig().font_scale_ = 1.3f;
  manager->GetLynxEnvConfig().font_scale_sp_only_ = false;

  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableZIndex(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberPage("page", 11);
  auto parent = manager->CreateFiberView();
  page->InsertNode(parent);

  auto child_with_z = manager->CreateFiberView();
  child_with_z->SetStyle(CSSPropertyID::kPropertyIDZIndex, lepus::Value(1));
  parent->InsertNode(child_with_z);

  auto child_no_z = manager->CreateFiberView();
  parent->InsertNode(child_no_z);

  page->FlushActionsAsRoot();

  auto mock_insertion_point = manager->CreateFiberView();
  child_with_z->RemovedFrom(mock_insertion_point.get());
  child_no_z->RemovedFrom(mock_insertion_point.get());

  SUCCEED();
}

TEST_P(FiberElementTest, PrepareAndGenerateChildrenActionsUsesHasZIndex) {
  manager->GetLynxEnvConfig().font_scale_ = 1.3f;
  manager->GetLynxEnvConfig().font_scale_sp_only_ = false;

  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableZIndex(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberPage("page", 11);
  auto parent = manager->CreateFiberView();
  page->InsertNode(parent);

  auto child_with_z = manager->CreateFiberView();
  child_with_z->SetStyle(CSSPropertyID::kPropertyIDZIndex, lepus::Value(1));
  parent->InsertNode(child_with_z);

  page->FlushActionsAsRoot();

  // Trigger kRemoveIntergenerationAct
  auto mock_insertion_point = manager->CreateFiberView();
  child_with_z->RemovedFrom(mock_insertion_point.get());

  // mock_insertion_point must be dirty to process actions
  mock_insertion_point->MarkDirty(FiberElement::kDirtyTree);
  mock_insertion_point->PrepareAndGenerateChildrenActions();

  SUCCEED();
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
