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

TEST_P(FiberElementTest, TestSetFontSizeDirectlyToComputedStyle) {
  auto view = manager->CreateFiberView();

  // Condition: !EnableLayoutInElementMode() || IsShadowNodeCustom()
  // Ensure EnableLayoutInElementMode() is false
  manager->page_options_.SetEmbeddedMode(EmbeddedMode::UNSET);

  view->SetFontSize(tasm::CSSValue(25.0f, CSSValuePattern::NUMBER));

  // Verify it's set in computed_css_style
  EXPECT_EQ(view->GetFontSize(), 25.0f);
  EXPECT_TRUE(
      view->computed_css_style()->GetChangedBitset().Has(kPropertyIDFontSize));

  // The prop_bundle_ should NOT contain font-size yet because it hasn't been
  // flushed. Before the change, it was set directly to prop_bundle_.
  if (view->prop_bundle_) {
    EXPECT_FALSE(view->prop_bundle_->Contains(
        CSSProperty::GetPropertyName(CSSPropertyID::kPropertyIDFontSize)
            .c_str()));
  }

  // Now flush props and verify it reaches the painting node
  auto page = manager->CreateFiberPage("0", 0);
  page->InsertNode(view);
  page->FlushActionsAsRoot();

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  painting_context->Flush();

  auto* painting_node = painting_context->node_map_.at(view->impl_id()).get();
  EXPECT_TRUE(painting_node->props_.find("font-size") !=
              painting_node->props_.end());
  EXPECT_EQ(painting_node->props_["font-size"].Number(), 25.0f);
}

TEST_P(FiberElementTest, TestResetFontSizeDirectlyToComputedStyle) {
  auto page = manager->CreateFiberPage("0", 0);
  manager->SetFiberPageElement(page);
  auto view = manager->CreateFiberView();
  page->InsertNode(view);
  manager->page_options_.SetEmbeddedMode(EmbeddedMode::UNSET);

  float default_font_size = manager->GetLynxEnvConfig().PageDefaultFontSize() *
                            manager->GetLynxEnvConfig().font_scale_;

  view->SetFontSize(tasm::CSSValue(30.0f, CSSValuePattern::NUMBER));
  EXPECT_EQ(view->GetFontSize(), 30.0f);

  view->ResetFontSize();
  EXPECT_NEAR(view->GetFontSize(), default_font_size, 0.1);
  EXPECT_TRUE(
      view->computed_css_style()->GetChangedBitset().Has(kPropertyIDFontSize));
}

