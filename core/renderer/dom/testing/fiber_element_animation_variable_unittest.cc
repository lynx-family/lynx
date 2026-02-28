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

TEST_P(FiberElementTest, UpdateCSSVariables_0) {
  // construct css fragment
  StyleMap indexAttributes;
  CSSParserConfigs parser_configs;

  CSSParserTokenMap indexTokenMap;
  // class :root
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->style_variables_.insert_or_assign("--main-bg-color", "yellow");
    tokens->style_variables_.insert_or_assign("--main-height", "300px");

    std::string key = ":root";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  // class .one
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDColor] =
        CSSValue::MakePlainString("white");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDWidth] =
        CSSValue::MakePlainString("100%");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDHeight] =
        CSSValue::MakePlainString("300px");
    tokens->style_variables_.insert_or_assign("--main-bg-color", "yellow");
    tokens->style_variables_.insert_or_assign("--main-height", "300px");
    std::string key = ".one";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  // class .three
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDColor] =
        CSSValue::MakePlainString("white");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue("{{--main-bg-color}}", CSSValuePattern::STRING,
                 CSSValueType::VARIABLE);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDWidth] =
        CSSValue::MakePlainString("50%");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDHeight] = CSSValue(
        "{{--main-height}}", CSSValuePattern::STRING, CSSValueType::VARIABLE);
    std::string key = ".three";
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

  // parent
  auto page = manager->CreateFiberPage("page", 0);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // view1
  auto fiber_element_1 = manager->CreateFiberNode("view");
  fiber_element_1->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_1);
  fiber_element_1->SetClass("one");
  fiber_element_1->SetIdSelector("root");

  // view4
  auto fiber_element_4 = manager->CreateFiberNode("view");
  fiber_element_4->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_element_4);
  fiber_element_4->SetClass("three");
  fiber_element_4->SetIdSelector("test1");

  page->FlushActionsAsRoot();
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  EXPECT_TRUE(painting_context->node_map_.find(fiber_element_4->impl_id()) !=
              painting_context->node_map_.end());

  auto* painting_node_4 =
      painting_context->node_map_.at(fiber_element_4->impl_id()).get();
  std::string background_color_key = "background-color";

  auto node_4_background_color_value =
      painting_node_4->props_.at(background_color_key);
  EXPECT_TRUE(node_4_background_color_value.UInt32() == 0xffffff00);
}

TEST_P(FiberElementTest, UpdateCSSVariables_1) {
  // construct css fragment
  StyleMap indexAttributes;
  CSSParserConfigs parser_configs;

  CSSParserTokenMap indexTokenMap;
  // class :root
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->style_variables_.insert_or_assign("--main-bg-color", "yellow");
    tokens->style_variables_.insert_or_assign("--main-height", "300px");

    std::string key = ":root";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  // class .one
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDColor] =
        CSSValue::MakePlainString("white");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDWidth] =
        CSSValue::MakePlainString("100%");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDHeight] =
        CSSValue::MakePlainString("300px");
    tokens->style_variables_.insert_or_assign("--main-bg-color", "yellow");
    std::string key = ".one";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  // class .two
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDColor] =
        CSSValue::MakePlainString("white");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDWidth] =
        CSSValue::MakePlainString("100%");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDHeight] =
        CSSValue::MakePlainString("300px");
    tokens->style_variables_.insert_or_assign("--main-bg-color", "green");
    std::string key = ".two";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  // class .three
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDColor] =
        CSSValue::MakePlainString("white");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue("{{--main-bg-color}}", CSSValuePattern::STRING,
                 CSSValueType::VARIABLE);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDWidth] =
        CSSValue::MakePlainString("50%");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDHeight] =
        CSSValue::MakePlainString("50%");
    std::string key = ".three";
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

  // parent
  auto page = manager->CreateFiberPage("page", 0);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // view1
  auto fiber_element_1 = manager->CreateFiberNode("view");
  fiber_element_1->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_1);
  fiber_element_1->SetClass("one");
  fiber_element_1->SetIdSelector("root");

  // view4
  auto fiber_element_4 = manager->CreateFiberNode("view");
  fiber_element_4->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_element_4);
  fiber_element_4->SetClass("three");
  fiber_element_4->SetIdSelector("test1");

  page->FlushActionsAsRoot();
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* painting_node_4 =
      painting_context->node_map_.at(fiber_element_4->impl_id()).get();
  std::string background_color_key = "background-color";

  auto node_4_background_color_value =
      painting_node_4->props_.at(background_color_key);
  EXPECT_TRUE(node_4_background_color_value.UInt32() == 0xffffff00);

  fiber_element_1->SetClass("two");
  page->FlushActionsAsRoot();
  painting_context->Flush();

  node_4_background_color_value =
      painting_node_4->props_.at(background_color_key);
  EXPECT_TRUE(node_4_background_color_value.UInt32() == 0xff008000);
}

TEST_P(FiberElementTest, UpdateCSSVariables_CSS_NG_1) {
  // construct css fragment
  StyleMap indexAttributes;
  CSSParserConfigs parser_configs;

  CSSParserTokenMap indexTokenMap;
  // class :root
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->style_variables_.insert_or_assign("--main-bg-color", "yellow");
    tokens->style_variables_.insert_or_assign("--main-height", "300px");

    std::string key = ":root";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  // class .one
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDColor] =
        CSSValue::MakePlainString("white");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDWidth] =
        CSSValue::MakePlainString("100%");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDHeight] =
        CSSValue::MakePlainString("300px");
    tokens->style_variables_.insert_or_assign("--main-bg-color", "yellow");
    std::string key = ".one";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  // class .two
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDColor] =
        CSSValue::MakePlainString("white");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDWidth] =
        CSSValue::MakePlainString("100%");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDHeight] =
        CSSValue::MakePlainString("300px");
    tokens->style_variables_.insert_or_assign("--main-bg-color", "green");
    std::string key = ".two";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  // class .three
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDColor] =
        CSSValue::MakePlainString("white");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue("{{--main-bg-color}}", CSSValuePattern::STRING,
                 CSSValueType::VARIABLE);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDWidth] =
        CSSValue::MakePlainString("50%");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDHeight] =
        CSSValue::MakePlainString("50%");
    std::string key = ".three";
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

  indexFragment->SetEnableCSSSelector();
  for (const auto& pair : indexTokenMap) {
    encoder::LynxCSSSelectorTuple selector_tuple;
    css::CSSParserContext context;
    auto selector_key = pair.first;
    css::CSSTokenizer tokenizer(selector_key);

    const auto parser_tokens = tokenizer.TokenizeToEOF();
    css::CSSParserTokenRange range(parser_tokens);
    css::LynxCSSSelectorVector selector_vector =
        css::CSSSelectorParser::ParseSelector(range, &context);

    size_t flattened_size =
        css::CSSSelectorParser::FlattenedSize(selector_vector);

    selector_tuple.selector_key = selector_key;
    selector_tuple.flattened_size = flattened_size;
    selector_tuple.selector_arr =
        std::make_unique<css::LynxCSSSelector[]>(flattened_size);
    css::CSSSelectorParser::AdoptSelectorVector(
        selector_vector, selector_tuple.selector_arr.get(), flattened_size);

    selector_tuple.parse_token = pair.second;

    indexFragment->AddStyleRule(std::move(selector_tuple.selector_arr),
                                std::move(selector_tuple.parse_token));
  }

  // parent
  auto page = manager->CreateFiberPage("page", 0);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // view1
  auto fiber_element_1 = manager->CreateFiberNode("view");
  fiber_element_1->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_1);
  fiber_element_1->SetClass("one");
  fiber_element_1->SetIdSelector("root");

  // view4
  auto fiber_element_4 = manager->CreateFiberNode("view");
  fiber_element_4->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_element_4);
  fiber_element_4->SetClass("three");
  fiber_element_4->SetIdSelector("test1");

  page->FlushActionsAsRoot();
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* painting_node_4 =
      painting_context->node_map_.at(fiber_element_4->impl_id()).get();
  std::string background_color_key = "background-color";

  auto node_4_background_color_value =
      painting_node_4->props_.at(background_color_key);
  EXPECT_TRUE(node_4_background_color_value.UInt32() == 0xffffff00);

  fiber_element_1->RemoveAllClass();
  fiber_element_1->SetClass("two");
  page->FlushActionsAsRoot();
  painting_context->Flush();

  node_4_background_color_value =
      painting_node_4->props_.at(background_color_key);
  EXPECT_TRUE(node_4_background_color_value.UInt32() == 0xff008000);
}

