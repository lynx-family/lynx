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

TEST_P(FiberElementTest, TestRefType) {
  manager->GetLynxEnvConfig().font_scale_ = 1.3f;
  manager->GetLynxEnvConfig().font_scale_sp_only_ = false;

  auto fiber_element = manager->CreateFiberText("text");

  EXPECT_EQ(fiber_element->GetRefType(), lepus::RefType::kElement);

  auto table = lepus::Value(lepus::Dictionary::Create());
  EXPECT_EQ(table.Table()->GetRefType(), lepus::RefType::kLepusTable);

  auto ary = lepus::Value(lepus::CArray::Create());
  EXPECT_EQ(ary.Array()->GetRefType(), lepus::RefType::kLepusArray);

  auto byte_ary = lepus::ByteArray::Create();
  EXPECT_EQ(byte_ary->GetRefType(), lepus::RefType::kByteArray);

  auto prim_obj = lepus::LEPUSObject::Create();
  EXPECT_EQ(prim_obj->GetRefType(), lepus::RefType::kJSIObject);
}

TEST_P(FiberElementTest, TestSetOverflow) {
  manager->GetLynxEnvConfig().font_scale_ = 1.3f;
  manager->GetLynxEnvConfig().font_scale_sp_only_ = false;

  auto page = manager->CreateFiberPage("0", 0);

  auto impl = lepus::Value("visible");
  CSSPropertyID id = CSSPropertyID::kPropertyIDOverflow;
  CSSParserConfigs configs;
  auto map = UnitHandler::Process(id, impl, configs);
  page->computed_css_style()->SetValue(id, map[id]);
  EXPECT_TRUE(page->computed_css_style()->IsOverflowXY());

  impl = lepus::Value("hidden");
  id = CSSPropertyID::kPropertyIDOverflow;
  map = UnitHandler::Process(id, impl, configs);
  page->computed_css_style()->SetValue(id, map[id]);
  EXPECT_TRUE(page->computed_css_style()->IsOverflowHidden());

  impl = lepus::Value("visible");
  id = CSSPropertyID::kPropertyIDOverflowX;
  map = UnitHandler::Process(id, impl, configs);
  page->computed_css_style()->SetValue(id, map[id]);
  EXPECT_FALSE(page->computed_css_style()->IsOverflowXY());
  EXPECT_FALSE(page->computed_css_style()->IsOverflowHidden());

  impl = lepus::Value("visible");
  id = CSSPropertyID::kPropertyIDOverflowY;
  map = UnitHandler::Process(id, impl, configs);
  page->computed_css_style()->SetValue(id, map[id]);
  EXPECT_TRUE(page->computed_css_style()->IsOverflowXY());
  EXPECT_FALSE(page->computed_css_style()->IsOverflowHidden());

  impl = lepus::Value("hidden");
  id = CSSPropertyID::kPropertyIDOverflowX;
  map = UnitHandler::Process(id, impl, configs);
  page->computed_css_style()->SetValue(id, map[id]);
  EXPECT_FALSE(page->computed_css_style()->IsOverflowXY());
  EXPECT_FALSE(page->computed_css_style()->IsOverflowHidden());

  impl = lepus::Value("hidden");
  id = CSSPropertyID::kPropertyIDOverflowY;
  map = UnitHandler::Process(id, impl, configs);
  page->computed_css_style()->SetValue(id, map[id]);
  EXPECT_FALSE(page->computed_css_style()->IsOverflowXY());
  EXPECT_TRUE(page->computed_css_style()->IsOverflowHidden());

  impl = lepus::Value("visible");
  id = CSSPropertyID::kPropertyIDOverflow;
  map = UnitHandler::Process(id, impl, configs);
  page->computed_css_style()->SetValue(id, map[id]);
  EXPECT_TRUE(page->computed_css_style()->IsOverflowXY());
  EXPECT_TRUE(page->computed_css_style()->IsOverflowX());
  EXPECT_TRUE(page->computed_css_style()->IsOverflowY());
  EXPECT_FALSE(page->computed_css_style()->IsOverflowHidden());

  impl = lepus::Value("hidden");
  id = CSSPropertyID::kPropertyIDOverflow;
  map = UnitHandler::Process(id, impl, configs);
  page->computed_css_style()->SetValue(id, map[id]);
  EXPECT_FALSE(page->computed_css_style()->IsOverflowXY());
  EXPECT_TRUE(page->computed_css_style()->IsOverflowHidden());
  EXPECT_FALSE(page->computed_css_style()->IsOverflowX());
  EXPECT_FALSE(page->computed_css_style()->IsOverflowY());

  impl = lepus::Value("visible");
  id = CSSPropertyID::kPropertyIDOverflowX;
  map = UnitHandler::Process(id, impl, configs);
  page->computed_css_style()->SetValue(id, map[id]);
  EXPECT_FALSE(page->computed_css_style()->IsOverflowXY());
  EXPECT_FALSE(page->computed_css_style()->IsOverflowHidden());
  EXPECT_TRUE(page->computed_css_style()->IsOverflowX());
  EXPECT_FALSE(page->computed_css_style()->IsOverflowY());

  impl = lepus::Value("visible");
  id = CSSPropertyID::kPropertyIDOverflowY;
  map = UnitHandler::Process(id, impl, configs);
  page->computed_css_style()->SetValue(id, map[id]);
  EXPECT_TRUE(page->computed_css_style()->IsOverflowXY());
  EXPECT_FALSE(page->computed_css_style()->IsOverflowHidden());
  EXPECT_TRUE(page->computed_css_style()->IsOverflowX());
  EXPECT_TRUE(page->computed_css_style()->IsOverflowY());

  impl = lepus::Value("hidden");
  id = CSSPropertyID::kPropertyIDOverflowX;
  map = UnitHandler::Process(id, impl, configs);
  page->computed_css_style()->SetValue(id, map[id]);
  EXPECT_FALSE(page->computed_css_style()->IsOverflowXY());
  EXPECT_FALSE(page->computed_css_style()->IsOverflowHidden());
  EXPECT_FALSE(page->computed_css_style()->IsOverflowX());
  EXPECT_TRUE(page->computed_css_style()->IsOverflowY());
}

TEST_P(FiberElementTest, TestSetAttributeInternal00) {
  manager->GetLynxEnvConfig().font_scale_ = 1.3f;
  manager->GetLynxEnvConfig().font_scale_sp_only_ = false;

  auto page = manager->CreateFiberPage("0", 0);
  auto scroll = manager->CreateFiberScrollView("scroll-view");
  scroll->SetStyle(CSSPropertyID::kPropertyIDLinearOrientation,
                   lepus::Value("horizontal"));
  scroll->SetAttribute("scroll-y", lepus::Value("true"));

  page->InsertNode(scroll);
  page->FlushActionsAsRoot();

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* scroll_painting_node =
      painting_context->node_map_.at(scroll->impl_id()).get();
  auto scroll_props = scroll_painting_node->props_;

  EXPECT_FALSE(scroll_props.empty());
  EXPECT_EQ(tasm_mediator.captured_bundles_.back()->styles.back().first,
            CSSPropertyID::kPropertyIDLinearOrientation);
  EXPECT_EQ(tasm_mediator.captured_bundles_.back()->styles.back().second,
            tasm::CSSValue(1, CSSValuePattern::ENUM));
}

TEST_P(FiberElementTest, TestSetAttributeInternal01) {
  manager->GetLynxEnvConfig().font_scale_ = 1.3f;
  manager->GetLynxEnvConfig().font_scale_sp_only_ = false;

  auto page = manager->CreateFiberPage("0", 0);
  auto scroll = manager->CreateFiberScrollView("scroll-view");
  scroll->SetStyle(CSSPropertyID::kPropertyIDLinearOrientation,
                   lepus::Value("vertical"));
  scroll->SetAttribute("scroll-orientation", lepus::Value("horizontal"));

  page->InsertNode(scroll);
  page->FlushActionsAsRoot();

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* scroll_painting_node =
      painting_context->node_map_.at(scroll->impl_id()).get();
  auto scroll_props = scroll_painting_node->props_;

  EXPECT_FALSE(scroll_props.empty());
  EXPECT_EQ(tasm_mediator.captured_bundles_.back()->styles.back().first,
            CSSPropertyID::kPropertyIDLinearOrientation);
  EXPECT_EQ(tasm_mediator.captured_bundles_.back()->styles.back().second,
            tasm::CSSValue(0, CSSValuePattern::ENUM));
}

TEST_P(FiberElementTest, TestSetAttributeInternal02) {
  manager->GetLynxEnvConfig().font_scale_ = 1.3f;
  manager->GetLynxEnvConfig().font_scale_sp_only_ = false;

  auto page = manager->CreateFiberPage("0", 0);
  auto scroll = manager->CreateFiberScrollView("scroll-view");
  scroll->SetStyle(CSSPropertyID::kPropertyIDLinearOrientation,
                   lepus::Value("horizontal"));
  scroll->SetAttribute("scroll-orientation", lepus::Value("vertical"));

  page->InsertNode(scroll);
  page->FlushActionsAsRoot();

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* scroll_painting_node =
      painting_context->node_map_.at(scroll->impl_id()).get();
  auto scroll_props = scroll_painting_node->props_;

  EXPECT_FALSE(scroll_props.empty());
  EXPECT_EQ(tasm_mediator.captured_bundles_.back()->styles.back().first,
            CSSPropertyID::kPropertyIDLinearOrientation);
  EXPECT_EQ(tasm_mediator.captured_bundles_.back()->styles.back().second,
            tasm::CSSValue(1, CSSValuePattern::ENUM));
}

TEST_P(FiberElementTest, TestSetAttributeInternal03) {
  manager->GetLynxEnvConfig().font_scale_ = 1.3f;
  manager->GetLynxEnvConfig().font_scale_sp_only_ = false;

  auto page = manager->CreateFiberPage("0", 0);
  auto scroll = manager->CreateFiberScrollView("scroll-view");
  scroll->SetStyle(CSSPropertyID::kPropertyIDLinearOrientation,
                   lepus::Value("vertical"));
  scroll->SetAttribute("scroll-x", lepus::Value("true"));

  page->InsertNode(scroll);
  page->FlushActionsAsRoot();

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* scroll_painting_node =
      painting_context->node_map_.at(scroll->impl_id()).get();
  auto scroll_props = scroll_painting_node->props_;

  EXPECT_FALSE(scroll_props.empty());
  EXPECT_EQ(tasm_mediator.captured_bundles_.back()->styles.back().first,
            CSSPropertyID::kPropertyIDLinearOrientation);
  EXPECT_EQ(tasm_mediator.captured_bundles_.back()->styles.back().second,
            tasm::CSSValue(0, CSSValuePattern::ENUM));
}

TEST_P(FiberElementTest, TestSetAttributeInternal04) {
  manager->GetLynxEnvConfig().font_scale_ = 1.3f;
  manager->GetLynxEnvConfig().font_scale_sp_only_ = false;

  auto page = manager->CreateFiberPage("0", 0);
  auto scroll = manager->CreateFiberScrollView("scroll-view");
  scroll->SetStyle(CSSPropertyID::kPropertyIDLinearOrientation,
                   lepus::Value("vertical"));
  scroll->SetAttribute("scroll-x-reverse", lepus::Value("true"));

  page->InsertNode(scroll);
  page->FlushActionsAsRoot();

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* scroll_painting_node =
      painting_context->node_map_.at(scroll->impl_id()).get();
  auto scroll_props = scroll_painting_node->props_;

  EXPECT_FALSE(scroll_props.empty());
  EXPECT_EQ(tasm_mediator.captured_bundles_.back()->styles.back().first,
            CSSPropertyID::kPropertyIDLinearOrientation);
  EXPECT_EQ(tasm_mediator.captured_bundles_.back()->styles.back().second,
            tasm::CSSValue(2, CSSValuePattern::ENUM));
}

TEST_P(FiberElementTest, TestSetAttributeInternal05) {
  manager->GetLynxEnvConfig().font_scale_ = 1.3f;
  manager->GetLynxEnvConfig().font_scale_sp_only_ = false;

  auto page = manager->CreateFiberPage("0", 0);
  auto scroll = manager->CreateFiberScrollView("scroll-view");
  scroll->SetStyle(CSSPropertyID::kPropertyIDLinearOrientation,
                   lepus::Value("vertical"));
  scroll->SetAttribute("scroll-y-reverse", lepus::Value("true"));

  page->InsertNode(scroll);
  page->FlushActionsAsRoot();

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* scroll_painting_node =
      painting_context->node_map_.at(scroll->impl_id()).get();
  auto scroll_props = scroll_painting_node->props_;

  EXPECT_FALSE(scroll_props.empty());
  EXPECT_EQ(tasm_mediator.captured_bundles_.back()->styles.back().first,
            CSSPropertyID::kPropertyIDLinearOrientation);
  EXPECT_EQ(tasm_mediator.captured_bundles_.back()->styles.back().second,
            tasm::CSSValue(3, CSSValuePattern::ENUM));
}

TEST_P(FiberElementTest, ListItemTest) {
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
  comp->SetAttribute("full-span", lepus::Value(true));

  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), list->impl_id(), -1));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(list->impl_id(), comp->impl_id(), -1));

  page->FlushActionsAsRoot();

  EXPECT_FALSE(comp->is_layout_only_);
  EXPECT_TRUE(HasCaptureSignWithLayoutAttribute(
      list->impl_id(), starlight::LayoutAttribute::kColumnCount));
  EXPECT_TRUE(HasCaptureSignWithLayoutAttribute(
      list->impl_id(), starlight::LayoutAttribute::kScroll));
  EXPECT_TRUE(HasCaptureSignWithLayoutAttribute(
      comp->impl_id(), starlight::LayoutAttribute::kListCompType));
}

TEST_P(FiberElementTest, FiberElementConfig) {
  auto node = manager->CreateFiberNode("view");
  EXPECT_TRUE(node->config().IsObject());

  lepus::Value config = lepus::Value(lepus::Dictionary::Create());
  config.SetProperty("1", lepus::Value("1"));
  config.SetProperty("2", lepus::Value("2"));
  config.SetProperty("3", lepus::Value("3"));
  node->SetConfig(config);
  EXPECT_TRUE(node->config().IsObject());
  EXPECT_TRUE(node->config().GetProperty("1").IsString());
  EXPECT_TRUE(node->config().GetProperty("1").StringView() == "1");
  EXPECT_TRUE(node->config().GetProperty("2").IsString());
  EXPECT_TRUE(node->config().GetProperty("2").StringView() == "2");
  EXPECT_TRUE(node->config().GetProperty("3").IsString());
  EXPECT_TRUE(node->config().GetProperty("3").StringView() == "3");

  node->AddConfig("1", lepus::Value("0"));
  EXPECT_TRUE(node->config().GetProperty("1").IsString());
  EXPECT_TRUE(node->config().GetProperty("1").StringView() == "0");

  node->AddConfig("4", lepus::Value("4"));
  EXPECT_TRUE(node->config().GetProperty("4").IsString());
  EXPECT_TRUE(node->config().GetProperty("4").StringView() == "4");
}

TEST_P(FiberElementTest, TestComponentElement) {
  base::String component_id("21");
  int32_t css_id = 100;
  base::String entry_name("TTTT");
  base::String component_name("TestComp");
  base::String path("/index/components/TestComp");

  auto comp = manager->CreateFiberComponent(component_id, css_id, entry_name,
                                            component_name, path);

  EXPECT_TRUE(comp->GetData().IsEmpty());
  EXPECT_TRUE(comp->GetProperties().IsEmpty());
  EXPECT_FALSE(comp->IsPageForBaseComponent());
  EXPECT_EQ(comp->GetEntryName(), "TTTT");
  EXPECT_EQ(comp->ComponentStrId(), "21");
}

TEST_P(FiberElementTest, TestAttachedState) {
  auto parent = manager->CreateFiberNode("view");
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 0);
  EXPECT_TRUE(parent->IsDetached());

  auto element = manager->CreateFiberNode("view");
  element->SetStyleInternal(CSSPropertyID::kPropertyIDOverflow,
                            tasm::CSSValue::MakePlainString("visible"));
  parent->InsertNode(element);
  EXPECT_TRUE(parent->IsDetached());
  EXPECT_TRUE(element->IsDetached());

  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 1);

  EXPECT_EQ(parent->GetChildAt(0), element.get());

  auto page = manager->CreateFiberPage("page", 11);
  page->InsertNode(parent);

  EXPECT_TRUE(page->IsAttached());
  EXPECT_TRUE(parent->IsAttached());
  EXPECT_TRUE(element->IsAttached());

  page->RemoveNode(parent);
  EXPECT_TRUE(page->IsAttached());
  EXPECT_TRUE(parent->IsDetached());
  EXPECT_TRUE(element->IsDetached());
}

TEST_P(FiberElementTest, SetStyle) {
  auto page = manager->CreateFiberPage("page", 11);
  manager->SetFiberPageElement(page);
  EXPECT_TRUE(page.get() == manager->GetPageElement());
  auto element = manager->CreateFiberView();
  element->SetStyle(CSSPropertyID::kPropertyIDOverflow,
                    lepus::Value("visible"));
  auto raw_style_value = element->current_raw_inline_styles_->at(
      CSSPropertyID::kPropertyIDOverflow);

  page->InsertNode(element);
  EXPECT_TRUE(raw_style_value == lepus::Value("visible"));
  page->FlushActionsAsRoot();
  auto stored_raw_style_value = element->current_raw_inline_styles_->at(
      CSSPropertyID::kPropertyIDOverflow);
  EXPECT_TRUE(stored_raw_style_value == lepus::Value("visible"));
  auto parsed_style_value =
      element->parsed_styles_map_.at(CSSPropertyID::kPropertyIDOverflow);

  EXPECT_TRUE(page->IsPageForBaseComponent());
  EXPECT_TRUE(parsed_style_value.IsEnum());
  EXPECT_TRUE(
      static_cast<starlight::OverflowType>(parsed_style_value.GetNumber()) ==
      starlight::OverflowType::kVisible);
}

TEST_P(FiberElementTest, DestroyPlatformNode) {
  // FiberElement is referenced only by UI.
  auto parent = manager->CreateFiberNode("view");
  auto element = manager->CreateFiberNode("view");
  parent->FlushProps();
  element->FlushProps();
  parent->InsertNode(element);
  // parent destory
  parent->DestroyPlatformNode();
  // make a weak_ptr to monitor FiberElement.
  FiberElement* child = element.get();
  std::shared_ptr<FiberElement*> sp = std::make_shared<FiberElement*>(child);
  std::weak_ptr<FiberElement*> wp = sp;
  element = nullptr;  // clear local ref.
  sp = nullptr;
  EXPECT_FALSE(parent->HasPaintingNode());
  EXPECT_FALSE(wp.lock());

  // FIberElement is referenced by UI and JS.
  auto parent_new = manager->CreateFiberNode("view");
  auto element_new = manager->CreateFiberNode("view");
  parent_new->FlushProps();
  element_new->FlushProps();
  // mock JS ref
  fml::RefPtr<FiberElement> ref = fml::RefPtr<FiberElement>(element_new.get());
  parent_new->InsertNode(element_new);
  // parent destory
  parent_new->DestroyPlatformNode();
  // make a weak_ptr to monitor FiberElement.
  FiberElement* child_new = element_new.get();
  std::shared_ptr<FiberElement*> sp_new =
      std::make_shared<FiberElement*>(child_new);
  std::weak_ptr<FiberElement*> wp_new = sp_new;
  element_new = nullptr;  // clear local ref.
  EXPECT_FALSE(parent_new->HasPaintingNode());
  EXPECT_EQ(wp_new.use_count(), 1);
}

TEST_P(FiberElementTest, TestSetParentComponentUniqueIdAgain) {
  auto page = manager->CreateFiberPage("page", 11);
  page->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(page->impl_id()));
  auto child = manager->CreateFiberNode("view");
  child->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(page->impl_id()));
  child->ResolveParentComponentElement();
  EXPECT_TRUE(child->parent_component_element_ == page.get());
  child->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(page->impl_id()));
  EXPECT_TRUE(child->parent_component_element_ == page.get());
  child->SetParentComponentUniqueIdForFiber(100);
  // after SetParentComponentUniqueIdForFiber again, parent_component_element_
  // should be resolved again.
  EXPECT_TRUE(child->parent_component_element_ == nullptr);
}

TEST_P(FiberElementTest, TestSetSameComponentId) {
  base::String component_id("21");
  int32_t css_id = 100;
  base::String entry_name("__Card__");
  base::String component_name("TestComp");
  base::String path("/index/components/TestComp");

  auto comp = manager->CreateFiberComponent(component_id, css_id, entry_name,
                                            component_name, path);
  EXPECT_TRUE(comp->updated_attr_map_.count(BASE_STATIC_STRING(kComponentID)));
  comp->updated_attr_map_.clear();
  EXPECT_TRUE(comp->updated_attr_map_.empty());
  comp->set_component_id("1");
  EXPECT_TRUE(comp->updated_attr_map_.count(BASE_STATIC_STRING(kComponentID)));
  comp->updated_attr_map_.clear();
  EXPECT_TRUE(comp->updated_attr_map_.empty());
  comp->set_component_id("1");
  // Set Same ComponentId will not trigger update
  EXPECT_TRUE(comp->updated_attr_map_.empty());
}