TEST_P(FiberElementTest, ElementInitTest0) {
  manager->GetLynxEnvConfig().font_scale_ = 1.3f;
  manager->GetLynxEnvConfig().font_scale_sp_only_ = false;

  auto fiber_element = manager->CreateFiberText("text");

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

TEST_P(FiberElementTest, TestSetComputedFontSize0) {
  manager->GetLynxEnvConfig().font_scale_ = 1.3f;
  manager->GetLynxEnvConfig().font_scale_sp_only_ = false;

  auto page = manager->CreateFiberPage("0", 0);

  page->SetComputedFontSize(99, 199);
  EXPECT_TRUE(page->GetRecordedRootFontSize() - 199 < 0.1);
  EXPECT_TRUE(page->GetFontSize() - 99 < 0.1);
  EXPECT_TRUE(
      page->computed_css_style()->GetChangedBitset().Has(kPropertyIDFontSize));
}

TEST_P(FiberElementTest, TestSetComputedFontSize1) {
  manager->GetLynxEnvConfig().font_scale_ = 1.3f;
  manager->GetLynxEnvConfig().font_scale_sp_only_ = false;

  const auto& env_config = manager->GetLynxEnvConfig();

  auto page = manager->CreateFiberPage("0", 0);
  EXPECT_TRUE(page->GetRecordedRootFontSize() -
                  env_config.PageDefaultFontSize() * 1.3 <
              0.1);
  EXPECT_TRUE(page->GetFontSize() - env_config.PageDefaultFontSize() * 1.3 <
              0.1);

  page->FlushActionsAsRoot();

  page->SetComputedFontSize(page->GetFontSize(),
                            page->GetRecordedRootFontSize());

  EXPECT_FALSE(
      page->computed_css_style()->GetChangedBitset().Has(kPropertyIDFontSize));
}

TEST_P(FiberElementTest, ElementInitTest1) {
  manager->GetLynxEnvConfig().font_scale_ = 1.3f;
  manager->GetLynxEnvConfig().font_scale_sp_only_ = true;

  auto fiber_element = manager->CreateFiberText("text");

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

TEST_P(FiberElementTest, TestCheckDynamicUnit0) {
  auto view = manager->CreateFiberView();

  auto impl = lepus::Value("1vw");
  CSSPropertyID id = CSSPropertyID::kPropertyIDWidth;
  CSSParserConfigs configs;
  auto map = UnitHandler::Process(id, impl, configs);

  view->CheckDynamicUnit(CSSPropertyID::kPropertyIDWidth,
                         map[CSSPropertyID::kPropertyIDWidth], false);

  EXPECT_TRUE(view->dynamic_style_flags_ &
              DynamicCSSStylesManager::kUpdateViewport);
  EXPECT_FALSE(view->dynamic_style_flags_ &
               DynamicCSSStylesManager::kUpdateScreenMetrics);
  EXPECT_FALSE(view->dynamic_style_flags_ &
               DynamicCSSStylesManager::kUpdateFontScale);

  view->CheckDynamicUnit(CSSPropertyID::kPropertyIDWidth,
                         map[CSSPropertyID::kPropertyIDWidth], true);
}

TEST_P(FiberElementTest, TestCheckDynamicUnit1) {
  auto view = manager->CreateFiberView();
  view->FlushActionsAsRoot();

  auto impl = lepus::Value("1rpx");
  CSSPropertyID id = CSSPropertyID::kPropertyIDWidth;
  CSSParserConfigs configs;
  auto map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(CSSPropertyID::kPropertyIDWidth,
                         map[CSSPropertyID::kPropertyIDWidth], false);
  EXPECT_FALSE(view->dynamic_style_flags_ &
               DynamicCSSStylesManager::kUpdateViewport);
  EXPECT_TRUE(view->dynamic_style_flags_ &
              DynamicCSSStylesManager::kUpdateScreenMetrics);
  EXPECT_FALSE(view->dynamic_style_flags_ &
               DynamicCSSStylesManager::kUpdateFontScale);
}

TEST_P(FiberElementTest, TestCheckDynamicUnit11) {
  auto view = manager->CreateFiberView();
  view->FlushActionsAsRoot();

  auto impl = lepus::Value("calc(100px - 200px)");
  CSSPropertyID id = CSSPropertyID::kPropertyIDBorderWidth;
  CSSParserConfigs configs;
  auto map = UnitHandler::Process(id, impl, configs);
  for (const auto& pair : map) {
    view->CheckDynamicUnit(pair.first, pair.second, false);
  }
  EXPECT_FALSE(view->dynamic_style_flags_ &
               DynamicCSSStylesManager::kUpdateViewport);
  EXPECT_FALSE(view->dynamic_style_flags_ &
               DynamicCSSStylesManager::kUpdateScreenMetrics);
  EXPECT_FALSE(view->dynamic_style_flags_ &
               DynamicCSSStylesManager::kUpdateFontScale);
}

TEST_P(FiberElementTest, TestCheckDynamicUnit2) {
  auto view = manager->CreateFiberView();
  view->FlushActionsAsRoot();

  auto impl = lepus::Value("1rpx");
  CSSPropertyID id = CSSPropertyID::kPropertyIDFontSize;
  CSSParserConfigs configs;
  auto map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_FALSE(view->dynamic_style_flags_ &
               DynamicCSSStylesManager::kUpdateViewport);
  EXPECT_TRUE(view->dynamic_style_flags_ &
              DynamicCSSStylesManager::kUpdateScreenMetrics);
  EXPECT_TRUE(view->dynamic_style_flags_ &
              DynamicCSSStylesManager::kUpdateFontScale);
}

TEST_P(FiberElementTest, TestCheckDynamicUnit3) {
  auto view = manager->CreateFiberView();
  view->FlushActionsAsRoot();

  auto impl = lepus::Value("1vw");
  CSSPropertyID id = CSSPropertyID::kPropertyIDFontSize;
  CSSParserConfigs configs;
  auto map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_TRUE(view->dynamic_style_flags_ &
              DynamicCSSStylesManager::kUpdateViewport);
  EXPECT_FALSE(view->dynamic_style_flags_ &
               DynamicCSSStylesManager::kUpdateScreenMetrics);
  EXPECT_FALSE(view->dynamic_style_flags_ &
               DynamicCSSStylesManager::kUpdateFontScale);
}

TEST_P(FiberElementTest, TestCheckDynamicUnit4) {
  auto view = manager->CreateFiberView();
  view->FlushActionsAsRoot();

  auto impl = lepus::Value("1rem");
  CSSPropertyID id = CSSPropertyID::kPropertyIDFontSize;
  CSSParserConfigs configs;
  auto map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_FALSE(view->dynamic_style_flags_ &
               DynamicCSSStylesManager::kUpdateViewport);
  EXPECT_FALSE(view->dynamic_style_flags_ &
               DynamicCSSStylesManager::kUpdateScreenMetrics);
  EXPECT_FALSE(view->dynamic_style_flags_ &
               DynamicCSSStylesManager::kUpdateFontScale);
  EXPECT_TRUE(view->dynamic_style_flags_ & DynamicCSSStylesManager::kUpdateRem);
}

TEST_P(FiberElementTest, TestCheckDynamicUnit5) {
  auto view = manager->CreateFiberView();
  view->FlushActionsAsRoot();

  auto impl = lepus::Value("1em");
  CSSPropertyID id = CSSPropertyID::kPropertyIDFontSize;
  CSSParserConfigs configs;
  auto map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_FALSE(view->dynamic_style_flags_ &
               DynamicCSSStylesManager::kUpdateViewport);
  EXPECT_FALSE(view->dynamic_style_flags_ &
               DynamicCSSStylesManager::kUpdateScreenMetrics);
  EXPECT_FALSE(view->dynamic_style_flags_ &
               DynamicCSSStylesManager::kUpdateFontScale);
  EXPECT_FALSE(view->dynamic_style_flags_ &
               DynamicCSSStylesManager::kUpdateRem);
  EXPECT_TRUE(view->dynamic_style_flags_ & DynamicCSSStylesManager::kUpdateEm);
}

TEST_P(FiberElementTest, TestCheckDynamicUnit6) {
  auto view = manager->CreateFiberView();
  view->FlushActionsAsRoot();

  auto impl = lepus::Value("calc(1px)");
  CSSPropertyID id = CSSPropertyID::kPropertyIDFontSize;
  CSSParserConfigs configs;
  auto map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_FALSE(view->dynamic_style_flags_ &
               DynamicCSSStylesManager::kUpdateViewport);
  EXPECT_FALSE(view->dynamic_style_flags_ &
               DynamicCSSStylesManager::kUpdateScreenMetrics);
  EXPECT_TRUE(view->dynamic_style_flags_ &
              DynamicCSSStylesManager::kUpdateFontScale);
  EXPECT_FALSE(view->dynamic_style_flags_ &
               DynamicCSSStylesManager::kUpdateRem);
  EXPECT_FALSE(view->dynamic_style_flags_ & DynamicCSSStylesManager::kUpdateEm);
}

TEST_P(FiberElementTest, TestCheckDynamicUnit7) {
  auto view = manager->CreateFiberView();
  view->FlushActionsAsRoot();

  auto impl = lepus::Value("1vw 1vh");
  CSSPropertyID id = CSSPropertyID::kPropertyIDBackgroundSize;
  CSSParserConfigs configs;
  auto map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_TRUE(view->dynamic_style_flags_ &
              DynamicCSSStylesManager::kUpdateViewport);
  EXPECT_TRUE(view->dynamic_style_flags_ &
              DynamicCSSStylesManager::kUpdateScreenMetrics);
  EXPECT_TRUE(view->dynamic_style_flags_ &
              DynamicCSSStylesManager::kUpdateFontScale);
  EXPECT_TRUE(view->dynamic_style_flags_ & DynamicCSSStylesManager::kUpdateRem);
  EXPECT_TRUE(view->dynamic_style_flags_ & DynamicCSSStylesManager::kUpdateEm);

  view->dynamic_style_flags_ = 0;
  impl = lepus::Value("flex");
  id = CSSPropertyID::kPropertyIDFlex;
  map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_EQ(view->dynamic_style_flags_, 0);

  view->dynamic_style_flags_ = 0;
  impl = lepus::Value("1rpx");
  id = CSSPropertyID::kPropertyIDFontSize;
  map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_EQ(view->dynamic_style_flags_,
            DynamicCSSStylesManager::kUpdateScreenMetrics |
                DynamicCSSStylesManager::kUpdateFontScale);

  view->dynamic_style_flags_ = 0;
  impl = lepus::Value("1px");
  id = CSSPropertyID::kPropertyIDFontSize;
  map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_EQ(view->dynamic_style_flags_,
            DynamicCSSStylesManager::kUpdateFontScale);

  view->dynamic_style_flags_ = 0;
  impl = lepus::Value("1rpx");
  id = CSSPropertyID::kPropertyIDWidth;
  map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_EQ(view->dynamic_style_flags_,
            DynamicCSSStylesManager::kUpdateScreenMetrics);

  view->dynamic_style_flags_ = 0;
  impl = lepus::Value("100%");
  id = CSSPropertyID::kPropertyIDFontSize;
  map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_EQ(view->dynamic_style_flags_,
            DynamicCSSStylesManager::kUpdateFontScale);

  view->dynamic_style_flags_ = 0;
  impl = lepus::Value("100%");
  id = CSSPropertyID::kPropertyIDLineHeight;
  map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_EQ(view->dynamic_style_flags_,
            DynamicCSSStylesManager::kUpdateEm |
                DynamicCSSStylesManager::kUpdateFontScale);

  view->dynamic_style_flags_ = 0;
  impl = lepus::Value("1rem");
  id = CSSPropertyID::kPropertyIDLineHeight;
  map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_EQ(view->dynamic_style_flags_, DynamicCSSStylesManager::kUpdateRem);

  view->dynamic_style_flags_ = 0;
  impl = lepus::Value("1em");
  id = CSSPropertyID::kPropertyIDLineHeight;
  map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_EQ(view->dynamic_style_flags_, DynamicCSSStylesManager::kUpdateEm);

  view->dynamic_style_flags_ = 0;
  impl = lepus::Value("1vw");
  id = CSSPropertyID::kPropertyIDLineHeight;
  map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_EQ(view->dynamic_style_flags_,
            DynamicCSSStylesManager::kUpdateViewport);

  view->dynamic_style_flags_ = 0;
  impl = lepus::Value("1vh");
  id = CSSPropertyID::kPropertyIDLineHeight;
  map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_EQ(view->dynamic_style_flags_,
            DynamicCSSStylesManager::kUpdateViewport);

  view->dynamic_style_flags_ = 0;
  impl = lepus::Value("1vh");
  id = CSSPropertyID::kPropertyIDLineHeight;
  map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_EQ(view->dynamic_style_flags_,
            DynamicCSSStylesManager::kUpdateViewport);

  view->dynamic_style_flags_ = 0;
  impl = lepus::Value("calc(1rpx)");
  id = CSSPropertyID::kPropertyIDHeight;
  map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_EQ(view->dynamic_style_flags_,
            DynamicCSSStylesManager::kUpdateScreenMetrics);

  view->dynamic_style_flags_ = 0;
  impl = lepus::Value("calc(1rpx)");
  id = CSSPropertyID::kPropertyIDLineHeight;
  map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_EQ(view->dynamic_style_flags_,
            DynamicCSSStylesManager::kUpdateScreenMetrics |
                DynamicCSSStylesManager::kUpdateFontScale);

  view->dynamic_style_flags_ = 0;
  impl = lepus::Value("calc(1rem)");
  id = CSSPropertyID::kPropertyIDLineHeight;
  map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_EQ(view->dynamic_style_flags_, DynamicCSSStylesManager::kUpdateRem |
                                            DynamicCSSStylesManager::kUpdateEm);

  view->dynamic_style_flags_ = 0;
  impl = lepus::Value("calc(1em)");
  id = CSSPropertyID::kPropertyIDLineHeight;
  map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_EQ(view->dynamic_style_flags_, DynamicCSSStylesManager::kUpdateEm);

  view->dynamic_style_flags_ = 0;
  impl = lepus::Value("calc(100%)");
  id = CSSPropertyID::kPropertyIDLineHeight;
  map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_EQ(view->dynamic_style_flags_, DynamicCSSStylesManager::kUpdateEm);

  view->dynamic_style_flags_ = 0;
  impl = lepus::Value("calc(1px)");
  id = CSSPropertyID::kPropertyIDLineHeight;
  map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_EQ(view->dynamic_style_flags_,
            DynamicCSSStylesManager::kUpdateFontScale);

  view->dynamic_style_flags_ = 0;
  impl = lepus::Value("calc(1ppx)");
  id = CSSPropertyID::kPropertyIDLineHeight;
  map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_EQ(view->dynamic_style_flags_,
            DynamicCSSStylesManager::kUpdateFontScale);

  view->dynamic_style_flags_ = 0;
  impl = lepus::Value("calc(1vw)");
  id = CSSPropertyID::kPropertyIDLineHeight;
  map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_EQ(view->dynamic_style_flags_,
            DynamicCSSStylesManager::kUpdateViewport);

  view->dynamic_style_flags_ = 0;
  impl = lepus::Value("calc(1sp)");
  id = CSSPropertyID::kPropertyIDHeight;
  map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_EQ(view->dynamic_style_flags_,
            DynamicCSSStylesManager::kUpdateFontScale);

  view->dynamic_style_flags_ = 0;
  impl = lepus::Value("1sp");
  id = CSSPropertyID::kPropertyIDHeight;
  map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_EQ(view->dynamic_style_flags_,
            DynamicCSSStylesManager::kUpdateFontScale);

  view->dynamic_style_flags_ = 0;
  impl = lepus::Value("1px");
  id = CSSPropertyID::kPropertyIDBackgroundSize;
  map = UnitHandler::Process(id, impl, configs);
  view->CheckDynamicUnit(id, map[id], false);
  EXPECT_EQ(view->dynamic_style_flags_,
            DynamicCSSStylesManager::kUpdateScreenMetrics |
                DynamicCSSStylesManager::kUpdateEm |
                DynamicCSSStylesManager::kUpdateRem |
                DynamicCSSStylesManager::kUpdateFontScale |
                DynamicCSSStylesManager::kUpdateViewport);
}

TEST_P(FiberElementTest, TestUpdateDynamicElementStyle00) {
  auto view = manager->CreateFiberPage("0", 0);

  CSSPropertyID id = CSSPropertyID::kPropertyIDLineHeight;
  auto value = lepus::Value("10px");

  view->SetStyle(id, value);
  view->FlushActionsAsRoot();
  view->pre_prop_bundle_ = nullptr;

  auto& env_config = manager->GetLynxEnvConfig();
  env_config.SetFontScale(2);
  view->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateFontScale,
                                  false);
  EXPECT_TRUE(view->pre_prop_bundle_);
}

TEST_P(FiberElementTest, TestUpdateDynamicElementStyle0) {
  auto view = manager->CreateFiberPage("0", 0);

  CSSPropertyID id = CSSPropertyID::kPropertyIDFontSize;
  auto value = lepus::Value("10vw");

  view->SetStyle(id, value);
  view->FlushActionsAsRoot();
  view->pre_prop_bundle_ = nullptr;

  auto& env_config = manager->GetLynxEnvConfig();
  env_config.UpdateScreenSize(10, 100);
  view->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateScreenMetrics,
                                  false);
  EXPECT_FALSE(view->pre_prop_bundle_);
  view->pre_prop_bundle_ = nullptr;

  env_config.SetFontScale(2);
  view->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateFontScale,
                                  false);
  EXPECT_FALSE(view->pre_prop_bundle_);
  view->pre_prop_bundle_ = nullptr;

  env_config.UpdateViewport(100, SLMeasureModeDefinite, 1,
                            SLMeasureModeDefinite);
  view->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateViewport,
                                  false);
  EXPECT_TRUE(view->pre_prop_bundle_);

  auto* mock_painting_context = static_cast<FiberMockPaintingContext*>(
      view->painting_context()->platform_impl_.get());
  mock_painting_context->Flush();

  auto* mock_painting_node_ =
      mock_painting_context->node_map_.at(view->impl_id()).get();

  EXPECT_TRUE(mock_painting_node_->props_.find("font-size") !=
              mock_painting_node_->props_.end());
  EXPECT_EQ(mock_painting_node_->props_["font-size"].Number(), 10);
  view->pre_prop_bundle_ = nullptr;
}

