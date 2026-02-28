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

TEST_P(FiberElementTest, SetClass) {
  auto element = manager->CreateFiberNode("view");
  element->SetClass(".test0");
  element->SetClass(".test1");

  EXPECT_EQ(static_cast<int>(element->classes().size()), 2);

  auto class_list = element->classes();
  for (const auto& clz : class_list) {
    EXPECT_TRUE((clz.str() == ".test0") || (clz.str() == ".test1"));
  }
}

TEST_P(FiberElementTest, RemoveAllClass) {
  auto element = manager->CreateFiberNode("view");
  element->SetClass(".test0");
  element->SetClass(".test1");

  EXPECT_EQ(static_cast<int>(element->classes().size()), 2);
  element->RemoveAllClass();
  EXPECT_EQ(static_cast<int>(element->classes().size()), 0);
}

TEST_P(FiberElementTest, TestSetAndRemoveClass) {
  // constructor css fragment
  StyleMap indexAttributes;
  CSSParserConfigs parser_config;
  auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_config);

  CSSParserTokenMap indexTokensMap;
  // class .test-class
  {
    auto id = CSSPropertyID::kPropertyIDVisibility;
    auto impl = lepus::Value("hidden");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".test-class";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap fontfaces;
  auto indexFragment = std::make_unique<SharedCSSFragment>(
      1, dependent_ids, indexTokensMap, keyframes, fontfaces);

  int css_id = 11;
  auto page = manager->CreateFiberPage("page", css_id);

  page->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(page->impl_id()));

  page->set_style_sheet_manager(
      tasm->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME));

  auto style_sheet_manager = page->css_style_sheet_manager_;

  style_sheet_manager->raw_fragments_->insert(
      std::make_pair(css_id, std::move(indexFragment)));

  auto child = manager->CreateFiberNode("view");
  child->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(page->impl_id()));
  child->SetClass("test-class");
  page->InsertNodeBefore(child, nullptr);

  page->FlushActionsAsRoot();
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->platform_impl_.get());
  painting_context->Flush();

  auto* mock_painting_node_ =
      painting_context->node_map_.at(child->impl_id()).get();

  EXPECT_TRUE(mock_painting_node_->props_.size() == 1);
  std::string visibility("visibility");
  EXPECT_TRUE(mock_painting_node_->props_.at(visibility) == lepus::Value(0));

  child->RemoveAllClass();
  page->FlushActionsAsRoot();
  painting_context->Flush();

  EXPECT_TRUE(mock_painting_node_->props_.size() == 1);
  EXPECT_TRUE(mock_painting_node_->props_.at(visibility).IsEmpty());
}

TEST_P(FiberElementTest, TestCSSResolveCase01) {
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
    auto id = CSSPropertyID::kPropertyIDOpacity;
    auto impl = lepus::Value(0.6);
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".test01";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  // class .test02
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDOpacity;
    auto impl = lepus::Value(0.3);  // the same as .test
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    auto id2 = CSSPropertyID::kPropertyIDZIndex;
    auto impl2 = lepus::Value("10");
    tokens.get()->raw_attributes_[id2] =
        CSSValue(impl2, CSSValuePattern::STRING);

    std::string key = ".test02";
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
  auto fiber_element = manager->CreateFiberNode("view");

  fiber_element->parent_component_element_ = page.get();
  page->InsertNode(fiber_element);

  fiber_element->SetClass("test");

  page->FlushActionsAsRoot();

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* painting_node =
      painting_context->node_map_.at(fiber_element->impl_id()).get();

  std::string opacity_key("opacity");
  auto opacity_value = painting_node->props_.at(opacity_key);

  EXPECT_TRUE(fabs(opacity_value.Number() - 0.3) < COMPARE_EPSILON);

  // append class .test01
  fiber_element->SetClass("test01");
  page->FlushActionsAsRoot();
  painting_context->Flush();

  opacity_value = painting_node->props_.at(opacity_key);

  EXPECT_TRUE(fabs(opacity_value.Number() - 0.6) < COMPARE_EPSILON);

  // remove class .test01
  fiber_element->RemoveAllClass();

  page->FlushActionsAsRoot();
  painting_context->Flush();
  opacity_value = painting_node->props_.at(opacity_key);

  EXPECT_TRUE(opacity_value.IsEmpty());

  fiber_element->SetClass("test");
  page->FlushActionsAsRoot();
  painting_context->Flush();
  opacity_value = painting_node->props_.at(opacity_key);

  EXPECT_TRUE(fabs(opacity_value.Number() - 0.3) < COMPARE_EPSILON);

  fiber_element->SetClass("test02");
  page->FlushActionsAsRoot();  // nothing happend
  painting_context->Flush();
  opacity_value = painting_node->props_.at(opacity_key);
  EXPECT_TRUE(fabs(opacity_value.Number() - 0.3) < COMPARE_EPSILON);

  fiber_element->SetStyle(CSSPropertyID::kPropertyIDOpacity, lepus::Value(0.9));
  fiber_element->SetStyle(CSSPropertyID::kPropertyIDZIndex,
                          lepus::Value("999"));

  // remove all calss
  fiber_element->RemoveAllClass();
  page->FlushActionsAsRoot();
  painting_context->Flush();
  opacity_value = painting_node->props_.at(opacity_key);
  EXPECT_TRUE(fabs(opacity_value.Number() - 0.9) < COMPARE_EPSILON);

  std::string z_index_key = "z-index";
  auto z_index_value = painting_node->props_.at(z_index_key);
  EXPECT_TRUE(z_index_value.Number() == 999);
}

TEST_P(FiberElementTest, TestCSSResolveCase02) {
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

  fiber_element->SetClass("test");

  page->FlushActionsAsRoot();

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* painting_node =
      painting_context->node_map_.at(fiber_element->impl_id()).get();

  std::string opacity_key("opacity");
  auto opacity_value = painting_node->props_.at(opacity_key);
  EXPECT_TRUE(fabs(opacity_value.Number() - 0.3) < COMPARE_EPSILON);

  // set inline style
  fiber_element->SetStyle(CSSPropertyID::kPropertyIDOpacity, lepus::Value(0.6));
  page->FlushActionsAsRoot();
  painting_context->Flush();

  opacity_value = painting_node->props_.at(opacity_key);
  EXPECT_TRUE(fabs(opacity_value.Number() - 0.6) < COMPARE_EPSILON);

  // update inline style
  fiber_element->SetStyle(CSSPropertyID::kPropertyIDOpacity, lepus::Value(0.8));
  page->FlushActionsAsRoot();
  painting_context->Flush();

  opacity_value = painting_node->props_.at(opacity_key);

  EXPECT_TRUE(fabs(opacity_value.Number() - 0.8) < COMPARE_EPSILON);

  // set inline style to empty, should use css style
  fiber_element->SetStyle(CSSPropertyID::kPropertyIDOpacity, lepus::Value());

  page->FlushActionsAsRoot();
  painting_context->Flush();

  opacity_value = painting_node->props_.at(opacity_key);
  EXPECT_TRUE(fabs(opacity_value.Number() - 0.3) < COMPARE_EPSILON);

  // set inline style to 0.4, should use css style
  fiber_element->SetStyle(CSSPropertyID::kPropertyIDOpacity, lepus::Value(0.4));
  page->FlushActionsAsRoot();
  painting_context->Flush();

  opacity_value = painting_node->props_.at(opacity_key);
  EXPECT_TRUE(fabs(opacity_value.Number() - 0.4) < COMPARE_EPSILON);

  // set inline style to invalid value, fallback to use css style
  fiber_element->SetStyle(CSSPropertyID::kPropertyIDOpacity,
                          lepus::Value("xyz"));
  page->FlushActionsAsRoot();
  painting_context->Flush();

  opacity_value = painting_node->props_.at(opacity_key);
  EXPECT_TRUE(fabs(opacity_value.Number() - 0.3) < COMPARE_EPSILON);
}

TEST_P(FiberElementTest, GetParentComponentCSSFragment) {
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

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap fontfaces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokensMap, keyframes, fontfaces);

  // parent
  auto page = manager->CreateFiberPage("page", 11);

  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // child component
  base::String component_id("21");
  int32_t css_id = 100;
  base::String entry_name("__Card__");
  base::String component_name("TestComp");
  base::String path("/index/components/TestComp");

  auto comp = manager->CreateFiberComponent(component_id, css_id, entry_name,
                                            component_name, path);
  comp->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(page->impl_id()));
  page->InsertNode(comp);

  auto* css_fragment = comp->GetRelatedCSSFragment();
  EXPECT_TRUE(page->GetCSSFragment() == css_fragment);
}

TEST_P(FiberElementTest, DumpStyleInlineStyle) {
  auto page = manager->CreateFiberPage("page", 11);
  auto element = manager->CreateFiberView();
  element->SetStyle(CSSPropertyID::kPropertyIDOverflow,
                    lepus::Value("visible"));
  element->SetStyle(CSSPropertyID::kPropertyIDBackgroundColor,
                    lepus::Value("red"));

  // can DumpStyle before FlushActionsAsRoot
  {
    tasm::StyleMap dumped;
    element->DumpStyle(dumped);
    EXPECT_EQ(dumped.size(), 2);

    auto style1 = dumped.find(CSSPropertyID::kPropertyIDOverflow);
    EXPECT_EQ(tasm::CSSDecoder::CSSValueToString(style1->first, style1->second),
              "visible");

    auto style2 = dumped.find(CSSPropertyID::kPropertyIDBackgroundColor);
    EXPECT_EQ(tasm::CSSDecoder::CSSValueToString(style2->first, style2->second),
              "#ff0000");
  }

  page->FlushActionsAsRoot();

  // can DumpStyle after FlushActionsAsRoot
  {
    tasm::StyleMap dumped;
    element->DumpStyle(dumped);
    EXPECT_EQ(dumped.size(), 2);

    auto style1 = dumped.find(CSSPropertyID::kPropertyIDOverflow);
    EXPECT_EQ(tasm::CSSDecoder::CSSValueToString(style1->first, style1->second),
              "visible");

    auto style2 = dumped.find(CSSPropertyID::kPropertyIDBackgroundColor);
    EXPECT_EQ(tasm::CSSDecoder::CSSValueToString(style2->first, style2->second),
              "#ff0000");
  }

  // can DumpStyle after SetStyle (update)
  {
    element->SetStyle(CSSPropertyID::kPropertyIDOverflow,
                      lepus::Value("hidden"));

    tasm::StyleMap dumped;
    element->DumpStyle(dumped);
    EXPECT_EQ(dumped.size(), 2);

    auto style1 = dumped.find(CSSPropertyID::kPropertyIDOverflow);
    EXPECT_EQ(tasm::CSSDecoder::CSSValueToString(style1->first, style1->second),
              "hidden");
  }

  // can DumpStyle after RemoveStyle
  {
    element->RemoveAllInlineStyles();

    tasm::StyleMap dumped;
    element->DumpStyle(dumped);
    EXPECT_EQ(dumped.size(), 0);
  }
}

