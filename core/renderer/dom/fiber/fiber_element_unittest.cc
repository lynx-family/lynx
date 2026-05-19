// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/fiber/fiber_element.h"

#include <cmath>
#include <memory>
#include <mutex>
#include <tuple>

#include "base/include/auto_reset.h"
#include "core/animation/css_transition_manager.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/base/threading/vsync_monitor.h"
#include "core/event/event_dispatcher.h"
#include "core/event/touch_event.h"
#include "core/renderer/css/computed_css_style_css_text_helper.h"
#include "core/renderer/css/css_color.h"
#include "core/renderer/css/css_decoder.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/css/ng/parser/css_parser_token_range.h"
#include "core/renderer/css/ng/parser/css_tokenizer.h"
#include "core/renderer/css/ng/selector/css_parser_context.h"
#include "core/renderer/css/ng/selector/css_selector_parser.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/component_element.h"
#include "core/renderer/dom/fiber/for_element.h"
#include "core/renderer/dom/fiber/if_element.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/dom/fiber/none_element.h"
#include "core/renderer/dom/fiber/page_element.h"
#include "core/renderer/dom/fiber/raw_text_element.h"
#include "core/renderer/dom/fiber/scroll_element.h"
#include "core/renderer/dom/fiber/template_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/tree_resolver.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/fiber/wrapper_element.h"
#include "core/renderer/dom/list_component_info.h"
#include "core/renderer/dom/testing/fiber_element_test.h"
#include "core/renderer/dom/testing/fiber_mock_painting_context.h"
#include "core/renderer/simple_styling/style_object.h"
#include "core/renderer/starlight/types/layout_attribute.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/renderer/ui_wrapper/common/testing/prop_bundle_mock.h"
#include "core/renderer/utils/test/text_utils_mock.h"
#include "core/runtime/lepus/bindings/renderer_functions.h"
#include "core/runtime/lepus/bytecode_generator.h"
#include "core/runtime/lepus/js_object.h"
#include "core/runtime/lepusng/jsvalue_helper.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/services/event_report/event_tracker.h"
#include "core/shell/lynx_ui_operation_queue.h"
#include "core/shell/runtime/mts/mts_runtime.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/shared_css_fragment.h"
#include "core/template_bundle/template_codec/generator/ttml_holder.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

namespace {
CSSValue parseTransformStringValue(const lepus::Value& value_str,
                                   const CSSParserConfigs& configs) {
  CSSStringParser parser = CSSStringParser::FromLepusString(value_str, configs);
  return parser.ParseTransform();
}

style::DynamicStyleObjectRef MakeDynamicStyleObjectRef(StyleMap style_map) {
  if (style_map.empty()) {
    return nullptr;
  }
  return style::DynamicStyleObjectRef(
      new style::DynamicStyleObject(std::move(style_map)));
}

bool StyleMapHasValue(const StyleMap& style_map, CSSPropertyID id,
                      const CSSValue& value) {
  auto it = style_map.find(id);
  return it != style_map.end() && it->second == value;
}

bool LayoutBundleHasStyle(const std::unique_ptr<LayoutBundle>& layout_bundle,
                          CSSPropertyID id, const CSSValue& value) {
  if (layout_bundle == nullptr) {
    return false;
  }
  for (const auto& [style_id, style_value] : layout_bundle->styles) {
    if (style_id == id && style_value == value) {
      return true;
    }
  }
  return false;
}

bool LayoutBundleHasResetStyle(
    const std::unique_ptr<LayoutBundle>& layout_bundle, CSSPropertyID id) {
  if (layout_bundle == nullptr) {
    return false;
  }
  for (const auto reset_id : layout_bundle->reset_styles) {
    if (reset_id == id) {
      return true;
    }
  }
  return false;
}

const lepus::Value* DatasetValue(const FiberElement* element,
                                 const base::String& key) {
  auto it = element->data_model_->dataset().find(key);
  if (it == element->data_model_->dataset().end()) {
    return nullptr;
  }
  return &it->second;
}
}  // namespace