TEST_P(FiberElementTest, TestUpdateDynamicElementStyle1) {
  auto view = manager->CreateFiberPage("0", 0);

  CSSPropertyID id = CSSPropertyID::kPropertyIDBorder;
  auto value = lepus::Value("1rpx solid black");

  view->SetStyle(id, value);
  view->FlushActionsAsRoot();
  view->pre_prop_bundle_ = nullptr;

  auto& env_config = manager->GetLynxEnvConfig();
  env_config.UpdateScreenSize(10, 100);
  view->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateScreenMetrics,
                                  false);
  EXPECT_TRUE(view->pre_prop_bundle_);
  view->pre_prop_bundle_ = nullptr;

  env_config.SetFontScale(2);
  view->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateFontScale,
                                  false);
  EXPECT_FALSE(view->pre_prop_bundle_);

  env_config.UpdateViewport(1, SLMeasureModeDefinite, 1, SLMeasureModeDefinite);
  view->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateViewport,
                                  false);
  EXPECT_FALSE(view->pre_prop_bundle_);
}

TEST_P(FiberElementTest, TestUpdateDynamicElementStyle2) {
  auto view = manager->CreateFiberPage("0", 0);

  CSSPropertyID id = CSSPropertyID::kPropertyIDBackgroundSize;
  auto value = lepus::Value("100vh 100vw");

  view->SetStyle(id, value);
  view->FlushActionsAsRoot();
  view->pre_prop_bundle_ = nullptr;

  auto& env_config = manager->GetLynxEnvConfig();
  env_config.UpdateViewport(390, SLMeasureMode::SLMeasureModeDefinite, 880,
                            SLMeasureMode::SLMeasureModeDefinite);
  view->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateViewport,
                                  false);
  EXPECT_TRUE(view->pre_prop_bundle_);
  view->pre_prop_bundle_ = nullptr;

  env_config.SetFontScale(2);
  view->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateFontScale,
                                  false);
  EXPECT_FALSE(view->pre_prop_bundle_);
  view->pre_prop_bundle_ = nullptr;

  env_config.UpdateViewport(1, SLMeasureModeDefinite, 1, SLMeasureModeDefinite);
  view->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateViewport,
                                  false);
  EXPECT_TRUE(view->pre_prop_bundle_);
}