TEST_P(FiberElementTest, FiberElementSetAndResetAttribute) {
  auto page = manager->CreateFiberPage("page", 11);

  auto element0 = manager->CreateFiberNode("view");
  element0->SetAttribute("enable-layout", lepus::Value("false"));
  element0->SetAttribute("flatten", lepus::Value("false"));
  element0->SetIdSelector("#element0");

  page->InsertNode(element0);

  auto& attributes = element0->data_model_->attributes();

  EXPECT_TRUE(attributes.at("enable-layout") == lepus::Value("false"));
  EXPECT_TRUE(attributes.at("flatten") == lepus::Value("false"));
  EXPECT_TRUE(attributes.at(AttributeHolder::kIdSelectorAttrName) ==
              lepus::Value("#element0"));

  page->FlushActionsAsRoot();
  ASSERT_TRUE(page->prop_bundle_ == nullptr);
  ASSERT_TRUE(element0->prop_bundle_ == nullptr);

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* painting_node =
      painting_context->node_map_.at(element0->impl_id()).get();

  std::string key0 = "enable-layout";
  auto value0 = painting_node->props_.at(key0);
  EXPECT_TRUE(value0 == lepus::Value("false"));

  std::string key1 = "flatten";
  auto value1 = painting_node->props_.at(key1);
  EXPECT_TRUE(value1 == lepus::Value("false"));

  std::string key2(AttributeHolder::kIdSelectorAttrName);
  auto value2 = painting_node->props_.at(key2);
  EXPECT_TRUE(value2 == lepus::Value("#element0"));

  element0->SetAttribute("enable-layout", lepus::Value());
  const auto& it = attributes.find("enable-layout");
  EXPECT_TRUE(it == attributes.end());

  page->FlushActionsAsRoot();
  painting_context->Flush();
  ASSERT_TRUE(page->prop_bundle_ == nullptr);
  ASSERT_TRUE(element0->prop_bundle_ == nullptr);

  value0 = painting_node->props_.at(key0);
  EXPECT_TRUE(value0.IsEmpty());

  value1 = painting_node->props_.at(key1);
  EXPECT_TRUE(value1 == lepus::Value("false"));

  value2 = painting_node->props_.at(key2);
  EXPECT_TRUE(value2 == lepus::Value("#element0"));
}

TEST_P(FiberElementTest, FiberElementInheritCase00) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableCSSInheritance(true);
  manager->SetConfig(config);

  // css related
  StyleMap indexAttributes;

  CSSParserTokenMap indexTokensMap;
  CSSParserConfigs configs;
  // class .root
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDColor;
    auto impl = lepus::Value("blue");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    id = CSSPropertyID::kPropertyIDFontSize;
    impl = lepus::Value("2em");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".root";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  // class .ani
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDColor;
    auto impl = lepus::Value("red");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    id = CSSPropertyID::kPropertyIDLineHeight;
    impl = lepus::Value("2");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".ani";
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

  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  auto element0 = manager->CreateFiberNode("view");
  element0->parent_component_element_ = page.get();
  element0->SetAttribute("enable-layout", lepus::Value("false"));
  element0->SetClass("root");
  page->InsertNode(element0);

  auto element00 = manager->CreateFiberNode("view");
  element00->parent_component_element_ = page.get();
  element00->SetAttribute("enable-layout", lepus::Value("false"));
  element00->SetClass("ani");
  element0->InsertNode(element00);

  auto text = manager->CreateFiberText("text");
  text->parent_component_element_ = page.get();
  element00->InsertNode(text);

  page->FlushActionsAsRoot();

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* element0_painting_node =
      painting_context->node_map_.at(element0->impl_id()).get();
  auto element0_props = element0_painting_node->props_;

  auto* element00_painting_node =
      painting_context->node_map_.at(element00->impl_id()).get();
  auto element00_props = element00_painting_node->props_;

  auto* text_painting_node =
      painting_context->node_map_.at(text->impl_id()).get();

  auto text_props = text_painting_node->props_;

  EXPECT_TRUE(text_props.count("color"));
  EXPECT_TRUE(element0_props.at("color") != text_props.at("color"));
  EXPECT_TRUE(element00_props.at("color") == text_props.at("color"));
  EXPECT_EQ(element0_props.at("font-size"), lepus::Value(28.0));
  EXPECT_EQ(element0_props.at("font-size"), lepus::Value(28.0));
  EXPECT_EQ(element00_props.at("font-size"), lepus::Value(28.0));
  EXPECT_EQ(element00_props.at("line-height"), lepus::Value(56.0));
  EXPECT_EQ(text_props.at("font-size"), lepus::Value(28.0));
  EXPECT_EQ(text_props.at("line-height"), lepus::Value(56.0));

  const auto& page_styles = page->GetStylesForWorklet();
  EXPECT_TRUE(page_styles.empty());

  const auto& element0_styles = element0->GetStylesForWorklet();
  EXPECT_EQ(element0_styles.size(), 2);

  const auto& element00_styles = element00->GetStylesForWorklet();
  EXPECT_EQ(element00_styles.size(), 3);

  const auto& text_styles = text->GetStylesForWorklet();
  EXPECT_EQ(text_styles.size(), 3);

  // remove inherit styles
  element00->RemoveAllClass();
  page->FlushActionsAsRoot();
  painting_context->Flush();

  element0_props = element0_painting_node->props_;
  text_props = text_painting_node->props_;
  element00_props = element00_painting_node->props_;

  EXPECT_TRUE(text_props.at("color") == element0_props.at("color"));
  EXPECT_TRUE(element00_props.at("color") == element0_props.at("color"));
  EXPECT_EQ(element0_props.at("font-size"), lepus::Value(28.0));
  EXPECT_EQ(element0_props.at("font-size"), lepus::Value(28.0));
  EXPECT_EQ(element00_props.at("font-size"), lepus::Value(28.0));
  EXPECT_EQ(element00_props.at("line-height"), lepus::Value());
  EXPECT_EQ(text_props.at("font-size"), lepus::Value(28.0));
  EXPECT_EQ(text_props.at("line-height"), lepus::Value());
}

TEST_P(FiberElementTest, FiberElementInheritCase01) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableCSSInheritance(true);
  std::unordered_set<CSSPropertyID> list = {kPropertyIDColor,
                                            kPropertyIDFontSize};
  config->SetCustomCSSInheritList(std::move(list));
  manager->SetConfig(config);

  // css related
  StyleMap indexAttributes;

  CSSParserTokenMap indexTokensMap;
  CSSParserConfigs configs;
  // class .root
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDColor;
    auto impl = lepus::Value("blue");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".root";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  // class .ani
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDColor;
    auto impl = lepus::Value("red");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".ani";
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

  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  auto element0 = manager->CreateFiberNode("view");
  element0->parent_component_element_ = page.get();
  element0->SetAttribute("enable-layout", lepus::Value("false"));
  element0->SetClass("root");
  page->InsertNode(element0);

  auto element00 = manager->CreateFiberNode("view");
  element00->parent_component_element_ = page.get();
  element00->SetAttribute("enable-layout", lepus::Value("false"));
  element00->SetClass("ani");
  element0->InsertNode(element00);

  auto text = manager->CreateFiberText("text");
  text->parent_component_element_ = page.get();
  element00->InsertNode(text);

  page->FlushActionsAsRoot();

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* element0_painting_node =
      painting_context->node_map_.at(element0->impl_id()).get();
  auto element0_props = element0_painting_node->props_;

  auto* element00_painting_node =
      painting_context->node_map_.at(element00->impl_id()).get();
  auto element00_props = element00_painting_node->props_;

  auto* text_painting_node =
      painting_context->node_map_.at(text->impl_id()).get();

  auto text_props = text_painting_node->props_;

  EXPECT_TRUE(text_props.count("color"));
  EXPECT_TRUE(element0_props.at("color") != text_props.at("color"));
  EXPECT_TRUE(element00_props.at("color") == text_props.at("color"));

  // remove inherit styles
  element00->RemoveAllClass();
  page->FlushActionsAsRoot();
  painting_context->Flush();

  element0_props = element0_painting_node->props_;
  text_props = text_painting_node->props_;
  element00_props = element00_painting_node->props_;

  EXPECT_TRUE(text_props.at("color") == element0_props.at("color"));
  EXPECT_TRUE(element00_props.at("color") == element0_props.at("color"));
}

TEST_P(FiberElementTest, FiberElementInheritCase02) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableCSSInheritance(true);
  std::unordered_set<CSSPropertyID> list = {kPropertyIDColor,
                                            kPropertyIDFontSize};
  config->SetCustomCSSInheritList(std::move(list));
  manager->SetConfig(config);

  // css related
  StyleMap indexAttributes;

  CSSParserTokenMap indexTokensMap;
  CSSParserConfigs configs;

  // class .ani
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDColor;
    auto impl = lepus::Value("red");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".ani";
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

  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  auto element0 = manager->CreateFiberNode("view");
  element0->parent_component_element_ = page.get();
  element0->SetAttribute("enable-layout", lepus::Value("false"));
  element0->SetClass("ani");
  page->InsertNode(element0);

  auto element00 = manager->CreateFiberNode("view");
  element00->parent_component_element_ = page.get();
  element00->SetAttribute("enable-layout", lepus::Value("false"));
  element0->InsertNode(element00);

  auto text = manager->CreateFiberText("text");
  text->parent_component_element_ = page.get();
  element00->InsertNode(text);

  page->FlushActionsAsRoot();

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* element0_painting_node =
      painting_context->node_map_.at(element0->impl_id()).get();
  auto element0_props = element0_painting_node->props_;

  auto* element00_painting_node =
      painting_context->node_map_.at(element00->impl_id()).get();
  auto element00_props = element00_painting_node->props_;

  auto* text_painting_node =
      painting_context->node_map_.at(text->impl_id()).get();

  auto text_props = text_painting_node->props_;

  EXPECT_TRUE(text_props.count("color"));
  EXPECT_TRUE(element0_props.at("color") == text_props.at("color"));
  EXPECT_TRUE(element00_props.at("color") == text_props.at("color"));

  // remove inherit styles
  element0->RemoveAllClass();
  page->FlushActionsAsRoot();
  painting_context->Flush();

  element0_props = element0_painting_node->props_;
  text_props = text_painting_node->props_;

  EXPECT_TRUE(text_props.at("color") == lepus::Value());
}

TEST_P(FiberElementTest, FiberElementInheritCase03) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableCSSInheritance(true);
  std::unordered_set<CSSPropertyID> list = {kPropertyIDColor,
                                            kPropertyIDFontSize};
  config->SetCustomCSSInheritList(std::move(list));
  manager->SetConfig(config);

  // css related
  StyleMap indexAttributes;

  CSSParserTokenMap indexTokensMap;
  CSSParserConfigs configs;

  // class .ani
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDColor;
    auto impl = lepus::Value("red");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".ani";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  // class .ani2
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDColor;
    auto impl = lepus::Value("yellow");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".ani2";
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

  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  auto element0 = manager->CreateFiberNode("view");
  element0->parent_component_element_ = page.get();
  element0->SetAttribute("enable-layout", lepus::Value("false"));
  element0->SetClass("ani");
  page->InsertNode(element0);

  auto element00 = manager->CreateFiberNode("view");
  element00->parent_component_element_ = page.get();
  element00->SetAttribute("enable-layout", lepus::Value("false"));
  element0->InsertNode(element00);

  auto text = manager->CreateFiberText("text");
  text->parent_component_element_ = page.get();
  element00->InsertNode(text);

  page->FlushActionsAsRoot();

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* element0_painting_node =
      painting_context->node_map_.at(element0->impl_id()).get();
  auto element0_props = element0_painting_node->props_;

  auto* element00_painting_node =
      painting_context->node_map_.at(element00->impl_id()).get();
  auto element00_props = element00_painting_node->props_;

  auto* text_painting_node =
      painting_context->node_map_.at(text->impl_id()).get();

  auto text_props = text_painting_node->props_;

  auto color_value = text_props.at("color");
  EXPECT_TRUE(text_props.count("color"));
  EXPECT_TRUE(element0_props.at("color") == text_props.at("color"));
  EXPECT_TRUE(element00_props.at("color") == text_props.at("color"));

  // remove inherit styles
  element0->RemoveAllClass();
  element0->SetClass("ani2");
  page->FlushActionsAsRoot();
  painting_context->Flush();

  element0_props = element0_painting_node->props_;
  text_props = text_painting_node->props_;

  EXPECT_TRUE(element0_props.at("color") == text_props.at("color"));
  EXPECT_TRUE(color_value != text_props.at("color"));
}

TEST_P(FiberElementTest, FiberElementInheritCase04) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableCSSInheritance(true);
  std::unordered_set<CSSPropertyID> list = {kPropertyIDDirection};
  config->SetCustomCSSInheritList(std::move(list));
  manager->SetConfig(config);

  // css related
  StyleMap indexAttributes;

  CSSParserTokenMap indexTokensMap;
  CSSParserConfigs configs;

  // class .ani
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDDirection;
    auto impl = lepus::Value("lynx-rtl");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".ani";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  // class .ani2
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDDirection;
    auto impl = lepus::Value("rtl");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".ani2";
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

  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  auto element0 = manager->CreateFiberNode("view");
  element0->parent_component_element_ = page.get();
  element0->SetAttribute("enable-layout", lepus::Value("false"));
  element0->SetClass("ani");
  page->InsertNode(element0);

  auto wrapper = manager->CreateFiberWrapperElement();
  element0->InsertNode(wrapper);

  auto element00 = manager->CreateFiberNode("view");
  element00->parent_component_element_ = page.get();
  element00->SetAttribute("enable-layout", lepus::Value("false"));
  wrapper->InsertNode(element00);

  page->FlushActionsAsRoot();

  EXPECT_TRUE(
      element00->computed_css_style()->GetLayoutComputedStyle()->direction_ ==
      starlight::DirectionType::kLynxRtl);

  // insert new wrapper
  auto wrapper2 = manager->CreateFiberWrapperElement();
  element00->InsertNode(wrapper2);

  auto element000 = manager->CreateFiberNode("view");
  element000->parent_component_element_ = page.get();
  element000->SetAttribute("enable-layout", lepus::Value("false"));
  wrapper2->InsertNode(element000);

  page->FlushActionsAsRoot();
  EXPECT_TRUE(
      element000->computed_css_style()->GetLayoutComputedStyle()->direction_ ==
      starlight::DirectionType::kLynxRtl);

  // direction change
  element0->RemoveAllClass();
  page->FlushActionsAsRoot();
  EXPECT_TRUE(
      element00->computed_css_style()->GetLayoutComputedStyle()->direction_ ==
      starlight::DirectionType::kNormal);
  EXPECT_TRUE(
      element000->computed_css_style()->GetLayoutComputedStyle()->direction_ ==
      starlight::DirectionType::kNormal);

  element0->SetClass("ani2");
  page->FlushActionsAsRoot();
  EXPECT_TRUE(
      element00->computed_css_style()->GetLayoutComputedStyle()->direction_ ==
      starlight::DirectionType::kRtl);
  EXPECT_TRUE(
      element000->computed_css_style()->GetLayoutComputedStyle()->direction_ ==
      starlight::DirectionType::kRtl);
}

TEST_P(FiberElementTest, FiberElementInheritCase05) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableCSSInheritance(true);
  std::unordered_set<CSSPropertyID> list = {kPropertyIDDirection};
  config->SetCustomCSSInheritList(std::move(list));
  manager->SetConfig(config);

  // css related
  StyleMap indexAttributes;

  CSSParserTokenMap indexTokensMap;
  CSSParserConfigs configs;

  // class .ani
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDDirection;
    auto impl = lepus::Value("lynx-rtl");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".ani";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  // class .ani2
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDDirection;
    auto impl = lepus::Value("rtl");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".ani2";
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

  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  auto element0 = manager->CreateFiberNode("view");
  element0->parent_component_element_ = page.get();
  element0->SetAttribute("enable-layout", lepus::Value("false"));
  element0->SetClass("ani");
  page->InsertNode(element0);

  auto wrapper = manager->CreateFiberWrapperElement();
  element0->InsertNode(wrapper);

  auto element00 = manager->CreateFiberNode("view");
  element00->parent_component_element_ = page.get();
  element00->SetAttribute("enable-layout", lepus::Value("false"));
  wrapper->InsertNode(element00);

  page->FlushActionsAsRoot();

  EXPECT_TRUE(
      element00->computed_css_style()->GetLayoutComputedStyle()->direction_ ==
      starlight::DirectionType::kLynxRtl);

  // insert new wrapper
  auto wrapper2 = manager->CreateFiberWrapperElement();
  element00->InsertNode(wrapper2);

  auto element000 = manager->CreateFiberNode("view");
  element000->parent_component_element_ = page.get();
  element000->SetAttribute("enable-layout", lepus::Value("false"));
  wrapper2->InsertNode(element000);

  page->FlushActionsAsRoot();
  EXPECT_TRUE(
      element000->computed_css_style()->GetLayoutComputedStyle()->direction_ ==
      starlight::DirectionType::kLynxRtl);

  // direction change
  element0->RemoveAllClass();
  page->FlushActionsAsRoot();
  EXPECT_TRUE(
      element00->computed_css_style()->GetLayoutComputedStyle()->direction_ ==
      starlight::DirectionType::kNormal);
  EXPECT_TRUE(
      element000->computed_css_style()->GetLayoutComputedStyle()->direction_ ==
      starlight::DirectionType::kNormal);

  element0->SetClass("ani2");
  page->FlushActionsAsRoot();
  EXPECT_TRUE(
      element00->computed_css_style()->GetLayoutComputedStyle()->direction_ ==
      starlight::DirectionType::kRtl);
  EXPECT_TRUE(
      element000->computed_css_style()->GetLayoutComputedStyle()->direction_ ==
      starlight::DirectionType::kRtl);

  EXPECT_FALSE(element00->children_propagate_inherited_styles_flag_);
  element00->SetStyle(CSSPropertyID::kPropertyIDBackground,
                      lepus::Value("green"));
  element00->FlushSelf();
  EXPECT_FALSE(element00->children_propagate_inherited_styles_flag_);
}

TEST_P(FiberElementTest, FiberElementDirectionCase) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableCSSInheritance(true);
  std::unordered_set<CSSPropertyID> list = {kPropertyIDDirection};
  config->SetCustomCSSInheritList(std::move(list));
  manager->SetConfig(config);

  // css related
  StyleMap indexAttributes;

  CSSParserTokenMap indexTokensMap;
  CSSParserConfigs configs;

  // class .title
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDTextAlign;
    auto impl = lepus::Value("center");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".title";
    auto& sheets = tokens->sheets();
    auto shared_css_sheets = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheets);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  // class .root
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDDirection;
    auto impl = lepus::Value("ltr");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".root";
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

  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  auto root = manager->CreateFiberView();
  root->parent_component_element_ = page.get();
  root->SetClass("root");
  page->InsertNode(root);

  auto text_element0 = manager->CreateFiberText("text");
  text_element0->parent_component_element_ = page.get();
  text_element0->SetClass("title");
  text_element0->SetAttribute("text", lepus::Value("title"));
  root->InsertNode(text_element0);

  page->FlushActionsAsRoot();

  EXPECT_TRUE(text_element0->computed_css_style()
                  ->GetLayoutComputedStyle()
                  ->direction_ == starlight::DirectionType::kLtr);
  EXPECT_TRUE(
      text_element0->computed_css_style()->GetTextAttributes()->text_align ==
      starlight::TextAlignType::kCenter);

  text_element0->RemoveAllClass();
  page->FlushActionsAsRoot();
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->GetLayoutComputedStyle()
                  ->direction_ == starlight::DirectionType::kLtr);
  EXPECT_TRUE(
      text_element0->computed_css_style()->GetTextAttributes()->text_align ==
      starlight::TextAlignType::kLeft);
}

TEST_P(FiberElementTest, FiberElementDirectionCase01) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableCSSInheritance(true);
  std::unordered_set<CSSPropertyID> list = {kPropertyIDDirection};
  config->SetCustomCSSInheritList(std::move(list));
  manager->SetConfig(config);

  // css related
  StyleMap indexAttributes;

  CSSParserTokenMap indexTokensMap;
  CSSParserConfigs configs;

  // class .title
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDTextAlign;
    auto impl = lepus::Value("center");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".title";
    auto& sheets = tokens->sheets();
    auto shared_css_sheets = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheets);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  // class .root-ltr
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDDirection;
    auto impl = lepus::Value("ltr");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".root-ltr";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  // class.root-rtl
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDDirection;
    auto impl = lepus::Value("rtl");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".root-rtl";
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

  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  auto root = manager->CreateFiberView();
  root->parent_component_element_ = page.get();
  root->SetClass("root-ltr");
  page->InsertNode(root);

  auto text_element0 = manager->CreateFiberText("text");
  text_element0->parent_component_element_ = page.get();
  text_element0->SetClass("title");
  text_element0->SetAttribute("text", lepus::Value("title"));
  root->InsertNode(text_element0);

  page->FlushActionsAsRoot();

  EXPECT_TRUE(text_element0->computed_css_style()
                  ->GetLayoutComputedStyle()
                  ->direction_ == starlight::DirectionType::kLtr);
  EXPECT_TRUE(
      text_element0->computed_css_style()->GetTextAttributes()->text_align ==
      starlight::TextAlignType::kCenter);

  text_element0->RemoveAllClass();
  page->FlushActionsAsRoot();
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->GetLayoutComputedStyle()
                  ->direction_ == starlight::DirectionType::kLtr);
  EXPECT_TRUE(
      text_element0->computed_css_style()->GetTextAttributes()->text_align ==
      starlight::TextAlignType::kLeft);

  root->SetClass("root-rtl");
  page->FlushActionsAsRoot();
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->GetLayoutComputedStyle()
                  ->direction_ == starlight::DirectionType::kRtl);
  EXPECT_TRUE(
      text_element0->computed_css_style()->GetTextAttributes()->text_align ==
      starlight::TextAlignType::kRight);
}