TEST_P(FiberElementTest, DumpStyleClass) {
  // constructor css fragment
  StyleMap indexAttributes;
  CSSParserConfigs parser_config;
  auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_config);

  CSSParserTokenMap indexTokensMap;
  // class .test-class
  {
    auto id = CSSPropertyID::kPropertyIDVisibility;
    auto impl = lepus::Value("hidden");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".test-class";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap fontfaces;
  auto indexFragment = std::make_unique<SharedCSSFragment>(
      1, dependent_ids, indexTokensMap, keyframes, fontfaces);

  int css_id = 11;
  auto page = manager->CreateFiberPage("page", css_id);

  page->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(page->impl_id()));

  page->set_style_sheet_manager(
      tasm->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME));

  auto style_sheet_manager = page->css_style_sheet_manager_;

  style_sheet_manager->raw_fragments_->insert(
      std::make_pair(css_id, std::move(indexFragment)));

  auto child = manager->CreateFiberNode("view");
  child->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(page->impl_id()));
  child->SetClass("test-class");
  page->InsertNodeBefore(child, nullptr);

  // can DumpStyle before FlushActionsAsRoot
  {
    tasm::StyleMap dumped;
    child->DumpStyle(dumped);
    EXPECT_EQ(dumped.size(), 1);

    auto style1 = dumped.find(CSSPropertyID::kPropertyIDVisibility);
    EXPECT_EQ(tasm::CSSDecoder::CSSValueToString(style1->first, style1->second),
              "hidden");
  }

  // can DumpStyle after SetStyle before FlushActionsAsRoot
  {
    child->SetStyle(CSSPropertyID::kPropertyIDFontSize, lepus::Value("32px"));

    tasm::StyleMap dumped;
    child->DumpStyle(dumped);
    EXPECT_EQ(dumped.size(), 2);

    auto style1 = dumped.find(CSSPropertyID::kPropertyIDVisibility);
    EXPECT_EQ(tasm::CSSDecoder::CSSValueToString(style1->first, style1->second),
              "hidden");

    auto style2 = dumped.find(CSSPropertyID::kPropertyIDFontSize);
    EXPECT_EQ(tasm::CSSDecoder::CSSValueToString(style2->first, style2->second),
              "32px");
  }

  // can DumpStyle after RemoveAllClass
  {
    child->RemoveAllClass();

    tasm::StyleMap dumped;
    child->DumpStyle(dumped);
    EXPECT_EQ(dumped.size(), 1);

    auto style1 = dumped.find(CSSPropertyID::kPropertyIDFontSize);
    EXPECT_EQ(tasm::CSSDecoder::CSSValueToString(style1->first, style1->second),
              "32px");
  }
}

TEST_P(FiberElementTest, TestTagSelectorCase) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableStandardCSSSelector(true);
  manager->SetConfig(config);

  // Construct CSS Fragment
  StyleMap indexAttributes;
  CSSParserConfigs parser_config;
  CSSParserTokenMap indexTokenMap;

  // class text
  {
    auto token = fml::MakeRefCounted<CSSParseToken>(parser_config);
    auto id = CSSPropertyID::kPropertyIDColor;
    auto color = lepus::Value("red");
    token->raw_attributes_[id] = CSSValue(color, CSSValuePattern::STRING);

    std::string key = "text";
    auto& sheets = token->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(std::move(shared_css_sheet));
    indexTokenMap.insert(std::make_pair(key, token));
  }

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

  // text
  auto fiber_element_2 = manager->CreateFiberNode("text");
  fiber_element_2->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_element_2);
  fiber_element_2->SetAttribute("text", lepus::Value("This is a text."));

  page->FlushActionsAsRoot();
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* painting_node_2 =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  std::string background_color_key = "color";
  auto node_2_color_value = painting_node_2->props_.at(background_color_key);
  EXPECT_TRUE(node_2_color_value.UInt32() == 0xffff0000);
}

TEST_P(FiberElementTest, FromTemplateInfoTest) {
  ElementTemplateInfo template_info;
  template_info.exist_ = true;
  template_info.key_ = "key";

  auto info_0 = ElementInfo();
  info_0.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  info_0.id_selector_ = "#0";
  info_0.builtin_attrs_[ElementBuiltInAttributeEnum::DIRTY_ID] =
      lepus::Value("0");

  auto info_0_0 = ElementInfo();
  info_0_0.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  info_0_0.id_selector_ = "#0_0";
  info_0_0.builtin_attrs_[ElementBuiltInAttributeEnum::DIRTY_ID] =
      lepus::Value("0_0");

  auto info_0_0_0 = ElementInfo();
  info_0_0_0.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  info_0_0_0.id_selector_ = "#0_0_0";
  info_0_0_0.builtin_attrs_[ElementBuiltInAttributeEnum::DIRTY_ID] =
      lepus::Value("0_0_0");

  auto info_0_1 = ElementInfo();
  info_0_1.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;

  auto info_0_2 = ElementInfo();
  info_0_2.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;

  auto info_0_3 = ElementInfo();
  info_0_3.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;

  auto info_0_4 = ElementInfo();
  info_0_4.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;

  info_0_0.children_.emplace_back(std::move(info_0_0_0));
  info_0.children_.emplace_back(std::move(info_0_0));
  info_0.children_.emplace_back(std::move(info_0_1));
  info_0.children_.emplace_back(std::move(info_0_2));
  info_0.children_.emplace_back(std::move(info_0_3));
  info_0.children_.emplace_back(std::move(info_0_4));

  template_info.elements_.emplace_back(std::move(info_0));

  auto res = TreeResolver::InitElementTree(
      TreeResolver::FromTemplateInfo(template_info), 0, manager,
      tasm->style_sheet_manager(DEFAULT_ENTRY_NAME));

  auto root = res.GetProperty(0);
  EXPECT_EQ(root.IsRefCounted(), true);
  auto root_element = fml::static_ref_ptr_cast<FiberElement>(root.RefCounted());
  EXPECT_EQ(root_element->IsTemplateElement(), true);
  EXPECT_EQ(root_element->IsPartElement(), true);
  auto map = TreeResolver::GetTemplateParts(root_element);

  auto ref_0 = map->GetValueOrNull("0");
  EXPECT_EQ(ref_0.has_value(), false);
  auto ref_0_0 = map->GetValueOrNull("0_0");
  EXPECT_EQ(ref_0_0 && ref_0_0->IsRefCounted(), true);
  auto ref_0_0_0 = map->GetValueOrNull("0_0_0");
  EXPECT_EQ(ref_0_0_0 && ref_0_0_0->IsRefCounted(), true);
}

TEST_P(FiberElementTest, ClassChildSelectorTest) {
  // construct css fragment.
  StyleMap indexAttributes;
  CSSParserConfigs parser_configs;
  CSSParserTokenMap indexTokenMap;
  // class .A
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue::MakePlainString("blue");
    std::string key = ".A";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap font_faces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokenMap, keyframes, font_faces);

  // .A:first_child
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue::MakePlainString("green");
    std::string key = ".A:first-child";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    shared_css_sheet->ConfirmType();
    sheets.emplace_back(shared_css_sheet);
    indexFragment->child_pseudo_map_.insert(std::make_pair(key, tokens));
  }

  // .A:last_child
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue::MakePlainString("red");
    std::string key = ".A:last-child";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    shared_css_sheet->ConfirmType();
    sheets.emplace_back(shared_css_sheet);
    indexFragment->child_pseudo_map_.insert(std::make_pair(key, tokens));
  }

  // page
  auto page = manager->CreateFiberPage("page", 0);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // dad view
  auto fiber_element = manager->CreateFiberNode("view");
  fiber_element->parent_component_element_ = page.get();
  page->InsertNode(fiber_element);

  // son text 1
  auto fiber_element_1 = manager->CreateFiberText("text");
  fiber_element_1->parent_component_element_ = page.get();
  fiber_element->InsertNode(fiber_element_1);
  fiber_element_1->SetClass("A");
  fiber_element_1->arch_type_ = RadonArch;

  // son text 2
  auto fiber_element_2 = manager->CreateFiberText("text");
  fiber_element_2->parent_component_element_ = page.get();
  fiber_element->InsertNode(fiber_element_2);
  fiber_element_2->SetClass("A");
  fiber_element_2->arch_type_ = RadonArch;

  // son text 3
  auto fiber_element_3 = manager->CreateFiberText("text");
  fiber_element_3->parent_component_element_ = page.get();
  fiber_element->InsertNode(fiber_element_3);
  fiber_element_3->SetClass("A");
  fiber_element_3->arch_type_ = RadonArch;

  // flush fiber tree
  auto options = std::make_shared<PipelineOptions>();
  options->force_resolve_style_ = true;
  manager->OnPatchFinish(options, page.get());

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  std::string background_color_key = "background-color";

  // EXPECT element == green (.A:first_child)
  auto painting_node_1 =
      painting_context->node_map_.at(fiber_element_1->impl_id()).get();
  auto node_1_background_color_value =
      painting_node_1->props_.at(background_color_key);
  EXPECT_EQ(node_1_background_color_value.UInt32(), 0xff008000);

  // EXPECT element == blue (.A)
  auto painting_node_2 =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  auto node_2_background_color_value =
      painting_node_2->props_.at(background_color_key);
  EXPECT_EQ(node_2_background_color_value.UInt32(), 0xff0000ff);

  // EXPECT element == red (.A:last_child)
  auto painting_node_3 =
      painting_context->node_map_.at(fiber_element_3->impl_id()).get();
  auto node_3_background_color_value =
      painting_node_3->props_.at(background_color_key);
  EXPECT_EQ(node_3_background_color_value.UInt32(), 0xffff0000);
}