TEST_P(FiberElementTest, TestUpdateDynamicElementStyle3) {
  auto view = manager->CreateFiberPage("0", 0);

  CSSPropertyID id = CSSPropertyID::kPropertyIDFontSize;
  auto value = lepus::Value("calc(100rpx + 1vw)");

  view->SetStyle(id, value);
  view->FlushActionsAsRoot();
  view->pre_prop_bundle_ = nullptr;

  auto& env_config = manager->GetLynxEnvConfig();
  env_config.UpdateViewport(10, SLMeasureModeDefinite, 10,
                            SLMeasureModeDefinite);
  view->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateViewport,
                                  false);
  EXPECT_TRUE(view->pre_prop_bundle_);
  view->pre_prop_bundle_ = nullptr;

  env_config.UpdateScreenSize(10, 100);
  view->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateScreenMetrics,
                                  false);
  EXPECT_TRUE(view->pre_prop_bundle_);
  view->pre_prop_bundle_ = nullptr;

  env_config.SetFontScale(2);
  view->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateFontScale,
                                  false);
  EXPECT_TRUE(view->pre_prop_bundle_);
  view->pre_prop_bundle_ = nullptr;
}

TEST_P(FiberElementTest, TestUpdateDynamicElementStyle4) {
  auto view = manager->CreateFiberPage("0", 0);

  CSSPropertyID id = CSSPropertyID::kPropertyIDFontSize;
  auto value = lepus::Value("10vh");

  view->SetStyle(id, value);
  view->FlushActionsAsRoot();
  view->pre_prop_bundle_ = nullptr;

  auto& env_config = manager->GetLynxEnvConfig();
  env_config.UpdateViewport(10, SLMeasureModeDefinite, 10,
                            SLMeasureModeDefinite);
  view->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateViewport,
                                  false);
  EXPECT_TRUE(view->pre_prop_bundle_);
  view->pre_prop_bundle_ = nullptr;

  env_config.UpdateScreenSize(10, 100);
  view->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateScreenMetrics,
                                  false);
  EXPECT_FALSE(view->pre_prop_bundle_);
  view->pre_prop_bundle_ = nullptr;

  env_config.SetFontScale(10);
  view->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateFontScale,
                                  false);
  EXPECT_FALSE(view->pre_prop_bundle_);
  view->pre_prop_bundle_ = nullptr;
}