TEST_P(FiberElementTest, FiberElementDirectionCase_logicalCSSProperty) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableCSSInheritance(true);
  std::unordered_set<CSSPropertyID> list = {kPropertyIDDirection};
  config->SetCustomCSSInheritList(std::move(list));
  manager->SetConfig(config);

  // css related
  StyleMap indexAttributes;

  CSSParserTokenMap indexTokensMap;
  CSSParserConfigs configs;

  // class .title
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    tokens.get()->raw_attributes_[CSSPropertyID::kPropertyIDTextAlign] =
        CSSValue::MakePlainString("center");
    tokens.get()
        ->raw_attributes_[CSSPropertyID::kPropertyIDBorderEndEndRadius] =
        CSSValue::MakePlainString("30px");
    tokens.get()
        ->raw_attributes_[CSSPropertyID::kPropertyIDBorderStartEndRadius] =
        CSSValue::MakePlainString("20px");
    tokens.get()
        ->raw_attributes_[CSSPropertyID::kPropertyIDBorderEndStartRadius] =
        CSSValue::MakePlainString("10px");
    tokens.get()
        ->raw_attributes_[CSSPropertyID::kPropertyIDBorderStartStartRadius] =
        CSSValue::MakePlainString("5px");

    std::string key = ".title";
    auto& sheets = tokens->sheets();
    auto shared_css_sheets = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheets);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  // class .root-ltr
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDDirection;
    auto impl = lepus::Value("ltr");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".root-ltr";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  // class.root-rtl
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDDirection;
    auto impl = lepus::Value("rtl");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".root-rtl";
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

  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  auto root = manager->CreateFiberView();
  root->parent_component_element_ = page.get();
  root->SetClass("root-rtl");
  page->InsertNode(root);

  auto text_element0 = manager->CreateFiberText("text");
  text_element0->parent_component_element_ = page.get();
  text_element0->SetClass("title");
  text_element0->SetAttribute("text", lepus::Value("title"));
  root->InsertNode(text_element0);

  page->FlushActionsAsRoot();
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->GetLayoutComputedStyle()
                  ->direction_ == starlight::DirectionType::kRtl);
  EXPECT_TRUE(
      text_element0->computed_css_style()->GetTextAttributes()->text_align ==
      starlight::TextAlignType::kCenter);
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->layout_computed_style_.surround_data_.border_data_
                  ->radius_x_bottom_left ==
              starlight::NLength::MakeUnitNLength(30));
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->layout_computed_style_.surround_data_.border_data_
                  ->radius_y_bottom_left ==
              starlight::NLength::MakeUnitNLength(30));
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->layout_computed_style_.surround_data_.border_data_
                  ->radius_x_top_left ==
              starlight::NLength::MakeUnitNLength(20));
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->layout_computed_style_.surround_data_.border_data_
                  ->radius_y_top_left ==
              starlight::NLength::MakeUnitNLength(20));
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->layout_computed_style_.surround_data_.border_data_
                  ->radius_x_bottom_right ==
              starlight::NLength::MakeUnitNLength(10));
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->layout_computed_style_.surround_data_.border_data_
                  ->radius_y_bottom_right ==
              starlight::NLength::MakeUnitNLength(10));
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->layout_computed_style_.surround_data_.border_data_
                  ->radius_x_top_right ==
              starlight::NLength::MakeUnitNLength(5));
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->layout_computed_style_.surround_data_.border_data_
                  ->radius_y_top_right ==
              starlight::NLength::MakeUnitNLength(5));

  root->SetClass("root-ltr");
  page->FlushActionsAsRoot();

  EXPECT_TRUE(text_element0->computed_css_style()
                  ->GetLayoutComputedStyle()
                  ->direction_ == starlight::DirectionType::kLtr);
  EXPECT_TRUE(
      text_element0->computed_css_style()->GetTextAttributes()->text_align ==
      starlight::TextAlignType::kCenter);
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->layout_computed_style_.surround_data_.border_data_
                  ->radius_x_bottom_right ==
              starlight::NLength::MakeUnitNLength(30));
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->layout_computed_style_.surround_data_.border_data_
                  ->radius_y_bottom_right ==
              starlight::NLength::MakeUnitNLength(30));
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->layout_computed_style_.surround_data_.border_data_
                  ->radius_x_top_right ==
              starlight::NLength::MakeUnitNLength(20));
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->layout_computed_style_.surround_data_.border_data_
                  ->radius_y_top_right ==
              starlight::NLength::MakeUnitNLength(20));
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->layout_computed_style_.surround_data_.border_data_
                  ->radius_x_bottom_left ==
              starlight::NLength::MakeUnitNLength(10));
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->layout_computed_style_.surround_data_.border_data_
                  ->radius_y_bottom_left ==
              starlight::NLength::MakeUnitNLength(10));
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->layout_computed_style_.surround_data_.border_data_
                  ->radius_x_top_left ==
              starlight::NLength::MakeUnitNLength(5));
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->layout_computed_style_.surround_data_.border_data_
                  ->radius_y_top_left ==
              starlight::NLength::MakeUnitNLength(5));

  root->SetClass("root-rtl");
  page->FlushActionsAsRoot();

  EXPECT_TRUE(text_element0->computed_css_style()
                  ->GetLayoutComputedStyle()
                  ->direction_ == starlight::DirectionType::kRtl);
  EXPECT_TRUE(
      text_element0->computed_css_style()->GetTextAttributes()->text_align ==
      starlight::TextAlignType::kCenter);
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->layout_computed_style_.surround_data_.border_data_
                  ->radius_x_bottom_left ==
              starlight::NLength::MakeUnitNLength(30));
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->layout_computed_style_.surround_data_.border_data_
                  ->radius_y_bottom_left ==
              starlight::NLength::MakeUnitNLength(30));
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->layout_computed_style_.surround_data_.border_data_
                  ->radius_x_top_left ==
              starlight::NLength::MakeUnitNLength(20));
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->layout_computed_style_.surround_data_.border_data_
                  ->radius_y_top_left ==
              starlight::NLength::MakeUnitNLength(20));
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->layout_computed_style_.surround_data_.border_data_
                  ->radius_x_bottom_right ==
              starlight::NLength::MakeUnitNLength(10));
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->layout_computed_style_.surround_data_.border_data_
                  ->radius_y_bottom_right ==
              starlight::NLength::MakeUnitNLength(10));
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->layout_computed_style_.surround_data_.border_data_
                  ->radius_x_top_right ==
              starlight::NLength::MakeUnitNLength(5));
  EXPECT_TRUE(text_element0->computed_css_style()
                  ->layout_computed_style_.surround_data_.border_data_
                  ->radius_y_top_right ==
              starlight::NLength::MakeUnitNLength(5));
}

TEST_P(FiberElementTest, FiberElementDirectionCase02) {
  float kScreeWidth = 750;
  float kRpxRatio = 750.0f;

  manager->UpdateScreenMetrics(kScreeWidth, 1000);

  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableCSSInheritance(true);
  std::unordered_set<CSSPropertyID> list = {kPropertyIDDirection,
                                            kPropertyIDFontSize};
  config->SetCustomCSSInheritList(std::move(list));
  manager->SetConfig(config);

  // css related
  StyleMap indexAttributes;

  CSSParserTokenMap indexTokensMap;
  CSSParserConfigs configs;

  // class .left
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id_border_top_left_radius =
        CSSPropertyID::kPropertyIDBorderTopLeftRadius;
    auto impl_border_top_left_radius = lepus::Value("12rpx");
    tokens.get()->raw_attributes_[id_border_top_left_radius] =
        CSSValue(impl_border_top_left_radius, CSSValuePattern::STRING);
    auto id_border_top_right_radius =
        CSSPropertyID::kPropertyIDBorderTopRightRadius;
    auto impl_border_top_right_radius = lepus::Value("1rpx");
    tokens.get()->raw_attributes_[id_border_top_right_radius] =
        CSSValue(impl_border_top_right_radius, CSSValuePattern::STRING);

    std::string key = ".left";
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

  auto page = manager->CreateFiberPage("page", 10);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  auto parent = manager->CreateFiberNode("view");
  parent->parent_component_element_ = page.get();
  parent->SetStyle(kPropertyIDDirection, lepus::Value("lynx-rtl"));
  parent->SetStyle(kPropertyIDFontSize, lepus::Value("28rpx"));
  page->InsertNode(parent);

  auto element0 = manager->CreateFiberNode("view");
  element0->parent_component_element_ = page.get();
  base::String leftClass("left");
  element0->SetClass(leftClass);
  parent->InsertNode(element0);

  page->FlushActionsAsRoot();

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->platform_impl_.get());
  painting_context->Flush();
  auto* element_painting_node_ =
      painting_context->node_map_.at(element0->impl_id()).get();
  auto top_left_radius_it =
      element_painting_node_->props_.find("border-top-left-radius");
  EXPECT_TRUE(top_left_radius_it != element_painting_node_->props_.end());
  auto tl_value = top_left_radius_it->second.Array()->get(0).Number();
  EXPECT_TRUE(tl_value == 1 * kScreeWidth / kRpxRatio);

  auto top_right_radius_it =
      element_painting_node_->props_.find("border-top-right-radius");
  EXPECT_TRUE(top_right_radius_it != element_painting_node_->props_.end());
  auto tr_value = top_right_radius_it->second.Array()->get(0).Number();
  EXPECT_TRUE(tr_value == 12 * kScreeWidth / kRpxRatio);
}

TEST_P(FiberElementTest, FiberElementDirectionCase03) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableCSSInheritance(true);
  std::unordered_set<CSSPropertyID> list = {kPropertyIDDirection};
  config->SetCustomCSSInheritList(std::move(list));
  manager->SetConfig(config);

  // css related
  StyleMap indexAttributes;

  CSSParserTokenMap indexTokensMap;
  CSSParserConfigs configs;

  // class .title
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    tokens.get()->raw_attributes_[CSSPropertyID::kPropertyIDMarginRight] =
        CSSValue::MakePlainString("12px");

    std::string key = ".title";
    auto& sheets = tokens->sheets();
    auto shared_css_sheets = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheets);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  // class.root-rtl
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDDirection;
    auto impl = lepus::Value("lynx-rtl");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".root-rtl";
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

  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  auto root = manager->CreateFiberView();
  root->parent_component_element_ = page.get();
  root->SetClass("root-rtl");
  page->InsertNode(root);

  auto view_element0 = manager->CreateFiberView();
  view_element0->parent_component_element_ = page.get();
  view_element0->SetClass("title");
  root->InsertNode(view_element0);

  auto text_element0 = manager->CreateFiberText("text");
  text_element0->SetAttribute("text", lepus::Value("title"));
  text_element0->SetStyle(kPropertyIDFontSize, lepus::Value("50px"));
  view_element0->InsertNode(text_element0);

  page->FlushActionsAsRoot();

  EXPECT_TRUE(HasCaptureSignWithStyleKeyAndValueAtLeastNTimes(
      view_element0->impl_id(), CSSPropertyID::kPropertyIDMarginLeft,
      tasm::CSSValue(12, CSSValuePattern::PX), 1));

  tasm_mediator.captured_ids_.clear();
  tasm_mediator.captured_bundles_.clear();

  view_element0->RemoveAllClass();
  page->FlushActionsAsRoot();

  EXPECT_TRUE(HasCaptureSignWithResetStyleKeyAtLeastNTimes(
      view_element0->impl_id(), CSSPropertyID::kPropertyIDMarginLeft, 1));
  EXPECT_FALSE(HasCaptureSignWithResetStyleKeyAtLeastNTimes(
      view_element0->impl_id(), CSSPropertyID::kPropertyIDMarginRight, 1));
}

TEST_P(FiberElementTest, FiberElementDirectionCase04) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableCSSInheritance(true);
  std::unordered_set<CSSPropertyID> list = {kPropertyIDDirection};
  config->SetCustomCSSInheritList(std::move(list));
  manager->SetConfig(config);

  // css related
  StyleMap indexAttributes;

  CSSParserTokenMap indexTokensMap;
  CSSParserConfigs configs;

  // class .title
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    tokens.get()->raw_attributes_[CSSPropertyID::kPropertyIDMarginRight] =
        CSSValue::MakePlainString("12px");

    std::string key = ".title";
    auto& sheets = tokens->sheets();
    auto shared_css_sheets = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheets);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  // class.root-rtl
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDDirection;
    auto impl = lepus::Value("lynx-rtl");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".root-rtl";
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

  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  auto root = manager->CreateFiberView();
  root->parent_component_element_ = page.get();
  root->SetClass("root-rtl");
  page->InsertNode(root);

  auto extended_element = manager->CreateFiberNode("x-textarea");
  extended_element->parent_component_element_ = page.get();
  extended_element->SetClass("title");
  root->InsertNode(extended_element);

  page->FlushActionsAsRoot();

  EXPECT_TRUE(
      extended_element->computed_css_style()->GetTextAttributes()->text_align ==
      starlight::TextAlignType::kRight);
}

TEST_P(FiberElementTest, FiberElementFixedStyle) {
  auto page = manager->CreateFiberPage("page", 11);

  auto element0 = manager->CreateFiberNode("view");
  element0->SetStyle(CSSPropertyID::kPropertyIDBackground,
                     lepus::Value("green"));
  page->InsertNode(element0);

  // fixed
  auto element1 = manager->CreateFiberNode("view");
  element1->SetStyle(CSSPropertyID::kPropertyIDBackground, lepus::Value("red"));
  element1->SetStyle(CSSPropertyID::kPropertyIDPosition, lepus::Value("fixed"));
  element0->InsertNode(element1);

  auto text = manager->CreateFiberText("text");
  element1->InsertNode(text);

  auto element2 = manager->CreateFiberNode("view");
  element2->SetStyle(CSSPropertyID::kPropertyIDBackground,
                     lepus::Value("blue"));
  page->InsertNode(element2);

  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), element0->impl_id(), -1));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), element2->impl_id(), -1));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), element1->impl_id(), -1));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(element1->impl_id(), text->impl_id(), -1));
  page->FlushActionsAsRoot();

  // check painting node
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto* element0_painting_node =
      painting_context->node_map_.at(element0->impl_id()).get();
  auto* element1_painting_node =
      painting_context->node_map_.at(element1->impl_id()).get();
  auto* text_painting_node =
      painting_context->node_map_.at(text->impl_id()).get();
  auto* element2_painting_node =
      painting_context->node_map_.at(element2->impl_id()).get();
  auto page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 3);
  EXPECT_TRUE(page_children[0] == element0_painting_node);
  EXPECT_TRUE(page_children[1] == element2_painting_node);
  EXPECT_TRUE(page_children[2] == element1_painting_node);
  EXPECT_TRUE(element1_painting_node->children_[0] == text_painting_node);

  // remove fixed
  element0->RemoveNode(element1);
  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(page->impl_id(), element1->impl_id()));
  element0->FlushActionsAsRoot();
  painting_context->Flush();

  // check painting node
  page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 2);
  EXPECT_TRUE(page_children[0] == element0_painting_node);
  EXPECT_TRUE(page_children[1] == element2_painting_node);

  EXPECT_TRUE(element1_painting_node->children_[0] == text_painting_node);

  // re-insert fixed
  element0->InsertNode(element1);
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), element1->impl_id(), -1));
  element0->FlushActionsAsRoot();
  painting_context->Flush();

  page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 3);
  EXPECT_TRUE(page_children[0] == element0_painting_node);
  EXPECT_TRUE(page_children[1] == element2_painting_node);
  EXPECT_TRUE(page_children[2] == element1_painting_node);
  EXPECT_TRUE(element1_painting_node->children_[0] == text_painting_node);

  // reset position:fixed style
  element1->RemoveAllInlineStyles();
  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(page->impl_id(), element1->impl_id()));
  EXPECT_CALL(tasm_mediator, InsertLayoutNodeBefore(element0->impl_id(),
                                                    element1->impl_id(), -1));
  element1->FlushActionsAsRoot();
  painting_context->Flush();

  page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 2);
  EXPECT_TRUE(page_children[0] == element0_painting_node);
  EXPECT_TRUE(page_children[1] == element2_painting_node);
  EXPECT_TRUE(element0_painting_node->children_[0] == element1_painting_node);
  EXPECT_TRUE(element1_painting_node->children_[0] == text_painting_node);

  // re set position:fixed
  element1->SetStyle(CSSPropertyID::kPropertyIDPosition, lepus::Value("fixed"));
  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(element0->impl_id(), element1->impl_id()));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), element1->impl_id(), -1));
  element1->FlushActionsAsRoot();
  painting_context->Flush();

  page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 3);
  EXPECT_TRUE(page_children[0] == element0_painting_node);
  EXPECT_TRUE(page_children[1] == element2_painting_node);
  EXPECT_TRUE(page_children[2] == element1_painting_node);
  EXPECT_TRUE(element1_painting_node->children_[0] == text_painting_node);
}

TEST_P(FiberElementTest, FiberElementFixedRemovedCase) {
  auto page = manager->CreateFiberPage("page", 11);

  auto element_before = manager->CreateFiberNode("view");
  element_before->SetStyle(CSSPropertyID::kPropertyIDBackground,
                           lepus::Value("red"));
  page->InsertNode(element_before);

  auto element0 = manager->CreateFiberNode("view");
  element0->SetStyle(CSSPropertyID::kPropertyIDBackground,
                     lepus::Value("green"));
  page->InsertNode(element0);

  // fixed
  auto element1 = manager->CreateFiberNode("view");
  element1->SetStyle(CSSPropertyID::kPropertyIDBackground, lepus::Value("red"));
  element1->SetStyle(CSSPropertyID::kPropertyIDPosition, lepus::Value("fixed"));
  element0->InsertNode(element1);

  // check painting node
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());

  page->FlushActionsAsRoot();
  painting_context->Flush();
  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto* element_before_painting_node =
      painting_context->node_map_.at(element_before->impl_id()).get();
  auto* element0_painting_node =
      painting_context->node_map_.at(element0->impl_id()).get();
  auto* element1_painting_node =
      painting_context->node_map_.at(element1->impl_id()).get();
  auto page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 3);
  EXPECT_TRUE(page_children[0] == element_before_painting_node);
  EXPECT_TRUE(page_children[1] == element0_painting_node);
  EXPECT_TRUE(page_children[2] == element1_painting_node);

  // remove fixed
  page->RemoveNode(element0);
  element0->RemoveNode(element1);
  element_before->InsertNode(element0);

  page->FlushActionsAsRoot();
  painting_context->Flush();

  // check painting node
  page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 1);
  EXPECT_TRUE(page_children[0] == element_before_painting_node);
}

TEST_P(FiberElementTest, FiberElementFixedReplaceCase) {
  manager->config_->SetEnableFixedNew(true);

  auto page = manager->CreateFiberPage("page", 11);

  auto parent_container = manager->CreateFiberNode("view");
  page->InsertNode(parent_container);

  auto container = manager->CreateFiberNode("view");
  parent_container->InsertNode(container);

  auto parent = manager->CreateFiberNode("view");
  container->InsertNode(parent);

  auto fixed_child = manager->CreateFiberNode("view");
  fixed_child->SetStyle(CSSPropertyID::kPropertyIDPosition,
                        lepus::Value("fixed"));
  parent->InsertNode(fixed_child);

  page->FlushActionsAsRoot();

  auto parent_sibling = manager->CreateFiberNode("view");

  base::Vector<fml::RefPtr<FiberElement>> inserted_elements{};
  base::Vector<fml::RefPtr<FiberElement>> removed_elements{};
  inserted_elements.emplace_back(parent_sibling);
  inserted_elements.emplace_back(parent);
  removed_elements.emplace_back(parent);

  container->ReplaceElements(inserted_elements, removed_elements, nullptr);

  page->FlushActionsAsRoot();
}

TEST_P(FiberElementTest, FiberElementFixedDoubleReplaceCase) {
  manager->config_->SetEnableFixedNew(true);

  auto page = manager->CreateFiberPage("page", 11);

  auto parent_container = manager->CreateFiberNode("view");
  page->InsertNode(parent_container);

  auto container = manager->CreateFiberNode("view");
  parent_container->InsertNode(container);

  auto parent = manager->CreateFiberNode("view");
  container->InsertNode(parent);

  auto fixed_child = manager->CreateFiberNode("view");
  fixed_child->SetStyle(CSSPropertyID::kPropertyIDPosition,
                        lepus::Value("fixed"));
  parent->InsertNode(fixed_child);

  page->FlushActionsAsRoot();

  auto container_sibling = manager->CreateFiberNode("view");
  auto parent_sibling = manager->CreateFiberNode("view");

  base::Vector<fml::RefPtr<FiberElement>> parent_inserted_elements{};
  base::Vector<fml::RefPtr<FiberElement>> parent_removed_elements{};
  parent_inserted_elements.emplace_back(container_sibling);
  parent_inserted_elements.emplace_back(container);
  parent_removed_elements.emplace_back(container);

  parent_container->ReplaceElements(parent_inserted_elements,
                                    parent_removed_elements, nullptr);

  base::Vector<fml::RefPtr<FiberElement>> inserted_elements{};
  base::Vector<fml::RefPtr<FiberElement>> removed_elements{};
  inserted_elements.emplace_back(parent_sibling);
  inserted_elements.emplace_back(parent);
  removed_elements.emplace_back(parent);

  container->ReplaceElements(inserted_elements, removed_elements, nullptr);

  page->FlushActionsAsRoot();
}

TEST_P(FiberElementTest, FiberElementInsertBeforeFixedCase) {
  auto page = manager->CreateFiberPage("page", 11);

  auto element_before = manager->CreateFiberNode("view");
  element_before->SetStyle(CSSPropertyID::kPropertyIDBackground,
                           lepus::Value("red"));
  page->InsertNode(element_before);

  auto element0 = manager->CreateFiberNode("view");
  element0->SetStyle(CSSPropertyID::kPropertyIDBackground,
                     lepus::Value("green"));
  page->InsertNode(element0);

  // fixed
  auto element1 = manager->CreateFiberNode("view");
  element1->SetStyle(CSSPropertyID::kPropertyIDBackground, lepus::Value("red"));
  element1->SetStyle(CSSPropertyID::kPropertyIDPosition, lepus::Value("fixed"));
  element0->InsertNode(element1);

  // check painting node
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());

  page->FlushActionsAsRoot();
  painting_context->Flush();
  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto* element_before_painting_node =
      painting_context->node_map_.at(element_before->impl_id()).get();
  auto* element0_painting_node =
      painting_context->node_map_.at(element0->impl_id()).get();
  auto* element1_painting_node =
      painting_context->node_map_.at(element1->impl_id()).get();
  auto page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 3);
  EXPECT_TRUE(page_children[0] == element_before_painting_node);
  EXPECT_TRUE(page_children[1] == element0_painting_node);
  EXPECT_TRUE(page_children[2] == element1_painting_node);

  // insert before fixed
  auto element_insert_before = manager->CreateFiberNode("view");
  element0->InsertNodeBefore(element_insert_before, element1);

  auto element_not_ref = manager->CreateFiberNode("view");
  element0->InsertNode(element_not_ref);

  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(element0->impl_id(),
                                     element_insert_before->impl_id(), -1));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(element0->impl_id(),
                                     element_not_ref->impl_id(), -1));

  page->FlushActionsAsRoot();
  painting_context->Flush();
}