TEST_P(FiberElementTest, UpdateMultipleCSSVariables) {
  // construct css fragment
  StyleMap indexAttributes;
  CSSParserConfigs parser_configs;

  CSSParserTokenMap indexTokenMap;

  // class .one
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->style_variables_.insert_or_assign("--main-bg-color", "yellow");
    tokens->style_variables_.insert_or_assign("--color-2", "red");
    tokens->style_variables_.insert_or_assign("--color-4", "green");
    tokens->style_variables_.insert_or_assign("--color-6", "blue");
    tokens->style_variables_.insert_or_assign("--color-8", "yellow");
    tokens->style_variables_.insert_or_assign("--color-10", "pink");
    tokens->style_variables_.insert_or_assign("--color-12", "black");
    tokens->style_variables_.insert_or_assign("--color-14", "white");
    tokens->style_variables_.insert_or_assign("--color-16", "red");
    tokens->style_variables_.insert_or_assign("--color-18", "green");
    tokens->style_variables_.insert_or_assign("--color-20", "blue");
    tokens->style_variables_.insert_or_assign("--color-22", "yellow");
    tokens->style_variables_.insert_or_assign("--color-24", "pink");
    std::string key = ".one";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  // class .two
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->style_variables_.insert_or_assign("--main-height", "100px");
    tokens->style_variables_.insert_or_assign("--color-2", "red");
    tokens->style_variables_.insert_or_assign("--color-4", "green");
    tokens->style_variables_.insert_or_assign("--color-6", "blue");
    tokens->style_variables_.insert_or_assign("--color-8", "yellow");
    tokens->style_variables_.insert_or_assign("--color-10", "pink");
    tokens->style_variables_.insert_or_assign("--color-12", "black");
    tokens->style_variables_.insert_or_assign("--color-14", "white");
    tokens->style_variables_.insert_or_assign("--color-16", "red");
    tokens->style_variables_.insert_or_assign("--color-18", "green");
    tokens->style_variables_.insert_or_assign("--color-20", "blue");
    tokens->style_variables_.insert_or_assign("--color-22", "yellow");
    tokens->style_variables_.insert_or_assign("--color-24", "pink");
    std::string key = ".one";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  // class .three
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue("{{--main-bg-color}}", CSSValuePattern::STRING,
                 CSSValueType::VARIABLE);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDHeight] = CSSValue(
        "{{--main-height}}", CSSValuePattern::STRING, CSSValueType::VARIABLE);
    std::string key = ".three";
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

  // page
  auto page = manager->CreateFiberPage("page", 0);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // parent
  auto parent = manager->CreateFiberView();
  parent->parent_component_element_ = page.get();
  page->InsertNode(parent);

  // child 1-0
  auto child_1_0 = manager->CreateFiberView();
  child_1_0->parent_component_element_ = page.get();
  parent->InsertNode(child_1_0);

  // child 1-1
  auto child_1_1 = manager->CreateFiberView();
  child_1_1->parent_component_element_ = page.get();
  child_1_0->InsertNode(child_1_1);

  // child 2-0
  auto child_2_0 = manager->CreateFiberView();
  child_2_0->parent_component_element_ = page.get();
  parent->InsertNode(child_2_0);

  // child 2-1
  auto child_2_1 = manager->CreateFiberView();
  child_2_1->parent_component_element_ = page.get();
  child_2_0->InsertNode(child_2_1);

  // child 3-0
  auto child_3_0 = manager->CreateFiberView();
  child_3_0->parent_component_element_ = page.get();
  parent->InsertNode(child_3_0);

  // child 3-1
  auto child_3_1 = manager->CreateFiberView();
  child_3_1->parent_component_element_ = page.get();
  child_3_0->InsertNode(child_3_1);

  // child 4-0
  auto child_4_0 = manager->CreateFiberView();
  child_4_0->parent_component_element_ = page.get();
  parent->InsertNode(child_4_0);

  // child 3-1
  auto child_4_1 = manager->CreateFiberView();
  child_4_1->parent_component_element_ = page.get();
  child_4_0->InsertNode(child_4_1);

  page->FlushActionsAsRoot();

  parent->SetClass("one");
  child_1_0->SetClasses({"two", "three"});
  child_1_1->SetClasses({"two", "three"});
  child_2_0->SetClasses({"two", "three"});
  child_2_1->SetClasses({"two", "three"});
  child_3_0->SetClasses({"two", "three"});
  child_3_1->SetClasses({"two", "three"});
  child_4_0->SetClasses({"two", "three"});
  child_4_1->SetClasses({"two", "three"});

  page->FlushActionsAsRoot();
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* painting_node_4 =
      painting_context->node_map_.at(child_4_1->impl_id()).get();
  std::string background_color_key = "background-color";

  auto node_4_background_color_value =
      painting_node_4->props_.at(background_color_key);
  EXPECT_TRUE(node_4_background_color_value.UInt32() == 0xffffff00);
}