TEST_P(FiberElementTest, TagNotSelectorTest) {
  // construct css fragment.
  StyleMap indexAttributes;
  CSSParserConfigs parser_configs;
  CSSParserTokenMap indexTokenMap;
  // class .A
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue::MakePlainString("blue");
    std::string key = ".A";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap font_faces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokenMap, keyframes, font_faces);

  // .C:not(view)
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue::MakePlainString("yellow");
    std::string key = ".C:not(view)";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexFragment->pseudo_map_.insert(std::make_pair(key, tokens));
  }

  // page
  auto page = manager->CreateFiberPage("page", 0);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // dad view
  auto fiber_element_1 = manager->CreateFiberNode("view");
  fiber_element_1->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_1);
  fiber_element_1->SetClass("C");

  // son text
  auto fiber_element_2 = manager->CreateFiberText("text");
  fiber_element_2->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_element_2);
  fiber_element_2->SetClass("C");

  // flush fiber tree
  auto options = std::make_shared<PipelineOptions>();
  options->force_resolve_style_ = true;
  manager->OnPatchFinish(options, page.get());

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  std::string background_color_key = "background-color";

  // EXPECT element == yellow (.C:not(view))
  auto painting_node_2 =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  auto node_2_background_color_value =
      painting_node_2->props_.at(background_color_key);
  EXPECT_EQ(node_2_background_color_value.UInt32(), 0xffffff00);

  // EXPECT element == default
  auto painting_node_1 =
      painting_context->node_map_.at(fiber_element_1->impl_id()).get();
  EXPECT_TRUE(painting_node_1->props_.empty());

  // Remove class
  fiber_element_2->RemoveAllClass();

  // flush fiber tree again
  manager->OnPatchFinish(options, page.get());

  painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  node_2_background_color_value =
      painting_node_2->props_.at(background_color_key);
  EXPECT_TRUE(node_2_background_color_value.IsNil());
}

TEST_P(FiberElementTest, ClassNotSelectorTest) {
  // construct css fragment.
  StyleMap indexAttributes;
  CSSParserConfigs parser_configs;
  CSSParserTokenMap indexTokenMap;
  // class .C
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDColor] =
        CSSValue::MakePlainString("blue");
    std::string key = ".C";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap font_faces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokenMap, keyframes, font_faces);

  // .C:not(.B)
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue::MakePlainString("yellow");
    std::string key = ".C:not(.B)";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexFragment->pseudo_map_.insert(std::make_pair(key, tokens));
  }

  // page
  auto page = manager->CreateFiberPage("page", 0);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // dad view: class C
  auto fiber_element_1 = manager->CreateFiberNode("view");
  fiber_element_1->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_1);
  fiber_element_1->SetClass("C");

  // son view: class C B
  auto fiber_element_2 = manager->CreateFiberText("view");
  fiber_element_2->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_element_2);
  fiber_element_2->SetClass("C");
  fiber_element_2->SetClass("B");

  // flush fiber tree
  auto options = std::make_shared<PipelineOptions>();
  options->force_resolve_style_ = true;
  manager->OnPatchFinish(options, page.get());

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  std::string background_color_key = "background-color";
  std::string color_key = "color";

  // EXPECT element == yellow (.C:not(.B))
  auto painting_node_1 =
      painting_context->node_map_.at(fiber_element_1->impl_id()).get();
  auto node_1_background_color_value =
      painting_node_1->props_.at(background_color_key);
  EXPECT_EQ(node_1_background_color_value.UInt32(), 0xffffff00);
  auto node_1_color_value = painting_node_1->props_.at(color_key);
  EXPECT_EQ(node_1_color_value.UInt32(), 0xff0000ff);

  // EXPECT element == default
  auto painting_node_2 =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  auto node_2_color_value = painting_node_2->props_.at(color_key);
  EXPECT_EQ(painting_node_2->props_.size(), 1);
  EXPECT_EQ(node_2_color_value.UInt32(), 0xff0000ff);

  // Set B
  fiber_element_1->SetClass("B");

  // flush fiber tree again
  manager->OnPatchFinish(options, page.get());

  painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  node_1_background_color_value =
      painting_node_1->props_.at(background_color_key);
  EXPECT_TRUE(node_1_background_color_value.IsNil());
  node_1_color_value = painting_node_1->props_.at(color_key);
  EXPECT_EQ(node_1_color_value.UInt32(), 0xff0000ff);
}

TEST_P(FiberElementTest, IdNotSelectorTest) {
  // construct css fragment.
  StyleMap indexAttributes;
  CSSParserConfigs parser_configs;
  CSSParserTokenMap indexTokenMap;
  // class .C
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDColor] =
        CSSValue::MakePlainString("blue");
    std::string key = ".C";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap font_faces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokenMap, keyframes, font_faces);

  // .C:not(#B)
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue::MakePlainString("yellow");
    std::string key = ".C:not(#B)";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexFragment->pseudo_map_.insert(std::make_pair(key, tokens));
  }

  // page
  auto page = manager->CreateFiberPage("page", 0);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // dad view: class C
  auto fiber_element_1 = manager->CreateFiberNode("view");
  fiber_element_1->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_1);
  fiber_element_1->SetClass("C");

  // son view: class C B
  auto fiber_element_2 = manager->CreateFiberText("view");
  fiber_element_2->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_element_2);
  fiber_element_2->SetClass("C");
  fiber_element_2->SetIdSelector("B");

  // flush fiber tree
  auto options = std::make_shared<PipelineOptions>();
  options->force_resolve_style_ = true;
  manager->OnPatchFinish(options, page.get());

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  std::string background_color_key = "background-color";
  std::string color_key = "color";

  // EXPECT element == yellow (.C:not(#B))
  auto painting_node_1 =
      painting_context->node_map_.at(fiber_element_1->impl_id()).get();
  auto node_1_background_color_value =
      painting_node_1->props_.at(background_color_key);
  EXPECT_EQ(node_1_background_color_value.UInt32(), 0xffffff00);
  auto node_1_color_value = painting_node_1->props_.at(color_key);
  EXPECT_EQ(node_1_color_value.UInt32(), 0xff0000ff);

  // EXPECT element == default
  auto painting_node_2 =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  auto node_2_color_value = painting_node_2->props_.at(color_key);
  EXPECT_EQ(painting_node_2->props_.count(background_color_key), 0);
  EXPECT_EQ(node_2_color_value.UInt32(), 0xff0000ff);

  // Set B
  fiber_element_1->SetIdSelector("B");

  // flush fiber tree again
  manager->OnPatchFinish(options, page.get());

  painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  node_1_background_color_value =
      painting_node_1->props_.at(background_color_key);
  EXPECT_TRUE(node_1_background_color_value.IsNil());
  node_1_color_value = painting_node_1->props_.at(color_key);
  EXPECT_EQ(node_1_color_value.UInt32(), 0xff0000ff);
}

TEST_P(FiberElementTest, Class_ClassCascadeForceFlushTest) {
  // construct css fragment.
  StyleMap indexAttributes;
  CSSParserConfigs parser_configs;
  CSSParserTokenMap indexTokenMap;
  // class .A
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue::MakePlainString("blue");
    std::string key = ".A";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  // class .B
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue::MakePlainString("green");
    std::string key = ".B";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  // class .C
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue::MakePlainString("black");
    std::string key = ".C";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap font_faces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokenMap, keyframes, font_faces);

  // class .A.C
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue::MakePlainString("yellow");
    std::string key = ".C.A";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexFragment->cascade_map_.insert(std::make_pair(key, tokens));
  }

  // class .B.C
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue::MakePlainString("red");
    std::string key = ".C.B";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexFragment->cascade_map_.insert(std::make_pair(key, tokens));
  }

  // page
  auto page = manager->CreateFiberPage("page", 0);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // dad view
  auto fiber_element_1 = manager->CreateFiberNode("view");
  fiber_element_1->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_1);
  fiber_element_1->SetClass("A");

  // son view
  auto fiber_element_2 = manager->CreateFiberText("view");
  fiber_element_2->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_element_2);
  fiber_element_2->SetClass("C");

  // flush fiber tree
  auto options = std::make_shared<PipelineOptions>();
  options->force_resolve_style_ = true;
  manager->OnPatchFinish(options, page.get());

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  std::string background_color_key = "background-color";

  // EXPECT element == yellow (A C)
  auto painting_node_2 =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  auto node_2_background_color_value =
      painting_node_2->props_.at(background_color_key);
  EXPECT_EQ(node_2_background_color_value.UInt32(), 0xffffff00);

  // SetClass("B")
  fiber_element_1->RemoveAllClass();
  fiber_element_1->SetClass("B");

  // flush fiber tree again
  manager->OnPatchFinish(options, page.get());

  painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  // EXPECT element == red (B C)
  painting_node_2 =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  node_2_background_color_value =
      painting_node_2->props_.at(background_color_key);
  EXPECT_EQ(node_2_background_color_value.UInt32(), 0xffff0000);
}