TEST_P(FiberElementTest, FiberElementInsertBeforeFixedCase1) {
  auto page = manager->CreateFiberPage("page", 11);

  auto element_before = manager->CreateFiberNode("view");
  element_before->SetStyle(CSSPropertyID::kPropertyIDBackground,
                           lepus::Value("red"));
  page->InsertNode(element_before);

  auto element0 = manager->CreateFiberNode("view");
  element0->SetStyle(CSSPropertyID::kPropertyIDBackground,
                     lepus::Value("green"));
  page->InsertNode(element0);

  // fixed
  auto element1 = manager->CreateFiberNode("view");
  element1->SetStyle(CSSPropertyID::kPropertyIDBackground, lepus::Value("red"));
  element1->SetStyle(CSSPropertyID::kPropertyIDPosition, lepus::Value("fixed"));
  element0->InsertNode(element1);

  // check painting node
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());

  page->FlushActionsAsRoot();
  painting_context->Flush();
  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto* element_before_painting_node =
      painting_context->node_map_.at(element_before->impl_id()).get();
  auto* element0_painting_node =
      painting_context->node_map_.at(element0->impl_id()).get();
  auto* element1_painting_node =
      painting_context->node_map_.at(element1->impl_id()).get();
  auto page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 3);
  EXPECT_TRUE(page_children[0] == element_before_painting_node);
  EXPECT_TRUE(page_children[1] == element0_painting_node);
  EXPECT_TRUE(page_children[2] == element1_painting_node);

  element1->SetStyle(CSSPropertyID::kPropertyIDPosition,
                     lepus::Value("absolute"));

  auto element_insert_before = manager->CreateFiberNode("view");
  element0->InsertNodeBefore(element_insert_before, element1);

  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(page->impl_id(), element1->impl_id()));

  EXPECT_CALL(tasm_mediator, InsertLayoutNodeBefore(element0->impl_id(),
                                                    element1->impl_id(), -1));

  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(element0->impl_id(),
                                     element_insert_before->impl_id(), -1));

  page->FlushActionsAsRoot();
  painting_context->Flush();
}

TEST_P(FiberElementTest, FiberElementInsertBeforeFixedCase2) {
  auto page = manager->CreateFiberPage("page", 11);

  auto element_before = manager->CreateFiberNode("view");
  element_before->SetStyle(CSSPropertyID::kPropertyIDBackground,
                           lepus::Value("red"));
  page->InsertNode(element_before);

  auto element0 = manager->CreateFiberNode("view");
  element0->SetStyle(CSSPropertyID::kPropertyIDBackground,
                     lepus::Value("green"));
  page->InsertNode(element0);

  // fixed
  auto element1 = manager->CreateFiberNode("view");
  element1->SetStyle(CSSPropertyID::kPropertyIDBackground, lepus::Value("red"));
  element1->SetStyle(CSSPropertyID::kPropertyIDPosition, lepus::Value("fixed"));
  element0->InsertNode(element1);

  // check painting node
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());

  page->FlushActionsAsRoot();
  painting_context->Flush();
  auto* page_painting_node =
      painting_context->node_map_.at(page->impl_id()).get();
  auto* element_before_painting_node =
      painting_context->node_map_.at(element_before->impl_id()).get();
  auto* element0_painting_node =
      painting_context->node_map_.at(element0->impl_id()).get();
  auto* element1_painting_node =
      painting_context->node_map_.at(element1->impl_id()).get();
  auto page_children = page_painting_node->children_;
  EXPECT_TRUE(page_children.size() == 3);
  EXPECT_TRUE(page_children[0] == element_before_painting_node);
  EXPECT_TRUE(page_children[1] == element0_painting_node);
  EXPECT_TRUE(page_children[2] == element1_painting_node);

  element1->SetStyle(CSSPropertyID::kPropertyIDPosition,
                     lepus::Value("absolute"));

  auto element_insert_before = manager->CreateFiberNode("view");
  element0->InsertNodeBefore(element_insert_before, element1);

  auto element_not_ref = manager->CreateFiberNode("view");
  element0->InsertNode(element_not_ref);

  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(element0->impl_id(),
                                     element_insert_before->impl_id(), -1));

  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(page->impl_id(), element1->impl_id()));

  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(element0->impl_id(), element1->impl_id(),
                                     element_not_ref->impl_id()));

  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(element0->impl_id(),
                                     element_not_ref->impl_id(), -1));

  page->FlushActionsAsRoot();
  painting_context->Flush();
}

TEST_P(FiberElementTest, FiberElementInsertBeforeFixedCase3) {
  auto page = manager->CreateFiberPage("page", 11);

  auto element_before = manager->CreateFiberNode("view");
  element_before->SetStyle(CSSPropertyID::kPropertyIDBackground,
                           lepus::Value("red"));
  page->InsertNode(element_before);

  auto element0 = manager->CreateFiberNode("view");
  element0->SetStyle(CSSPropertyID::kPropertyIDBackground,
                     lepus::Value("green"));
  page->InsertNode(element0);

  auto element1 = manager->CreateFiberNode("view");
  element1->SetStyle(CSSPropertyID::kPropertyIDBackground, lepus::Value("red"));
  element1->SetStyle(CSSPropertyID::kPropertyIDPosition,
                     lepus::Value("absolute"));
  element0->InsertNode(element1);

  // check painting node
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());

  page->FlushActionsAsRoot();
  painting_context->Flush();

  element1->SetStyle(CSSPropertyID::kPropertyIDPosition, lepus::Value("fixed"));

  auto element_insert_before = manager->CreateFiberNode("view");
  element0->InsertNodeBefore(element_insert_before, element1);

  auto element_not_ref = manager->CreateFiberNode("view");
  element0->InsertNode(element_not_ref);

  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(element0->impl_id(),
                                     element_insert_before->impl_id(), -1));

  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(element0->impl_id(),
                                     element_not_ref->impl_id(), -1));

  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(element0->impl_id(), element1->impl_id()));

  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), element1->impl_id(), -1));

  page->FlushActionsAsRoot();
  painting_context->Flush();
}

TEST_P(FiberElementTest, FiberElementClassChangeTransmitTEST) {
  auto page = manager->CreateFiberPage("page", 11);

  auto element0 = manager->CreateFiberNode("view");
  element0->SetStyle(CSSPropertyID::kPropertyIDBackground,
                     lepus::Value("green"));
  page->InsertNode(element0);

  auto element1 = manager->CreateFiberNode("view");
  element1->SetStyle(CSSPropertyID::kPropertyIDBackground, lepus::Value("red"));
  element0->InsertNode(element1);

  auto element2 = manager->CreateFiberNode("view");
  element2->SetStyle(CSSPropertyID::kPropertyIDBackground,
                     lepus::Value("grey"));
  element1->InsertNode(element2);
  page->FlushActionsAsRoot();

  EXPECT_TRUE(element0->dirty_ == 0);
  EXPECT_TRUE(element1->dirty_ == 0);
  EXPECT_TRUE(element2->dirty_ == 0);

  element0->SetAttribute(kTransmitClassDirty, lepus::Value(true));
  base::String newClass("new");
  element0->SetClass(newClass);

  EXPECT_TRUE(element0->enable_class_change_transmit_ = true);
  EXPECT_TRUE((element0->dirty_ & FiberElement::kDirtyStyle) != 0);
  EXPECT_TRUE((element1->dirty_ & FiberElement::kDirtyStyle) != 0);
  EXPECT_TRUE((element2->dirty_ & FiberElement::kDirtyStyle) != 0);

  EXPECT_TRUE(element0->flush_required_ == true);
  EXPECT_TRUE(element1->flush_required_ == true);
  EXPECT_TRUE(element2->flush_required_ == true);

  page->FlushActionsAsRoot();

  EXPECT_TRUE((element0->dirty_ & FiberElement::kDirtyStyle) == 0);
  EXPECT_TRUE((element1->dirty_ & FiberElement::kDirtyStyle) == 0);
  EXPECT_TRUE((element2->dirty_ & FiberElement::kDirtyStyle) == 0);

  EXPECT_TRUE(element0->flush_required_ == false);
  EXPECT_TRUE(element1->flush_required_ == false);
  EXPECT_TRUE(element2->flush_required_ == false);
}

TEST_P(FiberElementTest, DataModelSibling) {
  auto page = manager->CreateFiberPage("page", 11);

  auto first = manager->CreateFiberView();
  page->InsertNode(first);

  auto second = manager->CreateFiberView();
  page->InsertNode(second);

  EXPECT_TRUE(second->previous_sibling() == first.get());
  EXPECT_TRUE(first->next_sibling() == second.get());

  auto third = manager->CreateFiberView();
  page->InsertNode(third);

  EXPECT_TRUE(page->GetChildCount() == 3);

  EXPECT_TRUE(third->previous_sibling() == second.get());
  EXPECT_TRUE(second->next_sibling() == third.get());

  // check the siblings
  EXPECT_TRUE(page->GetChildAt(0) == first.get());
  EXPECT_TRUE(second->previous_sibling()->next_sibling() == second.get());
  EXPECT_TRUE(second->next_sibling()->previous_sibling() == second.get());

  page->RemoveNode(second);

  EXPECT_TRUE(page->GetChildCount() == 2);
  EXPECT_TRUE(third->previous_sibling() == first.get());
  EXPECT_TRUE(first->next_sibling() == third.get());
  EXPECT_TRUE(third->previous_sibling()->next_sibling() == third.get());
  EXPECT_TRUE(first->next_sibling()->previous_sibling() == first.get());

  // check the data_model
  EXPECT_TRUE(third->data_model()->PreviousSibling() == first->data_model());
  EXPECT_TRUE(first->data_model()->NextSibling() == third->data_model());
  EXPECT_TRUE(third->data_model()->PreviousSibling()->NextSibling() ==
              third->data_model());
  EXPECT_TRUE(first->data_model()->NextSibling()->PreviousSibling() ==
              first->data_model());
}

TEST_P(FiberElementTest, CheckFlags) {
  auto page = manager->CreateFiberPage("page", 11);
  auto element = manager->CreateFiberNode("view");
  element->SetStyle(CSSPropertyID::kPropertyIDOverflow,
                    lepus::Value("visible"));

  page->InsertNode(element);
  page->FlushActionsAsRoot();
  element->CheckHasNonFlattenCSSProps(CSSPropertyID::kPropertyIDBoxShadow);
  EXPECT_TRUE(element->has_non_flatten_attrs_);
}

TEST_P(FiberElementTest, CheckFlattenRelatedFlags) {
  auto element = manager->CreateFiberNode("view");
  EXPECT_TRUE(element->has_non_flatten_attrs_ == false);
  element->SetStyleInternal(CSSPropertyID::kPropertyIDTransition,
                            tasm::CSSValue::MakePlainString("test"));
  EXPECT_TRUE(element->has_transition_props_changed_ == true);
  EXPECT_TRUE(element->has_non_flatten_attrs_ == true);

  element->SetStyleInternal(CSSPropertyID::kPropertyIDAnimation,
                            tasm::CSSValue::MakePlainString("test"));

  element->has_non_flatten_attrs_ = false;
  EXPECT_TRUE(element->has_keyframe_props_changed_ == true);
  element->SetStyleInternal(CSSPropertyID::kPropertyIDZIndex,
                            tasm::CSSValue::MakePlainString("3"));
  EXPECT_TRUE(element->has_z_props());

  element->ResetStyleInternal(CSSPropertyID::kPropertyIDTransition);
  element->ResetStyleInternal(CSSPropertyID::kPropertyIDAnimation);
  EXPECT_TRUE(element->has_non_flatten_attrs_ == true);

  auto page = manager->CreateFiberPage("page", 11);
  auto view = manager->CreateFiberView();
  view->SetStyle(CSSPropertyID::kPropertyIDZIndex, lepus::Value("100"));
  page->InsertNode(view);
  page->FlushActionsAsRoot();
  EXPECT_TRUE(view->TendToFlatten() == false);

  auto image = manager->CreateFiberImage("image");
  image->SetStyle(CSSPropertyID::kPropertyIDZIndex, lepus::Value("100"));
  page->InsertNode(image);
  page->FlushActionsAsRoot();
  EXPECT_TRUE(image->TendToFlatten() == true);

  auto text = manager->CreateFiberText("text");
  text->SetStyle(CSSPropertyID::kPropertyIDZIndex, lepus::Value("100"));
  page->InsertNode(text);
  page->FlushActionsAsRoot();
  EXPECT_TRUE(text->TendToFlatten() == true);
}

TEST_P(FiberElementTest, TestOnPseudoStatusChanged) {
  //  constructor css fragment
  StyleMap indexAttributes;
  CSSParserConfigs configs;

  CSSParserTokenMap indexTokensMap;
  // class .test
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDOpacity;
    auto impl = lepus::Value(0.3);
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".test";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  CSSParserTokenMap pseudo_map;
  // class .test:active
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDOpacity;
    auto impl = lepus::Value(0.8);
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".test:active";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    pseudo_map.emplace(key, tokens);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap fontfaces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokensMap, keyframes, fontfaces);

  // mock pseudo class
  indexFragment->MarkHasTouchPseudoToken();
  indexFragment->pseudo_map_ = pseudo_map;

  // parent
  auto page = manager->CreateFiberPage("page", 11);

  // child component
  base::String component_id("21");
  int32_t css_id = 100;
  base::String entry_name("__Card__");
  base::String component_name("TestComp");
  base::String path("/index/components/TestComp");

  auto comp = manager->CreateFiberComponent(component_id, css_id, entry_name,
                                            component_name, path);
  comp->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  page->InsertNode(comp);

  auto element = manager->CreateFiberView();
  element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(comp->impl_id()));
  comp->InsertNode(element);
  base::String clazz_name("test");
  element->SetClass(clazz_name);

  page->FlushActionsAsRoot();

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->platform_impl_.get());
  painting_context->Flush();

  auto* mock_painting_node_ =
      painting_context->node_map_.at(element->impl_id()).get();

  EXPECT_TRUE(mock_painting_node_->props_.size() == 1);
  std::string opa("opacity");
  EXPECT_TRUE(mock_painting_node_->props_.at(opa) == lepus::Value(0.3));

  element->OnPseudoStatusChanged(kPseudoStateNone, kPseudoStateActive);
  painting_context->Flush();

  EXPECT_TRUE(mock_painting_node_->props_.at(opa) == lepus::Value(0.8));
}

TEST_P(FiberElementTest, SetNativePropsCases) {
  // Construct CSS fragment
  StyleMap indexAttributes;
  CSSParserTokenMap indexTokenMap;

  // page
  std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap font_faces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokenMap, keyframes, font_faces);
  auto page = manager->CreateFiberPage("page", 0);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // view1
  auto fiber_element_1 = manager->CreateFiberNode("view");
  fiber_element_1->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_1);
  fiber_element_1->SetClass("one");

  // view2
  auto fiber_element_2 = manager->CreateFiberNode("text");
  fiber_element_2->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_element_2);
  fiber_element_2->SetIdSelector("intro");
  fiber_element_2->SetAttribute("text", lepus::Value("Hello World."));

  page->FlushActionsAsRoot();

  lepus::Value native_props = lepus::Value(lepus::Dictionary::Create());
  native_props.SetProperty("text", lepus::Value("testing..."));
  native_props.SetProperty("background-color", lepus::Value("red"));
  native_props.SetProperty("transform", lepus::Value("translateY(30px)"));
  auto pipeline_options = std::make_shared<PipelineOptions>();
  ;
  fiber_element_2->SetNativeProps(native_props, pipeline_options);

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  auto* painting_node_2_after_set_native_props =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  auto node_2_background_color_value_after_set_native_props =
      painting_node_2_after_set_native_props->props_.at("background-color");
  EXPECT_TRUE(node_2_background_color_value_after_set_native_props.UInt32() ==
              0xffff0000);

  auto node_2_text_value_after_set_native_props =
      painting_node_2_after_set_native_props->props_.at("text");
  EXPECT_TRUE(
      node_2_text_value_after_set_native_props.String().IsEqual("testing..."));
}

TEST_P(FiberElementTest, SetNativePropsNormalCases) {
  // Construct CSS fragment
  StyleMap indexAttributes;
  CSSParserTokenMap indexTokenMap;

  // page
  std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap font_faces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokenMap, keyframes, font_faces);
  auto page = manager->CreateFiberPage("page", 0);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // view1
  auto fiber_element_1 = manager->CreateFiberNode("view");
  fiber_element_1->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_1);
  fiber_element_1->SetClass("one");

  // view2
  auto fiber_element_2 = manager->CreateFiberNode("text");
  fiber_element_2->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_element_2);
  fiber_element_2->SetIdSelector("intro");
  fiber_element_2->SetAttribute("text", lepus::Value("Hello World."));

  page->FlushActionsAsRoot();

  lepus::Value native_props = lepus::Value(lepus::Dictionary::Create());
  native_props.SetProperty("background-color", lepus::Value("red"));
  auto pipeline_options = std::make_shared<PipelineOptions>();
  ;
  fiber_element_2->SetNativeProps(native_props, pipeline_options);

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  auto* painting_node_2_after_set_native_props =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  auto node_2_background_color_value_after_set_native_props =
      painting_node_2_after_set_native_props->props_.at("background-color");
  EXPECT_TRUE(node_2_background_color_value_after_set_native_props.UInt32() ==
              0xffff0000);
}

TEST_P(FiberElementTest, SetNativePropsTextCases) {
  // Construct CSS fragment
  StyleMap indexAttributes;
  CSSParserTokenMap indexTokenMap;

  // page
  std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap font_faces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokenMap, keyframes, font_faces);
  auto page = manager->CreateFiberPage("page", 0);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // view1
  auto fiber_element_1 = manager->CreateFiberNode("view");
  fiber_element_1->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_1);
  fiber_element_1->SetClass("intro");

  // text
  auto fiber_text_1 = manager->CreateFiberText("text");
  fiber_text_1->SetIdSelector("intro");
  fiber_text_1->SetAttribute("text", lepus::Value("Hello World."));
  fiber_text_1->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_text_1);

  // x-text
  auto fiber_x_text_1 = manager->CreateFiberText("x-text");
  fiber_x_text_1->SetIdSelector("x-text");
  fiber_x_text_1->SetAttribute("text", lepus::Value("hello world2."));
  fiber_x_text_1->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_x_text_1);

  page->FlushActionsAsRoot();

  {
    lepus::Value native_props = lepus::Value(lepus::Dictionary::Create());
    native_props.SetProperty("text", lepus::Value("testing..."));
    native_props.SetProperty("background-color", lepus::Value("red"));
    native_props.SetProperty("transform", lepus::Value("translateY(30px)"));
    auto pipeline_options = std::make_shared<PipelineOptions>();
    ;
    fiber_text_1->SetNativeProps(native_props, pipeline_options);

    auto painting_context = static_cast<FiberMockPaintingContext*>(
        manager->painting_context()->impl());
    painting_context->Flush();
    auto* fiber_text_1_after_set_native_props =
        painting_context->node_map_.at(fiber_text_1->impl_id()).get();
    auto node_2_background_color_value_after_set_native_props =
        fiber_text_1_after_set_native_props->props_.at("background-color");

    EXPECT_TRUE(node_2_background_color_value_after_set_native_props.UInt32() ==
                0xffff0000);
  }

  {
    lepus::Value native_props = lepus::Value(lepus::Dictionary::Create());
    native_props.SetProperty("text", lepus::Value("changing x-text"));
    native_props.SetProperty("background-color", lepus::Value("red"));
    native_props.SetProperty("transform", lepus::Value("translateY(30px)"));
    auto pipeline_options = std::make_shared<PipelineOptions>();
    ;
    fiber_text_1->SetNativeProps(native_props, pipeline_options);

    auto painting_context = static_cast<FiberMockPaintingContext*>(
        manager->painting_context()->impl());
    painting_context->Flush();
    auto* fiber_text_1_after_set_native_props =
        painting_context->node_map_.at(fiber_text_1->impl_id()).get();
    auto node_2_background_color_value_after_set_native_props =
        fiber_text_1_after_set_native_props->props_.at("background-color");
    EXPECT_TRUE(node_2_background_color_value_after_set_native_props.UInt32() ==
                0xffff0000);
  }
}

TEST_P(FiberElementTest, SetNativePropsTextBadCases) {
  // Construct CSS fragment
  StyleMap indexAttributes;
  CSSParserTokenMap indexTokenMap;

  // page
  auto page = manager->CreateFiberPage("page", 0);

  // view1

  auto parent = manager->CreateFiberNode("view");
  page->InsertNode(parent);

  // text
  auto fiber_text = manager->CreateFiberText("text");
  fiber_text->SetIdSelector("intro");
  parent->InsertNode(fiber_text);

  // raw-text
  auto fiber_raw_text = manager->CreateFiberRawText();
  fiber_raw_text->SetAttribute("text", lepus::Value("Hello World."));
  fiber_text->InsertNode(fiber_raw_text);

  page->FlushActionsAsRoot();

  lepus::Value native_props = lepus::Value(lepus::Dictionary::Create());
  native_props.SetProperty("text", lepus::Value("Test Content"));

  auto pipeline_options = std::make_shared<PipelineOptions>();
  ;

  tasm_mediator.captured_ids_.clear();

  fiber_text->SetNativeProps(native_props, pipeline_options);

  EXPECT_FALSE(std::find(tasm_mediator.captured_ids_.begin(),
                         tasm_mediator.captured_ids_.end(),
                         fiber_text->impl_id()) !=
               tasm_mediator.captured_ids_.end());

  EXPECT_TRUE(std::find(tasm_mediator.captured_ids_.begin(),
                        tasm_mediator.captured_ids_.end(),
                        fiber_raw_text->impl_id()) !=
              tasm_mediator.captured_ids_.end());

  page->RemoveNode(parent);

  native_props.SetProperty("text", lepus::Value("Test Content Update"));

  tasm_mediator.captured_ids_.clear();

  fiber_text->SetNativeProps(native_props, pipeline_options);

  EXPECT_FALSE(std::find(tasm_mediator.captured_ids_.begin(),
                         tasm_mediator.captured_ids_.end(),
                         fiber_text->impl_id()) !=
               tasm_mediator.captured_ids_.end());

  EXPECT_FALSE(std::find(tasm_mediator.captured_ids_.begin(),
                         tasm_mediator.captured_ids_.end(),
                         fiber_raw_text->impl_id()) !=
               tasm_mediator.captured_ids_.end());
}

TEST_P(FiberElementTest, TestEnsureTagInfoInParallelMode) {
  auto page = manager->CreateFiberPage("page", 11);

  for (int32_t i = 0; i < 10000; ++i) {
    auto element = manager->CreateFiberNode(std::to_string(i));
    page->InsertNode(element);
  }

  page->FlushActionsAsRoot();
}