static std::unordered_map<std::string, uint32_t> kTestColorMap = {
    {"red", 4294901760},
    {"green", 4278222848},
    {"blue", 4278190335},
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

TEST_P(FiberElementTest, DynamicSimpleStyleLayer_KVOrderingAndNilRestoresBase) {
  manager->SetEnableSimpleStyle(true);
  manager->config_->SetEnablePropertyBasedSimpleStyle(true);
  manager->SetConfig(manager->config_);
  tasm->page_config_ = manager->config_;

  auto page = manager->CreateFiberPage("0", 0);
  manager->SetFiberPageElement(page);
  auto view = manager->CreateFiberView();
  page->InsertNode(view);

  const std::string z_index_key =
      CSSProperty::GetPropertyNameCStr(CSSPropertyID::kPropertyIDZIndex);
  const std::string opacity_key =
      CSSProperty::GetPropertyNameCStr(CSSPropertyID::kPropertyIDOpacity);

  auto make_style_object_list = [](double z_index, double opacity) {
    StyleMap style_map;
    style_map.insert_or_assign(CSSPropertyID::kPropertyIDZIndex,
                               CSSValue(z_index, CSSValuePattern::NUMBER));
    style_map.insert_or_assign(CSSPropertyID::kPropertyIDOpacity,
                               CSSValue(opacity, CSSValuePattern::NUMBER));

    auto* style_object = new lynx::style::StyleObject(std::move(style_map));
    style_object->AddRef();

    auto list = lynx::style::CreateStyleObjectArray(2);
    list.get()[0] = style_object;
    list.get()[1] = nullptr;
    return list;
  };

  auto flush = [&]() {
    page->FlushActionsAsRoot();
    platform_impl_->Flush();
    ASSERT_TRUE(platform_impl_->node_map_.find(view->impl_id()) !=
                platform_impl_->node_map_.end());
  };

  auto props = [&]() -> std::map<std::string, lepus::Value>& {
    return platform_impl_->node_map_.at(view->impl_id())->props_;
  };

  view->SetStyleObjects(make_style_object_list(1, 0.5));
  flush();
  EXPECT_EQ(props()[z_index_key].Number(), 1);
  EXPECT_EQ(props()[opacity_key].Number(), 0.5);

  // KV before replace should be overwritten by the later full-replace object.
  view->AddDynamicSimpleStyleKV(CSSPropertyID::kPropertyIDZIndex,
                                CSSValue(8, CSSValuePattern::NUMBER));
  StyleMap dynamic_styles;
  dynamic_styles.insert_or_assign(CSSPropertyID::kPropertyIDOpacity,
                                  CSSValue(0.7, CSSValuePattern::NUMBER));
  view->ReplaceDynamicSimpleStyles(
      MakeDynamicStyleObjectRef(std::move(dynamic_styles)));
  flush();
  EXPECT_EQ(props()[z_index_key].Number(), 1);
  EXPECT_NEAR(props()[opacity_key].Number(), 0.7, 1e-6);

  // Replace then KV in the same flush should append onto the pending replace.
  dynamic_styles.clear();
  dynamic_styles.insert_or_assign(CSSPropertyID::kPropertyIDOpacity,
                                  CSSValue(0.8, CSSValuePattern::NUMBER));
  view->ReplaceDynamicSimpleStyles(
      MakeDynamicStyleObjectRef(std::move(dynamic_styles)));
  view->AddDynamicSimpleStyleKV(CSSPropertyID::kPropertyIDZIndex,
                                CSSValue(2, CSSValuePattern::NUMBER));
  flush();
  EXPECT_EQ(props()[z_index_key].Number(), 2);
  EXPECT_NEAR(props()[opacity_key].Number(), 0.8, 1e-6);

  view->AddDynamicSimpleStyleKV(CSSPropertyID::kPropertyIDZIndex, CSSValue());
  flush();
  EXPECT_EQ(props()[z_index_key].Number(), 1);
  EXPECT_NEAR(props()[opacity_key].Number(), 0.8, 1e-6);
}

TEST_P(FiberElementTest,
       DynamicSimpleStyleLayer_CloneResolvedPropsRebuildsDynamicSourceOnWrite) {
  manager->SetEnableSimpleStyle(true);
  manager->config_->SetEnablePropertyBasedSimpleStyle(true);
  manager->SetConfig(manager->config_);
  tasm->page_config_ = manager->config_;

  auto page = manager->CreateFiberPage("0", 0);
  manager->SetFiberPageElement(page);
  auto view = manager->CreateFiberView();
  page->InsertNode(view);

  auto flush = [&]() {
    page->FlushActionsAsRoot();
    platform_impl_->Flush();
  };

  view->AddDynamicSimpleStyleKV(CSSPropertyID::kPropertyIDZIndex,
                                CSSValue(2, CSSValuePattern::NUMBER));
  flush();

  ASSERT_TRUE(view->parsed_dynamic_styles_map_.has_value());
  ASSERT_FALSE(view->parsed_dynamic_styles_map_->empty());

  auto cloned = view->CloneElement(true);
  ASSERT_TRUE(cloned);
  EXPECT_FALSE(cloned->dynamic_simple_object_);
  ASSERT_TRUE(cloned->parsed_dynamic_styles_map_.has_value());
  EXPECT_EQ(cloned->parsed_dynamic_styles_map_->size(), 1u);

  cloned->AddDynamicSimpleStyleKV(CSSPropertyID::kPropertyIDOpacity,
                                  CSSValue(0.5, CSSValuePattern::NUMBER));
  ASSERT_TRUE(cloned->dynamic_simple_object_);
  const auto& cloned_dynamic = cloned->dynamic_simple_object_->Properties();
  auto z_index_it = cloned_dynamic.find(CSSPropertyID::kPropertyIDZIndex);
  ASSERT_TRUE(z_index_it != cloned_dynamic.end());
  EXPECT_EQ(z_index_it->second.GetNumber(), 2);
  auto opacity_it = cloned_dynamic.find(CSSPropertyID::kPropertyIDOpacity);
  ASSERT_TRUE(opacity_it != cloned_dynamic.end());
  EXPECT_NEAR(opacity_it->second.GetNumber(), 0.5, 1e-6);
}

TEST_P(
    FiberElementTest,
    RendererFunctions_SetDynamicStyleObject_AcceptsMapStyleObjectAndStringResetCarrier) {
  manager->SetEnableSimpleStyle(true);
  manager->config_->SetEnablePropertyBasedSimpleStyle(true);
  manager->SetConfig(manager->config_);
  tasm->page_config_ = manager->config_;

  auto lepus_ctx = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  ASSERT_TRUE(lepus_ctx);
  lepus_ctx->Initialize();
  lepus_ctx->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  auto* mts_ctx = runtime::MTSRuntime::ToQuickContext(lepus_ctx.get());
  ASSERT_TRUE(mts_ctx);

  auto page = manager->CreateFiberPage("0", 0);
  manager->SetFiberPageElement(page);
  auto view = manager->CreateFiberView();
  page->InsertNode(view);

  const std::string z_index_key =
      CSSProperty::GetPropertyNameCStr(CSSPropertyID::kPropertyIDZIndex);
  const std::string opacity_key =
      CSSProperty::GetPropertyNameCStr(CSSPropertyID::kPropertyIDOpacity);

  auto make_style_object_list = [](double z_index, double opacity) {
    StyleMap style_map;
    style_map.insert_or_assign(CSSPropertyID::kPropertyIDZIndex,
                               CSSValue(z_index, CSSValuePattern::NUMBER));
    style_map.insert_or_assign(CSSPropertyID::kPropertyIDOpacity,
                               CSSValue(opacity, CSSValuePattern::NUMBER));

    auto* style_object = new lynx::style::StyleObject(std::move(style_map));
    style_object->AddRef();

    auto list = lynx::style::CreateStyleObjectArray(2);
    list.get()[0] = style_object;
    list.get()[1] = nullptr;
    return list;
  };

  auto flush = [&]() {
    page->FlushActionsAsRoot();
    platform_impl_->Flush();
    ASSERT_TRUE(platform_impl_->node_map_.find(view->impl_id()) !=
                platform_impl_->node_map_.end());
  };

  auto props = [&]() -> std::map<std::string, lepus::Value>& {
    return platform_impl_->node_map_.at(view->impl_id())->props_;
  };

  auto expect_dynamic_style = [&](CSSPropertyID id, bool is_empty) {
    ASSERT_TRUE(view->parsed_dynamic_styles_map_.has_value());
    auto it = view->parsed_dynamic_styles_map_->find(id);
    ASSERT_TRUE(it != view->parsed_dynamic_styles_map_->end())
        << "missing dynamic style id: " << static_cast<int>(id);
    EXPECT_EQ(it->second.IsEmpty(), is_empty)
        << "unexpected empty state for id: " << static_cast<int>(id);
  };

  view->SetStyleObjects(make_style_object_list(1, 0.5));
  flush();

  const std::string z_index_id =
      std::to_string(static_cast<int32_t>(CSSPropertyID::kPropertyIDZIndex));
  const std::string opacity_id =
      std::to_string(static_cast<int32_t>(CSSPropertyID::kPropertyIDOpacity));

  {
    auto js_null = MK_JS_LEPUS_VALUE(mts_ctx->context(), LEPUS_NULL);
    auto dynamic_map = lepus::LEPUSValueHelper::CreateObject(nullptr);
    dynamic_map.SetProperty(z_index_id.c_str(), std::move(js_null));
    dynamic_map.SetProperty(opacity_id.c_str(), lepus::Value(0.8));

    lepus::Value argv[] = {lepus::Value(view), std::move(dynamic_map)};
    ::lynx::tasm::RendererFunctions::SetDynamicStyleObject(
        mts_ctx, argv, static_cast<int>(std::size(argv)));
    flush();

    EXPECT_EQ(props()[z_index_key].Number(), 1);
    EXPECT_NEAR(props()[opacity_key].Number(), 0.8, 1e-6);
    expect_dynamic_style(CSSPropertyID::kPropertyIDZIndex, true);
    expect_dynamic_style(CSSPropertyID::kPropertyIDOpacity, false);
  }

  {
    auto js_null = MK_JS_LEPUS_VALUE(mts_ctx->context(), LEPUS_NULL);
    auto dynamic_map = lepus::LEPUSValueHelper::CreateObject(nullptr);
    dynamic_map.SetProperty(z_index_id.c_str(), std::move(js_null));
    dynamic_map.SetProperty(opacity_id.c_str(), lepus::Value(0.9));

    lepus::Value create_argv[] = {std::move(dynamic_map)};
    auto dynamic_style_object =
        ::lynx::tasm::RendererFunctions::CreateStyleObject(
            mts_ctx, create_argv, static_cast<int>(std::size(create_argv)));

    lepus::Value argv[] = {lepus::Value(view), std::move(dynamic_style_object)};
    ::lynx::tasm::RendererFunctions::SetDynamicStyleObject(
        mts_ctx, argv, static_cast<int>(std::size(argv)));
    flush();

    EXPECT_EQ(props()[z_index_key].Number(), 1);
    EXPECT_NEAR(props()[opacity_key].Number(), 0.9, 1e-6);
    expect_dynamic_style(CSSPropertyID::kPropertyIDZIndex, true);
    expect_dynamic_style(CSSPropertyID::kPropertyIDOpacity, false);
  }

  {
    lepus::Value argv[] = {
        lepus::Value(view),
        lepus::Value(
            base::String("--bg-color: red; font-family: test-font; z-index: 4; "
                         "margin: 1px 2px;"))};
    ::lynx::tasm::RendererFunctions::SetDynamicStyleObject(
        mts_ctx, argv, static_cast<int>(std::size(argv)));
    flush();

    EXPECT_EQ(props()[z_index_key].Number(), 4);
    EXPECT_NEAR(props()[opacity_key].Number(), 0.5, 1e-6);
    ASSERT_TRUE(view->parsed_dynamic_styles_map_.has_value());
    EXPECT_EQ(view->parsed_dynamic_styles_map_->size(), 6u);
    EXPECT_FALSE(view->parsed_dynamic_styles_map_->contains(
        CSSPropertyID::kPropertyIDOpacity));
    auto font_family_it = view->parsed_dynamic_styles_map_->find(
        CSSPropertyID::kPropertyIDFontFamily);
    ASSERT_TRUE(font_family_it != view->parsed_dynamic_styles_map_->end());
    EXPECT_TRUE(font_family_it->second.IsString());
    EXPECT_EQ(font_family_it->second.AsString().str(), "test-font");
    expect_dynamic_style(CSSPropertyID::kPropertyIDZIndex, false);
    EXPECT_TRUE(view->parsed_dynamic_styles_map_->contains(
        CSSPropertyID::kPropertyIDMarginTop));
    EXPECT_TRUE(view->parsed_dynamic_styles_map_->contains(
        CSSPropertyID::kPropertyIDMarginRight));
    EXPECT_TRUE(view->parsed_dynamic_styles_map_->contains(
        CSSPropertyID::kPropertyIDMarginBottom));
    EXPECT_TRUE(view->parsed_dynamic_styles_map_->contains(
        CSSPropertyID::kPropertyIDMarginLeft));
  }

  {
    lepus::Value argv[] = {lepus::Value(view), lepus::Value(base::String())};
    ::lynx::tasm::RendererFunctions::SetDynamicStyleObject(
        mts_ctx, argv, static_cast<int>(std::size(argv)));
    flush();

    EXPECT_EQ(props()[z_index_key].Number(), 1);
    EXPECT_NEAR(props()[opacity_key].Number(), 0.5, 1e-6);
    EXPECT_FALSE(view->parsed_dynamic_styles_map_.has_value());
  }
}

TEST_P(FiberElementTest, DynamicSimpleStyleLayer_BehaviorMatrix) {
  manager->SetEnableSimpleStyle(true);
  manager->config_->SetEnablePropertyBasedSimpleStyle(true);
  manager->SetConfig(manager->config_);
  tasm->page_config_ = manager->config_;

  auto page = manager->CreateFiberPage("0", 0);
  manager->SetFiberPageElement(page);
  auto view = manager->CreateFiberView();
  page->InsertNode(view);

  const std::string z_index_key =
      CSSProperty::GetPropertyNameCStr(CSSPropertyID::kPropertyIDZIndex);
  const std::string opacity_key =
      CSSProperty::GetPropertyNameCStr(CSSPropertyID::kPropertyIDOpacity);

  auto make_style_object_list = [](double z_index, double opacity) {
    StyleMap style_map;
    style_map.insert_or_assign(CSSPropertyID::kPropertyIDZIndex,
                               CSSValue(z_index, CSSValuePattern::NUMBER));
    style_map.insert_or_assign(CSSPropertyID::kPropertyIDOpacity,
                               CSSValue(opacity, CSSValuePattern::NUMBER));

    auto* style_object = new lynx::style::StyleObject(std::move(style_map));
    style_object->AddRef();

    auto list = lynx::style::CreateStyleObjectArray(2);
    list.get()[0] = style_object;
    list.get()[1] = nullptr;
    return list;
  };

  auto flush = [&]() {
    page->FlushActionsAsRoot();
    platform_impl_->Flush();
    ASSERT_TRUE(platform_impl_->node_map_.find(view->impl_id()) !=
                platform_impl_->node_map_.end());
  };

  auto props = [&]() -> std::map<std::string, lepus::Value>& {
    return platform_impl_->node_map_.at(view->impl_id())->props_;
  };

  auto expect_number = [&](const std::string& key, double expected) {
    auto it = props().find(key);
    ASSERT_TRUE(it != props().end()) << "missing prop: " << key;
    ASSERT_FALSE(it->second.IsEmpty()) << "prop is empty: " << key;
    EXPECT_NEAR(it->second.Number(), expected, 1e-6) << "prop: " << key;
  };

  auto expect_default = [&](const std::string& key) {
    auto it = props().find(key);
    if (it == props().end()) {
      return;
    }
    EXPECT_TRUE(it->second.IsEmpty()) << "prop not reset: " << key;
  };

  auto set_dynamic_map = [&](bool set_z, double z, bool set_opacity,
                             double op) {
    StyleMap style_map;
    if (set_z) {
      style_map.insert_or_assign(CSSPropertyID::kPropertyIDZIndex,
                                 CSSValue(z, CSSValuePattern::NUMBER));
    }
    if (set_opacity) {
      style_map.insert_or_assign(CSSPropertyID::kPropertyIDOpacity,
                                 CSSValue(op, CSSValuePattern::NUMBER));
    }
    view->ReplaceDynamicSimpleStyles(
        MakeDynamicStyleObjectRef(std::move(style_map)));
  };

  // A) only static: set -> update -> clear.
  {
    SCOPED_TRACE("A_only_static_set_update_clear");
    view->SetStyleObjects(make_style_object_list(1, 0.5));
    flush();
    expect_number(z_index_key, 1);
    expect_number(opacity_key, 0.5);

    view->SetStyleObjects(make_style_object_list(3, 0.6));
    flush();
    expect_number(z_index_key, 3);
    expect_number(opacity_key, 0.6);

    view->SetStyleObjects(nullptr);
    flush();
    expect_default(z_index_key);
    expect_default(opacity_key);
  }

  // B) only dynamic: set -> replace (partial removal) -> clear.
  {
    SCOPED_TRACE("B_only_dynamic_set_replace_clear");
    set_dynamic_map(true, 2, true, 0.7);
    flush();
    expect_number(z_index_key, 2);
    expect_number(opacity_key, 0.7);

    // Replace to {z-index: 4} => opacity removed -> reset to default.
    set_dynamic_map(true, 4, false, 0);
    flush();
    expect_number(z_index_key, 4);
    expect_default(opacity_key);

    // Clear => all default.
    view->ReplaceDynamicSimpleStyles(nullptr);
    flush();
    expect_default(z_index_key);
    expect_default(opacity_key);
  }

  // C) static + dynamic: override; dynamic partial removal restores base;
  //    static update won't override dynamic; clear dynamic restores base.
  {
    SCOPED_TRACE("C_static_and_dynamic_override_update_restore");
    view->SetStyleObjects(make_style_object_list(1, 0.5));
    set_dynamic_map(true, 2, true, 0.7);
    flush();
    expect_number(z_index_key, 2);
    expect_number(opacity_key, 0.7);

    // Dynamic replace to {z-index: 3} => opacity removed -> restore base 0.5.
    set_dynamic_map(true, 3, false, 0);
    flush();
    expect_number(z_index_key, 3);
    expect_number(opacity_key, 0.5);

    // Static update => z-index still dynamic; opacity follows new base.
    view->SetStyleObjects(make_style_object_list(4, 0.6));
    flush();
    expect_number(z_index_key, 3);
    expect_number(opacity_key, 0.6);

    // Update static and dynamic in the same flush: dynamic wins for overlap.
    view->SetStyleObjects(make_style_object_list(6, 0.9));
    view->AddDynamicSimpleStyleKV(CSSPropertyID::kPropertyIDZIndex,
                                  CSSValue(7, CSSValuePattern::NUMBER));
    flush();
    expect_number(z_index_key, 7);
    expect_number(opacity_key, 0.9);

    // Clear dynamic => restore base.
    view->ReplaceDynamicSimpleStyles(nullptr);
    flush();
    expect_number(z_index_key, 6);
    expect_number(opacity_key, 0.9);
  }

  // D) static removed then dynamic cleared should not restore stale base.
  {
    SCOPED_TRACE("D_static_removed_then_dynamic_clear_no_stale_restore");
    view->SetStyleObjects(make_style_object_list(10, 0.4));
    flush();
    expect_number(z_index_key, 10);

    view->SetStyleObjects(nullptr);
    flush();
    expect_default(z_index_key);
    expect_default(opacity_key);

    view->AddDynamicSimpleStyleKV(CSSPropertyID::kPropertyIDZIndex,
                                  CSSValue(11, CSSValuePattern::NUMBER));
    flush();
    expect_number(z_index_key, 11);

    view->ReplaceDynamicSimpleStyles(nullptr);
    flush();
    expect_default(z_index_key);
    expect_default(opacity_key);
  }
}

TEST_P(FiberElementTest, RendererFunctions_SetStyleObject_IdsStringOrder) {
  manager->SetEnableSimpleStyle(true);
  manager->config_->SetEnablePropertyBasedSimpleStyle(true);
  manager->SetConfig(manager->config_);
  tasm->page_config_ = manager->config_;

  // Prepare global style object list used by SetStyleObject(ids_string).
  auto entry = tasm->FindEntry(DEFAULT_ENTRY_NAME);
  constexpr size_t kStyleObjectListSize = 8;
  auto& list_ref =
      entry->template_bundle().InitStyleObjectList(kStyleObjectListSize);
  auto* global_style_objects = list_ref.get();

  // InitStyleObjectList does not guarantee zero-initialized entries.
  // Only clear within the declared allocation size to avoid OOB writes.
  for (size_t i = 0; i < kStyleObjectListSize; ++i) {
    global_style_objects[i] = nullptr;
  }

  auto make_style_object = [](CSSPropertyID id, double number) {
    StyleMap style_map;
    style_map.insert_or_assign(id, CSSValue(number, CSSValuePattern::NUMBER));
    auto* obj = new lynx::style::StyleObject(std::move(style_map));
    obj->AddRef();
    return obj;
  };
  // id=1 => z-index: 1, id=2 => z-index: 2
  global_style_objects[1] =
      make_style_object(CSSPropertyID::kPropertyIDZIndex, 1);
  global_style_objects[2] =
      make_style_object(CSSPropertyID::kPropertyIDZIndex, 2);

  // Create a lepus context whose delegate is our TemplateAssembler.
  auto lepus_ctx = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  ASSERT_TRUE(lepus_ctx);
  lepus_ctx->Initialize();
  lepus_ctx->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  auto* mts_ctx = runtime::MTSRuntime::ToQuickContext(lepus_ctx.get());
  ASSERT_TRUE(mts_ctx);

  auto page = manager->CreateFiberPage("0", 0);
  manager->SetFiberPageElement(page);
  auto view = manager->CreateFiberView();
  page->InsertNode(view);

  const std::string z_index_key =
      CSSProperty::GetPropertyNameCStr(CSSPropertyID::kPropertyIDZIndex);

  auto flush = [&]() {
    page->FlushActionsAsRoot();
    platform_impl_->Flush();
    ASSERT_TRUE(platform_impl_->node_map_.find(view->impl_id()) !=
                platform_impl_->node_map_.end());
  };

  auto props = [&]() -> std::map<std::string, lepus::Value>& {
    return platform_impl_->node_map_.at(view->impl_id())->props_;
  };

  // "1 2" => later style object should override earlier.
  {
    lepus::Value argv[] = {lepus::Value(view), lepus::Value("1 2")};
    ::lynx::tasm::RendererFunctions::SetStyleObject(
        mts_ctx, argv, static_cast<int>(std::size(argv)));
    flush();
    EXPECT_EQ(props()[z_index_key].Number(), 2);
  }

  // "2 1" => override in reverse.
  {
    lepus::Value argv[] = {lepus::Value(view), lepus::Value("2 1")};
    ::lynx::tasm::RendererFunctions::SetStyleObject(
        mts_ctx, argv, static_cast<int>(std::size(argv)));
    flush();
    EXPECT_EQ(props()[z_index_key].Number(), 1);
  }

  // "" (empty string) => reset style objects (nullptr list).
  {
    auto before_it = props().find(z_index_key);
    ASSERT_TRUE(before_it != props().end());
    ASSERT_FALSE(before_it->second.IsEmpty());

    lepus::Value argv[] = {lepus::Value(view), lepus::Value("")};
    ::lynx::tasm::RendererFunctions::SetStyleObject(
        mts_ctx, argv, static_cast<int>(std::size(argv)));
    flush();
    auto it = props().find(z_index_key);
    if (it != props().end()) {
      EXPECT_TRUE(it->second.IsEmpty());
    }
  }
}

TEST_P(
    FiberElementTest,
    RendererFunctions_SetDynamicStyleObjectKV_ShorthandNilKeepsExpandedLonghandsAsEmpty) {
  manager->SetEnableSimpleStyle(true);
  manager->config_->SetEnablePropertyBasedSimpleStyle(true);
  manager->SetConfig(manager->config_);
  tasm->page_config_ = manager->config_;

  auto lepus_ctx = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  ASSERT_TRUE(lepus_ctx);
  lepus_ctx->Initialize();
  lepus_ctx->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  auto* mts_ctx = runtime::MTSRuntime::ToQuickContext(lepus_ctx.get());
  ASSERT_TRUE(mts_ctx);

  auto page = manager->CreateFiberPage("0", 0);
  manager->SetFiberPageElement(page);
  auto view = manager->CreateFiberView();
  page->InsertNode(view);

  auto flush = [&]() {
    page->FlushActionsAsRoot();
    platform_impl_->Flush();
    ASSERT_TRUE(platform_impl_->node_map_.find(view->impl_id()) !=
                platform_impl_->node_map_.end());
  };

  auto expect_dynamic_style = [&](CSSPropertyID id, bool is_empty) {
    ASSERT_TRUE(view->parsed_dynamic_styles_map_.has_value());
    auto it = view->parsed_dynamic_styles_map_->find(id);
    ASSERT_TRUE(it != view->parsed_dynamic_styles_map_->end())
        << "missing dynamic style id: " << static_cast<int>(id);
    EXPECT_EQ(it->second.IsEmpty(), is_empty)
        << "unexpected empty state for id: " << static_cast<int>(id);
  };
  {
    view->AddDynamicSimpleStyleKV(CSSPropertyID::kPropertyIDZIndex,
                                  CSSValue(9, CSSValuePattern::NUMBER));
    lepus::Value argv[] = {
        lepus::Value(view),
        lepus::Value(static_cast<int32_t>(CSSPropertyID::kPropertyIDMargin)),
        lepus::Value("1px 2px 3px 4px")};
    ::lynx::tasm::RendererFunctions::SetDynamicStyleObjectKV(
        mts_ctx, argv, static_cast<int>(std::size(argv)));
    flush();
    expect_dynamic_style(CSSPropertyID::kPropertyIDMarginTop, false);
    expect_dynamic_style(CSSPropertyID::kPropertyIDMarginRight, false);
    expect_dynamic_style(CSSPropertyID::kPropertyIDMarginBottom, false);
    expect_dynamic_style(CSSPropertyID::kPropertyIDMarginLeft, false);
  }

  {
    auto js_null = MK_JS_LEPUS_VALUE(mts_ctx->context(), LEPUS_NULL);
    lepus::Value argv[] = {
        lepus::Value(view),
        lepus::Value(static_cast<int32_t>(CSSPropertyID::kPropertyIDMargin)),
        std::move(js_null)};
    ::lynx::tasm::RendererFunctions::SetDynamicStyleObjectKV(
        mts_ctx, argv, static_cast<int>(std::size(argv)));
    flush();
    expect_dynamic_style(CSSPropertyID::kPropertyIDZIndex, false);
    expect_dynamic_style(CSSPropertyID::kPropertyIDMarginTop, true);
    expect_dynamic_style(CSSPropertyID::kPropertyIDMarginRight, true);
    expect_dynamic_style(CSSPropertyID::kPropertyIDMarginBottom, true);
    expect_dynamic_style(CSSPropertyID::kPropertyIDMarginLeft, true);
  }
}

TEST_P(FiberElementTest,
       RendererFunctions_SetDynamicStyleObjectKV_InvalidIdIsIgnored) {
  manager->SetEnableSimpleStyle(true);
  manager->config_->SetEnablePropertyBasedSimpleStyle(true);
  manager->SetConfig(manager->config_);
  tasm->page_config_ = manager->config_;

  auto lepus_ctx = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  ASSERT_TRUE(lepus_ctx);
  lepus_ctx->Initialize();
  lepus_ctx->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  auto* mts_ctx = runtime::MTSRuntime::ToQuickContext(lepus_ctx.get());
  ASSERT_TRUE(mts_ctx);

  auto page = manager->CreateFiberPage("0", 0);
  manager->SetFiberPageElement(page);
  auto view = manager->CreateFiberView();
  page->InsertNode(view);

  auto flush = [&]() {
    page->FlushActionsAsRoot();
    platform_impl_->Flush();
    ASSERT_TRUE(platform_impl_->node_map_.find(view->impl_id()) !=
                platform_impl_->node_map_.end());
  };

  view->AddDynamicSimpleStyleKV(CSSPropertyID::kPropertyIDOpacity,
                                CSSValue(0.8, CSSValuePattern::NUMBER));
  flush();

  ASSERT_TRUE(view->parsed_dynamic_styles_map_.has_value());
  EXPECT_EQ(view->parsed_dynamic_styles_map_->size(), 1u);
  EXPECT_TRUE(view->parsed_dynamic_styles_map_->contains(
      CSSPropertyID::kPropertyIDOpacity));

  auto js_null = MK_JS_LEPUS_VALUE(mts_ctx->context(), LEPUS_NULL);
  lepus::Value argv[] = {
      lepus::Value(view),
      lepus::Value(static_cast<int32_t>(CSSPropertyID::kPropertyEnd)),
      std::move(js_null)};
  ::lynx::tasm::RendererFunctions::SetDynamicStyleObjectKV(
      mts_ctx, argv, static_cast<int>(std::size(argv)));
  flush();

  ASSERT_TRUE(view->parsed_dynamic_styles_map_.has_value());
  EXPECT_EQ(view->parsed_dynamic_styles_map_->size(), 1u);
  auto it =
      view->parsed_dynamic_styles_map_->find(CSSPropertyID::kPropertyIDOpacity);
  ASSERT_TRUE(it != view->parsed_dynamic_styles_map_->end());
  EXPECT_DOUBLE_EQ(it->second.GetNumber(), 0.8);
}

TEST_P(FiberElementTest,
       DynamicSimpleStyleLayer_LonghandKVNilKeepsEmptyResetInMap) {
  manager->SetEnableSimpleStyle(true);
  manager->config_->SetEnablePropertyBasedSimpleStyle(true);
  manager->SetConfig(manager->config_);
  tasm->page_config_ = manager->config_;

  auto page = manager->CreateFiberPage("0", 0);
  manager->SetFiberPageElement(page);
  auto view = manager->CreateFiberView();
  page->InsertNode(view);

  view->AddDynamicSimpleStyleKV(CSSPropertyID::kPropertyIDZIndex,
                                CSSValue(2, CSSValuePattern::NUMBER));
  page->FlushActionsAsRoot();
  platform_impl_->Flush();

  ASSERT_TRUE(view->parsed_dynamic_styles_map_.has_value());
  auto it =
      view->parsed_dynamic_styles_map_->find(CSSPropertyID::kPropertyIDZIndex);
  ASSERT_TRUE(it != view->parsed_dynamic_styles_map_->end());
  EXPECT_FALSE(it->second.IsEmpty());

  view->AddDynamicSimpleStyleKV(CSSPropertyID::kPropertyIDOpacity,
                                CSSValue(0.8, CSSValuePattern::NUMBER));

  view->AddDynamicSimpleStyleKV(CSSPropertyID::kPropertyIDZIndex, CSSValue());
  page->FlushActionsAsRoot();
  platform_impl_->Flush();

  ASSERT_TRUE(view->parsed_dynamic_styles_map_.has_value());
  it = view->parsed_dynamic_styles_map_->find(CSSPropertyID::kPropertyIDZIndex);
  ASSERT_TRUE(it != view->parsed_dynamic_styles_map_->end());
  EXPECT_TRUE(it->second.IsEmpty());
  auto opacity_it =
      view->parsed_dynamic_styles_map_->find(CSSPropertyID::kPropertyIDOpacity);
  ASSERT_TRUE(opacity_it != view->parsed_dynamic_styles_map_->end());
  EXPECT_FALSE(opacity_it->second.IsEmpty());
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

TEST_P(FiberElementTest,
       TestUpdateDynamicElementStyleFilterBlurVwWithEnvGuard) {
  manager->UpdateViewport(100, SLMeasureModeDefinite, 600,
                          SLMeasureModeDefinite, false);

  auto page = manager->CreateFiberPage("page", 11);
  manager->fix_filter_dynamic_update_bug_ = false;
  auto element_without_fix = manager->CreateFiberView();
  element_without_fix->SetStyle(CSSPropertyID::kPropertyIDFilter,
                                lepus::Value("blur(10vw)"));
  page->InsertNode(element_without_fix);
  page->FlushActionsAsRoot();

  auto* painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->platform_impl_.get());
  painting_context->Flush();

  auto* node_without_fix =
      painting_context->node_map_.at(element_without_fix->impl_id()).get();
  ASSERT_TRUE(node_without_fix->props_.find("filter") !=
              node_without_fix->props_.end());
  ASSERT_TRUE(node_without_fix->props_["filter"].IsArray());

  auto filter_without_fix = node_without_fix->props_["filter"].Array();
  ASSERT_EQ(filter_without_fix->size(), static_cast<size_t>(3));
  EXPECT_EQ(filter_without_fix->get(1).Number(), 10);

  manager->UpdateViewport(200, SLMeasureModeDefinite, 600,
                          SLMeasureModeDefinite, false);
  painting_context->Flush();

  auto updated_filter_without_fix = node_without_fix->props_["filter"].Array();
  ASSERT_EQ(updated_filter_without_fix->size(), static_cast<size_t>(3));
  EXPECT_EQ(updated_filter_without_fix->get(1).Number(), 10);

  manager->fix_filter_dynamic_update_bug_ = true;
  manager->UpdateViewport(100, SLMeasureModeDefinite, 600,
                          SLMeasureModeDefinite, false);

  auto element_with_fix = manager->CreateFiberView();
  element_with_fix->SetStyle(CSSPropertyID::kPropertyIDFilter,
                             lepus::Value("blur(10vw)"));
  page->InsertNode(element_with_fix);
  page->FlushActionsAsRoot();
  painting_context->Flush();

  auto* node_with_fix =
      painting_context->node_map_.at(element_with_fix->impl_id()).get();
  ASSERT_TRUE(node_with_fix->props_.find("filter") !=
              node_with_fix->props_.end());
  ASSERT_TRUE(node_with_fix->props_["filter"].IsArray());

  auto filter_with_fix = node_with_fix->props_["filter"].Array();
  ASSERT_EQ(filter_with_fix->size(), static_cast<size_t>(3));
  EXPECT_EQ(filter_with_fix->get(1).Number(), 10);

  manager->UpdateViewport(200, SLMeasureModeDefinite, 600,
                          SLMeasureModeDefinite, false);
  painting_context->Flush();

  auto updated_filter_with_fix = node_with_fix->props_["filter"].Array();
  ASSERT_EQ(updated_filter_with_fix->size(), static_cast<size_t>(3));
  EXPECT_EQ(updated_filter_with_fix->get(1).Number(), 20);
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

// Test UpdateDynamicElementStyle with kUpdateViewport
// Self-owned style wouldn't be overriden by inherited value
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

// Test UpdateDynamicElementStyle with kUpdateFontScale
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

TEST_P(FiberElementTest,
       TestUpdateLayoutNodeAttributeWhenEnableLayoutInElement) {
  manager->page_options_.embedded_mode_ = static_cast<EmbeddedMode>(
      static_cast<int32_t>(manager->page_options_.embedded_mode_) |
      static_cast<int32_t>(EmbeddedMode::LAYOUT_IN_ELEMENT));
  manager->enable_native_list_ = true;
  tasm->layout_scheduler_ = std::make_unique<LayoutScheduler>(manager);
  manager->UpdateViewport(100, SLMeasureModeDefinite, 600,
                          SLMeasureModeDefinite, false);

  auto default_entry = std::make_shared<TemplateEntry>();
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = default_entry;

  auto ctx = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  tasm->template_entries_[DEFAULT_ENTRY_NAME]->SetVm(ctx);

  std::string js_source = R"(
    let count = 0;
    let callback = ()=> {
      count = count + 1;
    }
  )";

  lepus::BytecodeGenerator::GenerateBytecode(ctx->GetMTSContext(), js_source,
                                             ctx->GetSdkVersion(), "");
  ctx->Execute(nullptr);

  auto callback = ctx->GetGlobalData("callback");

  auto page = manager->CreateFiberPage("page", 11);

  auto scroll = manager->CreateFiberElement("scroll-view");
  scroll->SetAttribute("scroll-x", lepus::Value(true));
  page->InsertNode(scroll);

  auto list = manager->CreateFiberElement("list");
  list->SetAttribute("column-count", lepus::Value(2));
  list->SetAttribute("list-type", lepus::Value("waterfall"));
  static_cast<ListElement*>(list.get())
      ->UpdateCallbacks(callback, callback, callback);
  static_cast<ListElement*>(list.get())->tasm_ = tasm.get();

  list->SetStyle(CSSPropertyID::kPropertyIDWidth, lepus::Value("1000px"));
  list->SetStyle(CSSPropertyID::kPropertyIDHeight, lepus::Value("1000px"));

  lepus::Value single_action = lepus::LEPUSValueHelper::CreateObject(nullptr);
  single_action.SetProperty("item-key", lepus::Value("a"));
  single_action.SetProperty("position", lepus::Value(0));
  single_action.SetProperty("type", lepus::Value("__snapshot_a94a8_test_37"));

  lepus::Value insert_actions = lepus::LEPUSValueHelper::CreateArray(nullptr);
  insert_actions.SetProperty(0, single_action);

  lepus::Value single_info = lepus::LEPUSValueHelper::CreateObject(nullptr);
  single_info.SetProperty("insertAction", insert_actions);

  list->SetAttribute("update-list-info", single_info);
  page->InsertNode(list);

  auto list_item = manager->CreateFiberElement("list-item");
  list_item->SetAttribute("full-span", lepus::Value(true));
  list->InsertNode(list_item);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options);

  EXPECT_TRUE(*(scroll->sl_node_->attr_map().scroll_));
  EXPECT_EQ(*(list->sl_node_->attr_map().column_count_), 2);
  EXPECT_EQ(*(list_item->sl_node_->attr_map().list_comp_type_),
            static_cast<int>(ListComponentInfo::Type::LIST_ROW));

  auto count = ctx->GetGlobalData("count");
  EXPECT_EQ(count, lepus::Value(1));
}