TEST_P(FiberElementTest, ID_IDCascadeForceFlushTest) {
  // construct css fragment.
  StyleMap indexAttributes;
  CSSParserConfigs parser_configs;
  CSSParserTokenMap indexTokenMap;
  // class .A
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue::MakePlainString("blue");
    std::string key = ".A";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap font_faces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokenMap, keyframes, font_faces);

  // id #A#C
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue::MakePlainString("yellow");
    std::string key = "#C#A";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexFragment->cascade_map_.insert(std::make_pair(key, tokens));
  }

  // id #B#C
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue::MakePlainString("red");
    std::string key = "#C#B";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexFragment->cascade_map_.insert(std::make_pair(key, tokens));
  }

  // page
  auto page = manager->CreateFiberPage("page", 0);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // dad view
  auto fiber_element_1 = manager->CreateFiberNode("view");
  fiber_element_1->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_1);
  fiber_element_1->SetIdSelector("A");

  // son view
  auto fiber_element_2 = manager->CreateFiberText("view");
  fiber_element_2->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_element_2);
  fiber_element_2->SetIdSelector("C");

  // flush fiber tree
  auto options = std::make_shared<PipelineOptions>();
  options->force_resolve_style_ = true;
  manager->OnPatchFinish(options, page.get());

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  std::string background_color_key = "background-color";

  // EXPECT element == yellow (A C)
  auto painting_node_2 =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  auto node_2_background_color_value =
      painting_node_2->props_.at(background_color_key);
  EXPECT_EQ(node_2_background_color_value.UInt32(), 0xffffff00);

  // SetID("B")
  fiber_element_1->RemoveAllClass();
  fiber_element_1->SetIdSelector("B");

  // flush fiber tree again
  manager->OnPatchFinish(options, page.get());

  painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  // EXPECT element == red (B C)
  painting_node_2 =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  node_2_background_color_value =
      painting_node_2->props_.at(background_color_key);
  EXPECT_EQ(node_2_background_color_value.UInt32(), 0xffff0000);
}

TEST_P(FiberElementTest, Class_IDCascadeForceFlushTest) {
  // construct css fragment.
  StyleMap indexAttributes;
  CSSParserConfigs parser_configs;
  CSSParserTokenMap indexTokenMap;
  // class .A
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue::MakePlainString("blue");
    std::string key = ".A";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap font_faces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokenMap, keyframes, font_faces);

  // #A.C
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue::MakePlainString("yellow");
    std::string key = ".C#A";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexFragment->cascade_map_.insert(std::make_pair(key, tokens));
  }

  // #B.C
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue::MakePlainString("red");
    std::string key = ".C#B";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexFragment->cascade_map_.insert(std::make_pair(key, tokens));
  }

  // page
  auto page = manager->CreateFiberPage("page", 0);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // dad view
  auto fiber_element_1 = manager->CreateFiberNode("view");
  fiber_element_1->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_1);
  fiber_element_1->SetIdSelector("A");

  // son view
  auto fiber_element_2 = manager->CreateFiberText("view");
  fiber_element_2->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_element_2);
  fiber_element_2->SetClass("C");

  // flush fiber tree
  auto options = std::make_shared<PipelineOptions>();
  options->force_resolve_style_ = true;
  manager->OnPatchFinish(options, page.get());

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  std::string background_color_key = "background-color";

  // EXPECT element == yellow (A C)
  auto painting_node_2 =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  auto node_2_background_color_value =
      painting_node_2->props_.at(background_color_key);
  EXPECT_EQ(node_2_background_color_value.UInt32(), 0xffffff00);

  // SetID("B")
  fiber_element_1->RemoveAllClass();
  fiber_element_1->SetIdSelector("B");

  // flush fiber tree again
  manager->OnPatchFinish(options, page.get());

  painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  // EXPECT element == red (B C)
  painting_node_2 =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  node_2_background_color_value =
      painting_node_2->props_.at(background_color_key);
  EXPECT_EQ(node_2_background_color_value.UInt32(), 0xffff0000);
}

TEST_P(FiberElementTest, ID_ClassCascadeForceFlushTest) {
  // construct css fragment.
  StyleMap indexAttributes;
  CSSParserConfigs parser_configs;
  CSSParserTokenMap indexTokenMap;
  // class .A
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue::MakePlainString("blue");
    std::string key = ".A";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap font_faces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokenMap, keyframes, font_faces);

  // .A#C
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue::MakePlainString("yellow");
    std::string key = "#C.A";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexFragment->cascade_map_.insert(std::make_pair(key, tokens));
  }

  // .B#C
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue::MakePlainString("red");
    std::string key = "#C.B";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexFragment->cascade_map_.insert(std::make_pair(key, tokens));
  }

  // page
  auto page = manager->CreateFiberPage("page", 0);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // dad view
  auto fiber_element_1 = manager->CreateFiberNode("view");
  fiber_element_1->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_1);
  fiber_element_1->SetClass("A");

  // son view
  auto fiber_element_2 = manager->CreateFiberText("view");
  fiber_element_2->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_element_2);
  fiber_element_2->SetIdSelector("C");

  // flush fiber tree
  auto options = std::make_shared<PipelineOptions>();
  options->force_resolve_style_ = true;
  manager->OnPatchFinish(options, page.get());

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  std::string background_color_key = "background-color";

  // EXPECT element == yellow (A C)
  auto painting_node_2 =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  auto node_2_background_color_value =
      painting_node_2->props_.at(background_color_key);
  EXPECT_EQ(node_2_background_color_value.UInt32(), 0xffffff00);

  // SetID("B")
  fiber_element_1->RemoveAllClass();
  fiber_element_1->SetClass("B");

  // flush fiber tree again
  manager->OnPatchFinish(options, page.get());

  painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  // EXPECT element == red (B C)
  painting_node_2 =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  node_2_background_color_value =
      painting_node_2->props_.at(background_color_key);
  EXPECT_EQ(node_2_background_color_value.UInt32(), 0xffff0000);
}

TEST_P(FiberElementTest, ClassChildSelectorCSSParserTest) {
  // construct css fragment.
  StyleMap indexAttributes;
  CSSParserConfigs parser_configs;
  CSSParserTokenMap indexTokenMap;
  // class .A
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    UnitHandler::Process(CSSPropertyID::kPropertyIDBackgroundColor,
                         lepus::Value("blue"), tokens->attributes_,
                         parser_configs);
    tokens->MarkParsed();
    std::string key = ".A";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap font_faces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokenMap, keyframes, font_faces);

  // .A:first_child
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    UnitHandler::Process(CSSPropertyID::kPropertyIDBackgroundColor,
                         lepus::Value("green"), tokens->attributes_,
                         parser_configs);
    tokens->MarkParsed();
    std::string key = ".A:first-child";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    shared_css_sheet->ConfirmType();
    sheets.emplace_back(shared_css_sheet);
    indexFragment->child_pseudo_map_.insert(std::make_pair(key, tokens));
  }

  // .A:last_child
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    UnitHandler::Process(CSSPropertyID::kPropertyIDBackgroundColor,
                         lepus::Value("red"), tokens->attributes_,
                         parser_configs);
    tokens->MarkParsed();
    std::string key = ".A:last-child";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    shared_css_sheet->ConfirmType();
    sheets.emplace_back(shared_css_sheet);
    indexFragment->child_pseudo_map_.insert(std::make_pair(key, tokens));
  }

  // page
  auto page = manager->CreateFiberPage("page", 0);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // dad view
  auto fiber_element = manager->CreateFiberNode("view");
  fiber_element->parent_component_element_ = page.get();
  page->InsertNode(fiber_element);

  // son text 1
  auto fiber_element_1 = manager->CreateFiberText("text");
  fiber_element_1->parent_component_element_ = page.get();
  fiber_element->InsertNode(fiber_element_1);
  fiber_element_1->SetClass("A");
  fiber_element_1->arch_type_ = RadonArch;

  // son text 2
  auto fiber_element_2 = manager->CreateFiberText("text");
  fiber_element_2->parent_component_element_ = page.get();
  fiber_element->InsertNode(fiber_element_2);
  fiber_element_2->SetClass("A");
  fiber_element_2->arch_type_ = RadonArch;

  // son text 3
  auto fiber_element_3 = manager->CreateFiberText("text");
  fiber_element_3->parent_component_element_ = page.get();
  fiber_element->InsertNode(fiber_element_3);
  fiber_element_3->SetClass("A");
  fiber_element_3->arch_type_ = RadonArch;

  // flush fiber tree
  auto options = std::make_shared<PipelineOptions>();
  options->force_resolve_style_ = true;
  manager->OnPatchFinish(options, page.get());

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  std::string background_color_key = "background-color";

  // EXPECT element == green (.A:first_child)
  auto painting_node_1 =
      painting_context->node_map_.at(fiber_element_1->impl_id()).get();
  auto node_1_background_color_value =
      painting_node_1->props_.at(background_color_key);
  EXPECT_EQ(node_1_background_color_value.UInt32(), 0xff008000);

  // EXPECT element == blue (.A)
  auto painting_node_2 =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  auto node_2_background_color_value =
      painting_node_2->props_.at(background_color_key);
  EXPECT_EQ(node_2_background_color_value.UInt32(), 0xff0000ff);

  // EXPECT element == red (.A:last_child)
  auto painting_node_3 =
      painting_context->node_map_.at(fiber_element_3->impl_id()).get();
  auto node_3_background_color_value =
      painting_node_3->props_.at(background_color_key);
  EXPECT_EQ(node_3_background_color_value.UInt32(), 0xffff0000);
}

TEST_P(FiberElementTest, TagNotSelectorCSSParserTest) {
  // construct css fragment.
  StyleMap indexAttributes;
  CSSParserConfigs parser_configs;
  CSSParserTokenMap indexTokenMap;
  // class .A
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    UnitHandler::Process(CSSPropertyID::kPropertyIDBackgroundColor,
                         lepus::Value("blue"), tokens->attributes_,
                         parser_configs);
    tokens->MarkParsed();
    std::string key = ".A";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap font_faces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokenMap, keyframes, font_faces);

  // .C:not(view)
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    UnitHandler::Process(CSSPropertyID::kPropertyIDBackgroundColor,
                         lepus::Value("yellow"), tokens->attributes_,
                         parser_configs);
    tokens->MarkParsed();
    std::string key = ".C:not(view)";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexFragment->pseudo_map_.insert(std::make_pair(key, tokens));
  }

  // page
  auto page = manager->CreateFiberPage("page", 0);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // dad view
  auto fiber_element_1 = manager->CreateFiberNode("view");
  fiber_element_1->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_1);
  fiber_element_1->SetClass("C");

  // son text
  auto fiber_element_2 = manager->CreateFiberText("text");
  fiber_element_2->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_element_2);
  fiber_element_2->SetClass("C");

  // flush fiber tree
  auto options = std::make_shared<PipelineOptions>();
  options->force_resolve_style_ = true;
  manager->OnPatchFinish(options, page.get());

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  std::string background_color_key = "background-color";

  // EXPECT element == yellow (.C:not(view))
  auto painting_node_2 =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  auto node_2_background_color_value =
      painting_node_2->props_.at(background_color_key);
  EXPECT_EQ(node_2_background_color_value.UInt32(), 0xffffff00);

  // EXPECT element == default
  auto painting_node_1 =
      painting_context->node_map_.at(fiber_element_1->impl_id()).get();
  EXPECT_TRUE(painting_node_1->props_.empty());

  // Remove class
  fiber_element_2->RemoveAllClass();

  // flush fiber tree again
  manager->OnPatchFinish(options, page.get());

  painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  node_2_background_color_value =
      painting_node_2->props_.at(background_color_key);
  EXPECT_TRUE(node_2_background_color_value.IsNil());
}