TEST_P(FiberElementTest, TestUpdateDynamicElementStyle5) {
  auto page = manager->CreateFiberPage("0", 0);
  CSSPropertyID id = CSSPropertyID::kPropertyIDFontSize;
  auto value = lepus::Value("10rpx");
  page->SetStyle(id, value);
  page->FlushActionsAsRoot();
  page->pre_prop_bundle_ = nullptr;

  auto view = manager->CreateFiberView();
  CSSPropertyID new_id = CSSPropertyID::kPropertyIDWidth;
  auto new_value = lepus::Value("1rem");
  view->SetStyle(new_id, new_value);
  page->InsertNode(view);

  page->FlushActionsAsRoot();

  page->pre_prop_bundle_ = nullptr;
  view->pre_prop_bundle_ = nullptr;
  tasm_mediator.captured_ids_.clear();
  tasm_mediator.captured_bundles_.clear();

  auto& env_config = manager->GetLynxEnvConfig();
  env_config.UpdateViewport(10, SLMeasureModeDefinite, 10,
                            SLMeasureModeDefinite);
  page->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateViewport,
                                  false);
  EXPECT_FALSE(page->pre_prop_bundle_);
  EXPECT_FALSE(view->pre_prop_bundle_);
  page->pre_prop_bundle_ = nullptr;
  view->pre_prop_bundle_ = nullptr;
  tasm_mediator.captured_ids_.clear();
  tasm_mediator.captured_bundles_.clear();

  env_config.UpdateScreenSize(10, 100);
  page->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateScreenMetrics,
                                  false);
  EXPECT_TRUE(page->pre_prop_bundle_);
  EXPECT_FALSE(view->pre_prop_bundle_);
  EXPECT_TRUE(HasCaptureSignWithFontSize(page->impl_id(), page->GetFontSize(),
                                         page->GetCurrentRootFontSize(), 1, 1));
  EXPECT_TRUE(HasCaptureSignWithStyleKeyAndValue(
      view->impl_id(), CSSPropertyID::kPropertyIDWidth,
      tasm::CSSValue(1.0f, CSSValuePattern::REM)));
  EXPECT_TRUE(HasCaptureSignWithFontSize(view->impl_id(), view->GetFontSize(),
                                         view->GetCurrentRootFontSize(), 1, 1));
  page->pre_prop_bundle_ = nullptr;
  view->pre_prop_bundle_ = nullptr;
  tasm_mediator.captured_ids_.clear();
  tasm_mediator.captured_bundles_.clear();

  env_config.SetFontScale(10);
  page->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateFontScale,
                                  false);
  EXPECT_TRUE(page->pre_prop_bundle_);
  EXPECT_FALSE(view->pre_prop_bundle_);
  EXPECT_TRUE(HasCaptureSignWithFontSize(page->impl_id(), page->GetFontSize(),
                                         page->GetCurrentRootFontSize(), 10,
                                         1));
  EXPECT_TRUE(HasCaptureSignWithStyleKeyAndValue(
      view->impl_id(), CSSPropertyID::kPropertyIDWidth,
      tasm::CSSValue(1.0f, CSSValuePattern::REM)));
  EXPECT_TRUE(HasCaptureSignWithFontSize(view->impl_id(), view->GetFontSize(),
                                         view->GetCurrentRootFontSize(), 10,
                                         1));
}

TEST_P(FiberElementTest, TestUpdateDynamicElementStyle6) {
  auto page = manager->CreateFiberPage("0", 0);
  CSSPropertyID id = CSSPropertyID::kPropertyIDFontSize;
  auto value = lepus::Value("10rpx");
  page->SetStyle(id, value);
  page->FlushActionsAsRoot();
  page->pre_prop_bundle_ = nullptr;

  auto view = manager->CreateFiberView();
  CSSPropertyID new_id_0 = CSSPropertyID::kPropertyIDWidth;
  auto new_value_0 = lepus::Value("1em");
  view->SetStyle(new_id_0, new_value_0);
  CSSPropertyID new_id_1 = CSSPropertyID::kPropertyIDFontSize;
  auto new_value_1 = lepus::Value("1rem");
  view->SetStyle(new_id_1, new_value_1);
  page->InsertNode(view);

  page->FlushActionsAsRoot();

  page->pre_prop_bundle_ = nullptr;
  view->pre_prop_bundle_ = nullptr;
  tasm_mediator.captured_ids_.clear();
  tasm_mediator.captured_bundles_.clear();

  auto& env_config = manager->GetLynxEnvConfig();
  env_config.UpdateViewport(10, SLMeasureModeDefinite, 10,
                            SLMeasureModeDefinite);
  page->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateViewport,
                                  false);
  EXPECT_FALSE(page->pre_prop_bundle_);
  EXPECT_FALSE(view->pre_prop_bundle_);
  page->pre_prop_bundle_ = nullptr;
  view->pre_prop_bundle_ = nullptr;
  tasm_mediator.captured_ids_.clear();
  tasm_mediator.captured_bundles_.clear();

  env_config.UpdateScreenSize(10, 100);
  page->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateScreenMetrics,
                                  false);
  EXPECT_TRUE(page->pre_prop_bundle_);
  EXPECT_TRUE(view->pre_prop_bundle_);
  EXPECT_TRUE(HasCaptureSignWithFontSize(page->impl_id(), page->GetFontSize(),
                                         page->GetCurrentRootFontSize(), 1, 1));
  EXPECT_TRUE(HasCaptureSignWithStyleKeyAndValue(
      view->impl_id(), CSSPropertyID::kPropertyIDWidth,
      tasm::CSSValue(1.0f, CSSValuePattern::EM)));
  EXPECT_TRUE(HasCaptureSignWithFontSize(view->impl_id(), view->GetFontSize(),
                                         view->GetCurrentRootFontSize(), 1, 1));
  page->pre_prop_bundle_ = nullptr;
  view->pre_prop_bundle_ = nullptr;
  tasm_mediator.captured_ids_.clear();
  tasm_mediator.captured_bundles_.clear();

  env_config.SetFontScale(10);
  page->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateFontScale,
                                  false);
  EXPECT_TRUE(page->pre_prop_bundle_);
  EXPECT_TRUE(view->pre_prop_bundle_);
  EXPECT_TRUE(HasCaptureSignWithFontSize(page->impl_id(), page->GetFontSize(),
                                         page->GetCurrentRootFontSize(), 10,
                                         1));
  EXPECT_TRUE(HasCaptureSignWithStyleKeyAndValue(
      view->impl_id(), CSSPropertyID::kPropertyIDWidth,
      tasm::CSSValue(1.0f, CSSValuePattern::EM)));
  EXPECT_TRUE(HasCaptureSignWithFontSize(view->impl_id(), view->GetFontSize(),
                                         view->GetCurrentRootFontSize(), 10,
                                         1));
}