TEST_P(FiberElementTest,
       LayoutInElement_InsertNonVirtualChildUnderVirtualParent) {
  manager->page_options_.embedded_mode_ = static_cast<EmbeddedMode>(
      static_cast<int32_t>(manager->page_options_.embedded_mode_) |
      static_cast<int32_t>(EmbeddedMode::LAYOUT_IN_ELEMENT));

  auto page = manager->CreateFiberPage("page", 11);
  auto virtual_parent = manager->CreateFiberNode("inline-text");
  page->InsertNode(virtual_parent);

  auto child = manager->CreateFiberNode("view");
  virtual_parent->InsertNode(child);

  page->FlushActionsAsRoot();

  EXPECT_TRUE(virtual_parent->is_virtual_);
  EXPECT_EQ(child->render_parent(), virtual_parent.get());
  EXPECT_TRUE(child->attached_to_layout_parent_);
  ASSERT_NE(page->slnode(), nullptr);
  ASSERT_NE(child->slnode(), nullptr);
  EXPECT_EQ(child->slnode()->parent(), page->slnode());
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

TEST_P(FiberElementTest,
       LayoutInElement_RemoveLayoutNodeResetsStateWithoutLayoutParent) {
  manager->page_options_.embedded_mode_ = static_cast<EmbeddedMode>(
      static_cast<int32_t>(manager->page_options_.embedded_mode_) |
      static_cast<int32_t>(EmbeddedMode::LAYOUT_IN_ELEMENT));

  auto page = manager->CreateFiberPage("page", 11);
  auto parent = manager->CreateFiberNode("view");
  page->InsertNode(parent);
  page->FlushActionsAsRoot();

  auto child = manager->CreateFiberNode("view");
  child->EnsureSLNode();
  child->attached_to_layout_parent_ = true;
  ASSERT_NE(child->slnode(), nullptr);
  EXPECT_EQ(child->slnode()->parent(), nullptr);

  parent->RemoveLayoutNode(child.get());
  EXPECT_FALSE(child->attached_to_layout_parent_);
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

// insert wrapper to wrapper
//                     [page]
//                        |
//                   [element0]
//                /       |       \
//      [wrapper00] [element01] [element02]
//            |
//       [wrapper000]
//           /       \
//   [element0000] [element0001]
//
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

// update inline style
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

// insert wrapper to wrapper
//                     [page]
//                        |
//                   [element0]
//                /       |       \
//      [element00] [wrapper00] [element01]
//                   /        \
//               [wrapper000] [wrapper001]
//                   |
//               [element0000]
//
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

// Note that kPropertyIDFontSize, kPropertyIDLineSpacing,
// kPropertyIDLetterSpacing, kPropertyIDLineHeight are complex props.
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

// fiber inherit case 1: normal inherit and delete styles
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

// fiber inherit case 2: reset style not in inherited styles
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

// fiber inherit case 3: inherited style updated
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

// fiber inherit case 4: inherited style updated for wrapper element
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

// fiber inherit case 5: update sub element with css inheritance
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

// fiber direction case 2: font-size and direction are both updated by
// inheritance
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

// Verify reset CSS Property need to consider current direction
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

// Verify correct text-align is propagated to extended text node
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

// position:fixed related
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

// position:fixed removed twice
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

// position: fixed replace element with new fixed enabled
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

// position: fixed double replace element with new fixed enabled
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

// insert before fixed
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

// insert before fixed
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

// insert before fixed
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

// insert before fixed
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

// flush element which parent is wrapper
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

TEST_P(FiberElementTest,
       ResolveSimpleStylesHandlesPendingKeyframeChangesFromStyleRemoval) {
  manager->SetEnableSimpleStyle(true);

  auto page = manager->CreateFiberPage("page", 11);
  auto element = manager->CreateFiberView();
  page->InsertNode(element);

  auto style_objects = style::CreateStyleObjectArray(2);
  auto style_map = tasm::StyleMap{};
  style_map.emplace(CSSPropertyID::kPropertyIDAnimationName,
                    CSSValue::MakePlainString("move"));
  auto* style_object = new style::StyleObject(std::move(style_map));
  style_object->AddRef();
  style_objects.get()[0] = style_object;
  style_objects.get()[1] = nullptr;

  element->SetStyleObjects(std::move(style_objects));
  page->FlushActionsAsRoot();
  EXPECT_FALSE(element->has_keyframe_props_changed_);

  element->SetStyleObjects(nullptr);
  page->FlushActionsAsRoot();
  EXPECT_FALSE(element->has_keyframe_props_changed_);
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

TEST_P(FiberElementTest,
       DynamicViewportUpdate_SkipTransitionConsume_FiberArch) {
  manager->UpdateViewport(100, SLMeasureModeDefinite, 600,
                          SLMeasureModeDefinite, false);

  auto page = manager->CreateFiberPage("page", 11);
  auto element = manager->CreateFiberView();
  element->SetStyle(CSSPropertyID::kPropertyIDWidth, lepus::Value("50vh"));

  page->InsertNode(element);
  page->FlushActionsAsRoot();

  element->css_transition_manager_ =
      std::make_unique<animation::CSSTransitionManager>(element.get());
  starlight::TransitionData transition_data;
  transition_data.properties.push_back(
      starlight::AnimationPropertyType::kWidth);
  element->css_transition_manager_->setTransitionData(transition_data);

  tasm_mediator.captured_ids_.clear();
  tasm_mediator.captured_bundles_.clear();
  manager->UpdateViewport(200, SLMeasureModeDefinite, 800,
                          SLMeasureModeDefinite, false);
  EXPECT_TRUE(HasCaptureSignWithStyleKeyAndValuePattern(
      element->impl_id(), CSSPropertyID::kPropertyIDWidth,
      CSSValue(0.f, CSSValuePattern::VH), 1));
}

TEST_P(FiberElementTest,
       DynamicViewportUpdate_SkipTransitionConsume_FiberArch_FlagOff) {
  manager->UpdateViewport(100, SLMeasureModeDefinite, 600,
                          SLMeasureModeDefinite, false);

  auto page = manager->CreateFiberPage("page", 11);
  auto element = manager->CreateFiberView();
  element->SetStyle(CSSPropertyID::kPropertyIDWidth, lepus::Value("50vh"));
  manager->fix_fiber_dynamic_update_transition_consume_bug_ = false;

  page->InsertNode(element);
  page->FlushActionsAsRoot();

  element->css_transition_manager_ =
      std::make_unique<animation::CSSTransitionManager>(element.get());
  starlight::TransitionData data;
  data.properties.emplace_back(starlight::AnimationPropertyType::kWidth);
  element->css_transition_manager_->setTransitionData(data);

  tasm_mediator.captured_ids_.clear();
  tasm_mediator.captured_bundles_.clear();
  manager->UpdateViewport(200, SLMeasureModeDefinite, 800,
                          SLMeasureModeDefinite, false);
  EXPECT_FALSE(HasCaptureSignWithStyleKeyAndValuePattern(
      element->impl_id(), CSSPropertyID::kPropertyIDWidth,
      CSSValue(0.f, CSSValuePattern::VH), 1));
}

TEST_P(FiberElementTest, TestFiberNewAnimatorWithDynamicStyleUpdate) {
  CSSParserConfigs configs;
  manager->enable_animation_forward_update_preservation_ = true;

  auto page = manager->CreateFiberPage("page", 11);
  auto element = manager->CreateFiberView();
  element->SetStyle(CSSPropertyID::kPropertyIDTransform,
                    lepus::Value("scale(0,0) translateX(100px)"));
  element->enable_new_animator_ = true;
  page->InsertNode(element);

  page->FlushActionsAsRoot();
  auto original_transform_value = element->GetComputedStyleByKey("transform");

  element->SetStyle(CSSPropertyID::kPropertyIDTransform,
                    lepus::Value("scale(1,1) translateX(100px)"));
  page->FlushActionsAsRoot();
  auto forwards_transform_value = element->GetComputedStyleByKey("transform");

  EXPECT_TRUE(original_transform_value != forwards_transform_value);
  element->SetStyle(CSSPropertyID::kPropertyIDTransform,
                    lepus::Value("scale(0,0) translateX(100px)"));
  page->FlushActionsAsRoot();

  auto forwards_state_css_value = parseTransformStringValue(
      lepus::Value("scale(1,1) translateX(100px)"), configs);
  tasm::StyleMap fill_styles;
  fill_styles.emplace_or_assign(CSSPropertyID::kPropertyIDTransform,
                                forwards_state_css_value);
  element->PersistAnimationFillStyles(fill_styles);
  element->final_animator_map_->emplace_or_assign(
      CSSPropertyID::kPropertyIDTransform, forwards_state_css_value);

  // Flush animated styles to computed style
  element->FlushAnimatedStyle();
  EXPECT_TRUE(element->GetComputedStyleByKey("transform") ==
              forwards_transform_value);
  EXPECT_TRUE((element->dynamic_style_flags_ &
               DynamicCSSStylesManager::kUpdateViewport) > 0);
  EXPECT_TRUE((element->dynamic_style_flags_ &
               DynamicCSSStylesManager::kUpdateScreenMetrics) > 0);

  manager->UpdateScreenMetrics(100.0f, 300.0f);
  EXPECT_TRUE(element->GetComputedStyleByKey("transform") ==
              forwards_transform_value);
  EXPECT_TRUE(element->GetComputedStyleByKey("transform") !=
              original_transform_value);
}

TEST_P(FiberElementTest,
       DynamicViewportUpdate_SkipTransitionConsume_RadonArch_FlagOff) {
  manager->UpdateViewport(100, SLMeasureModeDefinite, 600,
                          SLMeasureModeDefinite, false);

  auto page = manager->CreateFiberPage("page", 11);
  auto element = manager->CreateFiberView();
  element->SetStyle(CSSPropertyID::kPropertyIDWidth, lepus::Value("50vh"));
  element->arch_type_ = RadonArch;
  manager->fix_dynamic_update_transition_consume_bug_ = false;

  page->InsertNode(element);
  page->FlushActionsAsRoot();

  element->css_transition_manager_ =
      std::make_unique<animation::CSSTransitionManager>(element.get());
  starlight::TransitionData transition_data;
  transition_data.properties.push_back(
      starlight::AnimationPropertyType::kWidth);
  element->css_transition_manager_->setTransitionData(transition_data);

  tasm_mediator.captured_ids_.clear();
  tasm_mediator.captured_bundles_.clear();
  manager->UpdateViewport(200, SLMeasureModeDefinite, 800,
                          SLMeasureModeDefinite, false);
  EXPECT_FALSE(HasCaptureSignWithStyleKeyAndValuePattern(
      element->impl_id(), CSSPropertyID::kPropertyIDWidth,
      CSSValue(0.f, CSSValuePattern::VH), 1));
}

TEST_P(FiberElementTest,
       DynamicViewportUpdate_SkipTransitionConsume_RadonArch_FlagOn) {
  manager->UpdateViewport(100, SLMeasureModeDefinite, 600,
                          SLMeasureModeDefinite, false);

  auto page = manager->CreateFiberPage("page", 11);
  auto element = manager->CreateFiberView();
  element->SetStyle(CSSPropertyID::kPropertyIDWidth, lepus::Value("50vh"));
  element->arch_type_ = RadonArch;
  manager->fix_dynamic_update_transition_consume_bug_ = true;

  page->InsertNode(element);
  page->FlushActionsAsRoot();

  element->css_transition_manager_ =
      std::make_unique<animation::CSSTransitionManager>(element.get());
  starlight::TransitionData transition_data;
  transition_data.properties.push_back(
      starlight::AnimationPropertyType::kWidth);
  element->css_transition_manager_->setTransitionData(transition_data);

  tasm_mediator.captured_ids_.clear();
  tasm_mediator.captured_bundles_.clear();
  manager->UpdateViewport(200, SLMeasureModeDefinite, 800,
                          SLMeasureModeDefinite, false);
  EXPECT_TRUE(HasCaptureSignWithStyleKeyAndValuePattern(
      element->impl_id(), CSSPropertyID::kPropertyIDWidth,
      CSSValue(0.f, CSSValuePattern::VH), 1));
}

TEST_P(FiberElementTest,
       DynamicViewportUpdate_SkipTransitionConsume_NoNeedTransition) {
  manager->UpdateViewport(100, SLMeasureModeDefinite, 600,
                          SLMeasureModeDefinite, false);

  auto page = manager->CreateFiberPage("page", 11);
  auto element = manager->CreateFiberView();
  element->SetStyle(CSSPropertyID::kPropertyIDWidth, lepus::Value("50vh"));

  page->InsertNode(element);
  page->FlushActionsAsRoot();

  element->css_transition_manager_ =
      std::make_unique<animation::CSSTransitionManager>(element.get());
  starlight::TransitionData transition_data;
  transition_data.properties.push_back(
      starlight::AnimationPropertyType::kOpacity);
  element->css_transition_manager_->setTransitionData(transition_data);

  tasm_mediator.captured_ids_.clear();
  tasm_mediator.captured_bundles_.clear();
  manager->UpdateViewport(200, SLMeasureModeDefinite, 800,
                          SLMeasureModeDefinite, false);
  EXPECT_TRUE(HasCaptureSignWithStyleKeyAndValuePattern(
      element->impl_id(), CSSPropertyID::kPropertyIDWidth,
      CSSValue(0.f, CSSValuePattern::VH), 1));
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

// Simplifed CSSVariable Demo Structure:
//                      [view1 class="one" id="root"]
//                      /
// [view4 class="three" id="test1"]
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

// Simplifed CSSVariable Demo Structure:
//                      [view1 class="one" id="root"]
//                      /
// [view4 class="three" id="test1"]
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

// Simplifed CSSVariable Demo Structure:
//                      [view1 class="one" id="root"]
//                      /
// [view4 class="three" id="test1"]
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

// CSSVariable Demo Structure:
//
//                      [view1 class="one" id="root]
//                       /                       \
//              [view2 class="two" id="test"]   [view3 class="four"]
//                /                     \
// [view4 class="three" id="test1"]     [count component]
//                                            |
//                                    [view5 class="test"]
//                                            |
//                                   [text text="counter"]
//
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

/*
                     [Page]
                      /
                     /
                [Container]
                /
               /
          [Child1]
*/
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

TEST_P(FiberElementTest, SerializeTemplateElementRecursively) {
  auto child = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  child->SetTemplateKey(base::String("child_template"));
  child->SetBundleUrl(base::String("child_bundle.js"));

  auto child_attribute_slots = lepus::CArray::Create();
  child_attribute_slots->emplace_back(lepus::Value(true));
  child->SetAttributeSlots(lepus::Value(std::move(child_attribute_slots)));

  auto child_element_slots = lepus::CArray::Create();
  child_element_slots->emplace_back(lepus::Value(lepus::CArray::Create()));
  child->SetElementSlots(lepus::Value(std::move(child_element_slots)));

  auto root = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  root->SetTemplateKey(base::String("root_template"));
  root->SetBundleUrl(base::String("root_bundle.js"));

  auto root_attribute_slots = lepus::CArray::Create();
  root_attribute_slots->emplace_back(lepus::Value("slot_0"));
  root_attribute_slots->emplace_back(lepus::Value(42));
  root->SetAttributeSlots(lepus::Value(std::move(root_attribute_slots)));

  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(child));
  auto root_element_slots = lepus::CArray::Create();
  root_element_slots->emplace_back(lepus::Value(std::move(slot_children)));
  root->SetElementSlots(lepus::Value(std::move(root_element_slots)));

  auto serialized = root->Serialize();
  EXPECT_TRUE(serialized.IsObject());
  EXPECT_EQ(serialized.GetProperty("templateKey").StdString(), "root_template");
  EXPECT_EQ(serialized.GetProperty("bundleUrl").StdString(), "root_bundle.js");
  EXPECT_FALSE(serialized.GetProperty("kind").IsString());

  auto serialized_attribute_slots = serialized.GetProperty("attributeSlots");
  EXPECT_TRUE(serialized_attribute_slots.IsArrayOrJSArray());
  ASSERT_EQ(serialized_attribute_slots.GetLength(), 2);
  EXPECT_EQ(serialized_attribute_slots.GetProperty(0).StdString(), "slot_0");
  EXPECT_EQ(serialized_attribute_slots.GetProperty(1).Number(), 42);

  auto serialized_element_slots = serialized.GetProperty("elementSlots");
  EXPECT_TRUE(serialized_element_slots.IsArrayOrJSArray());
  ASSERT_EQ(serialized_element_slots.GetLength(), 1);
  auto serialized_slot_children = serialized_element_slots.GetProperty(0);
  EXPECT_TRUE(serialized_slot_children.IsArrayOrJSArray());
  ASSERT_EQ(serialized_slot_children.GetLength(), 1);

  auto serialized_child = serialized_slot_children.GetProperty(0);
  EXPECT_TRUE(serialized_child.IsObject());
  EXPECT_EQ(serialized_child.GetProperty("templateKey").StdString(),
            "child_template");
  EXPECT_EQ(serialized_child.GetProperty("bundleUrl").StdString(),
            "child_bundle.js");
  EXPECT_FALSE(serialized_child.GetProperty("kind").IsString());
  EXPECT_TRUE(
      serialized_child.GetProperty("attributeSlots").IsArrayOrJSArray());
  EXPECT_EQ(
      serialized_child.GetProperty("attributeSlots").GetProperty(0).Bool(),
      true);
  EXPECT_TRUE(serialized_child.GetProperty("elementSlots").IsArrayOrJSArray());
}

TEST_P(FiberElementTest, SerializeTemplateElementSkipsInvalidSlotChildren) {
  auto root = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  root->SetTemplateKey(base::String("root_template"));
  root->SetBundleUrl(base::String("root_bundle.js"));

  auto invalid_slot_children = lepus::CArray::Create();
  invalid_slot_children->emplace_back(lepus::Value(manager->CreateFiberView()));
  invalid_slot_children->emplace_back(lepus::Value(1));

  auto root_element_slots = lepus::CArray::Create();
  root_element_slots->emplace_back(
      lepus::Value(std::move(invalid_slot_children)));
  root_element_slots->emplace_back(lepus::Value("invalid_slot_shape"));
  root->SetElementSlots(lepus::Value(std::move(root_element_slots)));

  auto serialized = root->Serialize();
  EXPECT_TRUE(serialized.IsObject());

  auto serialized_element_slots = serialized.GetProperty("elementSlots");
  EXPECT_TRUE(serialized_element_slots.IsArrayOrJSArray());
  ASSERT_EQ(serialized_element_slots.GetLength(), 2);
  EXPECT_TRUE(serialized_element_slots.GetProperty(0).IsArrayOrJSArray());
  EXPECT_EQ(serialized_element_slots.GetProperty(0).GetLength(), 0);
  EXPECT_EQ(serialized_element_slots.GetProperty(1).StdString(),
            "invalid_slot_shape");
}

TEST_P(FiberElementTest, SerializeTypedTemplateElement) {
  auto child = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  child->SetTypedTag(base::String("raw-text"));
  child->SetUid(lepus::Value("child_uid"));

  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(child));
  auto element_slots = lepus::CArray::Create();
  element_slots->emplace_back(lepus::Value(slot_children));

  auto root = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  root->SetElementSlots(lepus::Value(element_slots));
  root->SetUid(lepus::Value("root_uid"));
  root->SetTypedTag(base::String("view"));
  auto attributes = lepus::Dictionary::Create();
  attributes->SetValue(base::String("data-test"), lepus::Value("root_attr"));
  root->SetRootAttributes(lepus::Value(attributes));

  auto serialized = root->Serialize();
  EXPECT_TRUE(serialized.IsObject());
  EXPECT_FALSE(serialized.GetProperty("templateKey").IsString());
  EXPECT_EQ(serialized.GetProperty("tag").StdString(), "view");
  EXPECT_EQ(serialized.GetProperty("uid").StdString(), "root_uid");
  EXPECT_EQ(
      serialized.GetProperty("attributes").GetProperty("data-test").StdString(),
      "root_attr");

  auto serialized_slots = serialized.GetProperty("elementSlots");
  EXPECT_TRUE(serialized_slots.IsArrayOrJSArray());
  ASSERT_EQ(serialized_slots.GetLength(), 1);
  auto serialized_child = serialized_slots.GetProperty(0).GetProperty(0);
  EXPECT_EQ(serialized_child.GetProperty("tag").StdString(), "raw-text");
  EXPECT_EQ(serialized_child.GetProperty("uid").StdString(), "child_uid");
}

TEST_P(FiberElementTest, CreateElementTemplateDoesNotPrepareBeforeTree) {
  auto lepus_ctx = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  ASSERT_TRUE(lepus_ctx);
  lepus_ctx->Initialize();
  lepus_ctx->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  auto* mts_ctx = runtime::MTSRuntime::ToQuickContext(lepus_ctx.get());
  ASSERT_TRUE(mts_ctx);

  lepus::Value args[] = {lepus::Value("root_template")};
  auto created_value =
      RendererFunctions::FiberCreateElementTemplate(mts_ctx, args, 1);

  ASSERT_TRUE(created_value.IsRefCounted());
  auto created_element =
      fml::static_ref_ptr_cast<FiberElement>(created_value.RefCounted())
          .strongify();
  ASSERT_NE(created_element, nullptr);
  ASSERT_TRUE(created_element->is_template());
  auto* created_template = static_cast<TemplateElement*>(created_element.get());
  EXPECT_FALSE(created_template->is_in_template_tree_);
  EXPECT_EQ(created_template->async_create_task_, nullptr);
}

TEST_P(FiberElementTest, PageTemplateElementSlotsPrepareChildrenRecursively) {
  auto compiled_child =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  compiled_child->SetTemplateKey(base::String("compiled_child"));

  auto typed_parent =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  typed_parent->SetTypedTag(base::String("list"));
  auto typed_parent_slot_children = lepus::CArray::Create();
  typed_parent_slot_children->emplace_back(lepus::Value(compiled_child));
  auto typed_parent_slots = lepus::CArray::Create();
  typed_parent_slots->emplace_back(lepus::Value(typed_parent_slot_children));
  typed_parent->SetElementSlots(lepus::Value(typed_parent_slots));

  EXPECT_FALSE(typed_parent->is_in_template_tree_);
  EXPECT_FALSE(compiled_child->is_in_template_tree_);
  EXPECT_EQ(compiled_child->async_create_task_, nullptr);

  auto page = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  page->SetTypedTag(base::String("page"));
  auto page_slot_children = lepus::CArray::Create();
  page_slot_children->emplace_back(lepus::Value(typed_parent));
  auto page_slots = lepus::CArray::Create();
  page_slots->emplace_back(lepus::Value(page_slot_children));
  page->SetElementSlots(lepus::Value(page_slots));

  EXPECT_TRUE(page->is_in_template_tree_);
  EXPECT_TRUE(typed_parent->is_in_template_tree_);
  EXPECT_TRUE(compiled_child->is_in_template_tree_);
  EXPECT_EQ(page->async_create_task_, nullptr);
  EXPECT_EQ(typed_parent->async_create_task_, nullptr);
  EXPECT_NE(compiled_child->async_create_task_, nullptr);
}

TEST_P(FiberElementTest, NonPageTemplateElementSlotsDoNotPrepareBeforeTree) {
  auto compiled_child =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  compiled_child->SetTemplateKey(base::String("compiled_child"));

  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(compiled_child));
  auto element_slots = lepus::CArray::Create();
  element_slots->emplace_back(lepus::Value(slot_children));

  auto typed_parent =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  typed_parent->SetTypedTag(base::String("list"));
  typed_parent->SetElementSlots(lepus::Value(element_slots));

  EXPECT_FALSE(typed_parent->is_in_template_tree_);
  EXPECT_FALSE(compiled_child->is_in_template_tree_);
  EXPECT_EQ(compiled_child->async_create_task_, nullptr);
}

TEST_P(FiberElementTest, InsertElementSlotChildMarksChildInTreeBeforeResolve) {
  auto grandchild =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  grandchild->SetTemplateKey(base::String("grandchild_template"));

  auto child = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  child->SetTypedTag(base::String("list"));
  auto child_slot_children = lepus::CArray::Create();
  child_slot_children->emplace_back(lepus::Value(grandchild));
  auto child_slots = lepus::CArray::Create();
  child_slots->emplace_back(lepus::Value(child_slot_children));
  child->SetElementSlots(lepus::Value(child_slots));

  auto parent = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  parent->SetTypedTag(base::String("page"));
  parent->SetElementSlots(lepus::Value(lepus::CArray::Create()));
  ASSERT_TRUE(parent->is_in_template_tree_);

  parent->InsertElementSlotChild(0, child, nullptr);

  EXPECT_EQ(parent->result_, nullptr);
  ASSERT_EQ(parent->pending_operations_.size(), 1u);
  EXPECT_TRUE(child->is_in_template_tree_);
  EXPECT_TRUE(grandchild->is_in_template_tree_);
  EXPECT_EQ(child->async_create_task_, nullptr);
  EXPECT_NE(grandchild->async_create_task_, nullptr);

  parent->RemoveElementSlotChild(0, child);
  EXPECT_TRUE(child->is_in_template_tree_);
  EXPECT_TRUE(grandchild->is_in_template_tree_);
}

TEST_P(FiberElementTest, TypedTemplateElementAppliesRootAttributesAsSpread) {
  auto root = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  root->SetTypedTag(base::String("view"));

  auto initial_attributes = lepus::Dictionary::Create();
  initial_attributes->SetValue(base::String("data-test"),
                               lepus::Value("before"));
  initial_attributes->SetValue(base::String("data-stale"),
                               lepus::Value("stale"));
  root->SetRootAttributes(lepus::Value(initial_attributes));
  EXPECT_EQ(root->result_, nullptr);

  auto updated_attributes = lepus::Dictionary::Create();
  updated_attributes->SetValue(base::String("data-test"),
                               lepus::Value("after"));
  updated_attributes->SetValue(base::String("data-added"),
                               lepus::Value("added"));
  updated_attributes->SetValue(base::String("bindtap"), lepus::Value("onTap"));
  root->SetRootAttributes(lepus::Value(updated_attributes));
  EXPECT_EQ(root->result_, nullptr);

  auto serialized_before_resolve = root->Serialize();
  EXPECT_EQ(serialized_before_resolve.GetProperty("attributes")
                .GetProperty("data-test")
                .StdString(),
            "after");
  EXPECT_EQ(serialized_before_resolve.GetProperty("attributes")
                .GetProperty("data-added")
                .StdString(),
            "added");
  EXPECT_FALSE(serialized_before_resolve.GetProperty("attributes")
                   .Contains(base::String("data-stale")));

  auto resolved = root->GetRoot();
  ASSERT_NE(resolved, nullptr);
  auto* test_data = DatasetValue(resolved.get(), "test");
  ASSERT_NE(test_data, nullptr);
  EXPECT_EQ(test_data->StdString(), "after");
  auto* added_data = DatasetValue(resolved.get(), "added");
  ASSERT_NE(added_data, nullptr);
  EXPECT_EQ(added_data->StdString(), "added");
  EXPECT_EQ(DatasetValue(resolved.get(), "stale"), nullptr);
  EXPECT_EQ(resolved->data_model_->attributes().count("data-test"), 0u);
  EXPECT_EQ(resolved->event_map().count("tap"), 1u);

  auto reset_attributes = lepus::Dictionary::Create();
  reset_attributes->SetValue(base::String("data-added"),
                             lepus::Value("updated"));
  root->SetRootAttributes(lepus::Value(reset_attributes));

  EXPECT_EQ(DatasetValue(resolved.get(), "test"), nullptr);
  added_data = DatasetValue(resolved.get(), "added");
  ASSERT_NE(added_data, nullptr);
  EXPECT_EQ(added_data->StdString(), "updated");
  EXPECT_EQ(resolved->event_map().count("tap"), 0u);

  root->SetRootAttributes(lepus::Value(lepus::Dictionary::Create()));
  EXPECT_EQ(DatasetValue(resolved.get(), "added"), nullptr);
  EXPECT_TRUE(root->Serialize().GetProperty("attributes").IsEmpty());
}

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
  EXPECT_EQ(root->async_create_task_, nullptr);
  auto resolved = root->GetRoot();

  ASSERT_NE(resolved, nullptr);
  EXPECT_TRUE(resolved->is_list());
  ASSERT_EQ(resolved->children().size(), 1u);
  EXPECT_EQ(resolved->children()[0].get(), slot_child.get());
  EXPECT_TRUE(slot_child->is_list_item());
}