TEST_P(FiberElementTest, UpdateCSSVariables) {
  // construct css fragment
  StyleMap indexAttributes;
  CSSParserConfigs parser_configs;

  CSSParserTokenMap indexTokenMap;
  // class :root
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->style_variables_.insert_or_assign("--main-bg-color", "yellow");
    tokens->style_variables_.insert_or_assign("--main-height", "300px");

    std::string key = ":root";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  // class .one
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDColor] =
        CSSValue::MakePlainString("white");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDWidth] =
        CSSValue::MakePlainString("100%");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDHeight] =
        CSSValue::MakePlainString("300px");
    tokens->style_variables_.insert_or_assign("--main-bg-color", "yellow");
    tokens->style_variables_.insert_or_assign("--main-height", "300px");
    std::string key = ".one";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  // class .two
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->style_variables_.insert_or_assign("--main-height", "100px");
    tokens->style_variables_.insert_or_assign("--main-bg-color", "pink");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDColor] =
        CSSValue::MakePlainString("white");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue::MakePlainString("black");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDWidth] =
        CSSValue::MakePlainString("100%");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDHeight] =
        CSSValue::MakePlainString("100%");
    std::string key = ".two";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  // class .three
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDColor] =
        CSSValue::MakePlainString("white");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue("{{--main-bg-color}}", CSSValuePattern::STRING,
                 CSSValueType::VARIABLE);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDWidth] =
        CSSValue::MakePlainString("50%");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDHeight] = CSSValue(
        "{{--main-height}}", CSSValuePattern::STRING, CSSValueType::VARIABLE);
    std::string key = ".three";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  // class .four
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue("{{--main-bg-color}}", CSSValuePattern::STRING,
                 CSSValueType::VARIABLE);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDWidth] =
        CSSValue::MakePlainString("25%");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDHeight] =
        CSSValue("calc({{--main-height}} - 50px)", CSSValuePattern::STRING,
                 CSSValueType::VARIABLE);
    std::string key = ".four";
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

  // parent
  auto page = manager->CreateFiberPage("page", 0);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // view1
  auto fiber_element_1 = manager->CreateFiberNode("view");
  fiber_element_1->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_1);
  fiber_element_1->SetClass("one");
  fiber_element_1->SetIdSelector("root");

  // view2
  auto fiber_element_2 = manager->CreateFiberNode("view");
  fiber_element_2->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_element_2);
  fiber_element_2->SetClass("two");
  fiber_element_2->SetIdSelector("test");

  // view3
  auto fiber_element_3 = manager->CreateFiberNode("view");
  fiber_element_3->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_element_3);
  fiber_element_3->SetClass("four");

  // view4
  auto fiber_element_4 = manager->CreateFiberNode("view");
  fiber_element_4->parent_component_element_ = page.get();
  fiber_element_2->InsertNode(fiber_element_4);
  fiber_element_4->SetClass("three");
  fiber_element_4->SetIdSelector("test1");

  // count component
  CSSParserTokenMap counterIndexTokensMap;
  {
    // class .test
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDWidth] =
        CSSValue::MakePlainString("30px");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDHeight] =
        CSSValue::MakePlainString("40px");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue("{{--main-bg-color}}", CSSValuePattern::STRING,
                 CSSValueType::VARIABLE);
    std::string key = ".test";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    counterIndexTokensMap.insert(std::make_pair(key, tokens));
  }
  const std::vector<int32_t> counter_dependent_ids;
  CSSKeyframesTokenMap counter_keyframes;
  CSSFontFaceRuleMap counter_font_faces;
  auto counterIndexFragment = std::make_shared<SharedCSSFragment>(
      2, counter_dependent_ids, counterIndexTokensMap, counter_keyframes,
      counter_font_faces);

  base::String component_id("21");
  int32_t css_id = 100;
  base::String entry_name("__Card__");
  base::String component_name("TestComp");
  base::String path("/index/components/TestComp");
  auto counter_component = manager->CreateFiberComponent(
      component_id, css_id, entry_name, component_name, path);
  counter_component->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(counterIndexFragment.get());
  counter_component->parent_component_element_ = page.get();
  fiber_element_2->InsertNode(counter_component);

  // view5
  auto fiber_element_5 = manager->CreateFiberNode("view");
  fiber_element_5->parent_component_element_ = counter_component.get();
  counter_component->InsertNode(fiber_element_5);
  fiber_element_5->SetClass("test");

  EXPECT_EQ(fiber_element_5->ParentComponentEntryName(), "__Card__");

  page->FlushActionsAsRoot();
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* painting_node_4 =
      painting_context->node_map_.at(fiber_element_4->impl_id()).get();
  std::string background_color_key = "background-color";

  auto node_4_background_color_value =
      painting_node_4->props_.at(background_color_key);
  EXPECT_TRUE(node_4_background_color_value.UInt32() == 0xffffc0cb);

  auto* painting_node_3 =
      painting_context->node_map_.at(fiber_element_3->impl_id()).get();
  auto node_3_background_color_value =
      painting_node_3->props_.at(background_color_key);
  EXPECT_TRUE(node_3_background_color_value.UInt32() == 0xffffff00);

  auto painting_node_5 =
      painting_context->node_map_.at(fiber_element_5->impl_id()).get();
  auto node_5_background_color_value =
      painting_node_5->props_.at(background_color_key);
  EXPECT_TRUE(node_5_background_color_value.UInt32() == 0xffffc0cb);

  auto css_variable_value = lepus::Dictionary::Create();
  css_variable_value->SetValue("--main-bg-color", lepus::Value("red"));
  auto options = std::make_shared<PipelineOptions>();
  fiber_element_2->UpdateCSSVariable(lepus::Value(css_variable_value), options);
  painting_context->Flush();

  auto* painting_node_4_after_updated =
      painting_context->node_map_.at(fiber_element_4->impl_id()).get();
  auto node_4_background_color_value_after_updated =
      painting_node_4_after_updated->props_.at(background_color_key);
  EXPECT_TRUE(node_4_background_color_value_after_updated.UInt32() ==
              0xffff0000);

  auto* painting_node_5_after_updated =
      painting_context->node_map_.at(fiber_element_5->impl_id()).get();
  auto node_5_background_color_value_after_updated =
      painting_node_5_after_updated->props_.at(background_color_key);
  EXPECT_TRUE(node_5_background_color_value_after_updated.UInt32() ==
              0xffff0000);
}

TEST_P(FiberElementTest, CSSVariableShorthandProcess) {
  float kScreeWidth = 750;
  float kRpxRatio = 750.0f;

  manager->UpdateScreenMetrics(kScreeWidth, 1000);

  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableCSSInheritance(true);
  manager->SetConfig(config);

  // construct css fragment
  StyleMap indexAttributes;
  CSSParserConfigs parser_configs;

  CSSParserTokenMap indexTokenMap;
  // class :root
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->style_variables_.insert_or_assign("--radius-max", "12rpx");

    std::string key = ".root";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  // class .child
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBorderRadius] = CSSValue(
        "{{--radius-max}}", CSSValuePattern::STRING, CSSValueType::VARIABLE);
    std::string key = ".child";
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

  // parent
  auto page = manager->CreateFiberPage("page", 10);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // container
  auto container = manager->CreateFiberNode("view");
  container->parent_component_element_ = page.get();
  page->InsertNode(container);
  container->SetClass("root");

  // child1
  auto child1 = manager->CreateFiberNode("view");
  child1->parent_component_element_ = page.get();
  container->InsertNode(child1);
  child1->SetClass("child");

  page->FlushActionsAsRoot();

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->platform_impl_.get());
  painting_context->Flush();
  auto* child1_painting_node_ =
      painting_context->node_map_.at(child1->impl_id()).get();
  auto top_left_radius_it =
      child1_painting_node_->props_.find("border-top-left-radius");
  EXPECT_TRUE(top_left_radius_it != child1_painting_node_->props_.end());
  auto tl_value = top_left_radius_it->second.Array()->get(0).Number();
  EXPECT_TRUE(tl_value == 12 * kScreeWidth / kRpxRatio);

  auto top_right_radius_it =
      child1_painting_node_->props_.find("border-top-right-radius");
  EXPECT_TRUE(top_right_radius_it != child1_painting_node_->props_.end());
  auto tr_value = top_right_radius_it->second.Array()->get(0).Number();
  EXPECT_TRUE(tr_value == 12 * kScreeWidth / kRpxRatio);

  auto bottom_right_radius_it =
      child1_painting_node_->props_.find("border-bottom-right-radius");
  EXPECT_TRUE(bottom_right_radius_it != child1_painting_node_->props_.end());
  auto br_value = bottom_right_radius_it->second.Array()->get(0).Number();
  EXPECT_TRUE(br_value == 12 * kScreeWidth / kRpxRatio);

  auto bottom_left_radius_it =
      child1_painting_node_->props_.find("border-bottom-left-radius");
  EXPECT_TRUE(bottom_left_radius_it != child1_painting_node_->props_.end());
  auto bl_value = bottom_left_radius_it->second.Array()->get(0).Number();
  EXPECT_TRUE(bl_value == 12 * kScreeWidth / kRpxRatio);

  auto border_radius_it = child1_painting_node_->props_.find("border-radius");
  EXPECT_TRUE(border_radius_it == child1_painting_node_->props_.end());
}