TEST_P(FiberElementTest, ListItemTest0_0) {
  auto page = manager->CreateFiberPage("page", 11);

  auto list = manager->CreateFiberList(tasm.get(), "list", lepus::Value(),
                                       lepus::Value(), lepus::Value());
  page->InsertNode(list);

  auto wrapper_0 = manager->CreateFiberWrapperElement();
  list->InsertNode(wrapper_0);

  auto wrapper_1 = manager->CreateFiberWrapperElement();
  wrapper_0->InsertNode(wrapper_1);

  auto view_0 = manager->CreateFiberView();
  wrapper_1->InsertNode(view_0);

  auto view_1 = manager->CreateFiberView();
  view_0->InsertNode(view_1);

  EXPECT_TRUE(wrapper_0->is_list_item());
  EXPECT_TRUE(wrapper_1->is_list_item());
  EXPECT_TRUE(view_0->is_list_item());
  EXPECT_FALSE(view_1->is_list_item());

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options);
  EXPECT_FALSE(platform_impl_->HasFlushed());
  platform_impl_->ResetFlushFlag();

  manager->OnPatchFinish(options, wrapper_0.get());
  EXPECT_TRUE(platform_impl_->HasFlushed());
}

TEST_P(FiberElementTest, ListItemTest0_1) {
  auto page = manager->CreateFiberPage("page", 11);

  auto list = manager->CreateFiberList(tasm.get(), "list", lepus::Value(),
                                       lepus::Value(), lepus::Value());
  page->InsertNode(list);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options);
  EXPECT_FALSE(platform_impl_->HasFlushed());
  platform_impl_->ResetFlushFlag();

  auto wrapper_0 = manager->CreateFiberWrapperElement();
  list->InsertNode(wrapper_0);

  auto wrapper_1 = manager->CreateFiberWrapperElement();
  wrapper_0->InsertNode(wrapper_1);

  auto view_0 = manager->CreateFiberView();
  wrapper_1->InsertNode(view_0);

  auto view_1 = manager->CreateFiberView();
  view_0->InsertNode(view_1);

  EXPECT_TRUE(wrapper_0->is_list_item());
  EXPECT_TRUE(wrapper_1->is_list_item());
  EXPECT_TRUE(view_0->is_list_item());
  EXPECT_FALSE(view_1->is_list_item());

  manager->OnPatchFinish(options, wrapper_0.get());
  EXPECT_TRUE(platform_impl_->HasFlushed());
}

TEST_P(FiberElementTest, ListItemTest0_2) {
  auto page = manager->CreateFiberPage("page", 11);

  auto list = manager->CreateFiberList(tasm.get(), "list", lepus::Value(),
                                       lepus::Value(), lepus::Value());
  auto wrapper_0 = manager->CreateFiberWrapperElement();
  auto wrapper_1 = manager->CreateFiberWrapperElement();
  auto view_0 = manager->CreateFiberView();
  auto view_1 = manager->CreateFiberView();

  view_0->InsertNode(view_1);
  wrapper_1->InsertNode(view_0);
  wrapper_0->InsertNode(wrapper_1);

  page->InsertNode(list);
  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options);
  EXPECT_FALSE(platform_impl_->HasFlushed());
  platform_impl_->ResetFlushFlag();

  list->InsertNode(wrapper_0);
  EXPECT_TRUE(wrapper_0->is_list_item());
  EXPECT_TRUE(wrapper_1->is_list_item());
  EXPECT_TRUE(view_0->is_list_item());
  EXPECT_FALSE(view_1->is_list_item());

  manager->OnPatchFinish(options, wrapper_0.get());
  EXPECT_TRUE(platform_impl_->HasFlushed());
}

TEST_P(FiberElementTest, ListItemTest1) {
  auto page = manager->CreateFiberPage("page", 11);

  auto list = manager->CreateFiberList(tasm.get(), "list", lepus::Value(),
                                       lepus::Value(), lepus::Value());
  page->InsertNode(list);

  auto view_0 = manager->CreateFiberView();
  list->InsertNode(view_0);

  auto view_1 = manager->CreateFiberView();
  view_0->InsertNode(view_1);

  EXPECT_TRUE(view_0->is_list_item());
  EXPECT_FALSE(view_1->is_list_item());
}

TEST_P(FiberElementTest, GetCSSID) {
  base::String component_id("21");
  int32_t css_id = 123;
  base::String entry_name("TTTT");
  base::String component_name("TestComp");
  base::String path("/index/components/TestComp");

  auto comp = manager->CreateFiberComponent(component_id, css_id, entry_name,
                                            component_name, path);
  auto view = manager->CreateFiberView();
  view->parent_component_element_ = comp.get();

  view->MarkAttached();
  comp->MarkAttached();

  EXPECT_TRUE(view->GetCSSID() == 123);
  EXPECT_TRUE(comp->GetCSSID() == kInvalidCssId);
  EXPECT_TRUE(comp->GetComponentCSSID() == 123);

  comp->SetComponentCSSID(456);
  EXPECT_EQ(view->GetCSSID(), 456);
  EXPECT_EQ(comp->GetCSSID(), kInvalidCssId);
  EXPECT_EQ(comp->GetComponentCSSID(), 456);

  comp->SetCSSID(123);
  EXPECT_EQ(view->GetCSSID(), 456);
  EXPECT_EQ(comp->GetCSSID(), 123);
  EXPECT_EQ(comp->GetComponentCSSID(), 456);

  auto element = manager->CreateFiberView();
  element->SetCSSID(321);

  EXPECT_TRUE(element->GetCSSID() == 321);
}

TEST_P(FiberElementTest, CopyElementInitTest0) {
  manager->GetLynxEnvConfig().font_scale_ = 1.3f;
  manager->GetLynxEnvConfig().font_scale_sp_only_ = false;

  auto fiber_element = manager->CreateFiberText("text");

  fiber_element = fml::AdoptRef<TextElement>(
      new TextElement(*static_cast<TextElement*>(fiber_element.get()), true));
  fiber_element->AttachToElementManager(
      manager, tasm->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME), false);

  EXPECT_TRUE(fiber_element->GetRecordedRootFontSize() - 27.29 < 0.1);
  EXPECT_TRUE(fiber_element->GetFontSize() - 27.29 < 0.1);

  EXPECT_EQ(fiber_element->computed_css_style()->length_context_.font_scale_,
            1.3f);
  EXPECT_EQ(
      fiber_element->computed_css_style()->length_context_.cur_node_font_size_,
      18.1999989f);
  EXPECT_EQ(
      fiber_element->computed_css_style()->length_context_.root_node_font_size_,
      18.1999989f);
}

TEST_P(FiberElementTest, CopyElementInitTest1) {
  manager->GetLynxEnvConfig().font_scale_ = 1.3f;
  manager->GetLynxEnvConfig().font_scale_sp_only_ = true;

  auto fiber_element = manager->CreateFiberText("text");

  fiber_element = fml::AdoptRef<TextElement>(
      new TextElement(*static_cast<TextElement*>(fiber_element.get()), true));
  fiber_element->AttachToElementManager(
      manager, tasm->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME), false);

  EXPECT_TRUE(fiber_element->GetRecordedRootFontSize() - 27.29 < 0.1);
  EXPECT_TRUE(fiber_element->GetFontSize() - 27.29 < 0.1);

  EXPECT_EQ(fiber_element->computed_css_style()->length_context_.font_scale_,
            1.3f);
  EXPECT_EQ(
      fiber_element->computed_css_style()->length_context_.cur_node_font_size_,
      14);
  EXPECT_EQ(
      fiber_element->computed_css_style()->length_context_.root_node_font_size_,
      14);
}

TEST_P(FiberElementTest, CopyListItemTest) {
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

  fiber_element = fml::AdoptRef<ViewElement>(
      new ViewElement(*static_cast<ViewElement*>(fiber_element.get()), true));
  fiber_element->AttachToElementManager(
      manager, tasm->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME), false);

  page->InsertNode(fiber_element);
  fiber_element->SetClass("test01");
  // force the element to overflow hidden
  fiber_element->computed_css_style()->SetOverflowDefaultVisible(false);

  page->FlushActionsAsRoot();

  EXPECT_FALSE(fiber_element->is_layout_only_);

  // child0
  auto fiber_element_0 = manager->CreateFiberView();
  fiber_element_0->parent_component_element_ = page.get();

  fiber_element_0 = fml::AdoptRef<ViewElement>(
      new ViewElement(*static_cast<ViewElement*>(fiber_element_0.get()), true));
  fiber_element_0->AttachToElementManager(
      manager, tasm->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME), false);

  page->InsertNode(fiber_element_0);
  fiber_element_0->SetClass("test01");
  // force the element to overflow visible
  fiber_element_0->computed_css_style()->SetOverflowDefaultVisible(true);

  page->FlushActionsAsRoot();
  platform_impl_->Flush();
  EXPECT_TRUE(fiber_element_0->is_layout_only_);
  EXPECT_TRUE(platform_impl_->node_map_.find(fiber_element_0->impl_id()) ==
              platform_impl_->node_map_.end());

  fiber_element_0->SetStyle(kPropertyIDBackground, lepus::Value("black"));

  page->FlushActionsAsRoot();
  platform_impl_->Flush();
  EXPECT_FALSE(fiber_element_0->is_layout_only_);
  EXPECT_TRUE(platform_impl_->node_map_.find(fiber_element_0->impl_id()) !=
              platform_impl_->node_map_.end());

  auto& node =
      platform_impl_->node_map_.find(fiber_element_0->impl_id())->second;
  EXPECT_TRUE(!node->props_.empty());
  EXPECT_EQ(node->props_["background-color"], lepus::Value(4278190080U));
  EXPECT_EQ(node->props_["overflow"], lepus::Value(0));

  // child1
  auto fiber_element_1 = manager->CreateFiberView();
  fiber_element_1->parent_component_element_ = page.get();
  fiber_element_1->SetClass("test01");
  // force the element to overflow visible
  fiber_element_1->computed_css_style()->SetOverflowDefaultVisible(true);
  fiber_element_1 = fml::AdoptRef<ViewElement>(
      new ViewElement(*static_cast<ViewElement*>(fiber_element_1.get()), true));
  fiber_element_1->AttachToElementManager(
      manager, tasm->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME), false);

  auto scroll_view = manager->CreateFiberScrollView("scroll-view");
  scroll_view = fml::AdoptRef<ScrollElement>(
      new ScrollElement(*static_cast<ScrollElement*>(scroll_view.get()), true));
  scroll_view->AttachToElementManager(
      manager, tasm->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME), false);

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
  comp = fml::AdoptRef<ComponentElement>(
      new ComponentElement(*static_cast<ComponentElement*>(comp.get()), true));
  comp->AttachToElementManager(
      manager, tasm->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME), false);

  comp->SetClass("test01");
  // force the element to overflow visible
  comp->computed_css_style()->SetOverflowDefaultVisible(true);

  lepus::Value component_at_index(10);
  lepus::Value enqueue_component;
  lepus::Value component_at_indexes;

  auto list = manager->CreateFiberList(nullptr, "list", component_at_index,
                                       enqueue_component, component_at_indexes);
  list = fml::AdoptRef<ListElement>(
      new ListElement(*static_cast<ListElement*>(list.get()), true));
  list->AttachToElementManager(
      manager, tasm->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME), false);

  list->InsertNode(comp);
  list->SetAttribute("column-count", lepus::Value(2));
  page->InsertNode(list);
  comp->SetAttribute("full-span", lepus::Value(true));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(page->impl_id(), list->impl_id(), -1));
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNodeBefore(list->impl_id(), comp->impl_id(), -1));

  page->FlushActionsAsRoot();

  EXPECT_TRUE(HasCaptureSignWithTag(page->impl_id(), "page"));
  EXPECT_TRUE(HasCaptureSignWithTag(list->impl_id(), "list"));
  EXPECT_TRUE(HasCaptureSignWithTag(comp->impl_id(), "component"));

  EXPECT_TRUE(HasCaptureSignWithLayoutAttribute(
      list->impl_id(), starlight::LayoutAttribute::kColumnCount));
  EXPECT_TRUE(HasCaptureSignWithLayoutAttribute(
      list->impl_id(), starlight::LayoutAttribute::kScroll));
  EXPECT_TRUE(HasCaptureSignWithLayoutAttribute(
      comp->impl_id(), starlight::LayoutAttribute::kListCompType));
}

TEST_P(FiberElementTest, CopyTestComponentElement) {
  base::String component_id("21");
  int32_t css_id = 100;
  base::String entry_name("TTTT");
  base::String component_name("TestComp");
  base::String path("/index/components/TestComp");

  auto comp = manager->CreateFiberComponent(component_id, css_id, entry_name,
                                            component_name, path);
  comp = fml::AdoptRef<ComponentElement>(
      new ComponentElement(*static_cast<ComponentElement*>(comp.get()), true));
  comp->AttachToElementManager(
      manager, tasm->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME), false);

  EXPECT_TRUE(comp->GetData().IsEmpty());
  EXPECT_TRUE(comp->GetProperties().IsEmpty());
  EXPECT_FALSE(comp->IsPageForBaseComponent());
  EXPECT_EQ(comp->GetEntryName(), "TTTT");
  EXPECT_EQ(comp->ComponentStrId(), "21");
}

TEST_P(FiberElementTest, CopyInsertNode) {
  auto parent = manager->CreateFiberNode("view");
  parent = fml::AdoptRef<FiberElement>(
      new FiberElement(*static_cast<FiberElement*>(parent.get()), true));
  parent->AttachToElementManager(
      manager, tasm->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME), false);

  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 0);

  auto element = manager->CreateFiberNode("view");
  element = fml::AdoptRef<FiberElement>(
      new FiberElement(*static_cast<FiberElement*>(element.get()), true));
  element->AttachToElementManager(
      manager, tasm->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME), false);

  element->SetStyleInternal(CSSPropertyID::kPropertyIDOverflow,
                            tasm::CSSValue::MakePlainString("visible"));
  parent->InsertNode(element);

  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 1);
  EXPECT_EQ(parent->GetChildAt(0), element.get());
}

TEST_P(FiberElementTest, CopySetStyle) {
  // prepare environment for copied element
  LynxEnvConfig lynx_env_config_1(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
  auto tasm_mediator_1 = std::make_shared<
      ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
  auto unique_manager_1 = std::make_unique<lynx::tasm::ElementManager>(
      std::make_unique<FiberMockPaintingContext>(), tasm_mediator_1.get(),
      lynx_env_config_1);
  auto manager_1 = unique_manager_1.get();
  auto tasm_1 = std::make_shared<lynx::tasm::TemplateAssembler>(
      *tasm_mediator_1.get(), std::move(unique_manager_1),
      tasm_mediator_1.get(), 0);
  auto test_entry_1 = std::make_shared<TemplateEntry>();
  tasm_1->template_entries_.insert({"test_entry", test_entry_1});
  auto config_1 = std::make_shared<PageConfig>();
  config_1->SetEnableZIndex(true);
  manager_1->SetConfig(config_1);
  tasm_1->page_config_ = config_1;
  if (thread_strategy == 0) {
    manager_1->SetThreadStrategy(base::ThreadStrategyForRendering::ALL_ON_UI);
  } else {
    manager_1->SetThreadStrategy(
        base::ThreadStrategyForRendering::MULTI_THREADS);
  }
  manager_1->SetEnableParallelElement(enable_parallel_element_flush_strategy >
                                      0);
  manager_1->enable_level_order_traversing_ =
      (enable_parallel_element_flush_strategy &
       Element::kFlagLevelOrderParallel) > 0;

  auto page = manager->CreateFiberPage("page", 11);
  page = fml::AdoptRef<PageElement>(
      new PageElement(*static_cast<PageElement*>(page.get()), true));
  page->AttachToElementManager(
      manager_1, tasm_1->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME), false);

  auto element = manager->CreateFiberView();
  element = fml::AdoptRef<ViewElement>(
      new ViewElement(*static_cast<ViewElement*>(element.get()), true));
  element->AttachToElementManager(
      manager_1, tasm_1->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME), false);

  auto none_element = manager->CreateFiberNoneElement();
  none_element = fml::AdoptRef<NoneElement>(
      new NoneElement(*static_cast<NoneElement*>(none_element.get()), true));
  none_element->AttachToElementManager(
      manager_1, tasm_1->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME), false);

  auto text = manager->CreateFiberText(base::String("TEST"));
  text = fml::AdoptRef<TextElement>(
      new TextElement(*static_cast<TextElement*>(text.get()), true));
  text->AttachToElementManager(
      manager_1, tasm_1->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME), false);

  element->SetStyle(CSSPropertyID::kPropertyIDOverflow,
                    lepus::Value("visible"));
  auto raw_style_value = element->current_raw_inline_styles_->at(
      CSSPropertyID::kPropertyIDOverflow);

  page->InsertNode(element);
  page->InsertNode(none_element);
  page->InsertNode(text);
  EXPECT_TRUE(raw_style_value == lepus::Value("visible"));
  page->FlushActionsAsRoot();
  auto parsed_style_value =
      element->parsed_styles_map_.at(CSSPropertyID::kPropertyIDOverflow);

  EXPECT_TRUE(page->IsPageForBaseComponent());
  EXPECT_TRUE(parsed_style_value.IsEnum());
  EXPECT_TRUE(
      static_cast<starlight::OverflowType>(parsed_style_value.GetNumber()) ==
      starlight::OverflowType::kVisible);
}

TEST_P(FiberElementTest, CloneAPITest) {
  // prepare environment for copied element
  LynxEnvConfig lynx_env_config_1(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
  auto tasm_mediator_1 = std::make_shared<
      ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
  auto unique_manager_1 = std::make_unique<lynx::tasm::ElementManager>(
      std::make_unique<FiberMockPaintingContext>(), tasm_mediator_1.get(),
      lynx_env_config_1);
  auto manager_1 = unique_manager_1.get();
  auto tasm_1 = std::make_shared<lynx::tasm::TemplateAssembler>(
      *tasm_mediator_1.get(), std::move(unique_manager_1),
      tasm_mediator_1.get(), 0);
  auto test_entry_1 = std::make_shared<TemplateEntry>();
  tasm_1->template_entries_.insert({"test_entry", test_entry_1});
  auto config_1 = std::make_shared<PageConfig>();
  config_1->SetEnableZIndex(true);
  manager_1->SetConfig(config_1);
  tasm_1->page_config_ = config_1;
  if (thread_strategy == 0) {
    manager_1->SetThreadStrategy(base::ThreadStrategyForRendering::ALL_ON_UI);
  } else {
    manager_1->SetThreadStrategy(
        base::ThreadStrategyForRendering::MULTI_THREADS);
  }
  manager_1->SetEnableParallelElement(enable_parallel_element_flush_strategy >
                                      0);
  manager_1->enable_level_order_traversing_ =
      (enable_parallel_element_flush_strategy &
       Element::kFlagLevelOrderParallel) > 0;

  {
    base::String component_id("21");
    int32_t css_id = 100;
    base::String entry_name("__Card__");
    base::String component_name("TestComp");
    base::String path("/index/components/TestComp");
    auto node = manager->CreateFiberComponent(component_id, css_id, entry_name,
                                              component_name, path);
    auto cloned_node = node->CloneElement(true);
    cloned_node->AttachToElementManager(
        manager_1, tasm_1->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME),
        false);
    ComponentElement* cloned_component =
        reinterpret_cast<ComponentElement*>(cloned_node.get());
    EXPECT_TRUE(node->component_id_ == cloned_component->component_id_ &&
                node->component_css_id_ == cloned_component->component_css_id_);
  }

  {
    base::String tag("image");
    auto node = manager->CreateFiberImage(tag);
    auto new_node = node->CloneElement(true);
    new_node->AttachToElementManager(
        manager_1, tasm_1->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME),
        false);
    EXPECT_TRUE(node->impl_id() == new_node->impl_id() &&
                node->tag_ == new_node->tag_);
  }

  {
    auto node = manager->CreateFiberNoneElement();
    auto new_node = node->CloneElement(true);
    new_node->AttachToElementManager(
        manager_1, tasm_1->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME),
        false);
    EXPECT_TRUE(node->impl_id() == new_node->impl_id() &&
                node->tag_ == new_node->tag_);
  }

  {
    auto node = manager->CreateFiberPage("page", 11);
    auto new_node = node->CloneElement(true);
    new_node->AttachToElementManager(
        manager_1, tasm_1->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME),
        false);
    EXPECT_TRUE(node->impl_id() == new_node->impl_id() &&
                node->tag_ == new_node->tag_ &&
                node->css_id_ == new_node->css_id_);
  }

  {
    auto node = manager->CreateFiberRawText();
    auto new_node = node->CloneElement(true);
    new_node->AttachToElementManager(
        manager_1, tasm_1->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME),
        false);
    EXPECT_TRUE(node->impl_id() == new_node->impl_id() &&
                node->tag_ == new_node->tag_ &&
                node->css_id_ == new_node->css_id_);
  }

  {
    base::String tag("scroll-view");
    auto node = manager->CreateFiberScrollView(tag);
    auto new_node = node->CloneElement(true);
    new_node->AttachToElementManager(
        manager_1, tasm_1->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME),
        false);
    EXPECT_TRUE(node->impl_id() == new_node->impl_id() &&
                node->tag_ == new_node->tag_ &&
                node->css_id_ == new_node->css_id_);
  }

  {
    base::String tag("text");
    auto node = manager->CreateFiberText(tag);
    auto new_node = node->CloneElement(true);
    new_node->AttachToElementManager(
        manager_1, tasm_1->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME),
        false);
    EXPECT_TRUE(node->impl_id() == new_node->impl_id() &&
                node->tag_ == new_node->tag_ &&
                node->css_id_ == new_node->css_id_);
  }

  {
    base::String tag("text");
    auto node = manager->CreateFiberView();
    auto new_node = node->CloneElement(true);
    new_node->AttachToElementManager(
        manager_1, tasm_1->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME),
        false);
    EXPECT_TRUE(node->impl_id() == new_node->impl_id() &&
                node->tag_ == new_node->tag_ &&
                node->css_id_ == new_node->css_id_);
  }

  {
    auto node = manager->CreateFiberWrapperElement();
    auto new_node = node->CloneElement(true);
    new_node->AttachToElementManager(
        manager_1, tasm_1->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME),
        false);
    EXPECT_TRUE(node->impl_id() == new_node->impl_id() &&
                node->tag_ == new_node->tag_ &&
                node->css_id_ == new_node->css_id_);
  }
}