TEST_P(FiberElementTest, TypedTemplateElementDefersSlotMountUntilResolve) {
  auto first = manager->CreateFiberView();
  auto second = manager->CreateFiberView();

  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(first));
  auto element_slots = lepus::CArray::Create();
  element_slots->emplace_back(lepus::Value(slot_children));

  auto root = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  root->SetTASM(tasm.get());
  root->SetElementSlots(lepus::Value(element_slots));
  root->SetTypedTag(base::String("view"));

  EXPECT_EQ(root->result_, nullptr);

  root->InsertElementSlotChild(0, second, first);
  root->RemoveElementSlotChild(0, first);

  EXPECT_EQ(root->result_, nullptr);
  ASSERT_EQ(root->pending_operations_.size(), 2u);

  auto third = manager->CreateFiberView();
  lepus::Value invalid_insert_args[] = {lepus::Value(root), lepus::Value(1),
                                        lepus::Value(third)};
  RendererFunctions::FiberInsertNodeToElementTemplate(nullptr,
                                                      invalid_insert_args, 3);
  lepus::Value invalid_remove_args[] = {lepus::Value(root), lepus::Value(1),
                                        lepus::Value(second)};
  RendererFunctions::FiberRemoveNodeFromElementTemplate(nullptr,
                                                        invalid_remove_args, 3);
  ASSERT_EQ(root->pending_operations_.size(), 2u);

  auto resolved = root->GetRoot();

  ASSERT_NE(resolved, nullptr);
  ASSERT_EQ(resolved->children().size(), 1u);
  EXPECT_EQ(resolved->children()[0].get(), second.get());
  EXPECT_TRUE(root->pending_operations_.empty());
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
  EXPECT_EQ(cloned->async_create_task_, nullptr);

  auto cloned_root = cloned->GetRoot();

  ASSERT_NE(cloned_root, nullptr);
  EXPECT_TRUE(cloned_root->is_raw_text());
  EXPECT_EQ(cloned->async_create_task_, nullptr);
}

TEST_P(FiberElementTest, RendererFunctionCreateTypedTemplateElement) {
  auto lepus_ctx = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  ASSERT_TRUE(lepus_ctx);
  lepus_ctx->Initialize();
  lepus_ctx->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  auto* mts_ctx = runtime::MTSRuntime::ToQuickContext(lepus_ctx.get());
  ASSERT_TRUE(mts_ctx);

  auto child = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  child->SetTypedTag(base::String("raw-text"));
  child->SetUid(lepus::Value("child_uid"));

  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(child));
  auto element_slots = lepus::CArray::Create();
  element_slots->emplace_back(lepus::Value(slot_children));

  auto attributes = lepus::Dictionary::Create();
  attributes->SetValue(base::String("data-test"), lepus::Value("root_attr"));
  lepus::Value args[] = {lepus::Value("view"), lepus::Value(attributes),
                         lepus::Value(element_slots), lepus::Value("root_uid")};
  auto created_value =
      RendererFunctions::FiberCreateTypedElementTemplate(mts_ctx, args, 4);

  ASSERT_TRUE(created_value.IsRefCounted());
  auto created_element =
      fml::static_ref_ptr_cast<FiberElement>(created_value.RefCounted())
          .strongify();
  ASSERT_NE(created_element, nullptr);
  ASSERT_TRUE(created_element->is_template());
  auto* typed_template = static_cast<TemplateElement*>(created_element.get());
  EXPECT_EQ(typed_template->result_, nullptr);
  EXPECT_EQ(typed_template->async_create_task_, nullptr);

  auto updated_attributes = lepus::Dictionary::Create();
  updated_attributes->SetValue(base::String("data-test"),
                               lepus::Value("updated_attr"));
  updated_attributes->SetValue(base::String("data-added"),
                               lepus::Value("added_attr"));
  lepus::Value update_args[] = {lepus::Value(created_element), lepus::Value(0),
                                lepus::Value(updated_attributes)};
  RendererFunctions::FiberSetAttributeOfElementTemplate(nullptr, update_args,
                                                        3);
  EXPECT_EQ(typed_template->result_, nullptr);

  auto serialized_before_resolve = typed_template->Serialize();
  EXPECT_EQ(serialized_before_resolve.GetProperty("attributes")
                .GetProperty("data-test")
                .StdString(),
            "updated_attr");
  EXPECT_EQ(serialized_before_resolve.GetProperty("attributes")
                .GetProperty("data-added")
                .StdString(),
            "added_attr");

  auto invalid_attributes = lepus::Dictionary::Create();
  invalid_attributes->SetValue(base::String("data-test"),
                               lepus::Value("invalid_attr"));
  lepus::Value invalid_update_args[] = {lepus::Value(created_element),
                                        lepus::Value(1),
                                        lepus::Value(invalid_attributes)};
  RendererFunctions::FiberSetAttributeOfElementTemplate(nullptr,
                                                        invalid_update_args, 3);
  EXPECT_EQ(typed_template->Serialize()
                .GetProperty("attributes")
                .GetProperty("data-test")
                .StdString(),
            "updated_attr");

  auto root = typed_template->GetRoot();
  ASSERT_NE(root, nullptr);
  EXPECT_TRUE(root->is_view());
  auto* test_data = DatasetValue(root.get(), "test");
  ASSERT_NE(test_data, nullptr);
  EXPECT_EQ(test_data->StdString(), "updated_attr");
  auto* added_data = DatasetValue(root.get(), "added");
  ASSERT_NE(added_data, nullptr);
  EXPECT_EQ(added_data->StdString(), "added_attr");
  EXPECT_EQ(root->data_model_->attributes().count("data-test"), 0u);
  ASSERT_EQ(root->children().size(), 1u);
  auto* mounted_child = static_cast<FiberElement*>(root->children()[0].get());
  ASSERT_NE(mounted_child, nullptr);
  EXPECT_EQ(mounted_child, child.get());
  EXPECT_TRUE(mounted_child->is_template());
  EXPECT_EQ(child->result_, nullptr);

  auto serialized = typed_template->Serialize();
  EXPECT_EQ(serialized.GetProperty("tag").StdString(), "view");
  EXPECT_EQ(serialized.GetProperty("uid").StdString(), "root_uid");
  EXPECT_EQ(
      serialized.GetProperty("attributes").GetProperty("data-test").StdString(),
      "updated_attr");
  EXPECT_EQ(serialized.GetProperty("attributes")
                .GetProperty("data-added")
                .StdString(),
            "added_attr");
  EXPECT_EQ(serialized.GetProperty("elementSlots")
                .GetProperty(0)
                .GetProperty(0)
                .GetProperty("tag")
                .StdString(),
            "raw-text");
}