TEST_P(FiberElementTest, SetKeyframes) {
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

  // mock keyframes
  // raw keyframes

  constexpr const char* keyframe_name = "move";
  constexpr const char* keyframe_name1 = "a";

  CSSRawKeyframesContent raw_keyframes;
  RawStyleMap* raw_attrs_0 = new RawStyleMap();
  raw_attrs_0->insert_or_assign(CSSPropertyID::kPropertyIDTransform,
                                CSSValue::MakePlainString("translate(0%, 0%)"));
  std::shared_ptr<RawStyleMap> raw_attrs_ptr0(raw_attrs_0);
  raw_keyframes.insert(
      std::pair<float, std::shared_ptr<RawStyleMap>>(0.0f, raw_attrs_ptr0));

  RawStyleMap* raw_attrs_1 = new RawStyleMap();
  raw_attrs_1->insert_or_assign(
      CSSPropertyID::kPropertyIDTransform,
      CSSValue::MakePlainString("translate(100%, 100%)"));
  std::shared_ptr<RawStyleMap> raw_attrs_ptr1(raw_attrs_1);
  raw_keyframes.insert(
      std::pair<float, std::shared_ptr<RawStyleMap>>(1.0f, raw_attrs_ptr1));

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
  fml::RefPtr<CSSKeyframesToken> token_ptr1 =
      fml::AdoptRef(new CSSKeyframesToken(configs));

  CSSKeyframesTokenMap keyframes;
  keyframes.insert({keyframe_name, std::move(token_ptr)});
  keyframes.insert({keyframe_name1, std::move(token_ptr1)});

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
  element->enable_new_animator_ = false;
  element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(comp->impl_id()));
  comp->InsertNode(element);
  element->SetStyle(CSSPropertyID::kPropertyIDAnimation,
                    lepus::Value("move 5000ms linear infinite, b 50ms"));

  page->FlushActionsAsRoot();

  // check keyframes
  auto* css_fragment = element->GetRelatedCSSFragment();
  EXPECT_TRUE(comp->GetCSSFragment() == css_fragment);

  auto* painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->platform_impl_.get());
  painting_context->Flush();

  EXPECT_TRUE(painting_context->keyframes_.size() == 1);
  EXPECT_TRUE(painting_context->keyframes_.count("keyframes"));
  auto platform_keyframe_value = painting_context->keyframes_["keyframes"];

  EXPECT_TRUE(
      platform_keyframe_value.Table()->size() == 1 &&
      !platform_keyframe_value.Table()->GetValue(keyframe_name).IsEmpty());
  EXPECT_TRUE(platform_keyframe_value.Table()
                  ->GetValue(keyframe_name)
                  .Table()
                  ->size() == 2);

  auto* mock_painting_node_ =
      painting_context->node_map_.at(element->impl_id()).get();
  std::string animation("animation");
  EXPECT_TRUE(mock_painting_node_->props_.find(animation) !=
              mock_painting_node_->props_.end());

  element->SetStyle(
      CSSPropertyID::kPropertyIDAnimation,
      lepus::Value("move 5000ms linear infinite, a 100ms, b 50ms"));
  page->FlushActionsAsRoot();
  painting_context->Flush();
  EXPECT_TRUE(painting_context->keyframes_.size() == 1);
  EXPECT_TRUE(painting_context->keyframes_.count("keyframes"));
  platform_keyframe_value = painting_context->keyframes_["keyframes"];

  EXPECT_TRUE(platform_keyframe_value.Table()->size() == 1);
  EXPECT_TRUE(platform_keyframe_value.Table()->Contains(keyframe_name1));

  mock_painting_node_ =
      painting_context->node_map_.at(element->impl_id()).get();
  EXPECT_TRUE(mock_painting_node_->props_.find(animation) !=
              mock_painting_node_->props_.end());

  element->SetStyle(
      CSSPropertyID::kPropertyIDAnimation,
      lepus::Value("move 1000ms linear infinite, a 100ms, b 50ms, c 1000ms"));
  page->FlushActionsAsRoot();
  painting_context->Flush();

  EXPECT_TRUE(painting_context->keyframes_.size() == 1);
  EXPECT_TRUE(painting_context->keyframes_.count("keyframes"));
  platform_keyframe_value = painting_context->keyframes_["keyframes"];

  EXPECT_TRUE(platform_keyframe_value.Table()->size() != 0);
  EXPECT_TRUE(platform_keyframe_value.Table()->Contains(keyframe_name1));

  mock_painting_node_ =
      painting_context->node_map_.at(element->impl_id()).get();
  EXPECT_TRUE(mock_painting_node_->props_.find(animation) !=
              mock_painting_node_->props_.end());

  // check font faces
  auto text = manager->CreateFiberText("text");
  text->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(comp->impl_id()));
  text->SetStyle(CSSPropertyID::kPropertyIDFontFamily,
                 lepus::Value("font-base64"));
  comp->InsertNode(text);
  comp->FlushActionsAsRoot();
  EXPECT_TRUE(css_fragment->has_font_faces_resolved_);
}