TEST_P(FiberElementTest, ClassNotSelectorCSSParserTest) {
  // construct css fragment.
  StyleMap indexAttributes;
  CSSParserConfigs parser_configs;
  CSSParserTokenMap indexTokenMap;
  // class .C
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    UnitHandler::Process(CSSPropertyID::kPropertyIDColor, lepus::Value("blue"),
                         tokens->attributes_, parser_configs);
    tokens->MarkParsed();
    std::string key = ".C";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap font_faces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokenMap, keyframes, font_faces);

  // .C:not(.B)
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    UnitHandler::Process(CSSPropertyID::kPropertyIDBackgroundColor,
                         lepus::Value("yellow"), tokens->attributes_,
                         parser_configs);
    tokens->MarkParsed();
    std::string key = ".C:not(.B)";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexFragment->pseudo_map_.insert(std::make_pair(key, tokens));
  }

  // page
  auto page = manager->CreateFiberPage("page", 0);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // dad view: class C
  auto fiber_element_1 = manager->CreateFiberNode("view");
  fiber_element_1->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_1);
  fiber_element_1->SetClass("C");

  // son view: class C B
  auto fiber_element_2 = manager->CreateFiberText("view");
  fiber_element_2->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_element_2);
  fiber_element_2->SetClass("C");
  fiber_element_2->SetClass("B");

  // flush fiber tree
  auto options = std::make_shared<PipelineOptions>();
  options->force_resolve_style_ = true;
  manager->OnPatchFinish(options, page.get());

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  std::string background_color_key = "background-color";
  std::string color_key = "color";

  // EXPECT element == yellow (.C:not(.B))
  auto painting_node_1 =
      painting_context->node_map_.at(fiber_element_1->impl_id()).get();
  auto node_1_background_color_value =
      painting_node_1->props_.at(background_color_key);
  EXPECT_EQ(node_1_background_color_value.UInt32(), 0xffffff00);
  auto node_1_color_value = painting_node_1->props_.at(color_key);
  EXPECT_EQ(node_1_color_value.UInt32(), 0xff0000ff);

  // EXPECT element == default
  auto painting_node_2 =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  auto node_2_color_value = painting_node_2->props_.at(color_key);
  EXPECT_EQ(painting_node_2->props_.size(), 1);
  EXPECT_EQ(node_2_color_value.UInt32(), 0xff0000ff);

  // Set B
  fiber_element_1->SetClass("B");

  // flush fiber tree again
  manager->OnPatchFinish(options, page.get());

  painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  node_1_background_color_value =
      painting_node_1->props_.at(background_color_key);
  EXPECT_TRUE(node_1_background_color_value.IsNil());
  node_1_color_value = painting_node_1->props_.at(color_key);
  EXPECT_EQ(node_1_color_value.UInt32(), 0xff0000ff);
}

TEST_P(FiberElementTest, IdNotSelectorCSSParserTest) {
  // construct css fragment.
  StyleMap indexAttributes;
  CSSParserConfigs parser_configs;
  CSSParserTokenMap indexTokenMap;
  // class .C
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    UnitHandler::Process(CSSPropertyID::kPropertyIDColor, lepus::Value("blue"),
                         tokens->attributes_, parser_configs);
    tokens->MarkParsed();
    std::string key = ".C";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap font_faces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokenMap, keyframes, font_faces);

  // .C:not(#B)
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    UnitHandler::Process(CSSPropertyID::kPropertyIDBackgroundColor,
                         lepus::Value("yellow"), tokens->attributes_,
                         parser_configs);
    tokens->MarkParsed();
    std::string key = ".C:not(#B)";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexFragment->pseudo_map_.insert(std::make_pair(key, tokens));
  }

  // page
  auto page = manager->CreateFiberPage("page", 0);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // dad view: class C
  auto fiber_element_1 = manager->CreateFiberNode("view");
  fiber_element_1->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_1);
  fiber_element_1->SetClass("C");

  // son view: class C B
  auto fiber_element_2 = manager->CreateFiberText("view");
  fiber_element_2->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_element_2);
  fiber_element_2->SetClass("C");
  fiber_element_2->SetIdSelector("B");

  // flush fiber tree
  auto options = std::make_shared<PipelineOptions>();
  options->force_resolve_style_ = true;
  manager->OnPatchFinish(options, page.get());

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  std::string background_color_key = "background-color";
  std::string color_key = "color";

  // EXPECT element == yellow (.C:not(#B))
  auto painting_node_1 =
      painting_context->node_map_.at(fiber_element_1->impl_id()).get();
  auto node_1_background_color_value =
      painting_node_1->props_.at(background_color_key);
  EXPECT_EQ(node_1_background_color_value.UInt32(), 0xffffff00);
  auto node_1_color_value = painting_node_1->props_.at(color_key);
  EXPECT_EQ(node_1_color_value.UInt32(), 0xff0000ff);

  // EXPECT element == default
  auto painting_node_2 =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  auto node_2_color_value = painting_node_2->props_.at(color_key);
  EXPECT_EQ(painting_node_2->props_.count(background_color_key), 0);
  EXPECT_EQ(node_2_color_value.UInt32(), 0xff0000ff);

  // Set B
  fiber_element_1->SetIdSelector("B");

  // flush fiber tree again
  manager->OnPatchFinish(options, page.get());

  painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  node_1_background_color_value =
      painting_node_1->props_.at(background_color_key);
  EXPECT_TRUE(node_1_background_color_value.IsNil());
  node_1_color_value = painting_node_1->props_.at(color_key);
  EXPECT_EQ(node_1_color_value.UInt32(), 0xff0000ff);
}

TEST_P(FiberElementTest, Class_ClassCascadeForceFlushCSSParserTest) {
  // construct css fragment.
  StyleMap indexAttributes;
  CSSParserConfigs parser_configs;
  CSSParserTokenMap indexTokenMap;
  // class .A
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    UnitHandler::Process(CSSPropertyID::kPropertyIDBackgroundColor,
                         lepus::Value("blue"), tokens->attributes_,
                         parser_configs);
    tokens->MarkParsed();
    std::string key = ".A";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  // class .B
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    UnitHandler::Process(CSSPropertyID::kPropertyIDBackgroundColor,
                         lepus::Value("green"), tokens->attributes_,
                         parser_configs);
    tokens->MarkParsed();
    std::string key = ".B";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  // class .C
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    UnitHandler::Process(CSSPropertyID::kPropertyIDBackgroundColor,
                         lepus::Value("black"), tokens->attributes_,
                         parser_configs);
    tokens->MarkParsed();
    std::string key = ".C";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap font_faces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokenMap, keyframes, font_faces);

  // class .A.C
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    UnitHandler::Process(CSSPropertyID::kPropertyIDBackgroundColor,
                         lepus::Value("yellow"), tokens->attributes_,
                         parser_configs);
    tokens->MarkParsed();
    std::string key = ".C.A";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexFragment->cascade_map_.insert(std::make_pair(key, tokens));
  }

  // class .B.C
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    UnitHandler::Process(CSSPropertyID::kPropertyIDBackgroundColor,
                         lepus::Value("red"), tokens->attributes_,
                         parser_configs);
    tokens->MarkParsed();
    std::string key = ".C.B";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexFragment->cascade_map_.insert(std::make_pair(key, tokens));
  }

  // page
  auto page = manager->CreateFiberPage("page", 0);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // dad view
  auto fiber_element_1 = manager->CreateFiberNode("view");
  fiber_element_1->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_1);
  fiber_element_1->SetClass("A");

  // son view
  auto fiber_element_2 = manager->CreateFiberText("view");
  fiber_element_2->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_element_2);
  fiber_element_2->SetClass("C");

  // flush fiber tree
  auto options = std::make_shared<PipelineOptions>();
  options->force_resolve_style_ = true;
  manager->OnPatchFinish(options, page.get());

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  std::string background_color_key = "background-color";

  // EXPECT element == yellow (A C)
  auto painting_node_2 =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  auto node_2_background_color_value =
      painting_node_2->props_.at(background_color_key);
  EXPECT_EQ(node_2_background_color_value.UInt32(), 0xffffff00);

  // SetClass("B")
  fiber_element_1->RemoveAllClass();
  fiber_element_1->SetClass("B");

  // flush fiber tree again
  manager->OnPatchFinish(options, page.get());

  painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  // EXPECT element == red (B C)
  painting_node_2 =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  node_2_background_color_value =
      painting_node_2->props_.at(background_color_key);
  EXPECT_EQ(node_2_background_color_value.UInt32(), 0xffff0000);
}