TEST_P(FiberElementTest, ElementBundleTest00) {
  LynxTemplateBundle template_bundle;

  // prepare environment for copied element
  LynxEnvConfig lynx_env_config_1(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
  auto tasm_mediator_1 = std::make_shared<
      ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
  auto unique_manager_1 = std::make_unique<lynx::tasm::ElementManager>(
      std::make_unique<FiberMockPaintingContext>(), tasm_mediator_1.get(),
      lynx_env_config_1);
  auto manager_1 = unique_manager_1.get();
  auto tasm_1 = std::make_shared<lynx::tasm::TemplateAssembler>(
      *tasm_mediator_1.get(), std::move(unique_manager_1),
      tasm_mediator_1.get(), 0);
  auto test_entry_1 = std::make_shared<TemplateEntry>();
  tasm_1->template_entries_.insert({"test_entry", test_entry_1});
  auto config_1 = std::make_shared<PageConfig>();
  config_1->SetEnableZIndex(true);
  manager_1->SetConfig(config_1);
  tasm_1->page_config_ = config_1;
  if (thread_strategy == 0) {
    manager_1->SetThreadStrategy(base::ThreadStrategyForRendering::ALL_ON_UI);
  } else {
    manager_1->SetThreadStrategy(
        base::ThreadStrategyForRendering::MULTI_THREADS);
  }

  manager_1->SetEnableParallelElement(enable_parallel_element_flush_strategy >
                                      0);
  manager_1->enable_level_order_traversing_ =
      (enable_parallel_element_flush_strategy &
       Element::kFlagLevelOrderParallel) > 0;

  auto config = lepus::Value(lepus::Dictionary::Create());
  config.SetProperty(base::String("hydrateID"), lepus::Value("hydrateID"));
  config.SetProperty(base::String("dirtyID"), lepus::Value("dirtyID"));

  auto fiber_element = manager->CreateFiberView();
  fiber_element->SetConfig(config);

  auto page_node = lepus::Value(
      TreeResolver::CloneElementRecursively(fiber_element.get(), true));
  fml::RefPtr<PageElement> page_node_ref =
      fml::static_ref_ptr_cast<PageElement>(page_node.RefCounted());
  page_node_ref->AttachToElementManager(
      manager_1, tasm_1->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME), true);
  ElementBundle element_bundle = ElementBundle(std::move(page_node));
  template_bundle.SetElementBundle(std::move(element_bundle));

  EXPECT_TRUE(template_bundle.GetContainsElementTree());
  EXPECT_TRUE(template_bundle.GetElementBundle().GetPageNode().IsRefCounted());

  ElementBundle cloned_element_bundle =
      template_bundle.GetElementBundle().DeepClone();

  EXPECT_TRUE(cloned_element_bundle.IsValid());
  EXPECT_TRUE(cloned_element_bundle.GetPageNode().IsRefCounted());

  page_node = lepus::Value(
      TreeResolver::CloneElementRecursively(fiber_element.get(), true));
  ElementBundle element_bundle_1 = ElementBundle(std::move(page_node));
  EXPECT_TRUE(element_bundle_1.IsValid());
  EXPECT_TRUE(element_bundle_1.GetPageNode().IsRefCounted());

  auto test_entry_0 = std::make_shared<TemplateEntry>();
  test_entry_0->InitWithTemplateBundle(tasm.get(), std::move(template_bundle));
  EXPECT_TRUE(
      test_entry_0->GetCompleteTemplateBundle()->GetContainsElementTree());

  ElementBundle element_bundle_invalid = ElementBundle(lepus::Value());
  ElementBundle cloned_element_bundle_invalid =
      element_bundle_invalid.DeepClone();
  EXPECT_FALSE(element_bundle_invalid.IsValid());
  EXPECT_FALSE(cloned_element_bundle_invalid.IsValid());
}

TEST_P(FiberElementTest, ElementBundleTest01) {
  // prepare environment for copied element
  LynxEnvConfig lynx_env_config_1(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
  auto tasm_mediator_1 = std::make_shared<
      ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
  auto unique_manager_1 = std::make_unique<lynx::tasm::ElementManager>(
      std::make_unique<FiberMockPaintingContext>(), tasm_mediator_1.get(),
      lynx_env_config_1);
  auto manager_1 = unique_manager_1.get();
  auto tasm_1 = std::make_shared<lynx::tasm::TemplateAssembler>(
      *tasm_mediator_1.get(), std::move(unique_manager_1),
      tasm_mediator_1.get(), 0);
  auto test_entry_1 = std::make_shared<TemplateEntry>();
  tasm_1->template_entries_.insert({"test_entry", test_entry_1});
  auto config_1 = std::make_shared<PageConfig>();
  config_1->SetEnableZIndex(true);
  manager_1->SetConfig(config_1);
  tasm_1->page_config_ = config_1;
  if (thread_strategy == 0) {
    manager_1->SetThreadStrategy(base::ThreadStrategyForRendering::ALL_ON_UI);
  } else {
    manager_1->SetThreadStrategy(
        base::ThreadStrategyForRendering::MULTI_THREADS);
  }
  manager_1->SetEnableParallelElement(enable_parallel_element_flush_strategy >
                                      0);
  manager_1->enable_level_order_traversing_ =
      (enable_parallel_element_flush_strategy &
       Element::kFlagLevelOrderParallel) > 0;

  auto config = lepus::Value(lepus::Dictionary::Create());
  config.SetProperty(base::String("hydrateID"), lepus::Value("hydrateID"));
  config.SetProperty(base::String("dirtyID"), lepus::Value("dirtyID"));

  auto current_page = manager->CreateFiberPage("page", 11);

  auto current_container = manager->CreateFiberView();
  current_page->InsertNode(current_container);

  auto current_child1 = manager->CreateFiberView();
  current_container->InsertNode(current_child1);

  auto current_child2 = manager->CreateFiberView();
  current_container->InsertNode(current_child2);

  auto cloned_page_node = lepus::Value(
      TreeResolver::CloneElementRecursively(current_page.get(), true));
  fml::RefPtr<FiberElement> cloned_page_node_ref =
      fml::static_ref_ptr_cast<FiberElement>(cloned_page_node.RefCounted());
  TreeResolver::AttachRootToElementManager(
      cloned_page_node_ref, manager_1,
      tasm_1->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME), true);

  EXPECT_TRUE(manager_1->node_manager_->Get(current_page->impl_id()) !=
              nullptr);
  EXPECT_TRUE(manager_1->node_manager_->Get(current_container->impl_id()) !=
              nullptr);
  EXPECT_TRUE(manager_1->node_manager_->Get(current_child1->impl_id()) !=
              nullptr);
  EXPECT_TRUE(manager_1->node_manager_->Get(current_child2->impl_id()) !=
              nullptr);
}

TEST_P(FiberElementTest, ElementBundleTest02) {
  // construct css fragment
  StyleMap indexAttributes;
  CSSParserConfigs configs;
  auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);

  CSSParserTokenMap indexTokensMap;
  const std::vector<int32_t> dependent_ids;

  // mock keyframes
  // raw keyframes
  constexpr const char* keyframe_name = "ani-img-in";

  CSSRawKeyframesContent raw_keyframes;

  {
    RawStyleMap* raw_attrs_0 = new RawStyleMap();
    raw_attrs_0->insert_or_assign(CSSPropertyID::kPropertyIDTransform,
                                  CSSValue::MakePlainString("scale(0, 0)"));
    raw_attrs_0->insert_or_assign(CSSPropertyID::kPropertyIDOpacity,
                                  CSSValue(0.0, CSSValuePattern::NUMBER));
    std::shared_ptr<RawStyleMap> raw_attrs_ptr0(raw_attrs_0);
    raw_keyframes.insert(
        std::pair<float, std::shared_ptr<RawStyleMap>>(0.0f, raw_attrs_ptr0));

    RawStyleMap* raw_attrs_1 = new RawStyleMap();
    raw_attrs_1->insert_or_assign(CSSPropertyID::kPropertyIDTransform,
                                  CSSValue::MakePlainString("scale(1, 1)"));
    raw_attrs_1->insert_or_assign(CSSPropertyID::kPropertyIDOpacity,
                                  CSSValue(1.0, CSSValuePattern::NUMBER));
    std::shared_ptr<RawStyleMap> raw_attrs_ptr1(raw_attrs_1);
    raw_keyframes.insert(
        std::pair<float, std::shared_ptr<RawStyleMap>>(1.0f, raw_attrs_ptr1));
  }

  CSSKeyframesToken* token = new CSSKeyframesToken(configs);
  token->SetRawKeyframesContent(std::move(raw_keyframes));

  // parsed keyframes
  CSSKeyframesContent map;
  StyleMap* attrs0 = new StyleMap();
  map.insert(std::pair<float, std::shared_ptr<StyleMap>>(0.0f, attrs0));
  StyleMap* attrs1 = new StyleMap();
  map.insert(std::pair<float, std::shared_ptr<StyleMap>>(1.0f, attrs1));
  token->SetKeyframesContent(std::move(map));

  fml::RefPtr<CSSKeyframesToken> token_ptr = fml::AdoptRef(token);
  CSSKeyframesTokenMap keyframes;
  keyframes.insert({keyframe_name, std::move(token_ptr)});

  // class .recommend-ani-image-in
  {
    auto id = CSSPropertyID::kPropertyIDAnimation;
    auto impl = lepus::Value("ani-img-in 100ms linear 0.08ms normal both");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".recommend-ani-image-in";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  // mock fontfaces
  CSSFontFaceRuleMap fontfaces;
  std::vector<std::shared_ptr<CSSFontFaceRule>> face_token_list;
  CSSFontFaceRule* face_token = new CSSFontFaceRule();
  CSSFontTokenAddAttribute(face_token, "font-family", "font-base64");
  CSSFontTokenAddAttribute(
      face_token, "src",
      "url(data:application/x-font-woff;charset=utf-8;base64,test...)");
  std::shared_ptr<CSSFontFaceRule> face_token_ptr(face_token);
  face_token_list.emplace_back(face_token_ptr);
  fontfaces.insert(
      std::pair<std::string, std::vector<std::shared_ptr<CSSFontFaceRule>>>(
          "font-base64", face_token_list));

  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokensMap, keyframes, fontfaces);

  auto config = lepus::Value(lepus::Dictionary::Create());
  config.SetProperty(base::String("hydrateID"), lepus::Value("hydrateID"));
  config.SetProperty(base::String("dirtyID"), lepus::Value("dirtyID"));

  auto current_page = manager->CreateFiberPage("page", 11);
  current_page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  auto current_container = manager->CreateFiberView();
  current_page->InsertNode(current_container);

  auto current_child1 = manager->CreateFiberView();
  current_container->InsertNode(current_child1);
  current_child1->enable_new_animator_ = true;
  current_child1->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(current_page->impl_id()));
  base::String clazz_name("recommend-ani-image-in");
  current_child1->SetClass(clazz_name);

  current_page->FlushActionsAsRoot();
  EXPECT_TRUE(!current_child1->computed_css_style()->animation_data().empty());

  // Prepare environment for cloned element
  LynxEnvConfig lynx_env_config_1(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
  auto tasm_mediator_1 = std::make_shared<
      ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
  auto unique_manager_1 = std::make_unique<lynx::tasm::ElementManager>(
      std::make_unique<FiberMockPaintingContext>(), tasm_mediator_1.get(),
      lynx_env_config_1);
  auto manager_1 = unique_manager_1.get();
  auto tasm_1 = std::make_shared<lynx::tasm::TemplateAssembler>(
      *tasm_mediator_1.get(), std::move(unique_manager_1),
      tasm_mediator_1.get(), 0);
  auto test_entry_1 = std::make_shared<TemplateEntry>();
  tasm_1->template_entries_.insert({"test_entry", test_entry_1});
  auto config_1 = std::make_shared<PageConfig>();
  config_1->SetEnableZIndex(true);
  manager_1->SetConfig(config_1);
  tasm_1->page_config_ = config_1;
  if (thread_strategy == 0) {
    manager_1->SetThreadStrategy(base::ThreadStrategyForRendering::ALL_ON_UI);
  } else {
    manager_1->SetThreadStrategy(
        base::ThreadStrategyForRendering::MULTI_THREADS);
  }
  manager_1->SetEnableParallelElement(enable_parallel_element_flush_strategy >
                                      0);
  manager_1->enable_level_order_traversing_ =
      (enable_parallel_element_flush_strategy &
       Element::kFlagLevelOrderParallel) > 0;

  auto cloned_page_node = lepus::Value(
      TreeResolver::CloneElementRecursively(current_page.get(), true));
  fml::RefPtr<FiberElement> cloned_page_node_ref =
      fml::static_ref_ptr_cast<FiberElement>(cloned_page_node.RefCounted());
  TreeResolver::AttachRootToElementManager(
      cloned_page_node_ref, manager_1,
      tasm_1->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME), true);

  EXPECT_TRUE(manager_1->node_manager_->Get(current_page->impl_id()) !=
              nullptr);
  EXPECT_TRUE(manager_1->node_manager_->Get(current_container->impl_id()) !=
              nullptr);
  EXPECT_TRUE(manager_1->node_manager_->Get(current_child1->impl_id()) !=
              nullptr);

  auto cloned_page = static_cast<FiberElement*>(
      manager_1->node_manager_->Get(current_page->impl_id()));
  cloned_page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());
  EXPECT_TRUE((cloned_page->dirty_ & FiberElement::kDirtyCloned) > 0);

  auto cloned_child1 = static_cast<FiberElement*>(
      manager_1->node_manager_->Get(current_child1->impl_id()));
  EXPECT_TRUE((cloned_child1->dirty_ & FiberElement::kDirtyCloned) > 0);
  EXPECT_TRUE(cloned_child1->computed_css_style()->animation_data().empty());

  cloned_page->FlushActionsAsRoot();
  EXPECT_TRUE(!cloned_child1->computed_css_style()->animation_data().empty());
  EXPECT_TRUE((cloned_child1->dirty_ & FiberElement::kDirtyCloned) == 0);
  EXPECT_TRUE((cloned_page->dirty_ & FiberElement::kDirtyCloned) == 0);
}

TEST_P(FiberElementTest, TestGetParentComponentElement) {
  // parent
  auto page = manager->CreateFiberPage("page", 11);

  // child component
  base::String component_id("21");
  int32_t css_id = 100;
  base::String entry_name("__Card__");
  base::String component_name("TestComp");
  base::String path("/index/components/TestComp");

  auto comp = manager->CreateFiberComponent(component_id, css_id, entry_name,
                                            component_name, path);

  page->InsertNode(comp);

  auto element = manager->CreateFiberView();
  element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(comp->impl_id()));
  comp->InsertNode(element);

  page->FlushActionsAsRoot();

  auto* element_parent_component = element->GetParentComponentElement();
  EXPECT_TRUE(element_parent_component == comp.get());
  EXPECT_TRUE(element->InComponent());

  page->RemoveNode(comp);

  // mock to force to release Component
  comp = nullptr;

  element_parent_component = element->GetParentComponentElement();
  EXPECT_TRUE(element_parent_component == nullptr);
  EXPECT_TRUE(!element->InComponent());
}

TEST_P(FiberElementTest, EventTest0) {
  // page
  auto page = manager->CreateFiberPage("page", 11);

  page->SetJSEventHandler("tap", "bindEvent", "onTap");
  page->SetJSEventHandler("xxxx", "global-bindEvent", "onTap");

  EXPECT_TRUE(!page->event_map().empty());
  EXPECT_TRUE(!page->global_bind_event_map().empty());

  page->ResetDataModel();
  EXPECT_TRUE(page->event_map().empty());
  EXPECT_TRUE(page->global_bind_event_map().empty());
}

TEST_P(FiberElementTest, EventTest1) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  tasm->EnsureTouchEventHandler();

  // css related
  StyleMap indexAttributes;

  CSSParserTokenMap indexTokensMap;
  CSSParserConfigs configs;
  // class .root
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDColor;
    auto impl = lepus::Value("blue");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".root";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  // class .ani
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDColor;
    auto impl = lepus::Value("red");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".ani";
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

  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  auto element0 = manager->CreateFiberNode("view");
  element0->parent_component_element_ = page.get();
  // element0->SetClass("root");
  element0->SetJSEventHandler("customEvent", "global-bindEvent", "onEvent");
  page->InsertNode(element0);

  auto element1 = manager->CreateFiberNode("view");
  element1->parent_component_element_ = page.get();
  // element1->SetClass("root");
  element1->SetJSEventHandler("customEvent", "global-bindEvent", "onEvent");
  page->InsertNode(element1);

  auto element2 = manager->CreateFiberNode("view");
  element2->parent_component_element_ = page.get();
  // element2->SetClass("root");
  element2->SetJSEventHandler("customEvent", "global-bindEvent", "onEvent");
  page->InsertNode(element2);

  auto element3 = manager->CreateFiberNode("view");
  element3->parent_component_element_ = page.get();
  // element3->SetClass("root");
  element3->SetJSEventHandler("customEvent", "global-bindEvent", "onEvent");
  page->InsertNode(element3);

  auto element4 = manager->CreateFiberNode("view");
  element4->parent_component_element_ = page.get();
  // element4->SetClass("root");
  element4->SetJSEventHandler("customEvent", "global-bindEvent", "onEvent");
  page->InsertNode(element4);

  auto element5 = manager->CreateFiberNode("view");
  element5->parent_component_element_ = page.get();
  // element5->SetClass("root");
  element5->SetJSEventHandler("customEvent", "global-bindEvent", "onEvent");
  page->InsertNode(element5);

  auto element6 = manager->CreateFiberNode("view");
  element6->parent_component_element_ = page.get();
  // element6->SetClass("root");
  element6->SetJSEventHandler("customEvent", "global-bindEvent", "onEvent");
  page->InsertNode(element6);

  auto element7 = manager->CreateFiberNode("view");
  element7->parent_component_element_ = page.get();
  // element7->SetClass("root");
  element7->SetJSEventHandler("customEvent", "global-bindEvent", "onEvent");
  page->InsertNode(element7);

  auto element8 = manager->CreateFiberNode("view");
  element8->parent_component_element_ = page.get();
  // element8->SetClass("root");
  element8->SetJSEventHandler("customEvent", "global-bindEvent", "onEvent");
  page->InsertNode(element8);

  auto element9 = manager->CreateFiberNode("view");
  element9->parent_component_element_ = page.get();
  // element9->SetClass("root");
  element9->SetJSEventHandler("customEvent", "global-bindEvent", "onEvent");
  page->InsertNode(element9);

  auto element10 = manager->CreateFiberNode("view");
  element10->parent_component_element_ = page.get();
  // element10->SetClass("root");
  element10->SetJSEventHandler("customEvent", "global-bindEvent", "onEvent");
  page->InsertNode(element10);

  page->FlushActionsAsRoot();

  EXPECT_TRUE(manager->global_bind_name_to_ids_["customEvent"].size() == 11);
}

TEST_P(FiberElementTest, TestGenerateResponseChain0) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableCSSInheritance(true);
  std::unordered_set<CSSPropertyID> list = {kPropertyIDColor,
                                            kPropertyIDFontSize};
  config->SetCustomCSSInheritList(std::move(list));
  manager->SetConfig(config);
  tasm->EnsureTouchEventHandler();

  // css related
  StyleMap indexAttributes;

  CSSParserTokenMap indexTokensMap;
  CSSParserConfigs configs;
  // class .root
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDColor;
    auto impl = lepus::Value("blue");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".root";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  // class .ani
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDColor;
    auto impl = lepus::Value("red");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".ani";
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

  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  auto element0 = manager->CreateFiberNode("view");
  element0->parent_component_element_ = page.get();
  element0->SetAttribute("enable-layout", lepus::Value("false"));
  element0->SetClass("root");
  page->InsertNode(element0);

  auto element00 = manager->CreateFiberNode("view");
  element00->parent_component_element_ = page.get();
  element00->SetAttribute("enable-layout", lepus::Value("false"));
  element00->SetClass("ani");
  element0->InsertNode(element00);

  auto text = manager->CreateFiberText("text");
  text->parent_component_element_ = page.get();
  element00->InsertNode(text);

  auto raw_text = manager->CreateFiberRawText();
  text->InsertNode(raw_text);

  page->FlushActionsAsRoot();

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* element0_painting_node =
      painting_context->node_map_.at(element0->impl_id()).get();
  auto element0_props = element0_painting_node->props_;

  auto* element00_painting_node =
      painting_context->node_map_.at(element00->impl_id()).get();
  auto element00_props = element00_painting_node->props_;

  auto* text_painting_node =
      painting_context->node_map_.at(text->impl_id()).get();

  auto text_props = text_painting_node->props_;

  // remove inherit styles
  element00->RemoveAllClass();
  page->FlushActionsAsRoot();
  EXPECT_FALSE(
      tasm->touch_event_handler_
          ->GenerateResponseChain(nullptr, text->impl_id(), EventOption())
          .empty());

  element0_props = element0_painting_node->props_;
  text_props = text_painting_node->props_;

  element0->RemoveNode(element00);
  page->FlushActionsAsRoot();

  EXPECT_TRUE(
      tasm->touch_event_handler_
          ->GenerateResponseChain(nullptr, text->impl_id(), EventOption())
          .empty());
  EXPECT_TRUE(raw_text->IsAttached());
}

TEST_P(FiberElementTest, TestGenerateResponseChain1) {
  tasm->EnsureTouchEventHandler();

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

  // child
  auto fiber_element = manager->CreateFiberView();
  fiber_element->parent_component_element_ = page.get();
  page->InsertNode(fiber_element);
  fiber_element->SetClass("test01");
  // force the element to overflow hidden
  fiber_element->computed_css_style()->SetOverflowDefaultVisible(false);

  page->FlushActionsAsRoot();

  // child0
  auto fiber_element_0 = manager->CreateFiberView();
  fiber_element_0->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_0);
  fiber_element_0->SetClass("test01");
  // force the element to overflow visible
  fiber_element_0->computed_css_style()->SetOverflowDefaultVisible(true);

  page->FlushActionsAsRoot();

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

  EXPECT_FALSE(tasm->touch_event_handler_
                   ->GenerateResponseChain(nullptr, comp.get(), EventOption())
                   .empty());

  page->RemoveNode(list);
  page->FlushActionsAsRoot();

  EXPECT_TRUE(tasm->touch_event_handler_
                  ->GenerateResponseChain(nullptr, comp.get(), EventOption())
                  .empty());
}