TEST_P(FiberElementTest, SetMultipleKeyframes) {
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

  // mock keyframes
  // raw keyframes

  constexpr const char* keyframe_name = "move";
  constexpr const char* keyframe_name1 = "a";

  CSSRawKeyframesContent raw_keyframes;
  RawStyleMap* raw_attrs_0 = new RawStyleMap();
  raw_attrs_0->insert_or_assign(CSSPropertyID::kPropertyIDTransform,
                                CSSValue::MakePlainString("translate(0%, 0%)"));
  std::shared_ptr<RawStyleMap> raw_attrs_ptr0(raw_attrs_0);
  raw_keyframes.insert(
      std::pair<float, std::shared_ptr<RawStyleMap>>(0.0f, raw_attrs_ptr0));

  RawStyleMap* raw_attrs_1 = new RawStyleMap();
  raw_attrs_1->insert_or_assign(
      CSSPropertyID::kPropertyIDTransform,
      CSSValue::MakePlainString("translate(100%, 100%)"));
  std::shared_ptr<RawStyleMap> raw_attrs_ptr1(raw_attrs_1);
  raw_keyframes.insert(
      std::pair<float, std::shared_ptr<RawStyleMap>>(1.0f, raw_attrs_ptr1));

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
  fml::RefPtr<CSSKeyframesToken> token_ptr1 =
      fml::AdoptRef(new CSSKeyframesToken(configs));

  CSSKeyframesTokenMap keyframes;
  keyframes.insert({keyframe_name, std::move(token_ptr)});
  keyframes.insert({keyframe_name1, std::move(token_ptr1)});

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
  element->enable_new_animator_ = false;
  element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(comp->impl_id()));
  comp->InsertNode(element);
  element->SetStyle(CSSPropertyID::kPropertyIDAnimation,
                    lepus::Value("move 5000ms linear infinite, b 50ms"));

  auto element1 = manager->CreateFiberView();
  element1->enable_new_animator_ = false;
  element1->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(comp->impl_id()));
  comp->InsertNode(element1);
  element1->SetStyle(CSSPropertyID::kPropertyIDAnimation,
                     lepus::Value("move 5000ms linear infinite, b 50ms"));

  auto element2 = manager->CreateFiberView();
  element2->enable_new_animator_ = false;
  element2->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(comp->impl_id()));
  comp->InsertNode(element2);
  element2->SetStyle(CSSPropertyID::kPropertyIDAnimation,
                     lepus::Value("move 5000ms linear infinite, b 50ms"));

  page->FlushActionsAsRoot();

  // check keyframes
  auto* css_fragment = element->GetRelatedCSSFragment();
  EXPECT_TRUE(comp->GetCSSFragment() == css_fragment);

  auto* painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->platform_impl_.get());
  painting_context->Flush();

  EXPECT_TRUE(painting_context->keyframes_.size() == 1);
  EXPECT_TRUE(painting_context->keyframes_.count("keyframes"));
  auto platform_keyframe_value = painting_context->keyframes_["keyframes"];

  EXPECT_TRUE(
      platform_keyframe_value.Table()->size() == 1 &&
      !platform_keyframe_value.Table()->GetValue(keyframe_name).IsEmpty());
  EXPECT_TRUE(platform_keyframe_value.Table()
                  ->GetValue(keyframe_name)
                  .Table()
                  ->size() == 2);

  std::string animation("animation");
  auto* mock_painting_node_ =
      painting_context->node_map_.at(element->impl_id()).get();
  EXPECT_TRUE(mock_painting_node_->props_.find(animation) !=
              mock_painting_node_->props_.end());

  auto* mock_painting_node1_ =
      painting_context->node_map_.at(element1->impl_id()).get();
  EXPECT_TRUE(mock_painting_node1_->props_.find(animation) !=
              mock_painting_node1_->props_.end());

  auto* mock_painting_node2_ =
      painting_context->node_map_.at(element2->impl_id()).get();
  EXPECT_TRUE(mock_painting_node2_->props_.find(animation) !=
              mock_painting_node2_->props_.end());

  element->SetStyle(
      CSSPropertyID::kPropertyIDAnimation,
      lepus::Value("move 5000ms linear infinite, a 100ms, b 50ms"));
  page->FlushActionsAsRoot();
  painting_context->Flush();
  EXPECT_TRUE(painting_context->keyframes_.size() == 1);
  EXPECT_TRUE(painting_context->keyframes_.count("keyframes"));
  platform_keyframe_value = painting_context->keyframes_["keyframes"];

  EXPECT_TRUE(platform_keyframe_value.Table()->size() == 1);
  EXPECT_TRUE(platform_keyframe_value.Table()->Contains(keyframe_name1));

  mock_painting_node_ =
      painting_context->node_map_.at(element->impl_id()).get();
  EXPECT_TRUE(mock_painting_node_->props_.find(animation) !=
              mock_painting_node_->props_.end());

  element->SetStyle(
      CSSPropertyID::kPropertyIDAnimation,
      lepus::Value("move 1000ms linear infinite, a 100ms, b 50ms, c 1000ms"));
  page->FlushActionsAsRoot();
  painting_context->Flush();

  EXPECT_TRUE(painting_context->keyframes_.size() == 1);
  EXPECT_TRUE(painting_context->keyframes_.count("keyframes"));
  platform_keyframe_value = painting_context->keyframes_["keyframes"];

  EXPECT_TRUE(platform_keyframe_value.Table()->size() != 0);
  EXPECT_TRUE(platform_keyframe_value.Table()->Contains(keyframe_name1));

  mock_painting_node_ =
      painting_context->node_map_.at(element->impl_id()).get();
  EXPECT_TRUE(mock_painting_node_->props_.find(animation) !=
              mock_painting_node_->props_.end());

  // check font faces
  auto text = manager->CreateFiberText("text");
  text->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(comp->impl_id()));
  text->SetStyle(CSSPropertyID::kPropertyIDFontFamily,
                 lepus::Value("font-base64"));
  comp->InsertNode(text);
  comp->FlushActionsAsRoot();
  EXPECT_TRUE(css_fragment->has_font_faces_resolved_);
}

TEST_P(FiberElementTest, SetKeyframes_new_animator) {
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

  // mock keyframes
  // raw keyframes

  constexpr const char* keyframe_name = "move";
  constexpr const char* keyframe_name1 = "a";

  CSSRawKeyframesContent raw_keyframes;
  RawStyleMap* raw_attrs_0 = new RawStyleMap();
  raw_attrs_0->insert_or_assign(CSSPropertyID::kPropertyIDTransform,
                                CSSValue::MakePlainString("translate(0%, 0%)"));
  std::shared_ptr<RawStyleMap> raw_attrs_ptr0(raw_attrs_0);
  raw_keyframes.insert(
      std::pair<float, std::shared_ptr<RawStyleMap>>(0.0f, raw_attrs_ptr0));

  RawStyleMap* raw_attrs_1 = new RawStyleMap();
  raw_attrs_1->insert_or_assign(
      CSSPropertyID::kPropertyIDTransform,
      CSSValue::MakePlainString("translate(100%, 100%)"));
  std::shared_ptr<RawStyleMap> raw_attrs_ptr1(raw_attrs_1);
  raw_keyframes.insert(
      std::pair<float, std::shared_ptr<RawStyleMap>>(1.0f, raw_attrs_ptr1));

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
  fml::RefPtr<CSSKeyframesToken> token_ptr1 =
      fml::AdoptRef(new CSSKeyframesToken(configs));

  CSSKeyframesTokenMap keyframes;
  keyframes.insert({keyframe_name, std::move(token_ptr)});
  keyframes.insert({keyframe_name1, std::move(token_ptr1)});

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
  manager->task_wait_timeout_ = 5000;

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
  element->enable_new_animator_ = true;
  element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(comp->impl_id()));
  comp->InsertNode(element);
  element->SetStyle(CSSPropertyID::kPropertyIDAnimation,
                    lepus::Value("move 5000ms linear infinite, b 50ms"));
  page->FlushActionsAsRoot();

  // check keyframes
  auto* css_fragment = element->GetRelatedCSSFragment();
  EXPECT_TRUE(comp->GetCSSFragment() == css_fragment);

  auto* painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->platform_impl_.get());
  painting_context->Flush();

  EXPECT_TRUE(painting_context->keyframes_.size() == 0);

  element->SetStyle(
      CSSPropertyID::kPropertyIDAnimation,
      lepus::Value("move 5000ms linear infinite, a 100ms, b 50ms"));
  manager->element_vsync_proxy_->MarkNextFrameHasArrived();
  page->FlushActionsAsRoot();
  EXPECT_TRUE(painting_context->keyframes_.size() == 0);

  element->SetStyle(
      CSSPropertyID::kPropertyIDAnimation,
      lepus::Value("move 1000ms linear infinite, a 100ms, b 50ms, c 1000ms"));
  manager->element_vsync_proxy_->MarkNextFrameHasArrived();
  page->FlushActionsAsRoot();
  EXPECT_TRUE(painting_context->keyframes_.size() == 0);
}