TEST_P(FiberElementTest, ID_IDCascadeForceFlushCSSParserTest) {
  // construct css fragment.
  StyleMap indexAttributes;
  CSSParserConfigs parser_configs;
  CSSParserTokenMap indexTokenMap;
  // class .A
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    UnitHandler::Process(CSSPropertyID::kPropertyIDBackgroundColor,
                         lepus::Value("blue"), tokens->attributes_,
                         parser_configs);
    tokens->MarkParsed();
    std::string key = ".A";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap font_faces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokenMap, keyframes, font_faces);

  // id #A#C
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    UnitHandler::Process(CSSPropertyID::kPropertyIDBackgroundColor,
                         lepus::Value("yellow"), tokens->attributes_,
                         parser_configs);
    tokens->MarkParsed();
    std::string key = "#C#A";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexFragment->cascade_map_.insert(std::make_pair(key, tokens));
  }

  // id #B#C
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    UnitHandler::Process(CSSPropertyID::kPropertyIDBackgroundColor,
                         lepus::Value("red"), tokens->attributes_,
                         parser_configs);
    tokens->MarkParsed();
    std::string key = "#C#B";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexFragment->cascade_map_.insert(std::make_pair(key, tokens));
  }

  // page
  auto page = manager->CreateFiberPage("page", 0);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // dad view
  auto fiber_element_1 = manager->CreateFiberNode("view");
  fiber_element_1->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_1);
  fiber_element_1->SetIdSelector("A");

  // son view
  auto fiber_element_2 = manager->CreateFiberText("view");
  fiber_element_2->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_element_2);
  fiber_element_2->SetIdSelector("C");

  // flush fiber tree
  auto options = std::make_shared<PipelineOptions>();
  options->force_resolve_style_ = true;
  manager->OnPatchFinish(options, page.get());

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  std::string background_color_key = "background-color";

  // EXPECT element == yellow (A C)
  auto painting_node_2 =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  auto node_2_background_color_value =
      painting_node_2->props_.at(background_color_key);
  EXPECT_EQ(node_2_background_color_value.UInt32(), 0xffffff00);

  // SetID("B")
  fiber_element_1->RemoveAllClass();
  fiber_element_1->SetIdSelector("B");

  // flush fiber tree again
  manager->OnPatchFinish(options, page.get());

  painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  // EXPECT element == red (B C)
  painting_node_2 =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  node_2_background_color_value =
      painting_node_2->props_.at(background_color_key);
  EXPECT_EQ(node_2_background_color_value.UInt32(), 0xffff0000);
}

TEST_P(FiberElementTest, Class_IDCascadeForceFlushCSSParserTest) {
  // construct css fragment.
  StyleMap indexAttributes;
  CSSParserConfigs parser_configs;
  CSSParserTokenMap indexTokenMap;
  // class .A
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    UnitHandler::Process(CSSPropertyID::kPropertyIDBackgroundColor,
                         lepus::Value("blue"), tokens->attributes_,
                         parser_configs);
    tokens->MarkParsed();
    std::string key = ".A";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap font_faces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokenMap, keyframes, font_faces);

  // #A.C
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    UnitHandler::Process(CSSPropertyID::kPropertyIDBackgroundColor,
                         lepus::Value("yellow"), tokens->attributes_,
                         parser_configs);
    tokens->MarkParsed();
    std::string key = ".C#A";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexFragment->cascade_map_.insert(std::make_pair(key, tokens));
  }

  // #B.C
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    UnitHandler::Process(CSSPropertyID::kPropertyIDBackgroundColor,
                         lepus::Value("red"), tokens->attributes_,
                         parser_configs);
    tokens->MarkParsed();
    std::string key = ".C#B";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexFragment->cascade_map_.insert(std::make_pair(key, tokens));
  }

  // page
  auto page = manager->CreateFiberPage("page", 0);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // dad view
  auto fiber_element_1 = manager->CreateFiberNode("view");
  fiber_element_1->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_1);
  fiber_element_1->SetIdSelector("A");

  // son view
  auto fiber_element_2 = manager->CreateFiberText("view");
  fiber_element_2->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_element_2);
  fiber_element_2->SetClass("C");

  // flush fiber tree
  auto options = std::make_shared<PipelineOptions>();
  options->force_resolve_style_ = true;
  manager->OnPatchFinish(options, page.get());

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  std::string background_color_key = "background-color";

  // EXPECT element == yellow (A C)
  auto painting_node_2 =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  auto node_2_background_color_value =
      painting_node_2->props_.at(background_color_key);
  EXPECT_EQ(node_2_background_color_value.UInt32(), 0xffffff00);

  // SetID("B")
  fiber_element_1->RemoveAllClass();
  fiber_element_1->SetIdSelector("B");

  // flush fiber tree again
  manager->OnPatchFinish(options, page.get());

  painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  // EXPECT element == red (B C)
  painting_node_2 =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  node_2_background_color_value =
      painting_node_2->props_.at(background_color_key);
  EXPECT_EQ(node_2_background_color_value.UInt32(), 0xffff0000);
}

TEST_P(FiberElementTest, ID_ClassCascadeForceFlushCSSParserTest) {
  // construct css fragment.
  StyleMap indexAttributes;
  CSSParserConfigs parser_configs;
  CSSParserTokenMap indexTokenMap;
  // class .A
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    UnitHandler::Process(CSSPropertyID::kPropertyIDBackgroundColor,
                         lepus::Value("blue"), tokens->attributes_,
                         parser_configs);
    tokens->MarkParsed();
    std::string key = ".A";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap font_faces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokenMap, keyframes, font_faces);

  // .A#C
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    UnitHandler::Process(CSSPropertyID::kPropertyIDBackgroundColor,
                         lepus::Value("yellow"), tokens->attributes_,
                         parser_configs);
    tokens->MarkParsed();
    std::string key = "#C.A";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexFragment->cascade_map_.insert(std::make_pair(key, tokens));
  }

  // .B#C
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    UnitHandler::Process(CSSPropertyID::kPropertyIDBackgroundColor,
                         lepus::Value("red"), tokens->attributes_,
                         parser_configs);
    tokens->MarkParsed();
    std::string key = "#C.B";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexFragment->cascade_map_.insert(std::make_pair(key, tokens));
  }

  // page
  auto page = manager->CreateFiberPage("page", 0);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // dad view
  auto fiber_element_1 = manager->CreateFiberNode("view");
  fiber_element_1->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_1);
  fiber_element_1->SetClass("A");

  // son view
  auto fiber_element_2 = manager->CreateFiberText("view");
  fiber_element_2->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_element_2);
  fiber_element_2->SetIdSelector("C");

  // flush fiber tree
  auto options = std::make_shared<PipelineOptions>();
  options->force_resolve_style_ = true;
  manager->OnPatchFinish(options, page.get());

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  std::string background_color_key = "background-color";

  // EXPECT element == yellow (A C)
  auto painting_node_2 =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  auto node_2_background_color_value =
      painting_node_2->props_.at(background_color_key);
  EXPECT_EQ(node_2_background_color_value.UInt32(), 0xffffff00);

  // SetID("B")
  fiber_element_1->RemoveAllClass();
  fiber_element_1->SetClass("B");

  // flush fiber tree again
  manager->OnPatchFinish(options, page.get());

  painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  // EXPECT element == red (B C)
  painting_node_2 =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  node_2_background_color_value =
      painting_node_2->props_.at(background_color_key);
  EXPECT_EQ(node_2_background_color_value.UInt32(), 0xffff0000);
}

TEST_P(FiberElementTest, SetClasses) {
  // page
  auto page = manager->CreateFiberPage("page", 11);
  ClassList input = {"dark", "blue", "black"};
  page->SetClasses(std::move(input));
  EXPECT_TRUE(page->classes().size() == 3);
  EXPECT_TRUE(page->classes()[0] == "dark");
  EXPECT_TRUE(page->classes()[1] == "blue");
  EXPECT_TRUE(page->classes()[2] == "black");
}

TEST_P(FiberElementTest, TestSetRawInlineStyles0) {
  auto view = manager->CreateFiberPage("0", 0);

  view->SetRawInlineStyles(
      base::String("background-color:red;border-width:1px;"));

  CSSPropertyID id = CSSPropertyID::kPropertyIDBackgroundColor;
  auto value = lepus::Value("black");
  view->SetStyle(id, value);

  id = CSSPropertyID::kPropertyIDBorderTopWidth;
  value = lepus::Value("2px");
  view->SetStyle(id, value);
  view->FlushActionsAsRoot();

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->platform_impl_.get());
  painting_context->Flush();

  auto node = painting_context->node_map_[view->impl_id()].get();
  EXPECT_TRUE(node != nullptr);

  EXPECT_EQ(node->props_["background-color"],
            lepus::Value(static_cast<uint32_t>(4278190080)));
  EXPECT_EQ(node->props_["border-bottom-width"],
            lepus::Value(static_cast<double>(1)));
  EXPECT_EQ(node->props_["border-left-width"],
            lepus::Value(static_cast<double>(1)));
  EXPECT_EQ(node->props_["border-right-width"],
            lepus::Value(static_cast<double>(1)));
  EXPECT_EQ(node->props_["border-top-width"],
            lepus::Value(static_cast<double>(2)));
}

TEST_P(FiberElementTest, TestSetRawInlineStyles01) {
  lynx::base::AutoReset<bool> css_inline_config(
      &(manager->GetConfig()->css_configs_.enable_css_inline_variables_), true);

  auto page = manager->CreateFiberPage("0", 0);
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->platform_impl_.get());

  {
    // page inline style: "background-color:var(--bg-color); --bg-color: red;"
    page->SetRawInlineStyles(
        "background-color:var(--bg-color); --bg-color: red;");
    page->FlushActionsAsRoot();

    auto painting_context = static_cast<FiberMockPaintingContext*>(
        manager->painting_context()->platform_impl_.get());
    painting_context->Flush();

    auto node = painting_context->node_map_[page->impl_id()].get();
    EXPECT_TRUE(node != nullptr);

    EXPECT_EQ(node->props_["background-color"],
              lepus::Value(static_cast<uint32_t>(kTestColorMap["red"])));
  }

  auto text = manager->CreateFiberText("text");
  {
    // text inline style: "background-color:var(--bg-color);"
    text->SetStyle(CSSPropertyID::kPropertyIDBackgroundColor,
                   lepus::Value("var(--bg-color)"));
    page->InsertNode(text);
    page->FlushActionsAsRoot();
    painting_context->Flush();
    auto node = painting_context->node_map_[text->impl_id()].get();
    EXPECT_TRUE(node != nullptr);
    EXPECT_EQ(node->props_["background-color"],
              lepus::Value(static_cast<uint32_t>(kTestColorMap["red"])));
  }

  {
    // text inline style: "background-color:var(--bg-color); --bg-color:
    // black"
    text->RemoveAllInlineStyles();
    text->SetRawInlineStyles(
        "background-color:var(--bg-color); --bg-color: black");
    page->FlushActionsAsRoot();
    painting_context->Flush();
    auto node = painting_context->node_map_[text->impl_id()].get();
    EXPECT_TRUE(node != nullptr);
    EXPECT_EQ(node->props_["background-color"],
              lepus::Value(static_cast<uint32_t>(kTestColorMap["black"])));
  }

  {
    // page inline style: "background-color:var(--bg-color); --bg-color:
    // green"
    page->RemoveAllInlineStyles();
    page->SetRawInlineStyles(
        "background-color:var(--bg-color); --bg-color: green");

    // text inline style: "background-color:var(--bg-color);"
    text->RemoveAllInlineStyles();
    text->SetRawInlineStyles("background-color:var(--bg-color);");
    page->FlushActionsAsRoot();
    painting_context->Flush();
    auto node = painting_context->node_map_[page->impl_id()].get();
    EXPECT_TRUE(node != nullptr);
    EXPECT_EQ(node->props_["background-color"],
              lepus::Value(static_cast<uint32_t>(kTestColorMap["green"])));
    node = painting_context->node_map_[text->impl_id()].get();
    EXPECT_TRUE(node != nullptr);
    EXPECT_EQ(node->props_["background-color"],
              lepus::Value(static_cast<uint32_t>(kTestColorMap["green"])));
  }
}