TEST_P(FiberElementTest, TestGenerateResponseChain2) {
  tasm->EnsureTouchEventHandler();

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

  // child
  auto fiber_element = manager->CreateFiberView();
  fiber_element->parent_component_element_ = page.get();
  page->InsertNode(fiber_element);
  fiber_element->SetClass("test01");
  // force the element to overflow hidden
  fiber_element->computed_css_style()->SetOverflowDefaultVisible(false);

  page->FlushActionsAsRoot();

  // child0
  auto fiber_element_0 = manager->CreateFiberView();
  fiber_element_0->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_0);
  fiber_element_0->SetClass("test01");
  // force the element to overflow visible
  fiber_element_0->computed_css_style()->SetOverflowDefaultVisible(true);

  page->FlushActionsAsRoot();

  // child1
  auto fiber_element_1 = manager->CreateFiberView();
  fiber_element_1->parent_component_element_ = page.get();
  fiber_element_1->SetClass("test01");
  // force the element to overflow visible
  fiber_element_1->computed_css_style()->SetOverflowDefaultVisible(true);

  // child2
  auto fiber_element_2 = manager->CreateFiberView();
  fiber_element_2->parent_component_element_ = page.get();
  fiber_element_2->SetClass("test01");
  // force the element to overflow visible
  fiber_element_2->computed_css_style()->SetOverflowDefaultVisible(true);
  fiber_element_2->SetStyle(CSSPropertyID::kPropertyIDPosition,
                            lepus::Value("fixed"));
  fiber_element_1->InsertNode(fiber_element_2);

  auto scroll_view = manager->CreateFiberScrollView("scroll-view");
  scroll_view->InsertNode(fiber_element_1);
  page->InsertNode(scroll_view);

  page->FlushActionsAsRoot();

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

  EXPECT_FALSE(tasm->touch_event_handler_
                   ->GenerateResponseChain(nullptr, comp.get(), EventOption())
                   .empty());

  EventOption options;
  options.bubbles_ = true;
  auto chain = tasm->touch_event_handler_->GenerateResponseChain(
      tasm->page_proxy(), fiber_element_2->impl_id(), options);
  EXPECT_EQ(chain.size(), 4);
  EXPECT_EQ(chain[0], fiber_element_2.get());
  EXPECT_EQ(chain[1], fiber_element_1.get());
  EXPECT_EQ(chain[2], scroll_view.get());
  EXPECT_EQ(chain[3], page.get());

  page->RemoveNode(list);
  page->FlushActionsAsRoot();

  EXPECT_TRUE(tasm->touch_event_handler_
                  ->GenerateResponseChain(nullptr, comp.get(), EventOption())
                  .empty());
}

TEST_P(FiberElementTest, TestGenerateResponseChain3) {
  tasm->EnsureTouchEventHandler();
  manager->config_->enable_fiber_arch_ = false;

  // parent
  auto page = manager->CreateFiberPage("page", 11);

  // child
  auto fiber_element = manager->CreateFiberView();
  fiber_element->parent_component_element_ = page.get();
  page->InsertNode(fiber_element);
  fiber_element->SetClass("test01");
  // force the element to overflow hidden
  fiber_element->computed_css_style()->SetOverflowDefaultVisible(false);

  page->FlushActionsAsRoot();

  // child0
  auto fiber_element_0 = manager->CreateFiberView();
  fiber_element_0->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_0);
  fiber_element_0->SetClass("test01");
  // force the element to overflow visible
  fiber_element_0->computed_css_style()->SetOverflowDefaultVisible(true);

  page->FlushActionsAsRoot();

  // child1
  auto fiber_element_1 = manager->CreateFiberView();
  fiber_element_1->parent_component_element_ = page.get();
  fiber_element_1->SetClass("test01");
  // force the element to overflow visible
  fiber_element_1->computed_css_style()->SetOverflowDefaultVisible(true);

  // child2
  auto fiber_element_2 = manager->CreateFiberView();
  fiber_element_2->parent_component_element_ = page.get();
  fiber_element_2->SetClass("test01");
  // force the element to overflow visible
  fiber_element_2->computed_css_style()->SetOverflowDefaultVisible(true);
  fiber_element_2->SetStyle(CSSPropertyID::kPropertyIDPosition,
                            lepus::Value("fixed"));
  fiber_element_1->InsertNode(fiber_element_2);

  auto scroll_view = manager->CreateFiberScrollView("scroll-view");
  scroll_view->InsertNode(fiber_element_1);
  page->InsertNode(scroll_view);

  page->FlushActionsAsRoot();

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

  EventOption options;
  options.bubbles_ = true;
  auto chain = tasm->touch_event_handler_->GenerateResponseChain(
      tasm->page_proxy(), fiber_element_2->impl_id(), options);
  EXPECT_EQ(chain.size(), 2);
  EXPECT_EQ(chain[0], fiber_element_2.get());
  EXPECT_EQ(chain[1], page.get());
}

TEST_P(FiberElementTest, TestGenerateResponseChain4) {
  tasm->EnsureTouchEventHandler();
  manager->config_->enable_fiber_arch_ = false;

  // parent
  auto page = manager->CreateFiberPage("page", 11);
  page->SetStyle(CSSPropertyID::kPropertyIDPosition, lepus::Value("fixed"));

  // child
  auto fiber_element = manager->CreateFiberView();
  fiber_element->parent_component_element_ = page.get();
  page->InsertNode(fiber_element);
  fiber_element->SetClass("test01");
  // force the element to overflow hidden
  fiber_element->computed_css_style()->SetOverflowDefaultVisible(false);

  page->FlushActionsAsRoot();

  // child0
  auto fiber_element_0 = manager->CreateFiberView();
  fiber_element_0->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_0);
  fiber_element_0->SetClass("test01");
  // force the element to overflow visible
  fiber_element_0->computed_css_style()->SetOverflowDefaultVisible(true);

  page->FlushActionsAsRoot();

  // child1
  auto fiber_element_1 = manager->CreateFiberView();
  fiber_element_1->parent_component_element_ = page.get();
  fiber_element_1->SetClass("test01");
  // force the element to overflow visible
  fiber_element_1->computed_css_style()->SetOverflowDefaultVisible(true);

  // child2
  auto fiber_element_2 = manager->CreateFiberView();
  fiber_element_2->parent_component_element_ = page.get();
  fiber_element_2->SetClass("test01");
  // force the element to overflow visible
  fiber_element_2->computed_css_style()->SetOverflowDefaultVisible(true);
  fiber_element_2->SetStyle(CSSPropertyID::kPropertyIDPosition,
                            lepus::Value("fixed"));
  fiber_element_1->InsertNode(fiber_element_2);

  auto scroll_view = manager->CreateFiberScrollView("scroll-view");
  scroll_view->InsertNode(fiber_element_1);
  page->InsertNode(scroll_view);

  page->FlushActionsAsRoot();

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

  EventOption options;
  options.bubbles_ = true;
  auto chain = tasm->touch_event_handler_->GenerateResponseChain(
      tasm->page_proxy(), fiber_element_2->impl_id(), options);
  EXPECT_EQ(chain.size(), 2);
  EXPECT_EQ(chain[0], fiber_element_2.get());
  EXPECT_EQ(chain[1], page.get());

  chain = tasm->touch_event_handler_->GenerateResponseChain(
      tasm->page_proxy(), page->impl_id(), options);
  EXPECT_EQ(chain.size(), 1);
  EXPECT_EQ(chain[0], page.get());
}

TEST_P(FiberElementTest, AttributeTiming) {
  // page
  auto page = manager->CreateFiberPage("page", 11);

  auto parent = manager->CreateFiberView();
  parent->SetAttribute("__lynx_timing_flag", lepus::Value("attr_1"));

  auto child = manager->CreateFiberView();
  child->SetAttribute("__lynx_timing_flag", lepus::Value("attr_2"));

  auto child2 = manager->CreateFiberView();
  child2->SetAttribute("__lynx_timing_flag", lepus::Value("attr_3"));

  page->InsertNode(parent);
  parent->InsertNode(child);
  parent->InsertNode(child2);

  page->FlushActionsAsRoot();

  auto flag_list = manager->ObtainTimingFlagList();

  EXPECT_TRUE(flag_list.size() == 3);
  EXPECT_TRUE(std::find(flag_list.begin(), flag_list.end(), "attr_1") !=
              flag_list.end());
  EXPECT_TRUE(std::find(flag_list.begin(), flag_list.end(), "attr_2") !=
              flag_list.end());
  EXPECT_TRUE(std::find(flag_list.begin(), flag_list.end(), "attr_3") !=
              flag_list.end());
}

TEST_P(FiberElementTest, AttributeTiming1) {
  // page
  auto page = manager->CreateFiberPage("page", 11);

  auto parent = manager->CreateFiberView();
  parent->SetAttribute("__lynx_timing_flag", lepus::Value("attr_1"));

  auto child = manager->CreateFiberView();
  child->SetAttribute("__lynx_timing_flag", lepus::Value("attr_2"));

  auto child2 = manager->CreateFiberView();
  child2->SetAttribute("__lynx_timing_flag", lepus::Value("attr_3"));

  page->InsertNode(parent);
  parent->InsertNode(child);
  parent->InsertNode(child2);

  page->FlushActionsAsRoot();

  auto flag_list = manager->ObtainTimingFlagList();

  EXPECT_TRUE(flag_list.size() == 3);
  EXPECT_TRUE(std::find(flag_list.begin(), flag_list.end(), "attr_1") !=
              flag_list.end());
  EXPECT_TRUE(std::find(flag_list.begin(), flag_list.end(), "attr_2") !=
              flag_list.end());
  EXPECT_TRUE(std::find(flag_list.begin(), flag_list.end(), "attr_3") !=
              flag_list.end());

  auto child4 = manager->CreateFiberView();
  child4->SetAttribute("__lynx_timing_flag", lepus::Value("attr_4"));
  parent->InsertNode(child4);

  page->FlushActionsAsRoot();

  flag_list = manager->ObtainTimingFlagList();
  EXPECT_TRUE(flag_list.size() == 1);
  EXPECT_TRUE(std::find(flag_list.begin(), flag_list.end(), "attr_4") !=
              flag_list.end());
}

TEST_P(FiberElementTest, TestDirtyPropagateInherited0) {
  manager->config_->css_configs_.enable_css_inheritance_ = true;
  // page
  auto page = manager->CreateFiberPage("page", 11);

  auto parent = manager->CreateFiberView();
  parent->SetAttribute("__lynx_timing_flag", lepus::Value("attr_1"));

  auto child = manager->CreateFiberView();
  child->SetAttribute("__lynx_timing_flag", lepus::Value("attr_2"));

  auto child2 = manager->CreateFiberView();
  child2->SetAttribute("__lynx_timing_flag", lepus::Value("attr_3"));

  page->InsertNode(parent);
  parent->InsertNode(child);
  parent->InsertNode(child2);

  EXPECT_FALSE(page->dirty_ & FiberElement::kDirtyPropagateInherited);
  EXPECT_TRUE(parent->dirty_ & FiberElement::kDirtyPropagateInherited);
  EXPECT_TRUE(child->dirty_ & FiberElement::kDirtyPropagateInherited);
  EXPECT_TRUE(child2->dirty_ & FiberElement::kDirtyPropagateInherited);
}

TEST_P(FiberElementTest, TestDirtyPropagateInherited1) {
  manager->config_->css_configs_.enable_css_inheritance_ = false;
  // page
  auto page = manager->CreateFiberPage("page", 11);

  auto parent = manager->CreateFiberView();
  parent->SetAttribute("__lynx_timing_flag", lepus::Value("attr_1"));

  auto child = manager->CreateFiberView();
  child->SetAttribute("__lynx_timing_flag", lepus::Value("attr_2"));

  auto child2 = manager->CreateFiberView();
  child2->SetAttribute("__lynx_timing_flag", lepus::Value("attr_3"));

  page->InsertNode(parent);
  parent->InsertNode(child);
  parent->InsertNode(child2);

  EXPECT_FALSE(page->dirty_ & FiberElement::kDirtyPropagateInherited);
  EXPECT_FALSE(parent->dirty_ & FiberElement::kDirtyPropagateInherited);
  EXPECT_FALSE(child->dirty_ & FiberElement::kDirtyPropagateInherited);
  EXPECT_FALSE(child2->dirty_ & FiberElement::kDirtyPropagateInherited);
}

TEST_P(FiberElementTest, TestFlushRequiredPropagateWithInheritance) {
  manager->config_->css_configs_.enable_css_inheritance_ = true;

  // css related
  StyleMap indexAttributes;

  CSSParserTokenMap indexTokensMap;
  CSSParserConfigs configs;

  // class .root
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDColor;
    auto impl = lepus::Value("blue");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".root";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  // class .component-parent-class-1
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDColor;
    auto impl = lepus::Value("blue");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".component-parent-class-1";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  // class .component-parent-class-2
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDFontSize;
    auto impl = lepus::Value("20px");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".component-parent-class-2";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  // class .component-class-1
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDColor;
    auto impl = lepus::Value("red");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".component-class-1";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  // class .component-class-2
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDWidth;
    auto impl = lepus::Value("20px");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".component-class-2";
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

  // page
  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // component-parent
  auto parent = manager->CreateFiberView();
  parent->parent_component_element_ = page.get();
  parent->SetClass("component-parent-class-1");
  page->InsertNode(parent);

  // component
  auto comp = manager->CreateFiberView();
  comp->parent_component_element_ = page.get();
  parent->InsertNode(comp);

  // view
  auto view = manager->CreateFiberView();
  view->parent_component_element_ = page.get();
  view->SetClass("component-class-1");
  comp->InsertNode(view);

  page->FlushActionsAsRoot();
  EXPECT_FALSE(page->flush_required_);
  EXPECT_FALSE(parent->flush_required_);
  EXPECT_FALSE(comp->flush_required_);
  EXPECT_FALSE(view->flush_required_);

  ClassList class_input = {"component-parent-class-1",
                           "component-parent-class-2"};
  parent->SetClasses(std::move(class_input));

  ClassList view_input_2 = {"component-class-1", "component-class-2"};
  view->SetClasses(std::move(view_input_2));

  page->FlushActionsAsRoot();
  EXPECT_FALSE(page->flush_required_);
  EXPECT_FALSE(parent->flush_required_);
  EXPECT_FALSE(comp->flush_required_);
  EXPECT_FALSE(view->flush_required_);

  // Trigger style reset
  parent->SetClass("component-parent-class-1");
  view->SetClass("component-class-1");
  page->FlushActionsAsRoot();
  EXPECT_FALSE(page->flush_required_);
  EXPECT_FALSE(parent->flush_required_);
  EXPECT_FALSE(comp->flush_required_);
  EXPECT_FALSE(view->flush_required_);
}

TEST_P(FiberElementTest, TestPageElementPostResolveTaskToThreadPool) {
  // page
  auto page = manager->CreateFiberPage("page", 11);
  ClassList input = {"dark", "blue", "black"};
  page->SetClasses(std::move(input));

  page->PostResolveTaskToThreadPool(false, manager->ParallelTasks());
  EXPECT_TRUE(page->IsAsyncResolveResolving());
}

TEST_P(FiberElementTest, TestAsyncResolveProperty) {
  if (enable_parallel_element_flush_strategy == 0 ||
      (enable_parallel_element_flush_strategy &
       Element::kFlagLevelOrderParallel) > 0) {
    GTEST_SKIP();
  }
  // css related
  StyleMap indexAttributes;

  CSSParserTokenMap indexTokensMap;
  CSSParserConfigs configs;

  // class .root
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDColor;
    auto impl = lepus::Value("blue");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".root";
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
  // page
  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // parent
  auto parent = manager->CreateFiberView();
  parent->parent_component_element_ = page.get();
  parent->SetClass("root");
  parent->AsyncResolveProperty();
  EXPECT_TRUE(parent->resolve_status_ ==
              FiberElement::AsyncResolveStatus::kPrepareRequested);

  page->InsertNode(parent);
  page->FlushActionsAsRoot();
  EXPECT_TRUE(parent->resolve_status_ ==
              FiberElement::AsyncResolveStatus::kUpdated);
}

TEST_P(FiberElementTest, TestAsyncResolveProperty_ReplaceElements) {
  if (enable_parallel_element_flush_strategy == 0 ||
      (enable_parallel_element_flush_strategy &
       Element::kFlagLevelOrderParallel) > 0) {
    GTEST_SKIP();
  }
  // css related
  StyleMap indexAttributes;

  CSSParserTokenMap indexTokensMap;
  CSSParserConfigs configs;

  // class .root
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDColor;
    auto impl = lepus::Value("blue");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".root";
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
  // page
  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  auto parent = manager->CreateFiberView();
  parent->parent_component_element_ = page.get();
  page->InsertNode(parent);

  auto element0 = manager->CreateFiberView();
  parent->parent_component_element_ = page.get();
  parent->SetClass("root");
  parent->InsertNode(element0);
  element0->AsyncResolveProperty();
  EXPECT_TRUE(element0->resolve_status_ >=
              FiberElement::AsyncResolveStatus::kPrepareTriggered);

  base::Vector<fml::RefPtr<FiberElement>> inserted_elements{};
  base::Vector<fml::RefPtr<FiberElement>> removed_elements{};
  inserted_elements.emplace_back(element0);
  page->ReplaceElements(inserted_elements, removed_elements, nullptr);

  page->FlushActionsAsRoot();
  EXPECT_TRUE(element0->resolve_status_ ==
              FiberElement::AsyncResolveStatus::kUpdated);
}

TEST_P(FiberElementTest, TestAsyncResolveProperty_CheckElementResolveStatus) {
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

  page->FlushActionsAsRoot();
  EXPECT_TRUE(
      page->resolve_status_ == FiberElement::AsyncResolveStatus::kUpdated ||
      page->resolve_status_ == FiberElement::AsyncResolveStatus::kCreated);
  EXPECT_TRUE(
      element0->resolve_status_ == FiberElement::AsyncResolveStatus::kUpdated ||
      element0->resolve_status_ == FiberElement::AsyncResolveStatus::kCreated);
  EXPECT_TRUE(element_before_black->resolve_status_ ==
                  FiberElement::AsyncResolveStatus::kUpdated ||
              element_before_black->resolve_status_ ==
                  FiberElement::AsyncResolveStatus::kCreated);
  EXPECT_TRUE(
      text->resolve_status_ == FiberElement::AsyncResolveStatus::kUpdated ||
      text->resolve_status_ == FiberElement::AsyncResolveStatus::kCreated);
  EXPECT_TRUE(
      element->resolve_status_ == FiberElement::AsyncResolveStatus::kUpdated ||
      element->resolve_status_ == FiberElement::AsyncResolveStatus::kCreated);
  EXPECT_TRUE(
      wrapper->resolve_status_ == FiberElement::AsyncResolveStatus::kUpdated ||
      wrapper->resolve_status_ == FiberElement::AsyncResolveStatus::kCreated);
}

TEST_P(FiberElementTest, TestAsyncResolveProperty_CheckElementResolveStatus02) {
  if (enable_parallel_element_flush_strategy == 0 ||
      (enable_parallel_element_flush_strategy &
       Element::kFlagLevelOrderParallel) > 0) {
    GTEST_SKIP();
  }

  // css related
  StyleMap indexAttributes;

  CSSParserTokenMap indexTokensMap;
  CSSParserConfigs configs;

  // class .root
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDColor;
    auto impl = lepus::Value("blue");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".root";
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

  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());
  auto element0 = manager->CreateFiberNode("view");
  element0->parent_component_element_ = page.get();
  auto element_before_black = manager->CreateFiberNode("view");
  element_before_black->parent_component_element_ = page.get();
  auto element = manager->CreateFiberNode("view");
  auto text = manager->CreateFiberNode("text");
  auto wrapper = manager->CreateFiberWrapperElement();

  wrapper->InsertNode(element);
  wrapper->InsertNode(text);

  page->InsertNode(element0);
  page->InsertNode(element_before_black);
  page->InsertNode(wrapper);

  page->FlushActionsAsRoot();
  EXPECT_TRUE(
      element0->resolve_status_ == FiberElement::AsyncResolveStatus::kUpdated ||
      element0->resolve_status_ == FiberElement::AsyncResolveStatus::kCreated);
  EXPECT_TRUE(element_before_black->resolve_status_ ==
                  FiberElement::AsyncResolveStatus::kUpdated ||
              element_before_black->resolve_status_ ==
                  FiberElement::AsyncResolveStatus::kCreated);

  element0->AsyncResolveProperty();
  // For non-dirty element, resolve_status_ should not be updated to
  // kPrepareRequested.
  EXPECT_TRUE(element0->resolve_status_ ==
              FiberElement::AsyncResolveStatus::kUpdated);
  element_before_black->SetClass("root");
  element_before_black->AsyncResolveProperty();
  // For non-dirty element, resolve_status_ should not be updated to
  // kPrepareRequested.
  // For dirty element, resolve_status_ should be updated.
  EXPECT_TRUE(element_before_black->resolve_status_ !=
                  FiberElement::AsyncResolveStatus::kCreated &&
              element_before_black->resolve_status_ !=
                  FiberElement::AsyncResolveStatus::kUpdated);
  page->FlushActionsAsRoot();
  EXPECT_TRUE(
      element0->resolve_status_ == FiberElement::AsyncResolveStatus::kUpdated ||
      element0->resolve_status_ == FiberElement::AsyncResolveStatus::kCreated);
  EXPECT_TRUE(element_before_black->resolve_status_ ==
                  FiberElement::AsyncResolveStatus::kUpdated ||
              element_before_black->resolve_status_ ==
                  FiberElement::AsyncResolveStatus::kCreated);
}