TEST_P(FiberElementTest, ElementTemplateDynamicAPIsCachePendingOpsBeforeRoot) {
  auto first = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  first->SetTemplateKey(base::String("first_template"));

  auto second = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  second->SetTemplateKey(base::String("second_template"));

  auto attribute_slots = lepus::CArray::Create();
  attribute_slots->emplace_back(lepus::Value("old_value"));

  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(first));
  auto element_slots = lepus::CArray::Create();
  element_slots->emplace_back(lepus::Value(slot_children));

  auto root = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  root->SetTemplateKey(base::String("root_template"));
  root->SetAttributeSlots(lepus::Value(attribute_slots));
  root->SetElementSlots(lepus::Value(element_slots));

  lepus::Value set_attribute_args[] = {lepus::Value(root), lepus::Value(0),
                                       lepus::Value("new_value")};
  RendererFunctions::FiberSetAttributeOfElementTemplate(nullptr,
                                                        set_attribute_args, 3);

  lepus::Value insert_args[] = {lepus::Value(root), lepus::Value(0),
                                lepus::Value(second), lepus::Value(first)};
  RendererFunctions::FiberInsertNodeToElementTemplate(nullptr, insert_args, 4);

  lepus::Value remove_args[] = {lepus::Value(root), lepus::Value(0),
                                lepus::Value(first)};
  RendererFunctions::FiberRemoveNodeFromElementTemplate(nullptr, remove_args,
                                                        3);

  ASSERT_EQ(root->pending_operations_.size(), 3u);

  auto serialized = root->Serialize();
  auto serialized_attribute_slots = serialized.GetProperty("attributeSlots");
  ASSERT_TRUE(serialized_attribute_slots.IsArrayOrJSArray());
  EXPECT_EQ(serialized_attribute_slots.GetProperty(0).StdString(), "old_value");

  auto serialized_element_slots = serialized.GetProperty("elementSlots");
  ASSERT_TRUE(serialized_element_slots.IsArrayOrJSArray());
  auto serialized_slot_children = serialized_element_slots.GetProperty(0);
  ASSERT_TRUE(serialized_slot_children.IsArrayOrJSArray());
  ASSERT_EQ(serialized_slot_children.GetLength(), 1);
  EXPECT_EQ(serialized_slot_children.GetProperty(0)
                .GetProperty("templateKey")
                .StdString(),
            "first_template");
}

TEST_P(FiberElementTest, ElementTemplateDynamicAPIsUpdateMaterializedTargets) {
  auto root = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  root->SetTemplateKey(base::String("root_template"));
  root->result_ = manager->CreateFiberView();

  auto attribute_slots = lepus::CArray::Create();
  attribute_slots->emplace_back(lepus::Value("old_value"));
  root->SetAttributeSlots(lepus::Value(attribute_slots));

  auto target = manager->CreateFiberView();
  auto template_attributes =
      std::make_shared<const TemplateAttributes>(TemplateAttributes{
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("data-test"),
                    lepus::Value(), 0}});
  target->SetTemplateAttributes(template_attributes);
  target->AddDataset("test", lepus::Value("old_value"));
  root->attribute_slot_targets_.push_back(target);

  lepus::Value set_attribute_args[] = {lepus::Value(root), lepus::Value(0),
                                       lepus::Value("new_value")};
  RendererFunctions::FiberSetAttributeOfElementTemplate(nullptr,
                                                        set_attribute_args, 3);
  auto* test_data = DatasetValue(target.get(), "test");
  ASSERT_NE(test_data, nullptr);
  EXPECT_EQ(test_data->StdString(), "new_value");
  EXPECT_EQ(target->data_model_->attributes().count("data-test"), 0u);

  auto slot_parent = manager->CreateFiberView();
  auto sentinel = manager->CreateFiberView();
  auto first = manager->CreateFiberView();
  auto second = manager->CreateFiberView();
  slot_parent->InsertNode(first);
  slot_parent->InsertNode(sentinel);
  root->element_slot_targets_.push_back(
      ElementSlotMountPoint{slot_parent, sentinel});

  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(first));
  auto element_slots = lepus::CArray::Create();
  element_slots->emplace_back(lepus::Value(slot_children));
  root->SetElementSlots(lepus::Value(element_slots));

  lepus::Value insert_args[] = {lepus::Value(root), lepus::Value(0),
                                lepus::Value(second)};
  RendererFunctions::FiberInsertNodeToElementTemplate(nullptr, insert_args, 3);
  ASSERT_EQ(slot_parent->children().size(), 3u);
  EXPECT_EQ(slot_parent->children()[0].get(), first.get());
  EXPECT_EQ(slot_parent->children()[1].get(), second.get());
  EXPECT_EQ(slot_parent->children()[2].get(), sentinel.get());

  lepus::Value remove_args[] = {lepus::Value(root), lepus::Value(0),
                                lepus::Value(first)};
  RendererFunctions::FiberRemoveNodeFromElementTemplate(nullptr, remove_args,
                                                        3);
  ASSERT_EQ(slot_parent->children().size(), 2u);
  EXPECT_EQ(slot_parent->children()[0].get(), second.get());
  EXPECT_EQ(slot_parent->children()[1].get(), sentinel.get());
}

TEST_P(FiberElementTest, ElementTemplateStaticCacheReusesCompiledTree) {
  auto parent = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  parent->SetTemplateKey(base::String("parent_template"));
  parent->result_ = manager->CreateFiberView();

  auto child = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  child->SetTemplateKey(base::String("item_template"));
  child->SetBundleUrl(base::String("item_bundle.js"));
  auto child_root = manager->CreateFiberView();
  child->result_ = child_root;

  auto slot_parent = manager->CreateFiberView();
  auto slot_sentinel = manager->CreateFiberView();
  slot_parent->InsertNode(child_root);
  slot_parent->InsertNode(slot_sentinel);
  parent->element_slot_targets_.push_back(
      ElementSlotMountPoint{slot_parent, slot_sentinel});

  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(child));
  auto element_slots = lepus::CArray::Create();
  element_slots->emplace_back(lepus::Value(slot_children));
  parent->SetElementSlots(lepus::Value(element_slots));

  parent->RemoveElementSlotChild(0, child);

  EXPECT_EQ(child->result_, nullptr);
  ASSERT_EQ(slot_parent->children().size(), 1u);
  EXPECT_EQ(slot_parent->children()[0].get(), slot_sentinel.get());

  auto fresh_child =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  fresh_child->SetTemplateKey(base::String("item_template"));
  fresh_child->SetBundleUrl(base::String("item_bundle.js"));

  EXPECT_EQ(fresh_child->GetRoot().get(), child_root.get());
  EXPECT_TRUE(manager->cached_template_element_trees_.empty());
}

TEST_P(FiberElementTest,
       ElementTemplateCacheReusesCompiledTreeAndReplaysState) {
  auto parent = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  parent->SetTemplateKey(base::String("parent_template"));
  parent->result_ = manager->CreateFiberView();

  auto child = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  child->SetTemplateKey(base::String("item_template"));
  child->SetBundleUrl(base::String("item_bundle.js"));

  auto child_root = manager->CreateFiberView();
  auto attr_target = manager->CreateFiberView();
  auto template_attributes =
      std::make_shared<const TemplateAttributes>(TemplateAttributes{
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("data-test"),
                    lepus::Value(), 0}});
  attr_target->SetTemplateAttributes(template_attributes);
  attr_target->AddDataset("test", lepus::Value("old_value"));
  child_root->InsertNode(attr_target);

  auto slot_parent = manager->CreateFiberView();
  auto slot_sentinel = manager->CreateFiberView();
  auto stale_slot_child = manager->CreateFiberView();
  slot_parent->InsertNode(stale_slot_child);
  slot_parent->InsertNode(slot_sentinel);
  child_root->InsertNode(slot_parent);

  auto child_attribute_slots = lepus::CArray::Create();
  child_attribute_slots->emplace_back(lepus::Value("old_value"));
  child->SetAttributeSlots(lepus::Value(child_attribute_slots));
  auto child_slot_children = lepus::CArray::Create();
  child_slot_children->emplace_back(lepus::Value(stale_slot_child));
  auto child_element_slots = lepus::CArray::Create();
  child_element_slots->emplace_back(lepus::Value(child_slot_children));
  child->SetElementSlots(lepus::Value(child_element_slots));
  child->result_ = child_root;
  child->attribute_slot_targets_.push_back(attr_target);
  child->element_slot_targets_.push_back(
      ElementSlotMountPoint{slot_parent, slot_sentinel});

  auto parent_slot_parent = manager->CreateFiberView();
  auto parent_slot_sentinel = manager->CreateFiberView();
  parent_slot_parent->InsertNode(child_root);
  parent_slot_parent->InsertNode(parent_slot_sentinel);
  parent->element_slot_targets_.push_back(
      ElementSlotMountPoint{parent_slot_parent, parent_slot_sentinel});
  auto parent_slot_children = lepus::CArray::Create();
  parent_slot_children->emplace_back(lepus::Value(child));
  auto parent_element_slots = lepus::CArray::Create();
  parent_element_slots->emplace_back(lepus::Value(parent_slot_children));
  parent->SetElementSlots(lepus::Value(parent_element_slots));

  parent->RemoveElementSlotChild(0, child);

  EXPECT_EQ(child->result_, nullptr);
  ASSERT_EQ(parent_slot_parent->children().size(), 1u);
  EXPECT_EQ(parent_slot_parent->children()[0].get(),
            parent_slot_sentinel.get());
  ASSERT_EQ(slot_parent->children().size(), 1u);
  EXPECT_EQ(slot_parent->children()[0].get(), slot_sentinel.get());
  EXPECT_EQ(stale_slot_child->parent(), nullptr);

  auto fresh_slot_child = manager->CreateFiberView();
  auto fresh_child =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  fresh_child->SetTemplateKey(base::String("item_template"));
  fresh_child->SetBundleUrl(base::String("item_bundle.js"));
  auto fresh_attribute_slots = lepus::CArray::Create();
  fresh_attribute_slots->emplace_back(lepus::Value("new_value"));
  fresh_child->SetAttributeSlots(lepus::Value(fresh_attribute_slots));
  auto fresh_slot_children = lepus::CArray::Create();
  fresh_slot_children->emplace_back(lepus::Value(fresh_slot_child));
  auto fresh_element_slots = lepus::CArray::Create();
  fresh_element_slots->emplace_back(lepus::Value(fresh_slot_children));
  fresh_child->SetElementSlots(lepus::Value(fresh_element_slots));

  auto resolved = fresh_child->GetRoot();

  EXPECT_EQ(resolved.get(), child_root.get());
  auto* test_data = DatasetValue(attr_target.get(), "test");
  ASSERT_NE(test_data, nullptr);
  EXPECT_EQ(test_data->StdString(), "new_value");
  ASSERT_EQ(slot_parent->children().size(), 2u);
  EXPECT_EQ(slot_parent->children()[0].get(), fresh_slot_child.get());
  EXPECT_EQ(slot_parent->children()[1].get(), slot_sentinel.get());
}

TEST_P(FiberElementTest,
       ElementTemplateCachePrepareConsumesCacheWithoutResolving) {
  auto parent = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  parent->SetTemplateKey(base::String("parent_template"));
  parent->result_ = manager->CreateFiberView();

  auto child = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  child->SetTemplateKey(base::String("item_template"));
  child->SetBundleUrl(base::String("item_bundle.js"));
  auto child_root = manager->CreateFiberView();
  auto parent_slot_parent = manager->CreateFiberView();
  auto parent_slot_sentinel = manager->CreateFiberView();
  parent_slot_parent->InsertNode(child_root);
  parent_slot_parent->InsertNode(parent_slot_sentinel);
  parent->element_slot_targets_.push_back(
      ElementSlotMountPoint{parent_slot_parent, parent_slot_sentinel});
  child->result_ = child_root;

  auto parent_slot_children = lepus::CArray::Create();
  parent_slot_children->emplace_back(lepus::Value(child));
  auto parent_element_slots = lepus::CArray::Create();
  parent_element_slots->emplace_back(lepus::Value(parent_slot_children));
  parent->SetElementSlots(lepus::Value(parent_element_slots));
  parent->RemoveElementSlotChild(0, child);

  ASSERT_FALSE(manager->cached_template_element_trees_.empty());

  auto fresh_child =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  fresh_child->SetTemplateKey(base::String("item_template"));
  fresh_child->SetBundleUrl(base::String("item_bundle.js"));

  fresh_child->PrepareAsyncCreateElementTree();

  EXPECT_EQ(fresh_child->async_create_task_, nullptr);
  EXPECT_EQ(fresh_child->result_, nullptr);
  EXPECT_TRUE(manager->cached_template_element_trees_.empty());

  EXPECT_EQ(fresh_child->GetRoot().get(), child_root.get());
}

TEST_P(FiberElementTest,
       ElementTemplatePreparedCacheCanReturnToPoolBeforeResolve) {
  auto parent = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  parent->SetTemplateKey(base::String("parent_template"));
  parent->result_ = manager->CreateFiberView();

  auto child = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  child->SetTemplateKey(base::String("item_template"));
  child->SetBundleUrl(base::String("item_bundle.js"));
  auto child_root = manager->CreateFiberView();
  auto parent_slot_parent = manager->CreateFiberView();
  auto parent_slot_sentinel = manager->CreateFiberView();
  parent_slot_parent->InsertNode(child_root);
  parent_slot_parent->InsertNode(parent_slot_sentinel);
  parent->element_slot_targets_.push_back(
      ElementSlotMountPoint{parent_slot_parent, parent_slot_sentinel});
  child->result_ = child_root;

  auto parent_slot_children = lepus::CArray::Create();
  parent_slot_children->emplace_back(lepus::Value(child));
  auto parent_element_slots = lepus::CArray::Create();
  parent_element_slots->emplace_back(lepus::Value(parent_slot_children));
  parent->SetElementSlots(lepus::Value(parent_element_slots));
  parent->RemoveElementSlotChild(0, child);

  auto prepared_child =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  prepared_child->SetTemplateKey(base::String("item_template"));
  prepared_child->SetBundleUrl(base::String("item_bundle.js"));
  prepared_child->PrepareAsyncCreateElementTree();
  ASSERT_TRUE(manager->cached_template_element_trees_.empty());

  auto second_parent =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  second_parent->SetTemplateKey(base::String("second_parent_template"));
  second_parent->result_ = manager->CreateFiberView();
  auto second_slot_parent = manager->CreateFiberView();
  auto second_slot_sentinel = manager->CreateFiberView();
  second_slot_parent->InsertNode(prepared_child);
  second_slot_parent->InsertNode(second_slot_sentinel);
  second_parent->element_slot_targets_.push_back(
      ElementSlotMountPoint{second_slot_parent, second_slot_sentinel});
  auto second_slot_children = lepus::CArray::Create();
  second_slot_children->emplace_back(lepus::Value(prepared_child));
  auto second_element_slots = lepus::CArray::Create();
  second_element_slots->emplace_back(lepus::Value(second_slot_children));
  second_parent->SetElementSlots(lepus::Value(second_element_slots));

  second_parent->RemoveElementSlotChild(0, prepared_child);

  ASSERT_FALSE(manager->cached_template_element_trees_.empty());
  auto fresh_child =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  fresh_child->SetTemplateKey(base::String("item_template"));
  fresh_child->SetBundleUrl(base::String("item_bundle.js"));
  EXPECT_EQ(fresh_child->GetRoot().get(), child_root.get());
}

TEST_P(FiberElementTest,
       ElementTemplateDynamicInsertPreparesFromCacheWithoutAsyncTask) {
  auto cache_parent =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  cache_parent->SetTemplateKey(base::String("cache_parent_template"));
  cache_parent->result_ = manager->CreateFiberView();

  auto cached_child =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  cached_child->SetTemplateKey(base::String("item_template"));
  cached_child->SetBundleUrl(base::String("item_bundle.js"));
  auto cached_root = manager->CreateFiberView();
  cached_child->result_ = cached_root;
  auto cache_slot_parent = manager->CreateFiberView();
  auto cache_slot_sentinel = manager->CreateFiberView();
  cache_slot_parent->InsertNode(cached_root);
  cache_slot_parent->InsertNode(cache_slot_sentinel);
  cache_parent->element_slot_targets_.push_back(
      ElementSlotMountPoint{cache_slot_parent, cache_slot_sentinel});
  auto cache_slot_children = lepus::CArray::Create();
  cache_slot_children->emplace_back(lepus::Value(cached_child));
  auto cache_element_slots = lepus::CArray::Create();
  cache_element_slots->emplace_back(lepus::Value(cache_slot_children));
  cache_parent->SetElementSlots(lepus::Value(cache_element_slots));
  cache_parent->RemoveElementSlotChild(0, cached_child);

  auto list_template =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  list_template->SetTypedTag(base::String("list"));
  list_template->result_ = manager->CreateFiberView();
  list_template->is_in_template_tree_ = true;
  auto list_slot_parent = manager->CreateFiberView();
  auto list_slot_sentinel = manager->CreateFiberView();
  list_slot_parent->InsertNode(list_slot_sentinel);
  auto page = manager->CreateFiberPage("page", 11);
  page->InsertNode(list_slot_parent);
  list_template->element_slot_targets_.push_back(
      ElementSlotMountPoint{list_slot_parent, list_slot_sentinel});

  auto fresh_child =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  fresh_child->SetTemplateKey(base::String("item_template"));
  fresh_child->SetBundleUrl(base::String("item_bundle.js"));

  list_template->InsertElementSlotChild(0, fresh_child, nullptr);

  EXPECT_EQ(fresh_child->async_create_task_, nullptr);
  EXPECT_EQ(fresh_child->result_, nullptr);
  EXPECT_TRUE(manager->cached_template_element_trees_.empty());
  ASSERT_EQ(list_slot_parent->children().size(), 2u);
  EXPECT_EQ(list_slot_parent->children()[0].get(), fresh_child.get());
  EXPECT_EQ(list_slot_parent->children()[1].get(), list_slot_sentinel.get());

  list_slot_parent->PrepareChildren();

  ASSERT_EQ(list_slot_parent->children().size(), 2u);
  EXPECT_EQ(list_slot_parent->children()[0].get(), cached_root.get());
  EXPECT_EQ(list_slot_parent->children()[1].get(), list_slot_sentinel.get());
}

TEST_P(FiberElementTest, ElementTemplateCacheDetachesSlotChildrenRecursively) {
  auto parent = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  parent->SetTemplateKey(base::String("parent_template"));
  parent->result_ = manager->CreateFiberView();

  auto child = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  child->SetTemplateKey(base::String("child_template"));
  auto child_root = manager->CreateFiberView();
  auto child_slot_parent = manager->CreateFiberView();
  auto child_slot_sentinel = manager->CreateFiberView();
  child_root->InsertNode(child_slot_parent);
  child->result_ = child_root;
  child->element_slot_targets_.push_back(
      ElementSlotMountPoint{child_slot_parent, child_slot_sentinel});

  auto grandchild =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  grandchild->SetTemplateKey(base::String("grandchild_template"));
  auto grandchild_root = manager->CreateFiberView();
  grandchild->result_ = grandchild_root;
  child_slot_parent->InsertNode(grandchild_root);
  child_slot_parent->InsertNode(child_slot_sentinel);

  auto child_slot_children = lepus::CArray::Create();
  child_slot_children->emplace_back(lepus::Value(grandchild));
  auto child_element_slots = lepus::CArray::Create();
  child_element_slots->emplace_back(lepus::Value(child_slot_children));
  child->SetElementSlots(lepus::Value(child_element_slots));

  auto parent_slot_parent = manager->CreateFiberView();
  auto parent_slot_sentinel = manager->CreateFiberView();
  parent_slot_parent->InsertNode(child_root);
  parent_slot_parent->InsertNode(parent_slot_sentinel);
  parent->element_slot_targets_.push_back(
      ElementSlotMountPoint{parent_slot_parent, parent_slot_sentinel});
  auto parent_slot_children = lepus::CArray::Create();
  parent_slot_children->emplace_back(lepus::Value(child));
  auto parent_element_slots = lepus::CArray::Create();
  parent_element_slots->emplace_back(lepus::Value(parent_slot_children));
  parent->SetElementSlots(lepus::Value(parent_element_slots));

  parent->RemoveElementSlotChild(0, child);

  EXPECT_EQ(child->result_, nullptr);
  EXPECT_EQ(grandchild->result_, nullptr);
  ASSERT_EQ(child_slot_parent->children().size(), 1u);
  EXPECT_EQ(child_slot_parent->children()[0].get(), child_slot_sentinel.get());

  auto fresh_grandchild =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  fresh_grandchild->SetTemplateKey(base::String("grandchild_template"));
  EXPECT_EQ(fresh_grandchild->GetRoot().get(), grandchild_root.get());

  auto fresh_child =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  fresh_child->SetTemplateKey(base::String("child_template"));
  EXPECT_EQ(fresh_child->GetRoot().get(), child_root.get());
}