TEST_P(FiberElementTest, TestUpdateDynamicElementStyle7) {
  auto page = manager->CreateFiberPage("0", 0);
  CSSPropertyID id = CSSPropertyID::kPropertyIDFontSize;
  auto value = lepus::Value("10rpx");
  page->SetStyle(id, value);
  page->FlushActionsAsRoot();
  page->pre_prop_bundle_ = nullptr;

  auto view = manager->CreateFiberView();
  CSSPropertyID new_id_0 = CSSPropertyID::kPropertyIDWidth;
  auto new_value_0 = lepus::Value("1em");
  view->SetStyle(new_id_0, new_value_0);
  CSSPropertyID new_id_1 = CSSPropertyID::kPropertyIDFontSize;
  auto new_value_1 = lepus::Value("10rpx");
  view->SetStyle(new_id_1, new_value_1);
  page->InsertNode(view);

  page->FlushActionsAsRoot();

  page->pre_prop_bundle_ = nullptr;
  view->pre_prop_bundle_ = nullptr;
  tasm_mediator.captured_ids_.clear();
  tasm_mediator.captured_bundles_.clear();

  auto& env_config = manager->GetLynxEnvConfig();
  env_config.UpdateViewport(10, SLMeasureModeDefinite, 10,
                            SLMeasureModeDefinite);
  page->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateViewport,
                                  false);
  EXPECT_FALSE(page->pre_prop_bundle_);
  EXPECT_FALSE(view->pre_prop_bundle_);
  page->pre_prop_bundle_ = nullptr;
  view->pre_prop_bundle_ = nullptr;
  tasm_mediator.captured_ids_.clear();
  tasm_mediator.captured_bundles_.clear();

  env_config.UpdateScreenSize(10, 100);
  page->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateScreenMetrics,
                                  false);
  EXPECT_TRUE(page->pre_prop_bundle_);
  EXPECT_TRUE(view->pre_prop_bundle_);
  EXPECT_TRUE(HasCaptureSignWithFontSize(page->impl_id(), page->GetFontSize(),
                                         page->GetCurrentRootFontSize(), 1, 1));
  EXPECT_TRUE(HasCaptureSignWithStyleKeyAndValue(
      view->impl_id(), CSSPropertyID::kPropertyIDWidth,
      tasm::CSSValue(1.0f, CSSValuePattern::EM)));
  EXPECT_TRUE(HasCaptureSignWithFontSize(view->impl_id(), view->GetFontSize(),
                                         view->GetCurrentRootFontSize(), 1, 1));
  page->pre_prop_bundle_ = nullptr;
  view->pre_prop_bundle_ = nullptr;
  tasm_mediator.captured_ids_.clear();
  tasm_mediator.captured_bundles_.clear();

  env_config.SetFontScale(10);
  page->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateFontScale,
                                  false);
  EXPECT_TRUE(page->pre_prop_bundle_);
  EXPECT_TRUE(view->pre_prop_bundle_);
  EXPECT_TRUE(HasCaptureSignWithFontSize(page->impl_id(), page->GetFontSize(),
                                         page->GetCurrentRootFontSize(), 10,
                                         1));
  EXPECT_TRUE(HasCaptureSignWithStyleKeyAndValue(
      view->impl_id(), CSSPropertyID::kPropertyIDWidth,
      tasm::CSSValue(1.0f, CSSValuePattern::EM)));
  EXPECT_TRUE(HasCaptureSignWithFontSize(view->impl_id(), view->GetFontSize(),
                                         view->GetCurrentRootFontSize(), 10,
                                         1));
}

TEST_P(FiberElementTest, TestUpdateDynamicElementStyle8) {
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
    auto id = CSSPropertyID::kPropertyIDLineHeight;
    auto impl = lepus::Value("18px");
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
  element0->SetClass("root");
  page->InsertNode(element0);

  auto text = manager->CreateFiberText("text");
  CSSPropertyID id = CSSPropertyID::kPropertyIDLineHeight;
  auto value = lepus::Value("28px");
  text->SetStyle(id, value);
  text->parent_component_element_ = page.get();
  element0->InsertNode(text);

  page->FlushActionsAsRoot();

  EXPECT_TRUE(
      text->platform_css_style_->text_attributes_->computed_line_height == 28);

  auto& env_config = manager->GetLynxEnvConfig();
  env_config.UpdateViewport(10, SLMeasureModeDefinite, 10,
                            SLMeasureModeDefinite);
  page->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateViewport,
                                  true);

  EXPECT_TRUE(
      text->platform_css_style_->text_attributes_->computed_line_height == 28);
}

TEST_P(FiberElementTest, TestUpdateStyleKeepFontScale) {
  manager->GetLynxEnvConfig().font_scale_sp_only_ = false;
  manager->GetLynxEnvConfig().font_scale_ = 1.3f;

  auto page = manager->CreateFiberPage("page", 11);
  auto fiber_element = manager->CreateFiberText("text");
  page->InsertNode(fiber_element);
  page->FlushActionsAsRoot();

  EXPECT_EQ(fiber_element->computed_css_style()->length_context_.font_scale_,
            1.3f);
  EXPECT_EQ(
      fiber_element->computed_css_style()->length_context_.cur_node_font_size_,
      18.1999989f);
  EXPECT_EQ(
      fiber_element->computed_css_style()->length_context_.root_node_font_size_,
      18.1999989f);

  fiber_element->SetStyle(CSSPropertyID::kPropertyIDWidth,
                          lepus::Value("100px"));
  page->SetStyle(CSSPropertyID::kPropertyIDPaddingTop, lepus::Value("10px"));

  page->FlushActionsAsRoot();

  EXPECT_EQ(fiber_element->computed_css_style()->length_context_.font_scale_,
            1.3f);
  EXPECT_EQ(
      fiber_element->computed_css_style()->length_context_.cur_node_font_size_,
      18.1999989f);
  EXPECT_EQ(
      fiber_element->computed_css_style()->length_context_.root_node_font_size_,
      18.1999989f);
}

TEST_P(FiberElementTest, TestUpdateDynamicElementStyle9) {
  manager->GetLynxEnvConfig().font_scale_sp_only_ = true;
  manager->GetLynxEnvConfig().font_scale_ = 1.0f;

  auto page = manager->CreateFiberPage("page", 11);
  auto fiber_element = manager->CreateFiberText("text");
  page->InsertNode(fiber_element);
  page->FlushActionsAsRoot();

  EXPECT_EQ(fiber_element->computed_css_style()->length_context_.font_scale_,
            1.0f);
  EXPECT_EQ(
      fiber_element->computed_css_style()->length_context_.cur_node_font_size_,
      14);
  EXPECT_EQ(
      fiber_element->computed_css_style()->length_context_.root_node_font_size_,
      14);

  auto& env_config = manager->GetLynxEnvConfig();
  env_config.font_scale_ = 1.3f;
  page->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateFontScale,
                                  true);

  EXPECT_EQ(fiber_element->computed_css_style()->length_context_.font_scale_,
            1.3f);
  EXPECT_EQ(
      fiber_element->computed_css_style()->length_context_.cur_node_font_size_,
      14);
  EXPECT_EQ(
      fiber_element->computed_css_style()->length_context_.root_node_font_size_,
      14);
}