TEST_P(FiberElementTest, TestSetRawInlineStyles02) {
  lynx::base::AutoReset<bool> css_inline_config(
      &(manager->GetConfig()->css_configs_.enable_css_inline_variables_), true);

  auto page = manager->CreateFiberPage("0", 0);
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->platform_impl_.get());
  auto text1 = manager->CreateFiberText("text1");
  auto text2 = manager->CreateFiberText("text2");
  page->InsertNode(text1);
  page->InsertNode(text2);

  {
    // page inline style: "background-color:var(--bg-color); --bg-color: red;"
    // text1 inline style: "background-color:var(--bg-color);"
    // text2 inline style: "background-color:var(--bg-color);"
    page->SetRawInlineStyles(
        "background-color:var(--bg-color); --bg-color: red;");
    text1->SetStyle(CSSPropertyID::kPropertyIDBackgroundColor,
                    lepus::Value("var(--bg-color)"));
    text2->SetStyle(CSSPropertyID::kPropertyIDBackgroundColor,
                    lepus::Value("var(--bg-color)"));
    page->FlushActionsAsRoot();
    painting_context->Flush();
    auto node = painting_context->node_map_[text1->impl_id()].get();
    EXPECT_TRUE(node != nullptr);
    EXPECT_EQ(node->props_["background-color"],
              lepus::Value(static_cast<uint32_t>(kTestColorMap["red"])));
    node = painting_context->node_map_[text2->impl_id()].get();
    EXPECT_TRUE(node != nullptr);
    EXPECT_EQ(node->props_["background-color"],
              lepus::Value(static_cast<uint32_t>(kTestColorMap["red"])));
  }

  {
    // page inline style: "background-color:var(--bg-color); --bg-color:
    // black" text1 inline style: "background-color:var(--bg-color);" text2
    // inline style: "background-color:var(--bg-color);"
    page->RemoveAllInlineStyles();
    page->SetRawInlineStyles(
        "background-color:var(--bg-color); --bg-color: black");
    page->FlushActionsAsRoot();
    painting_context->Flush();
    auto node = painting_context->node_map_[text1->impl_id()].get();
    EXPECT_TRUE(node != nullptr);
    EXPECT_EQ(node->props_["background-color"],
              lepus::Value(static_cast<uint32_t>(kTestColorMap["black"])));
    node = painting_context->node_map_[text2->impl_id()].get();
    EXPECT_TRUE(node != nullptr);
    EXPECT_EQ(node->props_["background-color"],
              lepus::Value(static_cast<uint32_t>(kTestColorMap["black"])));
  }

  {
    // page inline style: "background-color:var(--bg-color); --bg-color:
    // green" text1 inline style: "background-color:var(--bg-color);
    // --bg-color: red" text2 inline style:
    // "background-color:var(--bg-color);"
    page->SetRawInlineStyles(
        "background-color:var(--bg-color); --bg-color: green");
    text1->RemoveAllInlineStyles();
    text1->SetRawInlineStyles(
        "background-color:var(--bg-color); --bg-color: red");
    page->FlushActionsAsRoot();
    painting_context->Flush();
    auto node = painting_context->node_map_[text1->impl_id()].get();
    EXPECT_TRUE(node != nullptr);
    EXPECT_EQ(node->props_["background-color"],
              lepus::Value(static_cast<uint32_t>(kTestColorMap["red"])));
    node = painting_context->node_map_[text2->impl_id()].get();
    EXPECT_TRUE(node != nullptr);
    EXPECT_EQ(node->props_["background-color"],
              lepus::Value(static_cast<uint32_t>(kTestColorMap["green"])));
  }

  {
    // text1 inline style: "background-color:var(--bg-color);"
    text1->RemoveAllInlineStyles();
    text1->SetRawInlineStyles("background-color:var(--bg-color);");
    page->FlushActionsAsRoot();
    painting_context->Flush();
    auto node = painting_context->node_map_[text1->impl_id()].get();
    EXPECT_TRUE(node != nullptr);
    EXPECT_EQ(node->props_["background-color"],
              lepus::Value(static_cast<uint32_t>(kTestColorMap["green"])));
  }
}

TEST_P(FiberElementTest, TestSetRawInlineStyles03) {
  lynx::base::AutoReset<bool> css_inline_config(
      &(manager->GetConfig()->css_configs_.enable_css_inline_variables_), true);

  // page
  // └── text1
  //     └── text2
  auto page = manager->CreateFiberPage("0", 0);
  auto view1 = manager->CreateFiberView();
  auto view2 = manager->CreateFiberView();
  page->InsertNode(view1);
  view1->InsertNode(view2);

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->platform_impl_.get());

  {
    // page inline style: "--bg-color: red; --border: 1px solid
    // var(--bg-color);"
    // view1 inline style: "background-color: var(--no-color,
    // var(--bg-color)); border-bottom: var(--border);"
    // view2 inline style: "border-bottom: var(--no-border, var(--some-border,
    // var(--border)));"
    page->SetRawInlineStyles(
        "--bg-color: red; --border: 1px dashed var(--bg-color);");
    view1->SetRawInlineStyles(
        "background-color: var(--no-color, var(--bg-color)); border-bottom: "
        "var(--border);");
    view2->SetRawInlineStyles(
        "border-bottom: var(--no-border, var(--some-border, "
        "var(--border)));");
    page->FlushActionsAsRoot();
    painting_context->Flush();
    auto node = painting_context->node_map_[view1->impl_id()].get();
    EXPECT_TRUE(node != nullptr);
    EXPECT_EQ(node->props_["background-color"],
              lepus::Value(static_cast<uint32_t>(kTestColorMap["red"])));
    EXPECT_EQ(node->props_["border-bottom-width"],
              lepus::Value(static_cast<double>(1)));
    EXPECT_EQ(node->props_["border-bottom-style"],
              lepus::Value(
                  static_cast<uint8_t>(starlight::BorderStyleType::kDashed)));
    EXPECT_EQ(node->props_["border-bottom-color"],
              lepus::Value(static_cast<uint32_t>(kTestColorMap["red"])));
    node = painting_context->node_map_[view2->impl_id()].get();
    EXPECT_TRUE(node != nullptr);
    EXPECT_EQ(node->props_["border-bottom-width"],
              lepus::Value(static_cast<double>(1)));
    EXPECT_EQ(node->props_["border-bottom-style"],
              lepus::Value(
                  static_cast<uint8_t>(starlight::BorderStyleType::kDashed)));
    EXPECT_EQ(node->props_["border-bottom-color"],
              lepus::Value(static_cast<uint32_t>(kTestColorMap["red"])));
  }

  {
    // view1 inline style: "--some-border: 2px dotted green;"
    // view2 inline style: "border-bottom: var(--no-border, var(--some-border,
    // var(--border)));"
    view1->RemoveAllInlineStyles();
    view1->SetRawInlineStyles("--some-border: 2px dotted green;");
    page->FlushActionsAsRoot();
    painting_context->Flush();

    // view2 inline style: "border-bottom: 2px dotted green;"
    auto node = painting_context->node_map_[view2->impl_id()].get();
    EXPECT_TRUE(node != nullptr);
    EXPECT_EQ(node->props_["border-bottom-width"],
              lepus::Value(static_cast<double>(2)));
    EXPECT_EQ(node->props_["border-bottom-style"],
              lepus::Value(
                  static_cast<uint8_t>(starlight::BorderStyleType::kDotted)));
    EXPECT_EQ(node->props_["border-bottom-color"],
              lepus::Value(static_cast<uint32_t>(kTestColorMap["green"])));
  }
}

TEST_P(FiberElementTest, TestSetRawInlineStyles04) {
  lynx::base::AutoReset<bool> css_inline_config(
      &(manager->GetConfig()->css_configs_.enable_css_inline_variables_), true);

  // page
  // └── text1
  //     └── text2
  auto page = manager->CreateFiberPage("0", 0);
  auto view1 = manager->CreateFiberView();
  auto view2 = manager->CreateFiberView();
  page->InsertNode(view1);
  view1->InsertNode(view2);

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->platform_impl_.get());

  {
    // page inline style: "--color: var(--green, red);"
    // view1 inline style: "--green: green; color: var(--color);"
    // view2 inline style: "color: var(--color);"
    page->SetRawInlineStyles("--color: var(--green, red);");
    view1->SetRawInlineStyles("--green: green; color: var(--color);");
    view2->SetRawInlineStyles("color: var(--color);");
    page->FlushActionsAsRoot();
    painting_context->Flush();
    auto node = painting_context->node_map_[view1->impl_id()].get();
    EXPECT_TRUE(node != nullptr);
    EXPECT_EQ(node->props_["color"],
              lepus::Value(static_cast<uint32_t>(kTestColorMap["red"])));
    node = painting_context->node_map_[view2->impl_id()].get();
    EXPECT_TRUE(node != nullptr);
    EXPECT_EQ(node->props_["color"],
              lepus::Value(static_cast<uint32_t>(kTestColorMap["red"])));
  }

  {
    // view1 inline style: "--green: green; --color: var(--green, red);"
    // view2 inline style: "color: var(--color);"
    view1->RemoveAllInlineStyles();
    view1->SetRawInlineStyles("--green: green; --color: var(--green, red);");
    page->FlushActionsAsRoot();
    painting_context->Flush();

    auto node = painting_context->node_map_[view2->impl_id()].get();
    EXPECT_TRUE(node != nullptr);
    EXPECT_EQ(node->props_["color"],
              lepus::Value(static_cast<uint32_t>(kTestColorMap["green"])));
  }
}