TEST_P(FiberElementTest, TypedTemplateElementIsUnmountedButNotCached) {
  auto parent = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  parent->SetTemplateKey(base::String("parent_template"));
  parent->result_ = manager->CreateFiberView();

  auto typed_child =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  typed_child->SetTypedTag(base::String("view"));
  auto typed_root = manager->CreateFiberView();
  typed_child->result_ = typed_root;

  auto parent_slot_parent = manager->CreateFiberView();
  auto parent_slot_sentinel = manager->CreateFiberView();
  parent_slot_parent->InsertNode(typed_root);
  parent_slot_parent->InsertNode(parent_slot_sentinel);
  parent->element_slot_targets_.push_back(
      ElementSlotMountPoint{parent_slot_parent, parent_slot_sentinel});
  auto parent_slot_children = lepus::CArray::Create();
  parent_slot_children->emplace_back(lepus::Value(typed_child));
  auto parent_element_slots = lepus::CArray::Create();
  parent_element_slots->emplace_back(lepus::Value(parent_slot_children));
  parent->SetElementSlots(lepus::Value(parent_element_slots));

  parent->RemoveElementSlotChild(0, typed_child);

  EXPECT_EQ(typed_child->result_.get(), typed_root.get());
  EXPECT_EQ(typed_root->parent(), nullptr);
  EXPECT_TRUE(manager->cached_template_element_trees_.empty());
  ASSERT_EQ(parent_slot_parent->children().size(), 1u);
  EXPECT_EQ(parent_slot_parent->children()[0].get(),
            parent_slot_sentinel.get());
}

TEST_P(FiberElementTest, ElementTemplateInsertElementSlotChildKeepsOtherSlots) {
  auto root = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  root->SetTemplateKey(base::String("root_template"));
  root->result_ = manager->CreateFiberView();

  auto child_in_other_slot =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  child_in_other_slot->SetTemplateKey(base::String("other_template"));
  auto child_to_insert =
      fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  child_to_insert->SetTemplateKey(base::String("inserted_template"));
  auto ref_node = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  ref_node->SetTemplateKey(base::String("ref_template"));

  auto first_slot_children = lepus::CArray::Create();
  first_slot_children->emplace_back(lepus::Value(child_in_other_slot));
  auto second_slot_children = lepus::CArray::Create();
  second_slot_children->emplace_back(lepus::Value(ref_node));
  auto element_slots = lepus::CArray::Create();
  element_slots->emplace_back(lepus::Value(first_slot_children));
  element_slots->emplace_back(lepus::Value(second_slot_children));
  root->SetElementSlots(lepus::Value(element_slots));

  auto slot_parent = manager->CreateFiberView();
  auto sentinel = manager->CreateFiberView();
  slot_parent->InsertNode(ref_node);
  slot_parent->InsertNode(sentinel);
  root->element_slot_targets_.push_back(
      ElementSlotMountPoint{nullptr, nullptr});
  root->element_slot_targets_.push_back(
      ElementSlotMountPoint{slot_parent, sentinel});

  lepus::Value insert_args[] = {lepus::Value(root), lepus::Value(1),
                                lepus::Value(child_to_insert),
                                lepus::Value(ref_node)};
  RendererFunctions::FiberInsertNodeToElementTemplate(nullptr, insert_args, 4);

  auto serialized = root->Serialize();
  auto serialized_slots = serialized.GetProperty("elementSlots");
  ASSERT_TRUE(serialized_slots.IsArrayOrJSArray());
  auto first_serialized_slot = serialized_slots.GetProperty(0);
  ASSERT_TRUE(first_serialized_slot.IsArrayOrJSArray());
  ASSERT_EQ(first_serialized_slot.GetLength(), 1);
  EXPECT_EQ(first_serialized_slot.GetProperty(0)
                .GetProperty("templateKey")
                .StdString(),
            "other_template");

  auto second_serialized_slot = serialized_slots.GetProperty(1);
  ASSERT_TRUE(second_serialized_slot.IsArrayOrJSArray());
  ASSERT_EQ(second_serialized_slot.GetLength(), 2);
  EXPECT_EQ(second_serialized_slot.GetProperty(0)
                .GetProperty("templateKey")
                .StdString(),
            "inserted_template");
  EXPECT_EQ(second_serialized_slot.GetProperty(1)
                .GetProperty("templateKey")
                .StdString(),
            "ref_template");
  ASSERT_EQ(slot_parent->children().size(), 3u);
  EXPECT_EQ(slot_parent->children()[0].get(), child_to_insert.get());
  EXPECT_EQ(slot_parent->children()[1].get(), ref_node.get());
  EXPECT_EQ(slot_parent->children()[2].get(), sentinel.get());
}

TEST_P(FiberElementTest, ElementTemplateDynamicAPIsConsumePendingOpsOnGetRoot) {
  manager->config_->SetEnableEventHandleRefactor(true);
  manager->SetConfig(manager->config_);
  tasm->page_config_ = manager->config_;

  auto default_entry = std::make_shared<TemplateEntry>();
  default_entry->SetName(DEFAULT_ENTRY_NAME);
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = default_entry;

  auto root = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  root->entry_ = default_entry.get();
  root->SetTemplateKey(base::String("root_template"));

  auto attribute_slots = lepus::CArray::Create();
  attribute_slots->emplace_back(lepus::Value("old_value"));
  attribute_slots->emplace_back(lepus::Value("onTap"));
  root->SetAttributeSlots(lepus::Value(attribute_slots));

  auto target = ElementManager::StaticCreateFiberElement(ELEMENT_VIEW);
  auto template_attributes =
      std::make_shared<const TemplateAttributes>(TemplateAttributes{
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("data-test"),
                    lepus::Value(), 0},
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("bindtap"),
                    lepus::Value(), 1}});
  target->SetTemplateAttributes(template_attributes);
  target->AddDataset("test", lepus::Value("old_value"));

  auto slot_parent = ElementManager::StaticCreateFiberElement(ELEMENT_VIEW);
  auto sentinel = ElementManager::StaticCreateFiberElement(ELEMENT_VIEW);
  auto first = manager->CreateFiberView();
  auto second = manager->CreateFiberView();
  slot_parent->InsertNode(sentinel);

  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(first));
  auto element_slots = lepus::CArray::Create();
  element_slots->emplace_back(lepus::Value(slot_children));
  root->SetElementSlots(lepus::Value(element_slots));

  GeneratedElementsResult generated;
  generated.result_ = ElementManager::StaticCreateFiberElement(ELEMENT_VIEW);
  generated.result_->InsertNode(target);
  generated.result_->InsertNode(slot_parent);
  generated.attribute_slot_targets_.push_back(target);
  generated.element_slot_targets_.push_back(
      ElementSlotMountPoint{slot_parent, sentinel});
  generated.prepared_element_slot_insertions_.push_back(
      PreparedElementSlotInsertion{0, first});

  std::promise<GeneratedElementsResult> promise;
  auto future = promise.get_future();
  root->async_create_task_ =
      fml::MakeRefCounted<base::OnceTask<GeneratedElementsResult>>(
          [generated = std::move(generated),
           promise = std::move(promise)]() mutable {
            promise.set_value(std::move(generated));
          },
          std::move(future));

  lepus::Value set_attribute_args[] = {lepus::Value(root), lepus::Value(0),
                                       lepus::Value("new_value")};
  RendererFunctions::FiberSetAttributeOfElementTemplate(nullptr,
                                                        set_attribute_args, 3);

  lepus::Value insert_args[] = {lepus::Value(root), lepus::Value(0),
                                lepus::Value(second), lepus::Value(first)};
  RendererFunctions::FiberInsertNodeToElementTemplate(nullptr, insert_args, 4);

  lepus::Value remove_args[] = {lepus::Value(root), lepus::Value(0),
                                lepus::Value(first)};
  RendererFunctions::FiberRemoveNodeFromElementTemplate(nullptr, remove_args,
                                                        3);

  ASSERT_EQ(root->pending_operations_.size(), 3u);
  EXPECT_EQ(target->element_manager(), nullptr);

  root->GetRoot();

  EXPECT_TRUE(root->pending_operations_.empty());
  EXPECT_EQ(target->element_manager(), manager);
  auto* test_data = DatasetValue(target.get(), "test");
  ASSERT_NE(test_data, nullptr);
  EXPECT_EQ(test_data->StdString(), "new_value");
  EXPECT_EQ(target->data_model_->attributes().count("data-test"), 0u);
  auto* listeners = target->GetEventListenerMap()->Find("tap");
  ASSERT_NE(listeners, nullptr);
  ASSERT_EQ(listeners->size(), 1u);
  ASSERT_EQ(slot_parent->children().size(), 2u);
  EXPECT_EQ(slot_parent->children()[0].get(), second.get());
  EXPECT_EQ(slot_parent->children()[1].get(), sentinel.get());
}

TEST_P(FiberElementTest, ElementTemplateStaticEventsSyncAfterAttach) {
  manager->config_->SetEnableEventHandleRefactor(true);
  manager->SetConfig(manager->config_);
  tasm->page_config_ = manager->config_;

  auto default_entry = std::make_shared<TemplateEntry>();
  default_entry->SetName(DEFAULT_ENTRY_NAME);
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = default_entry;

  ElementTemplateInfo template_info;
  template_info.exist_ = true;
  template_info.key_ = "root_template";

  auto target_info = ElementInfo();
  target_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  target_info.attributes_ =
      std::make_shared<const TemplateAttributes>(TemplateAttributes{
          Attribute{ATTRIBUTE_BINDING_TYPE_STATIC, base::String("bindtap"),
                    lepus::Value("onStaticTap"), 0}});
  template_info.elements_.emplace_back(std::move(target_info));

  auto generated =
      TreeResolver::GenerateElementsFromTemplateInfo(template_info);
  auto* target = generated.result_.get();
  ASSERT_NE(target, nullptr);
  ASSERT_EQ(generated.static_event_targets_.size(), 1u);
  EXPECT_EQ(generated.static_event_targets_[0].get(), target);
  ASSERT_NE(target->event_map().find("tap"), target->event_map().end());
  EXPECT_EQ(target->GetEventListenerMap()->Find("tap"), nullptr);

  auto root = fml::AdoptRef<TemplateElement>(new TemplateElement(manager));
  root->entry_ = default_entry.get();
  std::promise<GeneratedElementsResult> promise;
  auto future = promise.get_future();
  root->async_create_task_ =
      fml::MakeRefCounted<base::OnceTask<GeneratedElementsResult>>(
          [generated = std::move(generated),
           promise = std::move(promise)]() mutable {
            promise.set_value(std::move(generated));
          },
          std::move(future));

  auto resolved = root->GetRoot();

  EXPECT_EQ(resolved.get(), target);
  EXPECT_EQ(target->element_manager(), manager);
  auto* listeners = target->GetEventListenerMap()->Find("tap");
  ASSERT_NE(listeners, nullptr);
  ASSERT_EQ(listeners->size(), 1u);
  EXPECT_FALSE(listeners->front()->GetOptions().IsCapture());
  EXPECT_FALSE(listeners->front()->GetOptions().IsCatch());
}

TEST_P(FiberElementTest, ApplyTemplateAttributesSpecialKeys) {
  auto attribute_slots = lepus::CArray::Create();
  attribute_slots->emplace_back(lepus::Value("new-class another"));
  auto style_object = lepus::Dictionary::Create();
  style_object->SetValue("backgroundColor", lepus::Value("blue"));
  style_object->SetValue("width", lepus::Value("100px"));
  attribute_slots->emplace_back(lepus::Value(style_object));
  attribute_slots->emplace_back(lepus::Value("new-id"));
  attribute_slots->emplace_back(lepus::Value(456));
  attribute_slots->emplace_back(lepus::Value("new-generic"));
  attribute_slots->emplace_back(lepus::Value(true));

  auto target = manager->CreateFiberView();
  target->SetClass("old-class");
  target->SetRawInlineStyles(base::String("width:10px;"));
  target->SetIdSelector("old-id");
  target->SetCSSID(123);
  target->AddDataset("test", lepus::Value("old-generic"));

  auto template_attributes =
      std::make_shared<const TemplateAttributes>(TemplateAttributes{
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("class"),
                    lepus::Value(), 0},
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("style"),
                    lepus::Value(), 1},
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("id"),
                    lepus::Value(), 2},
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("cssID"),
                    lepus::Value(), 3},
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("data-test"),
                    lepus::Value(), 4},
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC,
                    base::String("data-enabled"), lepus::Value(), 5}});
  target->SetTemplateAttributes(template_attributes);

  TreeResolver::ApplyTemplateAttributesToElement(
      target.get(), lepus::Value(std::move(attribute_slots)));

  ASSERT_EQ(target->classes().size(), 2u);
  EXPECT_EQ(target->classes()[0], "new-class");
  EXPECT_EQ(target->classes()[1], "another");
  ASSERT_TRUE(target->current_raw_inline_styles_.has_value());
  EXPECT_EQ(target->current_raw_inline_styles_->at(
                CSSPropertyID::kPropertyIDBackgroundColor),
            lepus::Value("blue"));
  EXPECT_EQ(
      target->current_raw_inline_styles_->at(CSSPropertyID::kPropertyIDWidth),
      lepus::Value("100px"));
  EXPECT_EQ(target->current_raw_inline_styles_->count(
                CSSPropertyID::kPropertyIDPosition),
            0u);
  EXPECT_EQ(target->GetIdSelector(), "new-id");
  EXPECT_EQ(target->GetCSSID(), 456);
  auto* test_data = DatasetValue(target.get(), "test");
  ASSERT_NE(test_data, nullptr);
  EXPECT_EQ(test_data->StdString(), "new-generic");
  auto* enabled_data = DatasetValue(target.get(), "enabled");
  ASSERT_NE(enabled_data, nullptr);
  EXPECT_TRUE(enabled_data->Bool());
  EXPECT_EQ(target->data_model_->attributes().count("data-test"), 0u);
  EXPECT_EQ(target->data_model_->attributes().count("data-enabled"), 0u);
  EXPECT_EQ(target->data_model_->attributes().count("class"), 0u);
  EXPECT_EQ(target->data_model_->attributes().count("style"), 0u);
  EXPECT_EQ(target->data_model_->attributes().count("id"), 0u);
  EXPECT_EQ(target->data_model_->attributes().count("cssID"), 0u);
}

TEST_P(FiberElementTest, ApplyTemplateDataAttributePreservesEmptyValue) {
  auto attribute_slots = lepus::CArray::Create();
  attribute_slots->emplace_back(lepus::Value());

  auto target = manager->CreateFiberView();
  target->AddDataset("test", lepus::Value("old-value"));
  auto template_attributes =
      std::make_shared<const TemplateAttributes>(TemplateAttributes{
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("data-test"),
                    lepus::Value(), 0}});
  target->SetTemplateAttributes(template_attributes);

  TreeResolver::ApplyTemplateAttributesToElement(
      target.get(), lepus::Value(std::move(attribute_slots)));

  auto* test_data = DatasetValue(target.get(), "test");
  ASSERT_NE(test_data, nullptr);
  EXPECT_TRUE(test_data->IsEmpty());
  EXPECT_EQ(target->data_model_->attributes().count("data-test"), 0u);
}

TEST_P(FiberElementTest, ApplyTemplateAttributesEventKeys) {
  auto attribute_slots = lepus::CArray::Create();
  attribute_slots->emplace_back(lepus::Value("onTap"));
  attribute_slots->emplace_back(lepus::Value("onCatchTap"));
  attribute_slots->emplace_back(lepus::Value("onFocus"));
  attribute_slots->emplace_back(lepus::Value("ordinary-value"));
  attribute_slots->emplace_back(lepus::Value("onGlobalTap"));

  auto target = manager->CreateFiberView();
  auto template_attributes =
      std::make_shared<const TemplateAttributes>(TemplateAttributes{
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("bindtap"),
                    lepus::Value(), 0},
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("catchtap"),
                    lepus::Value(), 1},
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC,
                    base::String("main-thread:bindfocus"), lepus::Value(), 2},
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("data-test"),
                    lepus::Value(), 3},
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC,
                    base::String("global-bindtap"), lepus::Value(), 4}});
  target->SetTemplateAttributes(template_attributes);

  TreeResolver::ApplyTemplateAttributesToElement(
      target.get(), lepus::Value(std::move(attribute_slots)));

  auto event_iter = target->event_map().find("tap");
  ASSERT_NE(event_iter, target->event_map().end());
  EXPECT_EQ(event_iter->second->type(), "catchEvent");
  EXPECT_EQ(event_iter->second->function(), "onCatchTap");
  auto main_thread_event_iter = target->event_map().find("focus");
  ASSERT_NE(main_thread_event_iter, target->event_map().end());
  EXPECT_EQ(main_thread_event_iter->second->type(), "bindEvent");
  EXPECT_EQ(main_thread_event_iter->second->function(), "onFocus");
  EXPECT_EQ(target->data_model_->attributes().count("bindtap"), 0u);
  EXPECT_EQ(target->data_model_->attributes().count("catchtap"), 0u);
  EXPECT_EQ(target->data_model_->attributes().count("main-thread:bindfocus"),
            0u);
  EXPECT_EQ(target->data_model_->attributes().count("data-test"), 0u);
  auto* test_data = DatasetValue(target.get(), "test");
  ASSERT_NE(test_data, nullptr);
  EXPECT_EQ(test_data->StdString(), "ordinary-value");
  EXPECT_EQ(target->data_model_->attributes().count("global-bindtap"), 0u);
  auto global_event_iter = target->global_bind_event_map().find("tap");
  ASSERT_NE(global_event_iter, target->global_bind_event_map().end());
  EXPECT_EQ(global_event_iter->second->type(), "global-bindEvent");
  EXPECT_EQ(global_event_iter->second->function(), "onGlobalTap");

  auto capture_attribute_slots = lepus::CArray::Create();
  capture_attribute_slots->emplace_back(lepus::Value("onCaptureTap"));
  capture_attribute_slots->emplace_back(lepus::Value("onCaptureCatchTap"));

  auto capture_target = manager->CreateFiberView();
  auto capture_template_attributes =
      std::make_shared<const TemplateAttributes>(TemplateAttributes{
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC,
                    base::String("capture-bindtap"), lepus::Value(), 0},
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC,
                    base::String("capture-catchtap"), lepus::Value(), 1}});
  capture_target->SetTemplateAttributes(capture_template_attributes);

  TreeResolver::ApplyTemplateAttributesToElement(
      capture_target.get(), lepus::Value(std::move(capture_attribute_slots)));

  auto capture_event_iter = capture_target->event_map().find("tap");
  ASSERT_NE(capture_event_iter, capture_target->event_map().end());
  EXPECT_EQ(capture_event_iter->second->type(), "capture-catch");
  EXPECT_EQ(capture_event_iter->second->function(), "onCaptureCatchTap");
  EXPECT_EQ(capture_target->data_model_->attributes().count("capture-bindtap"),
            0u);
  EXPECT_EQ(capture_target->data_model_->attributes().count("capture-catchtap"),
            0u);
}