TEST_P(FiberElementTest, DynamicViewportUpdate) {
  manager->UpdateViewport(100, SLMeasureModeDefinite, 600,
                          SLMeasureModeDefinite, false);

  auto page = manager->CreateFiberPage("page", 11);
  auto element = manager->CreateFiberView();
  element->SetStyle(CSSPropertyID::kPropertyIDWidth, lepus::Value("50vh"));
  element->SetStyle(CSSPropertyID::kPropertyIDHeight, lepus::Value("100vw"));

  page->InsertNode(element);
  page->FlushActionsAsRoot();

  EXPECT_TRUE(HasCaptureSignWithTag(page->impl_id(), "page"));
  EXPECT_TRUE(HasCaptureSignWithStyleKeyAndValuePattern(
      element->impl_id(), CSSPropertyID::kPropertyIDWidth,
      CSSValue(0.f, CSSValuePattern::VH)));
  EXPECT_TRUE(HasCaptureSignWithStyleKeyAndValuePattern(
      element->impl_id(), CSSPropertyID::kPropertyIDHeight,
      CSSValue(0.f, CSSValuePattern::VW)));

  tasm_mediator.captured_ids_.clear();
  tasm_mediator.captured_bundles_.clear();
  manager->UpdateViewport(200, SLMeasureModeDefinite, 800,
                          SLMeasureModeDefinite, false);
  EXPECT_TRUE(HasCaptureSignWithStyleKeyAndValuePattern(
      element->impl_id(), CSSPropertyID::kPropertyIDWidth,
      CSSValue(0.f, CSSValuePattern::VH), 1));
  EXPECT_TRUE(HasCaptureSignWithStyleKeyAndValuePattern(
      element->impl_id(), CSSPropertyID::kPropertyIDHeight,
      CSSValue(0.f, CSSValuePattern::VW), 1));

  tasm_mediator.captured_ids_.clear();
  tasm_mediator.captured_bundles_.clear();
  element->SetStyle(CSSPropertyID::kPropertyIDHeight,
                    lepus::Value("calc(100vh - 100px)"));
  page->FlushActionsAsRoot();
  EXPECT_TRUE(HasCaptureSignWithStyleKeyAndValuePattern(
      element->impl_id(), CSSPropertyID::kPropertyIDHeight,
      CSSValue(0.f, CSSValuePattern::CALC)));

  tasm_mediator.captured_ids_.clear();
  tasm_mediator.captured_bundles_.clear();
  manager->UpdateViewport(200, SLMeasureModeDefinite, 500,
                          SLMeasureModeDefinite, false);
  EXPECT_TRUE(HasCaptureSignWithStyleKeyAndValuePattern(
      element->impl_id(), CSSPropertyID::kPropertyIDWidth,
      CSSValue(0.f, CSSValuePattern::VH), 1));
  EXPECT_TRUE(HasCaptureSignWithStyleKeyAndValuePattern(
      element->impl_id(), CSSPropertyID::kPropertyIDHeight,
      CSSValue(0.f, CSSValuePattern::CALC), 1));
}

TEST_P(FiberElementTest, DynamicFontScaleUpdate) {
  manager->UpdateViewport(100, SLMeasureModeDefinite, 600,
                          SLMeasureModeDefinite, false);

  auto page = manager->CreateFiberPage("page", 11);
  auto element = manager->CreateFiberView();
  element->SetStyle(CSSPropertyID::kPropertyIDFontSize, lepus::Value("20px"));
  element->SetStyle(CSSPropertyID::kPropertyIDHeight, lepus::Value("1.5em"));

  page->InsertNode(element);

  page->FlushActionsAsRoot();

  const int32_t root_font_size = 14;
  EXPECT_TRUE(
      HasCaptureSignWithFontSize(element->impl_id(), 20, root_font_size, 1));

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->platform_impl_.get());
  painting_context->Flush();

  auto* mock_painting_node_ =
      painting_context->node_map_.at(element->impl_id()).get();

  EXPECT_TRUE(mock_painting_node_->props_.find("font-size") !=
              mock_painting_node_->props_.end());
  EXPECT_TRUE(mock_painting_node_->props_["font-size"].Number() == 20);

  manager->UpdateFontScale(3);
  EXPECT_TRUE(
      HasCaptureSignWithFontSize(element->impl_id(), 60, root_font_size, 3));
}

TEST_P(FiberElementTest, DynamicScreenMetricsUpdate) {
  float kScreeWidth = 750;
  float kRpxRatio = 750.0f;

  manager->UpdateScreenMetrics(kScreeWidth, 1000);

  auto page = manager->CreateFiberPage("page", 11);
  auto element = manager->CreateFiberView();
  element->SetStyle(CSSPropertyID::kPropertyIDFontSize, lepus::Value("20rpx"));
  element->SetStyle(CSSPropertyID::kPropertyIDHeight, lepus::Value("200rpx"));

  page->InsertNode(element);

  page->FlushActionsAsRoot();
  const int32_t root_font_size = 14;
  EXPECT_TRUE(HasCaptureSignWithTag(page->impl_id(), "page"));
  EXPECT_TRUE(
      HasCaptureSignWithFontSize(element->impl_id(), 20, root_font_size, 1));
  EXPECT_TRUE(HasCaptureSignWithStyleKeyAndValuePattern(
      element->impl_id(), CSSPropertyID::kPropertyIDHeight,
      CSSValue(0.f, CSSValuePattern::RPX)));

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->platform_impl_.get());
  painting_context->Flush();

  auto* mock_painting_node_ =
      painting_context->node_map_.at(element->impl_id()).get();
  EXPECT_TRUE(mock_painting_node_->props_.find("font-size") !=
              mock_painting_node_->props_.end());
  EXPECT_TRUE(mock_painting_node_->props_["font-size"].Number() ==
              20 * kScreeWidth / kRpxRatio);

  tasm_mediator.captured_ids_.clear();
  tasm_mediator.captured_bundles_.clear();
  manager->UpdateScreenMetrics(kScreeWidth / 2, 1000);
  painting_context->Flush();
  EXPECT_TRUE(mock_painting_node_->props_["font-size"].Number() ==
              10 * kScreeWidth / kRpxRatio);
  EXPECT_TRUE(HasCaptureSignWithFontSize(element->impl_id(), 10, 14, 1, 1));
  EXPECT_TRUE(HasCaptureSignWithStyleKeyAndValuePattern(
      element->impl_id(), CSSPropertyID::kPropertyIDHeight,
      CSSValue(0.f, CSSValuePattern::RPX), 1));
}