TEST_P(FiberElementTest, TestGetComputedStyleByKey) {
  auto page = manager->CreateFiberPage("page", 11);
  page->computed_css_style()->opacity_ = 0.900000f;
  EXPECT_TRUE(page->GetComputedStyleByKey("opacity").IsString());
  EXPECT_TRUE(page->GetComputedStyleByKey("opacity").StdString() == "0.9");
}

TEST_P(FiberElementTest, TestGetComputedStyleByKey_transform_translate) {
  //  constructor css fragment
  StyleMap indexAttributes;
  CSSParserTokenMap indexTokensMap;

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap font_faces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokensMap, keyframes, font_faces);

  // parent
  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  auto view = manager->CreateFiberView();
  view->parent_component_element_ = page.get();
  view->SetStyle(CSSPropertyID::kPropertyIDTransform,
                 lepus::Value("translateX(50px)"));
  page->InsertNode(view);

  page->FlushActionsAsRoot();

  auto lepus_transform_value = view->GetComputedStyleByKey("transform");
  EXPECT_TRUE(lepus_transform_value.IsString());
  EXPECT_TRUE(lepus_transform_value.StdString() == "matrix(1, 0, 0, 1, 50, 0)");

  view->SetStyle(CSSPropertyID::kPropertyIDTransform,
                 lepus::Value("translateY(75px)"));
  page->FlushActionsAsRoot();
  lepus_transform_value = view->GetComputedStyleByKey("transform");
  EXPECT_TRUE(lepus_transform_value.IsString());
  EXPECT_TRUE(lepus_transform_value.StdString() == "matrix(1, 0, 0, 1, 0, 75)");

  view->SetStyle(CSSPropertyID::kPropertyIDTransform,
                 lepus::Value("translateZ(100px)"));
  page->FlushActionsAsRoot();
  lepus_transform_value = view->GetComputedStyleByKey("transform");
  EXPECT_TRUE(lepus_transform_value.IsString());
  EXPECT_TRUE(lepus_transform_value.StdString() ==
              "matrix3d(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 100, 1)");

  view->SetStyle(CSSPropertyID::kPropertyIDTransform,
                 lepus::Value("translate(50px, 75px)"));
  page->FlushActionsAsRoot();
  lepus_transform_value = view->GetComputedStyleByKey("transform");
  EXPECT_TRUE(lepus_transform_value.IsString());
  EXPECT_TRUE(lepus_transform_value.StdString() ==
              "matrix(1, 0, 0, 1, 50, 75)");

  view->SetStyle(CSSPropertyID::kPropertyIDTransform,
                 lepus::Value("translate3d(10px, 20px, 30px)"));
  page->FlushActionsAsRoot();
  lepus_transform_value = view->GetComputedStyleByKey("transform");
  EXPECT_TRUE(lepus_transform_value.IsString());
  EXPECT_TRUE(lepus_transform_value.StdString() ==
              "matrix3d(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 10, 20, 30, 1)");
}

TEST_P(FiberElementTest, TestGetComputedStyleByKey_transform_rotate) {
  //  constructor css fragment
  StyleMap indexAttributes;
  CSSParserTokenMap indexTokensMap;

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap font_faces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokensMap, keyframes, font_faces);

  // parent
  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  auto view = manager->CreateFiberView();
  view->parent_component_element_ = page.get();
  view->SetStyle(CSSPropertyID::kPropertyIDTransform,
                 lepus::Value("rotateX(45deg)"));
  page->InsertNode(view);

  page->FlushActionsAsRoot();

  auto lepus_transform_value = view->GetComputedStyleByKey("transform");
  EXPECT_TRUE(lepus_transform_value.IsString());
  EXPECT_TRUE(lepus_transform_value.StdString() ==
              "matrix3d(1, 0, 0, 0, 0, 0.707107, 0.707107, 0, 0, -0.707107, "
              "0.707107, 0, 0, 0, 0, 1)");

  view->SetStyle(CSSPropertyID::kPropertyIDTransform,
                 lepus::Value("rotateY(60deg)"));
  page->FlushActionsAsRoot();
  lepus_transform_value = view->GetComputedStyleByKey("transform");
  EXPECT_TRUE(lepus_transform_value.IsString());
  EXPECT_TRUE(lepus_transform_value.StdString() ==
              "matrix3d(0.5, 0, -0.866025, 0, 0, 1, 0, 0, 0.866025, 0, 0.5, 0, "
              "0, 0, 0, 1)");

  view->SetStyle(CSSPropertyID::kPropertyIDTransform,
                 lepus::Value("rotateZ(90deg)"));
  page->FlushActionsAsRoot();
  lepus_transform_value = view->GetComputedStyleByKey("transform");
  EXPECT_TRUE(lepus_transform_value.IsString());
  EXPECT_TRUE(lepus_transform_value.StdString() == "matrix(0, 1, -1, 0, 0, 0)");
}

TEST_P(FiberElementTest, TestGetComputedStyleByKey_transform_scale) {
  //  constructor css fragment
  StyleMap indexAttributes;
  CSSParserTokenMap indexTokensMap;

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap font_faces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokensMap, keyframes, font_faces);

  // parent
  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  auto view = manager->CreateFiberView();
  view->parent_component_element_ = page.get();
  view->SetStyle(CSSPropertyID::kPropertyIDTransform,
                 lepus::Value("scale(1.5)"));
  page->InsertNode(view);

  page->FlushActionsAsRoot();
  auto lepus_transform_value = view->GetComputedStyleByKey("transform");
  EXPECT_TRUE(lepus_transform_value.IsString());
  EXPECT_TRUE(lepus_transform_value.StdString() ==
              "matrix(1.5, 0, 0, 1.5, 0, 0)");

  view->SetStyle(CSSPropertyID::kPropertyIDTransform,
                 lepus::Value("scaleX(1.2)"));
  page->FlushActionsAsRoot();
  lepus_transform_value = view->GetComputedStyleByKey("transform");
  EXPECT_TRUE(lepus_transform_value.IsString());
  EXPECT_TRUE(lepus_transform_value.StdString() ==
              "matrix(1.2, 0, 0, 1, 0, 0)");

  view->SetStyle(CSSPropertyID::kPropertyIDTransform,
                 lepus::Value("scaleY(0.8)"));
  page->FlushActionsAsRoot();
  lepus_transform_value = view->GetComputedStyleByKey("transform");
  EXPECT_TRUE(lepus_transform_value.IsString());
  EXPECT_TRUE(lepus_transform_value.StdString() ==
              "matrix(1, 0, 0, 0.8, 0, 0)");

  view->SetStyle(CSSPropertyID::kPropertyIDTransform,
                 lepus::Value("scale(1.2, 0.8)"));
  page->FlushActionsAsRoot();
  lepus_transform_value = view->GetComputedStyleByKey("transform");
  EXPECT_TRUE(lepus_transform_value.IsString());
  EXPECT_TRUE(lepus_transform_value.StdString() ==
              "matrix(1.2, 0, 0, 0.8, 0, 0)");
}

TEST_P(FiberElementTest, TestGetComputedStyleByKey_transform_skew) {
  //  constructor css fragment
  StyleMap indexAttributes;
  CSSParserTokenMap indexTokensMap;

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap font_faces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokensMap, keyframes, font_faces);

  // parent
  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  auto view = manager->CreateFiberView();
  view->parent_component_element_ = page.get();
  view->SetStyle(CSSPropertyID::kPropertyIDTransform,
                 lepus::Value("skewX(15deg)"));
  page->InsertNode(view);

  page->FlushActionsAsRoot();
  auto lepus_transform_value = view->GetComputedStyleByKey("transform");
  EXPECT_TRUE(lepus_transform_value.IsString());
  auto skew_value = lepus_transform_value.StdString();
  EXPECT_TRUE(skew_value == "matrix(1, 0, 0.267949, 1, 0, 0)");

  view->SetStyle(CSSPropertyID::kPropertyIDTransform,
                 lepus::Value("skewY(10deg)"));
  page->FlushActionsAsRoot();
  lepus_transform_value = view->GetComputedStyleByKey("transform");
  EXPECT_TRUE(lepus_transform_value.IsString());
  skew_value = lepus_transform_value.StdString();
  EXPECT_TRUE(skew_value == "matrix(1, 0.176327, 0, 1, 0, 0)");
}

TEST_P(FiberElementTest, TestGetComputedStyleByKey_transform_raw_matrix) {
  //  constructor css fragment
  StyleMap indexAttributes;
  CSSParserTokenMap indexTokensMap;

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap font_faces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokensMap, keyframes, font_faces);

  // parent
  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  auto view = manager->CreateFiberView();
  view->parent_component_element_ = page.get();
  view->SetStyle(CSSPropertyID::kPropertyIDTransform,
                 lepus::Value("matrix(1, 0, 0, 1, 0, 0)"));
  page->InsertNode(view);

  page->FlushActionsAsRoot();
  auto lepus_transform_value = view->GetComputedStyleByKey("transform");
  EXPECT_TRUE(lepus_transform_value.IsString());
  EXPECT_TRUE(lepus_transform_value.StdString() == "matrix(1, 0, 0, 1, 0, 0)");

  view->SetStyle(
      CSSPropertyID::kPropertyIDTransform,
      lepus::Value(
          "matrix3d(0.983578, 0.174418, 0.059386, 0, -0.222342, 0.811802, "
          "-0.198322, 0, -0.015242, 0.22915, 1.25884, 0, 10, 20, 30, 1)"));
  page->FlushActionsAsRoot();
  lepus_transform_value = view->GetComputedStyleByKey("transform");
  EXPECT_TRUE(lepus_transform_value.IsString());
  EXPECT_TRUE(lepus_transform_value.StdString() ==
              "matrix3d(0.983578, 0.174418, 0.059386, 0, -0.222342, 0.811802, "
              "-0.198322, 0, -0.015242, 0.22915, 1.25884, 0, 10, 20, 30, 1)");
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