TEST_P(FiberElementTest, ApplyTemplateAttributesEventKeysSyncsEventListeners) {
  manager->config_->SetEnableEventHandleRefactor(true);
  manager->SetConfig(manager->config_);
  tasm->page_config_ = manager->config_;

  auto default_entry = std::make_shared<TemplateEntry>();
  default_entry->SetName(DEFAULT_ENTRY_NAME);
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = default_entry;

  auto page = manager->CreateFiberPage("0", 0);
  manager->SetFiberPageElement(page);
  auto target = manager->CreateFiberView();
  page->InsertNode(target);

  auto attribute_slots = lepus::CArray::Create();
  attribute_slots->emplace_back(lepus::Value("onTap"));

  auto template_attributes =
      std::make_shared<const TemplateAttributes>(TemplateAttributes{
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("bindtap"),
                    lepus::Value(), 0}});
  target->SetTemplateAttributes(template_attributes);

  TreeResolver::ApplyTemplateAttributesToElement(target.get(),
                                                 lepus::Value(attribute_slots));

  auto* listener_map = target->GetEventListenerMap();
  auto* listeners = listener_map->Find("tap");
  ASSERT_NE(listeners, nullptr);
  ASSERT_EQ(listeners->size(), 1u);
  EXPECT_FALSE(listeners->front()->GetOptions().IsCapture());
  EXPECT_FALSE(listeners->front()->GetOptions().IsCatch());

  auto event = fml::MakeRefCounted<event::TouchEvent>("tap");
  auto result = event::EventDispatcher::DispatchEvent(*target, event);
  EXPECT_TRUE(result.consumed);
  EXPECT_NE(tasm_mediator.DumpDelegate().find("SendPageEvent"),
            std::string::npos);

  auto empty_slots = lepus::CArray::Create();
  empty_slots->emplace_back(lepus::Value());
  TreeResolver::ApplyTemplateAttributesToElement(
      target.get(), lepus::Value(attribute_slots), lepus::Value(empty_slots));

  listeners = listener_map->Find("tap");
  EXPECT_TRUE(listeners == nullptr || listeners->empty());
  event = fml::MakeRefCounted<event::TouchEvent>("tap");
  result = event::EventDispatcher::DispatchEvent(*target, event);
  EXPECT_FALSE(result.consumed);
}

TEST_P(FiberElementTest,
       ApplyTemplateSpreadEventAndDataKeysClearsPreviousValues) {
  auto previous_attribute_slots = lepus::CArray::Create();
  auto previous_spread_object = lepus::Dictionary::Create();
  previous_spread_object->SetValue("bindtap", lepus::Value("onTap"));
  previous_spread_object->SetValue("data-test", lepus::Value("old-value"));
  previous_spread_object->SetValue("data-foo-bar",
                                   lepus::Value("old-kebab-value"));
  previous_attribute_slots->emplace_back(lepus::Value(previous_spread_object));

  auto attribute_slots = lepus::CArray::Create();
  auto spread_object = lepus::Dictionary::Create();
  spread_object->SetValue("data-foo-bar", lepus::Value("new-kebab-value"));
  attribute_slots->emplace_back(lepus::Value(spread_object));

  auto target = manager->CreateFiberView();
  auto template_attributes = std::make_shared<const TemplateAttributes>(
      TemplateAttributes{Attribute{ATTRIBUTE_BINDING_TYPE_SPREAD,
                                   base::String("spread"), lepus::Value(), 0}});
  target->SetTemplateAttributes(template_attributes);
  target->SetJSEventHandler("tap", "bindEvent", "onTap");
  target->AddDataset("test", lepus::Value("old-value"));
  target->AddDataset("foo-bar", lepus::Value("old-kebab-value"));

  TreeResolver::ApplyTemplateAttributesToElement(
      target.get(), lepus::Value(std::move(previous_attribute_slots)),
      lepus::Value(std::move(attribute_slots)));

  EXPECT_EQ(target->event_map().count("tap"), 0u);
  EXPECT_EQ(target->data_model_->dataset().count("test"), 0u);
  auto* kebab_data = DatasetValue(target.get(), "foo-bar");
  ASSERT_NE(kebab_data, nullptr);
  EXPECT_EQ(kebab_data->StdString(), "new-kebab-value");
  EXPECT_EQ(target->data_model_->attributes().count("data-test"), 0u);
  EXPECT_EQ(target->data_model_->attributes().count("data-foo-bar"), 0u);
}

TEST_P(FiberElementTest, ApplyTemplateSpreadMainThreadNonEventKeys) {
  auto attribute_slots = lepus::CArray::Create();
  auto spread_object = lepus::Dictionary::Create();
  spread_object->SetValue("main-thread:ref", lepus::Value("ref-value"));
  spread_object->SetValue("main-thread:gesture", lepus::Value("gesture-value"));
  spread_object->SetValue("main-thread:bindtap", lepus::Value("onTap"));
  attribute_slots->emplace_back(lepus::Value(spread_object));

  auto target = manager->CreateFiberView();
  auto template_attributes = std::make_shared<const TemplateAttributes>(
      TemplateAttributes{Attribute{ATTRIBUTE_BINDING_TYPE_SPREAD,
                                   base::String("spread"), lepus::Value(), 0}});
  target->SetTemplateAttributes(template_attributes);

  TreeResolver::ApplyTemplateAttributesToElement(
      target.get(), lepus::Value(std::move(attribute_slots)));

  EXPECT_EQ(target->data_model_->attributes().at("main-thread:ref").StdString(),
            "ref-value");
  EXPECT_EQ(
      target->data_model_->attributes().at("main-thread:gesture").StdString(),
      "gesture-value");
  EXPECT_EQ(target->data_model_->attributes().count("main-thread:bindtap"), 0u);
  auto event_iter = target->event_map().find("tap");
  ASSERT_NE(event_iter, target->event_map().end());
  EXPECT_EQ(event_iter->second->type(), "bindEvent");
  EXPECT_EQ(event_iter->second->function(), "onTap");
}

TEST_P(FiberElementTest, ApplyTemplateAttributesWithSpreadReappliesStatics) {
  auto attribute_slots = lepus::CArray::Create();
  auto spread_object = lepus::Dictionary::Create();
  spread_object->SetValue("class", lepus::Value("spread-class"));
  spread_object->SetValue("data-extra", lepus::Value("spread-extra"));
  spread_object->SetValue("data-later", lepus::Value("spread-later"));
  spread_object->SetValue("id", lepus::Value("spread-id"));
  attribute_slots->emplace_back(lepus::Value(spread_object));

  auto target = manager->CreateFiberView();
  auto template_attributes =
      std::make_shared<const TemplateAttributes>(TemplateAttributes{
          Attribute{ATTRIBUTE_BINDING_TYPE_STATIC, base::String("id"),
                    lepus::Value("static-id"), 0},
          Attribute{ATTRIBUTE_BINDING_TYPE_STATIC, base::String("data-static"),
                    lepus::Value("static-value"), 0},
          Attribute{ATTRIBUTE_BINDING_TYPE_SPREAD, base::String("spread"),
                    lepus::Value(), 0},
          Attribute{ATTRIBUTE_BINDING_TYPE_STATIC, base::String("data-later"),
                    lepus::Value("static-later"), 0},
          Attribute{ATTRIBUTE_BINDING_TYPE_STATIC, base::String("id"),
                    lepus::Value("static-id-after-spread"), 0}});
  target->SetTemplateAttributes(template_attributes);
  target->SetIdSelector("static-id-after-spread");
  target->AddDataset("static", lepus::Value("static-value"));
  target->AddDataset("later", lepus::Value("static-later"));

  TreeResolver::ApplyTemplateAttributesToElement(
      target.get(), lepus::Value(std::move(attribute_slots)));

  EXPECT_EQ(target->GetIdSelector(), "static-id-after-spread");
  EXPECT_EQ(target->classes().size(), 1u);
  EXPECT_EQ(target->classes()[0], "spread-class");
  auto* static_data = DatasetValue(target.get(), "static");
  ASSERT_NE(static_data, nullptr);
  EXPECT_EQ(static_data->StdString(), "static-value");
  auto* extra_data = DatasetValue(target.get(), "extra");
  ASSERT_NE(extra_data, nullptr);
  EXPECT_EQ(extra_data->StdString(), "spread-extra");
  auto* later_data = DatasetValue(target.get(), "later");
  ASSERT_NE(later_data, nullptr);
  EXPECT_EQ(later_data->StdString(), "static-later");
  EXPECT_EQ(target->data_model_->attributes().count("data-static"), 0u);
  EXPECT_EQ(target->data_model_->attributes().count("data-extra"), 0u);
  EXPECT_EQ(target->data_model_->attributes().count("data-later"), 0u);
}

// CSSVariable Demo Structure
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