TEST_P(FiberElementTest, SetMultipleKeyframes_new_animator) {
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

  // mock keyframes
  // raw keyframes

  constexpr const char* keyframe_name = "move";
  constexpr const char* keyframe_name1 = "a";

  CSSRawKeyframesContent raw_keyframes;
  RawStyleMap* raw_attrs_0 = new RawStyleMap();
  raw_attrs_0->insert_or_assign(CSSPropertyID::kPropertyIDTransform,
                                CSSValue::MakePlainString("translate(0%, 0%)"));
  std::shared_ptr<RawStyleMap> raw_attrs_ptr0(raw_attrs_0);
  raw_keyframes.insert(
      std::pair<float, std::shared_ptr<RawStyleMap>>(0.0f, raw_attrs_ptr0));

  RawStyleMap* raw_attrs_1 = new RawStyleMap();
  raw_attrs_1->insert_or_assign(
      CSSPropertyID::kPropertyIDTransform,
      CSSValue::MakePlainString("translate(100%, 100%)"));
  std::shared_ptr<RawStyleMap> raw_attrs_ptr1(raw_attrs_1);
  raw_keyframes.insert(
      std::pair<float, std::shared_ptr<RawStyleMap>>(1.0f, raw_attrs_ptr1));

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
  fml::RefPtr<CSSKeyframesToken> token_ptr1 =
      fml::AdoptRef(new CSSKeyframesToken(configs));

  CSSKeyframesTokenMap keyframes;
  keyframes.insert({keyframe_name, std::move(token_ptr)});
  keyframes.insert({keyframe_name1, std::move(token_ptr1)});

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
  manager->task_wait_timeout_ = 5000;

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
  element->enable_new_animator_ = true;
  element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(comp->impl_id()));
  comp->InsertNode(element);
  element->SetStyle(CSSPropertyID::kPropertyIDAnimation,
                    lepus::Value("move 5000ms linear infinite, b 50ms"));

  auto element1 = manager->CreateFiberView();
  element1->enable_new_animator_ = true;
  element1->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(comp->impl_id()));
  comp->InsertNode(element1);
  element1->SetStyle(CSSPropertyID::kPropertyIDAnimation,
                     lepus::Value("move 5000ms linear infinite, b 50ms"));

  auto element2 = manager->CreateFiberView();
  element2->enable_new_animator_ = true;
  element2->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(comp->impl_id()));
  comp->InsertNode(element2);
  element2->SetStyle(CSSPropertyID::kPropertyIDAnimation,
                     lepus::Value("move 5000ms linear infinite, b 50ms"));
  page->FlushActionsAsRoot();

  // check keyframes
  auto* css_fragment = element->GetRelatedCSSFragment();
  EXPECT_TRUE(comp->GetCSSFragment() == css_fragment);

  auto* painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->platform_impl_.get());
  painting_context->Flush();

  EXPECT_TRUE(painting_context->keyframes_.size() == 0);

  element->SetStyle(
      CSSPropertyID::kPropertyIDAnimation,
      lepus::Value("move 5000ms linear infinite, a 100ms, b 50ms"));
  manager->element_vsync_proxy_->MarkNextFrameHasArrived();
  page->FlushActionsAsRoot();
  EXPECT_TRUE(painting_context->keyframes_.size() == 0);

  element->SetStyle(
      CSSPropertyID::kPropertyIDAnimation,
      lepus::Value("move 1000ms linear infinite, a 100ms, b 50ms, c 1000ms"));
  manager->element_vsync_proxy_->MarkNextFrameHasArrived();
  page->FlushActionsAsRoot();
  EXPECT_TRUE(painting_context->keyframes_.size() == 0);
}

TEST_P(FiberElementTest, ConsumeAnimationPropBundle) {
  //  constructor css fragment
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

  manager->task_wait_timeout_ = 1000;
  // page
  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // children
  auto parent = manager->CreateFiberView();
  page->InsertNode(parent);
  auto element1 = manager->CreateFiberView();
  element1->enable_new_animator_ = false;
  element1->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(page->impl_id()));
  parent->InsertNode(element1);
  base::String clazz_name("recommend-ani-image-in");
  element1->SetClass(clazz_name);

  page->FlushActionsAsRoot();
  EXPECT_TRUE(element1->prop_bundle_ == nullptr);

  element1->RemoveAllClass();

  page->FlushActionsAsRoot();
  EXPECT_TRUE(element1->prop_bundle_ == nullptr);
}

TEST_P(FiberElementTest, ConsumeAnimationPropBundle_new_animator) {
  //  constructor css fragment
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

  manager->task_wait_timeout_ = 1000;
  // page
  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // children
  auto parent = manager->CreateFiberView();
  page->InsertNode(parent);
  auto element1 = manager->CreateFiberView();
  element1->enable_new_animator_ = true;
  element1->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(page->impl_id()));
  parent->InsertNode(element1);
  base::String clazz_name("recommend-ani-image-in");
  element1->SetClass(clazz_name);

  page->FlushActionsAsRoot();
  EXPECT_TRUE(element1->prop_bundle_ == nullptr);

  element1->RemoveAllClass();

  page->FlushActionsAsRoot();
  EXPECT_TRUE(element1->prop_bundle_ == nullptr);
}

TEST_P(FiberElementTest, GetCSSKeyframesToken) {
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

  // mock keyframes
  // raw keyframes

  constexpr const char* keyframe_name = "move";

  CSSRawKeyframesContent raw_keyframes;
  RawStyleMap* raw_attrs_0 = new RawStyleMap();
  raw_attrs_0->insert_or_assign(CSSPropertyID::kPropertyIDTransform,
                                CSSValue::MakePlainString("translate(0%, 0%)"));
  std::shared_ptr<RawStyleMap> raw_attrs_ptr0(raw_attrs_0);
  raw_keyframes.insert(
      std::pair<float, std::shared_ptr<RawStyleMap>>(0.0f, raw_attrs_ptr0));

  RawStyleMap* raw_attrs_1 = new RawStyleMap();
  raw_attrs_1->insert_or_assign(
      CSSPropertyID::kPropertyIDTransform,
      CSSValue::MakePlainString("translate(100%, 100%)"));
  std::shared_ptr<RawStyleMap> raw_attrs_ptr1(raw_attrs_1);
  raw_keyframes.insert(
      std::pair<float, std::shared_ptr<RawStyleMap>>(1.0f, raw_attrs_ptr1));

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
  element->SetStyle(CSSPropertyID::kPropertyIDAnimation,
                    lepus::Value("move 5000ms linear infinite"));

  page->FlushActionsAsRoot();

  EXPECT_TRUE(element->GetCSSKeyframesToken("test") == nullptr);
  EXPECT_TRUE(element->GetCSSKeyframesToken("move") != nullptr);
}