TEST_P(FiberElementTest, DynamicViewportUpdateAndRTL) {
  auto& env_config = manager->GetLynxEnvConfig();
  env_config.UpdateViewport(100, SLMeasureModeDefinite, 1,
                            SLMeasureModeDefinite);

  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableCSSInheritance(true);
  std::unordered_set<CSSPropertyID> list = {kPropertyIDDirection};
  config->SetCustomCSSInheritList(std::move(list));

  manager->SetConfig(config);
  const_cast<DynamicCSSConfigs&>(manager->GetDynamicCSSConfigs())
      .unify_vw_vh_behavior_ = true;

  auto page = manager->CreateFiberPage("page", 11);
  page->SetStyle(CSSPropertyID::kPropertyIDDirection, lepus::Value("lynx-rtl"));

  auto element = manager->CreateFiberView();
  element->SetStyle(CSSPropertyID::kPropertyIDBorderTopLeftRadius,
                    lepus::Value("100vw"));
  element->SetStyle(CSSPropertyID::kPropertyIDBorderTopRightRadius,
                    lepus::Value("200vw"));
  page->InsertNode(element);
  page->FlushActionsAsRoot();

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->platform_impl_.get());
  painting_context->Flush();
  auto* element_painting_node_ =
      painting_context->node_map_.at(element->impl_id()).get();
  auto top_left_radius_it =
      element_painting_node_->props_.find("border-top-left-radius");
  EXPECT_TRUE(top_left_radius_it != element_painting_node_->props_.end());
  auto tl_value = top_left_radius_it->second.Array()->get(0).Number();
  EXPECT_TRUE(tl_value == (100 / 100) * 200);  // 200vw

  auto top_right_radius_it =
      element_painting_node_->props_.find("border-top-right-radius");
  EXPECT_TRUE(top_right_radius_it != element_painting_node_->props_.end());
  auto tr_value = top_right_radius_it->second.Array()->get(0).Number();
  EXPECT_TRUE(tr_value == (100 / 100) * 100);  // 100vw

  manager->UpdateViewport(200, SLMeasureModeDefinite, 1, SLMeasureModeDefinite,
                          false);
  painting_context->Flush();

  top_left_radius_it =
      element_painting_node_->props_.find("border-top-left-radius");
  tl_value = top_left_radius_it->second.Array()->get(0).Number();
  EXPECT_EQ(tl_value, (200 / 100) * 200);  // 200vw

  top_right_radius_it =
      element_painting_node_->props_.find("border-top-right-radius");
  tr_value = top_right_radius_it->second.Array()->get(0).Number();
  EXPECT_EQ(tr_value, (200 / 100) * 100);  // 100vw
}

TEST_P(FiberElementTest, TestREMPattern) {
  auto page = manager->CreateFiberPage("page", 11);
  page->SetStyle(CSSPropertyID::kPropertyIDFontSize, lepus::Value("10px"));
  auto parent = manager->CreateFiberView();
  auto element = manager->CreateFiberView();
  element->SetStyle(CSSPropertyID::kPropertyIDFontSize, lepus::Value("1.5rem"));
  element->SetStyle(CSSPropertyID::kPropertyIDHeight, lepus::Value("20rem"));

  parent->InsertNode(element);
  page->InsertNode(parent);

  page->FlushActionsAsRoot();
  const int32_t default_font_size = 14;
  EXPECT_TRUE(HasCaptureSignWithFontSize(page->impl_id(), 10, 10, 1));
  EXPECT_TRUE(
      HasCaptureSignWithFontSize(parent->impl_id(), default_font_size, 10, 1));
  EXPECT_TRUE(HasCaptureSignWithFontSize(element->impl_id(), 15, 10, 1));
  EXPECT_TRUE(HasCaptureSignWithStyleKeyAndValuePattern(
      element->impl_id(), CSSPropertyID::kPropertyIDHeight,
      CSSValue(0.f, CSSValuePattern::REM)));

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->platform_impl_.get());
  painting_context->Flush();
  auto* mock_painting_node_ =
      painting_context->node_map_.at(element->impl_id()).get();
  EXPECT_TRUE(mock_painting_node_->props_.find("font-size") !=
              mock_painting_node_->props_.end());
  EXPECT_TRUE(mock_painting_node_->props_["font-size"].Number() == 15);

  page->SetStyle(CSSPropertyID::kPropertyIDFontSize, lepus::Value("20px"));
  page->FlushActionsAsRoot();
  painting_context->Flush();
  EXPECT_EQ(mock_painting_node_->props_["font-size"].Number(), 30);

  const int32_t root_font_size = 20;
  EXPECT_TRUE(HasCaptureSignWithFontSize(page->impl_id(), 20, 20, 1));
  EXPECT_TRUE(HasCaptureSignWithFontSize(parent->impl_id(), default_font_size,
                                         root_font_size, 1));
  EXPECT_TRUE(
      HasCaptureSignWithFontSize(element->impl_id(), 30, root_font_size, 1));
  EXPECT_TRUE(HasCaptureSignWithStyleKeyAndValuePattern(
      element->impl_id(), CSSPropertyID::kPropertyIDHeight,
      CSSValue(0.f, CSSValuePattern::REM), 2));
}

TEST_P(FiberElementTest, TestFontSizeWhenUnifyVwVhBehaviorFalse) {
  const_cast<DynamicCSSConfigs&>(manager->GetDynamicCSSConfigs())
      .unify_vw_vh_behavior_ = false;

  auto page = manager->CreateFiberPage("page", 11);

  auto text = manager->CreateFiberText("text");
  text->SetStyle(CSSPropertyID::kPropertyIDFontSize, lepus::Value("1rem"));
  page->InsertNode(text);
  page->FlushActionsAsRoot();
  EXPECT_TRUE(text->GetFontSize() - 14 < 0.1);
}

TEST_P(FiberElementTest, FontSizeResetTest) {
  auto page = manager->CreateFiberPage("page", 11);

  auto text = manager->CreateFiberText("text");
  text->SetRawInlineStyles(base::String("font-size:20px"));
  page->InsertNode(text);
  page->FlushActionsAsRoot();
  EXPECT_TRUE(text->GetFontSize() == 20);

  text->RemoveAllInlineStyles();
  text->SetRawInlineStyles(base::String());
  page->FlushActionsAsRoot();
  EXPECT_TRUE(text->GetFontSize() ==
              manager->GetLynxEnvConfig().PageDefaultFontSize());
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