TEST_P(FiberElementTest, TestGetParentFontSize0) {
  auto page = manager->CreateFiberPage("page", 11);
  page->computed_css_style()->SetFontSize(10, 10);
  EXPECT_EQ(page->GetFontSize(), 10);

  auto view1 = manager->CreateFiberView();
  view1->computed_css_style()->SetFontSize(11, 10);
  page->InsertNode(view1);
  EXPECT_EQ(view1->GetFontSize(), 11);
  EXPECT_EQ(view1->GetParentFontSize(), 14);

  int32_t css_id = 100;
  base::String component_id("21");
  base::String entry_name("TTTT");
  base::String component_name("TestComp");
  base::String path("/index/components/TestComp");
  auto comp2 = manager->CreateFiberComponent(component_id, css_id, entry_name,
                                             component_name, path);
  comp2->computed_css_style()->SetFontSize(12, 10);
  auto view2 = manager->CreateFiberView();
  view2->computed_css_style()->SetFontSize(13, 10);
  comp2->InsertNode(view2);
  page->InsertNode(comp2);
  EXPECT_EQ(view2->GetFontSize(), 13);
  EXPECT_EQ(view2->GetParentFontSize(), 14);

  auto wrapper3 = manager->CreateFiberWrapperElement();
  auto view3 = manager->CreateFiberView();
  view3->computed_css_style()->SetFontSize(15, 10);
  wrapper3->InsertNode(view3);
  page->InsertNode(wrapper3);
  EXPECT_EQ(view3->GetFontSize(), 15);
  EXPECT_EQ(view3->GetParentFontSize(), 14);

  auto comp4 = manager->CreateFiberComponent(component_id, css_id, entry_name,
                                             component_name, path);
  comp4->MarkAsWrapperComponent();
  auto view4 = manager->CreateFiberView();
  view4->computed_css_style()->SetFontSize(17, 10);
  comp4->InsertNode(view4);
  page->InsertNode(comp4);
  EXPECT_EQ(view4->GetFontSize(), 17);
  EXPECT_EQ(view4->GetParentFontSize(), 14);
}

TEST_P(FiberElementTest, TestIsInheritable) {
  auto page = manager->CreateFiberPage("page", 11);

  std::vector<CSSPropertyID> all_props_vec = {
#define DECLARE_PROPERTY_NAME(name, c, value) kPropertyID##name,
      FOREACH_ALL_PROPERTY(DECLARE_PROPERTY_NAME)
#undef DECLARE_PROPERTY_NAME
          CSSPropertyID::kPropertyEnd};

  std::unordered_set<CSSPropertyID> all_props(all_props_vec.begin(),
                                              all_props_vec.end());
  all_props.erase(CSSPropertyID::kPropertyEnd);

  auto props = DynamicCSSStylesManager::GetInheritableProps();
  for (const auto& prop : props) {
    EXPECT_FALSE(page->IsInheritable(prop));
    all_props.erase(prop);
  }

  for (const auto& prop : all_props) {
    EXPECT_FALSE(page->IsInheritable(prop));
  }

  manager->config_->css_configs_.enable_css_inheritance_ = true;

  for (const auto& prop : props) {
    EXPECT_TRUE(page->IsInheritable(prop));
  }

  for (const auto& prop : all_props) {
    EXPECT_FALSE(page->IsInheritable(prop));
  }

  manager->config_->css_configs_.custom_inherit_list_.insert(
      CSSPropertyID::kPropertyIDFontSize);
  manager->config_->css_configs_.custom_inherit_list_.insert(
      CSSPropertyID::kPropertyIDWidth);

  for (const auto& prop : props) {
    if (prop == CSSPropertyID::kPropertyIDFontSize) {
      EXPECT_TRUE(page->IsInheritable(prop));
    } else {
      EXPECT_FALSE(page->IsInheritable(prop));
    }
  }

  for (const auto& prop : all_props) {
    if (prop == CSSPropertyID::kPropertyIDWidth) {
      EXPECT_TRUE(page->IsInheritable(prop));
    } else {
      EXPECT_FALSE(page->IsInheritable(prop));
    }
  }
}

TEST_P(FiberElementTest, TestGetParentFontSize) {
  const_cast<tasm::DynamicCSSConfigs&>(manager->GetDynamicCSSConfigs())
      .enable_css_inheritance_ = true;

  auto page = manager->CreateFiberPage("page", 11);
  page->computed_css_style()->SetFontSize(10, 10);
  EXPECT_EQ(page->GetFontSize(), 10);

  auto view1 = manager->CreateFiberView();
  view1->computed_css_style()->SetFontSize(11, 10);
  page->InsertNode(view1);
  EXPECT_EQ(view1->GetFontSize(), 11);
  EXPECT_EQ(view1->GetParentFontSize(), 10);

  int32_t css_id = 100;
  base::String component_id("21");
  base::String entry_name("TTTT");
  base::String component_name("TestComp");
  base::String path("/index/components/TestComp");
  auto comp2 = manager->CreateFiberComponent(component_id, css_id, entry_name,
                                             component_name, path);
  comp2->computed_css_style()->SetFontSize(12, 10);
  auto view2 = manager->CreateFiberView();
  view2->computed_css_style()->SetFontSize(13, 10);
  comp2->InsertNode(view2);
  page->InsertNode(comp2);
  EXPECT_EQ(view2->GetFontSize(), 13);
  EXPECT_EQ(view2->GetParentFontSize(), 12);

  auto wrapper3 = manager->CreateFiberWrapperElement();
  auto view3 = manager->CreateFiberView();
  view3->computed_css_style()->SetFontSize(15, 10);
  wrapper3->InsertNode(view3);
  page->InsertNode(wrapper3);
  EXPECT_EQ(view3->GetFontSize(), 15);
  EXPECT_EQ(view3->GetParentFontSize(), 10);

  auto comp4 = manager->CreateFiberComponent(component_id, css_id, entry_name,
                                             component_name, path);
  comp4->MarkAsWrapperComponent();
  auto view4 = manager->CreateFiberView();
  view4->computed_css_style()->SetFontSize(17, 10);
  comp4->InsertNode(view4);
  page->InsertNode(comp4);
  EXPECT_EQ(view4->GetFontSize(), 17);
  EXPECT_EQ(view4->GetParentFontSize(), 10);
}

TEST_P(FiberElementTest, TestComponentElementIDToAttribute) {
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

  // ComponentID attribute should not stop this component being layout only
  // Test has_layout_only_props_
  auto page = manager->CreateFiberPage("page", 11);
  page->InsertNode(comp);
  page->FlushActionsAsRoot();
  EXPECT_TRUE(comp->has_layout_only_props_);
}

TEST_P(FiberElementTest, TestCheckFlattenRelatedProp) {
  auto page = manager->CreateFiberPage("page", 11);
  page->CheckFlattenRelatedProp("flatten", lepus::Value(false));
  EXPECT_FALSE(page->config_flatten_);

  page->CheckFlattenRelatedProp("flatten", lepus::Value(true));
  EXPECT_TRUE(page->config_flatten_);

  page->CheckFlattenRelatedProp("flatten", lepus::Value("false"));
  EXPECT_FALSE(page->config_flatten_);

  page->CheckFlattenRelatedProp("flatten", lepus::Value("xxx"));
  EXPECT_TRUE(page->config_flatten_);

  page->CheckFlattenRelatedProp("name", lepus::Value("xxx"));
  EXPECT_TRUE(page->has_non_flatten_attrs_);

  page->has_non_flatten_attrs_ = false;
  page->CheckFlattenRelatedProp("native-interaction-enabled",
                                lepus::Value("xxx"));
  EXPECT_TRUE(page->has_non_flatten_attrs_);

  page->has_non_flatten_attrs_ = false;
  page->CheckFlattenRelatedProp("user-interaction-enabled",
                                lepus::Value("xxx"));
  EXPECT_TRUE(page->has_non_flatten_attrs_);

  page->has_non_flatten_attrs_ = false;
  page->CheckFlattenRelatedProp("overlap", lepus::Value("xxx"));
  EXPECT_TRUE(page->has_non_flatten_attrs_);

  page->has_non_flatten_attrs_ = false;
  page->CheckFlattenRelatedProp("exposure-scene", lepus::Value());
  EXPECT_FALSE(page->has_non_flatten_attrs_);

  page->has_non_flatten_attrs_ = false;
  page->CheckFlattenRelatedProp("exposure-id", lepus::Value());
  EXPECT_FALSE(page->has_non_flatten_attrs_);

  page->has_non_flatten_attrs_ = false;
  page->CheckFlattenRelatedProp("exposure-scene", lepus::Value("xxx"));
  EXPECT_TRUE(page->has_non_flatten_attrs_);

  page->has_non_flatten_attrs_ = false;
  page->CheckFlattenRelatedProp("exposure-scene", lepus::Value(1));
  EXPECT_TRUE(page->has_non_flatten_attrs_);

  page->has_non_flatten_attrs_ = false;
  page->CheckFlattenRelatedProp("exposure-id", lepus::Value("xxxxx"));
  EXPECT_TRUE(page->has_non_flatten_attrs_);

  page->has_non_flatten_attrs_ = false;
  page->CheckFlattenRelatedProp("exposure-id", lepus::Value(2));
  EXPECT_TRUE(page->has_non_flatten_attrs_);

  page->has_non_flatten_attrs_ = false;
  page->CheckFlattenRelatedProp("clip-radius", lepus::Value("true"));
  EXPECT_TRUE(page->has_non_flatten_attrs_);

  page->has_non_flatten_attrs_ = false;
  page->CheckFlattenRelatedProp("clip-radius", lepus::Value(true));
  EXPECT_TRUE(page->has_non_flatten_attrs_);

  page->CheckFlattenRelatedProp("flatten", lepus::Value(false));
  EXPECT_FALSE(page->config_flatten_);

  page->has_non_flatten_attrs_ = false;
  page->CheckFlattenRelatedProp("clip-radius", lepus::Value(true));
  EXPECT_FALSE(page->has_non_flatten_attrs_);
}

TEST_P(FiberElementTest, TestCheckHasPlaceholder) {
  auto page = manager->CreateFiberPage("page", 11);
  EXPECT_FALSE(page->has_placeholder_);

  page->CheckHasPlaceholder("placeholder", lepus::Value("xxx"));
  EXPECT_TRUE(page->has_placeholder_);

  page->has_placeholder_ = false;
  page->CheckHasPlaceholder("placeholder", lepus::Value(1));
  EXPECT_FALSE(page->has_placeholder_);

  page->CheckHasPlaceholder("placeholder", lepus::Value(false));
  EXPECT_FALSE(page->has_placeholder_);

  page->CheckHasPlaceholder("placeholder", lepus::Value(true));
  EXPECT_FALSE(page->has_placeholder_);
}

TEST_P(FiberElementTest, TestCheckHasTextSelection) {
  auto page = manager->CreateFiberPage("page", 11);
  EXPECT_FALSE(page->has_text_selection_);

  page->CheckHasTextSelection("text-selection", lepus::Value(true));
  EXPECT_TRUE(page->has_text_selection_);

  page->has_text_selection_ = false;
  page->CheckHasTextSelection("text-selection", lepus::Value(1));
  EXPECT_FALSE(page->has_text_selection_);

  page->CheckHasTextSelection("text-selection", lepus::Value(false));
  EXPECT_FALSE(page->has_text_selection_);

  page->CheckHasTextSelection("text-selection", lepus::Value("xxx"));
  EXPECT_FALSE(page->has_text_selection_);
}

TEST_P(FiberElementTest, TestCheckTriggerGlobalEvent) {
  auto page = manager->CreateFiberPage("page", 11);
  EXPECT_FALSE(page->trigger_global_event_);

  page->CheckTriggerGlobalEvent("trigger-global-event", lepus::Value(true));
  EXPECT_TRUE(page->trigger_global_event_);

  page->trigger_global_event_ = false;
  page->CheckTriggerGlobalEvent("trigger-global-event", lepus::Value(1));
  EXPECT_FALSE(page->trigger_global_event_);

  page->trigger_global_event_ = false;
  page->CheckTriggerGlobalEvent("trigger-global-event", lepus::Value(false));
  EXPECT_FALSE(page->trigger_global_event_);

  page->trigger_global_event_ = false;
  page->CheckTriggerGlobalEvent("trigger-global-event", lepus::Value("xxx"));
  EXPECT_FALSE(page->trigger_global_event_);
}

TEST_P(FiberElementTest, TestCheckGlobalBindTarget) {
  auto page = manager->CreateFiberPage("page", 11);
  EXPECT_TRUE(page->global_bind_target_set_->empty());

  page->CheckGlobalBindTarget("global-target", lepus::Value(true));
  EXPECT_TRUE(page->global_bind_target_set_->empty());

  page->CheckGlobalBindTarget("global-target", lepus::Value(1));
  EXPECT_TRUE(page->global_bind_target_set_->empty());

  page->CheckGlobalBindTarget("global-target", lepus::Value(false));
  EXPECT_TRUE(page->global_bind_target_set_->empty());

  page->CheckGlobalBindTarget("global-target", lepus::Value("xxx"));
  EXPECT_FALSE(page->global_bind_target_set_->empty());
  EXPECT_EQ(page->global_bind_target_set_->count("xxx"), 1);

  page->CheckGlobalBindTarget("global-target",
                              lepus::Value("xxxx, yyyy,zzzz,"));
  EXPECT_FALSE(page->global_bind_target_set_->empty());
  EXPECT_EQ(page->global_bind_target_set_->count("xxx"), 0);
  EXPECT_EQ(page->global_bind_target_set_->count("xxxx"), 1);
  EXPECT_EQ(page->global_bind_target_set_->count("yyyy"), 1);
  EXPECT_EQ(page->global_bind_target_set_->count("zzzz"), 1);
}

TEST_P(FiberElementTest, CheckNewAnimatorAttr) {
  auto element = manager->CreateFiberNode("view");
  base::String key("enable-new-animator");
  lepus::Value value1(true);
  element->CheckNewAnimatorAttr(key, value1);
  EXPECT_TRUE(element->enable_new_animator_);
  lepus::Value value2(false);
  element->CheckNewAnimatorAttr(key, value2);
  EXPECT_FALSE(element->enable_new_animator_);
  lepus::Value value3("true");
  element->CheckNewAnimatorAttr(key, value3);
  EXPECT_TRUE(element->enable_new_animator_);
  lepus::Value value4("false");
  element->CheckNewAnimatorAttr(key, value4);
  EXPECT_FALSE(element->enable_new_animator_);

  auto element0 = manager->CreateFiberNode("view");
  lepus::Value value5("invalid");
  element0->CheckNewAnimatorAttr(key, value5);
  EXPECT_TRUE(element0->enable_new_animator_);

  auto comp = std::shared_ptr<RadonComponent>(
      new RadonComponent(nullptr, 0, nullptr, nullptr, 0, 0, 0));
  auto element1 = manager->CreateFiberElement("view");
  element1->SetAttributeHolder(comp.get()->attribute_holder());
  lepus::Value value6(true);
  element1->CheckNewAnimatorAttr(key, value6);
  EXPECT_TRUE(element1->enable_new_animator_);

  lepus::Value value7(false);
  element1->CheckNewAnimatorAttr(key, value7);
  EXPECT_FALSE(element1->enable_new_animator_);

  lepus::Value value8("true");
  element1->CheckNewAnimatorAttr(key, value8);
  EXPECT_TRUE(element1->enable_new_animator_);

  lepus::Value value9("false");
  element1->CheckNewAnimatorAttr(key, value9);
  EXPECT_FALSE(element1->enable_new_animator_);

  lepus::Value value10("invalid");
  element1->CheckNewAnimatorAttr(key, value10);
  EXPECT_FALSE(element1->enable_new_animator_);
}

TEST_P(FiberElementTest, TestCheckTimingAttribute) {
  auto page = manager->CreateFiberPage("page", 11);
  EXPECT_TRUE(manager->attribute_timing_flag_list_.Empty());

  page->CheckTimingAttribute("__lynx_timing_flag", lepus::Value(false));
  page->CheckTimingAttribute("__lynx_timing_flag", lepus::Value("false"));
  page->CheckTimingAttribute("__lynx_timing_flag", lepus::Value(1));
  page->CheckTimingAttribute("__lynx_timing_flag", lepus::Value(true));
  page->CheckTimingAttribute("__lynx_timing_flag", lepus::Value(2.2));
  page->CheckTimingAttribute("__lynx_timing_flag", lepus::Value("xxx"));

  EXPECT_FALSE(manager->attribute_timing_flag_list_.Empty());

  auto result = manager->attribute_timing_flag_list_.PopAll();
  auto begin = result.begin();
  for (int i = 0; i < result.size(); ++i) {
    if (i == 0) {
      EXPECT_EQ(*begin, "false");
    }
    if (i == 1) {
      EXPECT_EQ(*begin, "xxx");
    }
    ++begin;
  }
}

TEST_P(FiberElementTest, RadonFiberArchFontFace) {
  //  constructor css fragment
  StyleMap indexAttributes;
  CSSParserConfigs configs;
  auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);

  // mock fontfaces
  CSSFontFaceRuleMap fontfaces;
  std::vector<std::shared_ptr<CSSFontFaceRule>> face_token_list;
  CSSFontFaceRule* face_token = new CSSFontFaceRule();
  CSSFontTokenAddAttribute(face_token, "font-family", "font-base64");
  CSSFontTokenAddAttribute(
      face_token, "src",
      "url(data:application/x-font-woff;charset=utf-8;base64,test...)");
  std::shared_ptr<CSSFontFaceRule> face_token_ptr(face_token);
  face_token_list.emplace_back(face_token_ptr);
  fontfaces.insert(
      std::pair<std::string, std::vector<std::shared_ptr<CSSFontFaceRule>>>(
          "font-base64", face_token_list));
  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSParserTokenMap indexTokensMap;

  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokensMap, keyframes, fontfaces);

  // create component
  base::String component_id("21");
  int32_t css_id = 100;
  base::String entry_name("__Card__");
  base::String component_name("TestComp");
  base::String path("/index/components/TestComp");

  auto comp = manager->CreateFiberComponent(component_id, css_id, entry_name,
                                            component_name, path);
  comp->arch_type_ = RadonArch;
  comp->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());
  EXPECT_FALSE(comp->style_sheet_->GetFontFaceRuleMap().empty());
  EXPECT_FALSE(comp->style_sheet_->HasFontFacesResolved());

  comp->PrepareForFontFaceIfNeeded();
  EXPECT_TRUE(comp->style_sheet_->HasFontFacesResolved());
}

TEST_P(FiberElementTest, MarkRenderRootElementTest) {
  auto page = manager->CreateFiberPage("page", 11);

  auto parent = manager->CreateFiberWrapperElement();
  page->InsertNode(parent);
  EXPECT_TRUE(page->render_root_element_ == nullptr);
  EXPECT_TRUE(parent->render_root_element_ == nullptr);

  lepus::Value component_at_index(10);
  lepus::Value enqueue_component;
  lepus::Value component_at_indexes;

  auto list = manager->CreateFiberList(nullptr, "list", component_at_index,
                                       enqueue_component, component_at_indexes);
  parent->InsertNode(list);
  list->disable_list_platform_implementation_ = true;
  list->enable_decoupled_list_ = true;
  list->list_mediator_ = std::make_unique<ListMediator>(list.get());
  list->batch_render_strategy_ =
      list::BatchRenderStrategy::kAsyncResolveProperty;
  EXPECT_TRUE(list->render_root_element_ == nullptr);

  base::String component_id("21");
  int32_t css_id = 100;
  base::String entry_name("__Card__");
  base::String component_name("TestComp");
  base::String path("/index/components/TestComp");

  auto comp = manager->CreateFiberComponent(component_id, css_id, entry_name,
                                            component_name, path);
  list->InsertNode(comp);
  EXPECT_TRUE(comp->render_root_element_ == comp.get());

  auto subtree_wrapper = manager->CreateFiberWrapperElement();
  comp->InsertNode(subtree_wrapper);
  EXPECT_TRUE(subtree_wrapper->render_root_element_ == comp.get());

  base::String component_id_1("22");
  auto comp_1 = manager->CreateFiberComponent(component_id_1, css_id,
                                              entry_name, component_name, path);
  list->InsertNode(comp_1);
  comp->RemoveNode(subtree_wrapper);
  comp_1->InsertNode(subtree_wrapper);
  EXPECT_TRUE(subtree_wrapper->render_root_element_ == comp_1.get());

  comp_1->RemoveNode(subtree_wrapper);
  EXPECT_TRUE(subtree_wrapper->render_root_element_ == comp_1.get());

  auto root_wrapper = manager->CreateFiberWrapperElement();
  page->InsertNode(root_wrapper);
  root_wrapper->InsertNode(subtree_wrapper);
  EXPECT_TRUE(root_wrapper->render_root_element_ == nullptr);
  EXPECT_TRUE(subtree_wrapper->render_root_element_ == nullptr);
}

TEST_P(FiberElementTest, TestBackgroundToLepus) {
  // styles for fiber_element
  //  constructor css fragment
  StyleMap index_attributes;
  CSSParserConfigs configs;
  auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);

  CSSParserTokenMap index_tokens_map;
  // class .test
  {
    auto id = CSSPropertyID::kPropertyIDBackground;
    auto impl = lepus::Value(
        "radial-gradient(71.43% 62.3% at 46.43% 36.43%, rgba(18, 229, 229, "
        "0) "
        "15%, rgba(239, 155, 255, 0.3) 56.35%, #ff6448 100%)");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".background";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    index_tokens_map.insert(std::make_pair(key, tokens));
  }

  // class .test01
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDMask;
    auto impl = lepus::Value(
        "radial-gradient(71.43% 62.3% at 46.43% 36.43%, rgba(18, 229, 229, "
        "0) "
        "15%, rgba(239, 155, 255, 0.3) 56.35%, #ff6448 100%)");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".mask";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    index_tokens_map.insert(std::make_pair(key, tokens));
  }

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap fontfaces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, index_tokens_map, keyframes, fontfaces);

  // parent
  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  for (int i = 0; i <= 10000; ++i) {
    auto view = manager->CreateFiberView();
    view->parent_component_element_ = page.get();
    view->SetClass("background");
    view->SetClass("mask");
    page->InsertNode(view);
  }

  page->FlushActionsAsRoot();
}

INSTANTIATE_TEST_SUITE_P(FiberElementTestModule, FiberElementTest,
                         ::testing::ValuesIn(fiber_element_generation_params));

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