TEST_P(FiberElementTest, CSSVariableOrderTest) {
  // construct css fragment.
  StyleMap indexAttributes;
  CSSParserConfigs parser_configs;
  CSSParserTokenMap indexTokenMap;

  // class .container
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBorder] =
        CSSValue::MakePlainString("1px solid red");
    tokens->style_variables_["--bg-color"] = "yellow";
    std::string key = ".container";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  // class .text
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue("{{--bg-color}}", CSSValuePattern::STRING,
                 CSSValueType::VARIABLE);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackground] =
        CSSValue::MakePlainString("red");
    std::string key = ".text";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  // class .text1
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackground] =
        CSSValue::MakePlainString("red");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue("{{--bg-color}}", CSSValuePattern::STRING,
                 CSSValueType::VARIABLE);
    std::string key = ".text1";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  // class .text2
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue::MakePlainString("red");
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackground] = CSSValue(
        "{{--bg-color}}", CSSValuePattern::STRING, CSSValueType::VARIABLE);
    std::string key = ".text2";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokenMap.insert(std::make_pair(key, tokens));
  }

  // class .text3
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(parser_configs);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackground] = CSSValue(
        "{{--bg-color}}", CSSValuePattern::STRING, CSSValueType::VARIABLE);
    tokens->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
        CSSValue::MakePlainString("red");
    std::string key = ".text3";
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

  // parent
  auto page = manager->CreateFiberPage("page", 0);
  page->style_sheet_ =
      std::make_unique<CSSFragmentDecorator>(indexFragment.get());

  // view1
  auto fiber_element_1 = manager->CreateFiberNode("view");
  fiber_element_1->parent_component_element_ = page.get();
  page->InsertNode(fiber_element_1);
  fiber_element_1->SetClass("container");

  // text
  auto fiber_element_2 = manager->CreateFiberText("text");
  fiber_element_2->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_element_2);
  fiber_element_2->SetAttribute("text", lepus::Value("Hello, Speedy"));
  fiber_element_2->SetClass("text");

  // text1
  auto fiber_element_3 = manager->CreateFiberText("text");
  fiber_element_3->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_element_3);
  fiber_element_3->SetAttribute("text", lepus::Value("Hello, Speedy"));
  fiber_element_3->SetClass("text1");

  // text2
  auto fiber_element_4 = manager->CreateFiberText("text");
  fiber_element_4->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_element_4);
  fiber_element_4->SetAttribute("text", lepus::Value("Hello, Speedy"));
  fiber_element_4->SetClass("text2");

  // text3
  auto fiber_element_5 = manager->CreateFiberText("text");
  fiber_element_5->parent_component_element_ = page.get();
  fiber_element_1->InsertNode(fiber_element_5);
  fiber_element_5->SetAttribute("text", lepus::Value("Hello, Speedy"));
  fiber_element_5->SetClass("text3");

  page->FlushActionsAsRoot();
  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();
  std::string background_color_key = "background-color";

  auto painting_node_2 =
      painting_context->node_map_.at(fiber_element_2->impl_id()).get();
  auto node_2_background_color_value =
      painting_node_2->props_.at(background_color_key);

  auto painting_node_3 =
      painting_context->node_map_.at(fiber_element_3->impl_id()).get();
  auto node_3_background_color_value =
      painting_node_3->props_.at(background_color_key);

  auto painting_node_4 =
      painting_context->node_map_.at(fiber_element_4->impl_id()).get();
  auto node_4_background_color_value =
      painting_node_4->props_.at(background_color_key);

  auto painting_node_5 =
      painting_context->node_map_.at(fiber_element_5->impl_id()).get();
  auto node_5_background_color_value =
      painting_node_5->props_.at(background_color_key);

  EXPECT_TRUE(node_2_background_color_value.UInt32() == 0xffff0000);
  EXPECT_TRUE(node_3_background_color_value.UInt32() == 0xffffff00);
  EXPECT_TRUE(node_4_background_color_value.UInt32() == 0xffffff00);
  EXPECT_TRUE(node_5_background_color_value.UInt32() == 0xffff0000);
}

TEST_P(FiberElementTest, TestTransitionInResetMapAndUpdateMap) {
  StyleMap indexAttributes;
  CSSParserConfigs configs;
  auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);

  CSSParserTokenMap indexTokensMap;

  // class .a has opacity style
  {
    auto id = CSSPropertyID::kPropertyIDOpacity;
    auto impl = lepus::Value(0.3);
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".a";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  // class .b has transition style
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDTransition;
    auto impl = lepus::Value("opacity 10s");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    std::string key = ".b";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  // class .c is empty
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);

    std::string key = ".c";
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
  fiber_element->enable_new_animator_ = true;
  fiber_element->parent_component_element_ = page.get();
  page->InsertNode(fiber_element);

  fiber_element->SetClass("a");

  page->FlushActionsAsRoot();

  fiber_element->SetClass("c");
  fiber_element->SetClass("b");
  page->FlushActionsAsRoot();

  EXPECT_TRUE(fiber_element->has_transition_props_);
}