TEST_P(FiberElementTest, TestEnsureTagInfoInParallelMode) {
  auto page = manager->CreateFiberPage("page", 11);

  for (int32_t i = 0; i < 10000; ++i) {
    auto element = manager->CreateFiberNode(std::to_string(i));
    page->InsertNode(element);
  }

  page->FlushActionsAsRoot();
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

// Verify AttachToElementManager will guarantee that the new ElementManager
// retains all the cloned elements.
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

// Verify animation is re-applied after cloning elements
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

TEST_P(FiberElementTest, FiberAddWorkletEventUsesRegisterContextName) {
  auto page = manager->CreateFiberPage("page", 11);
  auto worklet_info = lepus::Dictionary::Create();
  BASE_STATIC_STRING_DECL(kType, "type");
  BASE_STATIC_STRING_DECL(kValue, "value");
  BASE_STATIC_STRING_DECL(kTap, "tap");
  BASE_STATIC_STRING_DECL(kBindEvent, "bindEvent");
  worklet_info->SetValue(kType, lepus::Value(tasm::kWorklet));
  worklet_info->SetValue(kValue, lepus::Value("worklet"));
  auto* context = tasm->FindEntry(DEFAULT_ENTRY_NAME)->GetVm().get();

  page->FiberAddEvent(kBindEvent, kTap, lepus::Value(worklet_info),
                      DEFAULT_ENTRY_NAME);

  auto it = page->lepus_event_map().find(kTap);
  ASSERT_NE(it, page->lepus_event_map().end());
  EXPECT_EQ(context, it->second->lepus_context());
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

// Test that CSS custom property changes propagate to descendants when
// using the object-based inline style path (FiberSetInlineStyles with object).
// This is the regression test for
// https://github.com/lynx-family/lynx/issues/5889
TEST_P(FiberElementTest, TestObjectPathCSSVariablePropagation) {
  lynx::base::AutoReset<bool> css_inline_config(
      &(manager->GetConfig()->css_configs_.enable_css_inline_variables_), true);

  // page
  // └── view1 (sets --bg-color)
  //     └── view2 (uses var(--bg-color))
  auto page = manager->CreateFiberPage("0", 0);
  auto view1 = manager->CreateFiberView();
  auto view2 = manager->CreateFiberView();
  page->InsertNode(view1);
  view1->InsertNode(view2);

  auto painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->platform_impl_.get());

  {
    // Establish initial state using the string path (known working).
    // view1: "--bg-color: red;"
    // view2: "background-color: var(--bg-color);"
    view1->SetRawInlineStyles("--bg-color: red;");
    view2->SetRawInlineStyles("background-color: var(--bg-color);");
    page->FlushActionsAsRoot();
    painting_context->Flush();

    auto node = painting_context->node_map_[view2->impl_id()].get();
    EXPECT_TRUE(node != nullptr);
    EXPECT_EQ(node->props_["background-color"],
              lepus::Value(static_cast<uint32_t>(kTestColorMap["red"])));
  }

  {
    // Simulate FiberSetInlineStyles object path: update --bg-color to blue.
    // This exercises the code path that was missing dirty-marking.
    view1->RemoveAllInlineStyles();

    CSSVariableMap changed_css_vars;
    view1->data_model()->MoveAndClearCSSInlineVariables(&changed_css_vars);
    view1->data_model()->UpdateCSSInlineVariables("--bg-color", "blue");

    // Dirty-marking (the fix for issue #5889).
    view1->data_model()->UpdateInlineStyleChangedVars(&changed_css_vars);
    if (!changed_css_vars.empty()) {
      auto table = lepus::Dictionary::Create();
      for (const auto& iter : changed_css_vars) {
        table.get()->SetValue(iter.first, iter.second);
      }
      auto css_var_table = lepus::Value(std::move(table));
      view1->MarkCustomPropertiesDirty();
      if (view1->IsRelatedCSSVariableUpdated(view1->data_model(),
                                             css_var_table)) {
        view1->MarkStyleDirty(false);
      }
      view1->RecursivelyMarkChildrenCSSVariableDirty(css_var_table);
    }

    page->FlushActionsAsRoot();
    painting_context->Flush();

    // view2 should now see --bg-color: blue from view1.
    auto node = painting_context->node_map_[view2->impl_id()].get();
    EXPECT_TRUE(node != nullptr);
    EXPECT_EQ(node->props_["background-color"],
              lepus::Value(static_cast<uint32_t>(kTestColorMap["blue"])));
  }
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

TEST_P(FiberElementTest,
       TestGetComputedStyleByKey_transform_multiple_functions) {
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
  view->SetStyle(
      CSSPropertyID::kPropertyIDTransform,
      lepus::Value(
          "translateX(10px) translateY(20px) translateZ(30px) rotateX(15deg) "
          "rotateY(25deg) rotateZ(35deg) scale(1.2) scaleX(1.1) scaleY(0.9) "
          "skewX(10deg) skewY(5deg)"));
  page->InsertNode(view);

  page->FlushActionsAsRoot();
  auto lepus_transform_value = view->GetComputedStyleByKey("transform");
  EXPECT_TRUE(lepus_transform_value.IsString());
  auto matrix_value = lepus_transform_value.StdString();
  EXPECT_TRUE(matrix_value ==
              "matrix3d(0.945973, 0.931536, -0.207071, 0, -0.388628, 0.936588, "
              "0.438571, 0, 0.422618, -0.23457, 0.875426, 0, 10, 20, 30, 1)");
}

// Test case for PageElement holding a ComponentElement, which holds a
// ViewElement. The ComponentElement's CSSFragment has a :root selector with
// CSS variables. Repeatedly call PageElement's FlushActionsAsRoot 1000 times.
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

// Helper: create a SharedCSSFragment with CSS selector support and add a rule.
// The rule text should be a valid CSS selector (e.g. ".a + .b").
static std::shared_ptr<SharedCSSFragment> MakeFragmentWithRule(
    const std::string& selector_text) {
  auto fragment = std::make_shared<SharedCSSFragment>();
  fragment->SetEnableCSSSelector();

  css::CSSParserContext context;
  css::CSSTokenizer tokenizer(selector_text);
  const auto tokens = tokenizer.TokenizeToEOF();
  css::CSSParserTokenRange range(tokens);
  css::LynxCSSSelectorVector vector =
      css::CSSSelectorParser::ParseSelector(range, &context);
  size_t flattened_size = css::CSSSelectorParser::FlattenedSize(vector);
  auto selector_array =
      std::make_unique<css::LynxCSSSelector[]>(flattened_size);
  css::CSSSelectorParser::AdoptSelectorVector(vector, selector_array.get(),
                                              flattened_size);
  fragment->AddStyleRule(std::move(selector_array), nullptr);
  return fragment;
}

// Verify that removing a child marks its next sibling as style-dirty when
// adjacent sibling rules (+ combinator) exist.
TEST_P(FiberElementTest, RemoveNode_InvalidatesNextSibling_WithAdjacentRules) {
  auto fragment = MakeFragmentWithRule(".a + .b");

  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ = std::make_unique<CSSFragmentDecorator>(fragment.get());

  auto child1 = manager->CreateFiberNode("view");
  child1->parent_component_element_ = page.get();
  page->InsertNode(child1);

  auto child2 = manager->CreateFiberNode("view");
  child2->parent_component_element_ = page.get();
  page->InsertNode(child2);

  auto child3 = manager->CreateFiberNode("view");
  child3->parent_component_element_ = page.get();
  page->InsertNode(child3);

  page->FlushActionsAsRoot();

  // Sanity: child3 should not be style-dirty after flush.
  EXPECT_FALSE(child3->StyleDirty());

  // Remove child2 — child3 (the next sibling of the removed node) should
  // become style-dirty because adjacent sibling rules exist.
  page->RemoveNode(child2);
  EXPECT_TRUE(child3->StyleDirty());
}

// Verify that removing a child does NOT mark the next sibling as style-dirty
// when no adjacent sibling rules exist.
TEST_P(FiberElementTest,
       RemoveNode_DoesNotInvalidateNextSibling_WithoutAdjacentRules) {
  auto fragment = MakeFragmentWithRule(".a .b");

  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ = std::make_unique<CSSFragmentDecorator>(fragment.get());

  auto child1 = manager->CreateFiberNode("view");
  child1->parent_component_element_ = page.get();
  page->InsertNode(child1);

  auto child2 = manager->CreateFiberNode("view");
  child2->parent_component_element_ = page.get();
  page->InsertNode(child2);

  auto child3 = manager->CreateFiberNode("view");
  child3->parent_component_element_ = page.get();
  page->InsertNode(child3);

  page->FlushActionsAsRoot();

  EXPECT_FALSE(child3->StyleDirty());

  // Remove child2 — child3 should NOT become style-dirty because there are
  // no adjacent sibling rules.
  page->RemoveNode(child2);
  EXPECT_FALSE(child3->StyleDirty());
}

// Verify that inserting a node before a ref_node marks ref_node as style-dirty
// when adjacent sibling rules exist.
TEST_P(FiberElementTest,
       InsertNodeBefore_InvalidatesRefNode_WithAdjacentRules) {
  auto fragment = MakeFragmentWithRule(".a + .b");

  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ = std::make_unique<CSSFragmentDecorator>(fragment.get());

  auto child1 = manager->CreateFiberNode("view");
  child1->parent_component_element_ = page.get();
  page->InsertNode(child1);

  auto child2 = manager->CreateFiberNode("view");
  child2->parent_component_element_ = page.get();
  page->InsertNode(child2);

  page->FlushActionsAsRoot();

  EXPECT_FALSE(child2->StyleDirty());

  // Insert a new node before child2 — child2 (the ref_node) should become
  // style-dirty because adjacent sibling rules exist.
  auto new_child = manager->CreateFiberNode("view");
  new_child->parent_component_element_ = page.get();
  page->InsertNodeBefore(new_child, child2);
  EXPECT_TRUE(child2->StyleDirty());
}

// Verify that inserting a node before a ref_node does NOT mark ref_node as
// style-dirty when no adjacent sibling rules exist.
TEST_P(FiberElementTest,
       InsertNodeBefore_DoesNotInvalidateRefNode_WithoutAdjacentRules) {
  auto fragment = MakeFragmentWithRule(".a > .b");

  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ = std::make_unique<CSSFragmentDecorator>(fragment.get());

  auto child1 = manager->CreateFiberNode("view");
  child1->parent_component_element_ = page.get();
  page->InsertNode(child1);

  auto child2 = manager->CreateFiberNode("view");
  child2->parent_component_element_ = page.get();
  page->InsertNode(child2);

  page->FlushActionsAsRoot();

  EXPECT_FALSE(child2->StyleDirty());

  // Insert a new node before child2 — child2 should NOT become style-dirty
  // because no adjacent sibling rules exist.
  auto new_child = manager->CreateFiberNode("view");
  new_child->parent_component_element_ = page.get();
  page->InsertNodeBefore(new_child, child2);
  EXPECT_FALSE(child2->StyleDirty());
}

// Verify that removing the last child does not crash (no next sibling).
TEST_P(FiberElementTest, RemoveNode_LastChild_NoCrash) {
  auto fragment = MakeFragmentWithRule(".a + .b");

  auto page = manager->CreateFiberPage("page", 11);
  page->style_sheet_ = std::make_unique<CSSFragmentDecorator>(fragment.get());

  auto child1 = manager->CreateFiberNode("view");
  child1->parent_component_element_ = page.get();
  page->InsertNode(child1);

  page->FlushActionsAsRoot();

  // Remove the only child — should not crash even with adjacent rules.
  page->RemoveNode(child1);
  SUCCEED();
}

TEST_P(FiberElementTest,
       NewStylingResolveComputedStylesBindsFirstRenderAndUpdateStorage) {
  auto page = manager->CreateFiberPage("page", 11);
  manager->SetFiberPageElement(page);
  auto element = manager->CreateFiberView();
  page->InsertNode(element);

  element->SetStyle(CSSPropertyID::kPropertyIDWidth, lepus::Value("40px"));
  const auto* previous_style = element->computed_css_style();
  auto first_result = element->ResolveComputedStyles(
      previous_style, previous_style->GetFontSize(),
      previous_style->GetRootFontSize());

  EXPECT_EQ(first_result.previous_final_style, previous_style);
  EXPECT_EQ(first_result.parent_inheritance_style, page->computed_css_style());
  EXPECT_EQ(first_result.final_style, element->platform_css_style_.get());
  EXPECT_EQ(first_result.base_style, element->platform_css_style_.get());
  EXPECT_EQ(first_result.owned_base_style, nullptr);
  EXPECT_TRUE(StyleMapHasValue(first_result.resolved_style_map,
                               CSSPropertyID::kPropertyIDWidth,
                               CSSValue(40, CSSValuePattern::PX)));
  EXPECT_TRUE(StyleMapHasValue(first_result.final_style->GetResolvedValues(),
                               CSSPropertyID::kPropertyIDWidth,
                               CSSValue(40, CSSValuePattern::PX)));

  element->ResetAllDirtyBits();
  element->SetStyle(CSSPropertyID::kPropertyIDWidth, lepus::Value("56px"));
  auto update_result = element->ResolveComputedStyles(
      previous_style, previous_style->GetFontSize(),
      previous_style->GetRootFontSize());

  ASSERT_NE(update_result.owned_base_style, nullptr);
  EXPECT_EQ(update_result.final_style, update_result.owned_base_style.get());
  EXPECT_EQ(update_result.base_style, update_result.owned_base_style.get());
  EXPECT_TRUE(StyleMapHasValue(update_result.resolved_style_map,
                               CSSPropertyID::kPropertyIDWidth,
                               CSSValue(56, CSSValuePattern::PX)));
}

TEST_P(FiberElementTest,
       NewStylingReplaySingleChangedAndResetStyleSideEffects) {
  auto element = manager->CreateFiberView();

  element->ReplayChangedStyleSideEffect(CSSPropertyID::kPropertyIDWidth,
                                        CSSValue(1, CSSValuePattern::VW));

  EXPECT_TRUE(LayoutBundleHasStyle(element->layout_bundle_,
                                   CSSPropertyID::kPropertyIDWidth,
                                   CSSValue(1, CSSValuePattern::VW)));
  EXPECT_TRUE(element->dynamic_style_flags_ &
              DynamicCSSStylesManager::kUpdateViewport);

  element->has_layout_only_props_ = true;
  element->ReplayChangedStyleSideEffect(CSSPropertyID::kPropertyIDOpacity,
                                        CSSValue(0.5, CSSValuePattern::NUMBER));
  EXPECT_FALSE(element->has_layout_only_props_);

  element->has_non_flatten_attrs_ = false;
  element->ReplayChangedStyleSideEffect(CSSPropertyID::kPropertyIDTransform,
                                        CSSValue::MakePlainString("test"));
  EXPECT_TRUE(element->has_non_flatten_attrs_);

  element->layout_bundle_.reset();
  element->is_fixed_ = true;
  element->is_sticky_ = true;
  element->fixed_changed_ = false;
  element->dynamic_style_flags_ = DynamicCSSStylesManager::kUpdateViewport;

  element->ReplayResetStyleSideEffect(CSSPropertyID::kPropertyIDPosition);

  EXPECT_TRUE(LayoutBundleHasResetStyle(element->layout_bundle_,
                                        CSSPropertyID::kPropertyIDPosition));
  EXPECT_TRUE(element->fixed_changed_);
  EXPECT_FALSE(element->is_fixed_);
  EXPECT_FALSE(element->is_sticky_);
}

TEST_P(FiberElementTest,
       NewStylingReplayCommitSideEffectsIncludesInheritedFirstRenderValues) {
  manager->config_->SetEnableCSSInheritance(true);
  auto element = manager->CreateFiberView();
  element->dirty_ = Element::kDirtyCreated;

  starlight::ComputedCSSStyle computed_style(*manager->platform_computed_css());
  computed_style.SetResolvedValue(CSSPropertyID::kPropertyIDWidth,
                                  CSSValue(32, CSSValuePattern::PX));
  computed_style.SetResolvedValue(CSSPropertyID::kPropertyIDDirection,
                                  CSSValue(starlight::DirectionType::kRtl));

  StyleMap resolved_style_map;
  resolved_style_map.insert_or_assign(CSSPropertyID::kPropertyIDWidth,
                                      CSSValue(32, CSSValuePattern::PX));

  element->ReplayCommitSideEffects(computed_style, resolved_style_map);

  EXPECT_TRUE(LayoutBundleHasStyle(element->layout_bundle_,
                                   CSSPropertyID::kPropertyIDWidth,
                                   CSSValue(32, CSSValuePattern::PX)));
  EXPECT_TRUE(LayoutBundleHasStyle(element->layout_bundle_,
                                   CSSPropertyID::kPropertyIDDirection,
                                   CSSValue(starlight::DirectionType::kRtl)));
}

TEST_P(FiberElementTest,
       NewStylingReplayCommitSideEffectsUsesResolvedValuesAndResetsOnUpdate) {
  auto element = manager->CreateFiberView();
  element->ResetAllDirtyBits();

  starlight::ComputedCSSStyle computed_style(*manager->platform_computed_css());
  computed_style.SetResolvedValue(CSSPropertyID::kPropertyIDDirection,
                                  CSSValue(starlight::DirectionType::kRtl));
  computed_style.MarkChanged(CSSPropertyID::kPropertyIDWidth);
  computed_style.MarkChanged(CSSPropertyID::kPropertyIDDirection);
  computed_style.MarkReset(CSSPropertyID::kPropertyIDHeight);
  computed_style.MarkReset(CSSPropertyID::kPropertyIDPaddingTop);

  StyleMap resolved_style_map;
  resolved_style_map.insert_or_assign(CSSPropertyID::kPropertyIDWidth,
                                      CSSValue(44, CSSValuePattern::PX));
  resolved_style_map.insert_or_assign(CSSPropertyID::kPropertyIDPaddingTop,
                                      CSSValue(8, CSSValuePattern::PX));

  element->ReplayCommitSideEffects(computed_style, resolved_style_map);

  EXPECT_TRUE(LayoutBundleHasStyle(element->layout_bundle_,
                                   CSSPropertyID::kPropertyIDWidth,
                                   CSSValue(44, CSSValuePattern::PX)));
  EXPECT_TRUE(LayoutBundleHasStyle(element->layout_bundle_,
                                   CSSPropertyID::kPropertyIDDirection,
                                   CSSValue(starlight::DirectionType::kRtl)));
  EXPECT_TRUE(LayoutBundleHasResetStyle(element->layout_bundle_,
                                        CSSPropertyID::kPropertyIDHeight));
  EXPECT_FALSE(LayoutBundleHasResetStyle(element->layout_bundle_,
                                         CSSPropertyID::kPropertyIDPaddingTop));
}

TEST_P(FiberElementTest,
       NewStylingResolveCSSStylesNewPipelineReplaysLayoutOnlyResets) {
  auto page = manager->CreateFiberPage("page", 11);
  manager->SetFiberPageElement(page);
  auto element = manager->CreateFiberView();
  page->InsertNode(element);

  element->platform_css_style_->SetResolvedValue(
      CSSPropertyID::kPropertyIDWidth, CSSValue(20, CSSValuePattern::PX));
  element->platform_css_style_->ClearChanged();
  element->platform_css_style_->ClearReset();
  element->dirty_ = Element::kDirtyStyle;

  bool need_update = false;
  element->ResolveCSSStylesNewPipeline(need_update);

  EXPECT_TRUE(need_update);
  EXPECT_TRUE(LayoutBundleHasResetStyle(element->layout_bundle_,
                                        CSSPropertyID::kPropertyIDWidth));
}

TEST_P(FiberElementTest,
       NewStylingReplayDynamicResolvedStyleSideEffectsSkipsCommittedIds) {
  auto element = manager->CreateFiberView();

  StyleMap resolved_style_map;
  resolved_style_map.insert_or_assign(CSSPropertyID::kPropertyIDWidth,
                                      CSSValue(1, CSSValuePattern::VW));
  resolved_style_map.insert_or_assign(CSSPropertyID::kPropertyIDTop,
                                      CSSValue(2, CSSValuePattern::VW));
  resolved_style_map.insert_or_assign(CSSPropertyID::kPropertyIDFontSize,
                                      CSSValue(3, CSSValuePattern::VW));

  CSSIDBitset replayed_ids;
  replayed_ids.Set(CSSPropertyID::kPropertyIDWidth);
  element->ReplayDynamicResolvedStyleSideEffects(
      resolved_style_map, DynamicCSSStylesManager::kUpdateViewport,
      replayed_ids);

  EXPECT_FALSE(LayoutBundleHasStyle(element->layout_bundle_,
                                    CSSPropertyID::kPropertyIDWidth,
                                    CSSValue(1, CSSValuePattern::VW)));
  EXPECT_TRUE(LayoutBundleHasStyle(element->layout_bundle_,
                                   CSSPropertyID::kPropertyIDTop,
                                   CSSValue(2, CSSValuePattern::VW)));
  EXPECT_FALSE(LayoutBundleHasStyle(element->layout_bundle_,
                                    CSSPropertyID::kPropertyIDFontSize,
                                    CSSValue(3, CSSValuePattern::VW)));
}

TEST_P(FiberElementTest, NewStylingCollectDynamicFlagsForResolvedStyleMap) {
  auto element = manager->CreateFiberView();

  StyleMap resolved_style_map;
  resolved_style_map.insert_or_assign(CSSPropertyID::kPropertyIDWidth,
                                      CSSValue(10, CSSValuePattern::VW));
  resolved_style_map.insert_or_assign(CSSPropertyID::kPropertyIDHeight,
                                      CSSValue(20, CSSValuePattern::PX));

  const auto flags =
      element->CollectDynamicFlagsForNewPipeline(resolved_style_map);

  EXPECT_TRUE(flags & DynamicCSSStylesManager::kUpdateViewport);
}

TEST_P(FiberElementTest,
       NewStylingCollectsDynamicFlagsFromInheritedResolvedValues) {
  manager->enable_new_styling_pipeline_ = true;
  manager->config_->SetEnableCSSInheritance(true);
  manager->UpdateViewport(100, SLMeasureModeDefinite, 600,
                          SLMeasureModeDefinite, false);

  auto page = manager->CreateFiberPage("page", 11);
  manager->SetFiberPageElement(page);
  auto parent = manager->CreateFiberView();
  auto child = manager->CreateFiberView();
  parent->SetStyle(CSSPropertyID::kPropertyIDLineHeight, lepus::Value("10vw"));

  parent->InsertNode(child);
  page->InsertNode(parent);
  page->FlushActionsAsRoot();

  EXPECT_TRUE(parent->dynamic_style_flags_ &
              DynamicCSSStylesManager::kUpdateViewport);
  EXPECT_TRUE(child->dynamic_style_flags_ &
              DynamicCSSStylesManager::kUpdateViewport);
}

TEST_P(FiberElementTest,
       NewStylingExplicitStyleOverridesInheritedDynamicFlags) {
  manager->enable_new_styling_pipeline_ = true;
  manager->config_->SetEnableCSSInheritance(true);
  manager->UpdateViewport(100, SLMeasureModeDefinite, 600,
                          SLMeasureModeDefinite, false);

  auto page = manager->CreateFiberPage("page", 11);
  manager->SetFiberPageElement(page);
  auto parent = manager->CreateFiberView();
  auto child = manager->CreateFiberView();
  parent->SetStyle(CSSPropertyID::kPropertyIDLineHeight, lepus::Value("10vw"));
  child->SetStyle(CSSPropertyID::kPropertyIDLineHeight, lepus::Value("18px"));

  parent->InsertNode(child);
  page->InsertNode(parent);
  page->FlushActionsAsRoot();

  EXPECT_TRUE(parent->dynamic_style_flags_ &
              DynamicCSSStylesManager::kUpdateViewport);
  EXPECT_FALSE(child->dynamic_style_flags_ &
               DynamicCSSStylesManager::kUpdateViewport);
}

TEST_P(FiberElementTest,
       NewStylingInheritedPropertyUpdatePropagatesToChildren) {
  manager->enable_new_styling_pipeline_ = true;
  manager->config_->SetEnableCSSInheritance(true);

  auto page = manager->CreateFiberPage("page", 11);
  manager->SetFiberPageElement(page);
  auto parent = manager->CreateFiberView();
  auto child = manager->CreateFiberView();

  parent->SetStyle(CSSPropertyID::kPropertyIDColor, lepus::Value("red"));
  parent->InsertNode(child);
  page->InsertNode(parent);
  page->FlushActionsAsRoot();

  const auto first_child_color = child->GetComputedStyleByKey("color");
  EXPECT_EQ(first_child_color.StdString(),
            parent->GetComputedStyleByKey("color").StdString());

  parent->SetStyle(CSSPropertyID::kPropertyIDColor, lepus::Value("blue"));
  page->FlushActionsAsRoot();

  const auto updated_child_color = child->GetComputedStyleByKey("color");
  EXPECT_EQ(updated_child_color.StdString(),
            parent->GetComputedStyleByKey("color").StdString());
  EXPECT_NE(first_child_color.StdString(), updated_child_color.StdString());
}

TEST_P(FiberElementTest,
       NewStylingInheritedPropertyUpdateKeepsChildExplicitStyle) {
  manager->enable_new_styling_pipeline_ = true;
  manager->config_->SetEnableCSSInheritance(true);

  auto page = manager->CreateFiberPage("page", 11);
  manager->SetFiberPageElement(page);
  auto parent = manager->CreateFiberView();
  auto child = manager->CreateFiberView();

  parent->SetStyle(CSSPropertyID::kPropertyIDColor, lepus::Value("red"));
  child->SetStyle(CSSPropertyID::kPropertyIDColor, lepus::Value("green"));
  parent->InsertNode(child);
  page->InsertNode(parent);
  page->FlushActionsAsRoot();

  const auto explicit_child_color = child->GetComputedStyleByKey("color");

  parent->SetStyle(CSSPropertyID::kPropertyIDColor, lepus::Value("blue"));
  page->FlushActionsAsRoot();

  EXPECT_EQ(explicit_child_color.StdString(),
            child->GetComputedStyleByKey("color").StdString());
  EXPECT_NE(parent->GetComputedStyleByKey("color").StdString(),
            child->GetComputedStyleByKey("color").StdString());
}

TEST_P(FiberElementTest, NewStylingSideApisReadCommittedInheritedStyle) {
  manager->enable_new_styling_pipeline_ = true;
  manager->config_->SetEnableCSSInheritance(true);

  auto page = manager->CreateFiberPage("page", 11);
  manager->SetFiberPageElement(page);
  auto parent = manager->CreateFiberView();
  auto child = manager->CreateFiberView();

  parent->SetStyle(CSSPropertyID::kPropertyIDColor, lepus::Value("red"));
  parent->InsertNode(child);
  page->InsertNode(parent);
  page->FlushActionsAsRoot();

  auto inherited_color =
      child->GetElementStyle(CSSPropertyID::kPropertyIDColor);
  ASSERT_TRUE(inherited_color.has_value());
  EXPECT_TRUE(StyleMapHasValue(child->GetStylesForWorklet(),
                               CSSPropertyID::kPropertyIDColor,
                               *inherited_color));

  parent->SetStyle(CSSPropertyID::kPropertyIDColor, lepus::Value("blue"));
  page->FlushActionsAsRoot();

  auto updated_color = child->GetElementStyle(CSSPropertyID::kPropertyIDColor);
  ASSERT_TRUE(updated_color.has_value());
  EXPECT_TRUE(StyleMapHasValue(child->GetStylesForWorklet(),
                               CSSPropertyID::kPropertyIDColor,
                               *updated_color));
  EXPECT_NE(CSSDecoder::CSSValueToString(CSSPropertyID::kPropertyIDColor,
                                         *inherited_color),
            CSSDecoder::CSSValueToString(CSSPropertyID::kPropertyIDColor,
                                         *updated_color));
}

TEST_P(FiberElementTest,
       NewStylingResolveCoreReportsFontSizeChangesForChildren) {
  auto page = manager->CreateFiberPage("page", 11);
  manager->SetFiberPageElement(page);
  auto element = manager->CreateFiberView();
  page->InsertNode(element);
  element->ResetAllDirtyBits();
  element->SetStyle(CSSPropertyID::kPropertyIDFontSize, lepus::Value("20px"));

  FiberElement::NewPipelineResolveRequest request;
  auto outcome = element->ResolveCSSStylesNewPipelineCore(request);

  EXPECT_TRUE(outcome.need_update);
  EXPECT_TRUE(outcome.force_children);
  EXPECT_TRUE(outcome.child_update_flags & DynamicCSSStylesManager::kUpdateEm);
}

TEST_P(FiberElementTest,
       NewStylingParallelFlushFallsBackWithoutLevelOrderTraversing) {
  auto page = manager->CreateFiberPage("page", 11);

  manager->SetEnableParallelElement(true);
  manager->enable_new_styling_pipeline_ = true;
  manager->SetEnableLevelOrderTraversing(false);
  EXPECT_TRUE(page->ShouldFallbackToSerialForNewStylingPipeline());

  manager->SetEnableLevelOrderTraversing(true);
  EXPECT_FALSE(page->ShouldFallbackToSerialForNewStylingPipeline());

  manager->SetEnableParallelElement(false);
  manager->SetEnableLevelOrderTraversing(false);
  EXPECT_FALSE(page->ShouldFallbackToSerialForNewStylingPipeline());
}

INSTANTIATE_TEST_SUITE_P(FiberElementTestModule, FiberElementTest,
                         ::testing::ValuesIn(fiber_element_generation_params));

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