TEST_P(FiberElementTest, FlushActionsAsRootWithCssVarLoop) {
  for (int i = 0; i < 10; ++i) {
    // 1. Create CSSFragment for ComponentElement with :root selector and CSS
    // variables
    CSSParserConfigs configs;
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);

    CSSParserTokenMap component_tokens_map;
    // :root
    {
      auto id = CSSPropertyID::kPropertyIDOpacity;
      auto impl = lepus::Value(0.3);
      tokens.get()->raw_attributes_[id] =
          CSSValue(impl, CSSValuePattern::STRING);

      tokens.get()->style_variables_["--main-color"] = "#ff0000";
      tokens.get()->style_variables_["--font-size"] = "20px";
      tokens.get()->style_variables_["--font-size0"] = "30px";
      tokens.get()->style_variables_["--font-size1"] = "30px";
      for (int j = 0; j < 10000; ++j) {
        tokens.get()->style_variables_["--var" + std::to_string(j)] = "30px";
      }

      std::string key = ":root";
      auto& sheets = tokens->sheets();
      auto shared_css_sheet = std::make_shared<CSSSheet>(key);
      sheets.emplace_back(shared_css_sheet);
      component_tokens_map.insert(std::make_pair(key, tokens));
    }

    // class .component
    {
      auto id = CSSPropertyID::kPropertyIDOpacity;
      auto impl = lepus::Value(0.3);
      tokens.get()->raw_attributes_[id] =
          CSSValue(impl, CSSValuePattern::STRING);

      tokens.get()->style_variables_["--main-color"] = "#ff0000";
      tokens.get()->style_variables_["--font-size"] = "20px";
      tokens.get()->style_variables_["--font-size0"] = "30px";
      tokens.get()->style_variables_["--font-size1"] = "30px";
      for (int j = 0; j < 10000; ++j) {
        tokens.get()->style_variables_["--var" + std::to_string(j)] = "30px";
      }

      std::string key = ".component";
      auto& sheets = tokens->sheets();
      auto shared_css_sheet = std::make_shared<CSSSheet>(key);
      sheets.emplace_back(shared_css_sheet);
      component_tokens_map.insert(std::make_pair(key, tokens));
    }

    // class .view0-class
    {
      auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
      auto id = CSSPropertyID::kPropertyIDWidth;
      auto impl = lepus::Value("20px");
      tokens.get()->raw_attributes_[id] =
          CSSValue(impl, CSSValuePattern::STRING);

      tokens.get()->raw_attributes_[CSSPropertyID::kPropertyIDColor] =
          CSSValue::MakePlainString("var(--main-color)");
      tokens.get()->raw_attributes_[CSSPropertyID::kPropertyIDFontSize] =
          CSSValue::MakePlainString("var(--font-size)");

      std::string key = ".view0-class";
      auto& sheets = tokens->sheets();
      auto shared_css_sheet = std::make_shared<CSSSheet>(key);
      sheets.emplace_back(shared_css_sheet);
      component_tokens_map.insert(std::make_pair(key, tokens));
    }

    // class .view1-class
    {
      auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
      auto id = CSSPropertyID::kPropertyIDWidth;
      auto impl = lepus::Value("20px");
      tokens.get()->raw_attributes_[id] =
          CSSValue(impl, CSSValuePattern::STRING);

      tokens.get()->raw_attributes_[CSSPropertyID::kPropertyIDColor] =
          CSSValue::MakePlainString("var(--main-color)");
      tokens.get()->raw_attributes_[CSSPropertyID::kPropertyIDFontSize] =
          CSSValue::MakePlainString("var(--font-size)");

      std::string key = ".view1-class";
      auto& sheets = tokens->sheets();
      auto shared_css_sheet = std::make_shared<CSSSheet>(key);
      sheets.emplace_back(shared_css_sheet);
      component_tokens_map.insert(std::make_pair(key, tokens));
    }

    const std::vector<int32_t> dependent_ids;
    CSSKeyframesTokenMap keyframes;
    CSSFontFaceRuleMap font_faces;
    auto component_fragment = std::make_unique<SharedCSSFragment>(
        2, dependent_ids, component_tokens_map, keyframes, font_faces);

    // 2. Create element hierarchy: PageElement -> ComponentElement ->
    // ViewElement
    auto page = manager->CreateFiberPage("page", 2);
    page->set_style_sheet_manager(
        tasm->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME));

    base::String component_id("21");
    int32_t css_id = 2;
    base::String entry_name("__Card__");
    base::String component_name("TestComp");
    base::String path("/index/components/TestComp");
    auto component = manager->CreateFiberComponent(
        component_id, css_id, entry_name, component_name, path);
    component->SetClass("component");
    component->set_style_sheet_manager(
        tasm->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME));
    component->SetParentComponentUniqueIdForFiber(
        static_cast<int64_t>(page->impl_id()));

    auto view0 = manager->CreateFiberView();
    view0->SetClass("view0-class");
    view0->SetParentComponentUniqueIdForFiber(
        static_cast<int64_t>(component->impl_id()));

    auto view1 = manager->CreateFiberView();
    view1->SetClass("view0-class");
    view1->SetParentComponentUniqueIdForFiber(
        static_cast<int64_t>(component->impl_id()));

    auto style_sheet_manager = page->css_style_sheet_manager_;
    style_sheet_manager->raw_fragments_->insert(
        std::make_pair(css_id, std::move(component_fragment)));

    // Build the element tree
    component->InsertNode(view0);
    component->InsertNode(view1);

    page->InsertNode(component);

    // 3. Call FlushActionsAsRoot
    page->FlushActionsAsRoot();
  }
}

TEST_P(FiberElementTest, CollectCustomPropertiesCascading) {
  auto page = manager->CreateFiberPage("page", 0);
  auto parent = manager->CreateFiberView();
  auto child = manager->CreateFiberView();

  page->InsertNode(parent);
  parent->InsertNode(child);

  // 1. Test Inheritance
  page->data_model()->UpdateCSSVariable("--root-var", "root-val");
  parent->data_model()->UpdateCSSVariable("--parent-var", "parent-val");
  child->data_model()->UpdateCSSVariable("--child-var", "child-val");

  // Call CollectCustomProperties on child.
  child->CollectCustomProperties(child->data_model());

  // Check child's properties
  ASSERT_TRUE(child->custom_properties_.Get() != nullptr);
  auto& child_props = child->custom_properties_->Value();
  EXPECT_TRUE(child_props.find("--root-var") != child_props.end());
  EXPECT_TRUE(child_props.find("--parent-var") != child_props.end());
  EXPECT_TRUE(child_props.find("--child-var") != child_props.end());
  EXPECT_EQ(child_props.find("--root-var")->second.AsString().str(),
            "root-val");
  EXPECT_EQ(child_props.find("--parent-var")->second.AsString().str(),
            "parent-val");
  EXPECT_EQ(child_props.find("--child-var")->second.AsString().str(),
            "child-val");

  // 2. Test Override
  auto child2 = manager->CreateFiberView();
  parent->InsertNode(child2);
  child2->data_model()->UpdateCSSVariable("--root-var", "child2-val");
  child2->CollectCustomProperties(child2->data_model());

  ASSERT_TRUE(child2->custom_properties_.Get() != nullptr);
  auto& child2_props = child2->custom_properties_->Value();
  EXPECT_EQ(child2_props.find("--root-var")->second.AsString().str(),
            "child2-val");

  // 3. Test Variable References
  auto child3 = manager->CreateFiberView();
  parent->InsertNode(child3);
  child3->data_model()->UpdateCSSVariable("--local-var", "var(--root-var)");
  child3->CollectCustomProperties(child3->data_model());

  ASSERT_TRUE(child3->custom_properties_.Get() != nullptr);
  auto& child3_props = child3->custom_properties_->Value();
  // --root-var should be inherited from parent (which is from page) ->
  // "root-val"
  // --local-var should be resolved to "root-val"
  EXPECT_EQ(child3_props.find("--local-var")->second.AsString().str(),
            "root-val");

  // 4. Verify no pollution to parent/root if they were not resolved yet
  ASSERT_TRUE(page->custom_properties_.Get() != nullptr);
  ASSERT_TRUE(parent->custom_properties_.Get() != nullptr);
  EXPECT_TRUE(page->custom_properties_->Value().find("--child-var") ==
              page->custom_properties_->Value().end());
  EXPECT_TRUE(parent->custom_properties_->Value().find("--child-var") ==
              parent->custom_properties_->Value().end());

  // 5. Test Inline Variables
  auto child4 = manager->CreateFiberView();
  parent->InsertNode(child4);
  child4->data_model()->UpdateCSSInlineVariables("--inline-var", "inline-val");
  child4->CollectCustomProperties(child4->data_model());

  ASSERT_TRUE(child4->custom_properties_.Get() != nullptr);
  auto& child4_props = child4->custom_properties_->Value();
  EXPECT_TRUE(child4_props.find("--inline-var") != child4_props.end());
  EXPECT_EQ(child4_props.find("--inline-var")->second.AsString().str(),
            "inline-val");
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
