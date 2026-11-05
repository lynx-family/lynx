// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/testing/element_template_instance_test.h"

#include <utility>

#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "core/public/pipeline_option.h"
#include "core/renderer/css/css_style_sheet_manager.h"
#include "core/renderer/css/ng/parser/css_tokenizer.h"
#include "core/renderer/css/ng/selector/css_parser_context.h"
#include "core/renderer/css/ng/selector/css_selector_parser.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/events/gesture.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/template_entry.h"
#include "core/renderer/utils/base/element_template_info.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/runtime/lepus/bindings/renderer_functions.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/shell/runtime/mts/mts_runtime.h"

#undef protected
#undef private

namespace lynx {
namespace tasm {
namespace testing {

std::shared_ptr<runtime::MTSRuntime>
ElementTemplateInstanceTest::CreatePAPIRuntime() {
  auto runtime = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  runtime->Initialize();
  runtime->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  return runtime;
}

fml::RefPtr<ElementTemplateInstance>
ElementTemplateInstanceTest::CreateCompiledSpreadInstance(bool nested_target) {
  auto entry = std::make_shared<TemplateEntry>();
  entry->SetName(DEFAULT_ENTRY_NAME);
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = entry;

  auto info = std::make_shared<ElementTemplateInfo>();
  info->exist_ = true;
  info->key_ = "spread_template";
  auto root_info = ElementInfo();
  root_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  root_info.attributes_ = std::make_shared<const TemplateAttributes>(
      TemplateAttributes{Attribute{ATTRIBUTE_BINDING_TYPE_SPREAD,
                                   base::String("spread"), lepus::Value(), 0}});
  if (nested_target) {
    ElementInfo parent;
    parent.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
    parent.children_.emplace_back(std::move(root_info));
    info->elements_.emplace_back(std::move(parent));
  } else {
    info->elements_.emplace_back(std::move(root_info));
  }
  entry->template_bundle_.element_template_infos_["spread_template"] = info;

  auto instance = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  instance->SetTASM(tasm.get());
  instance->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  instance->SetTemplateKey(base::String("spread_template"));
  return instance;
}

fml::RefPtr<ElementTemplateInstance>
ElementTemplateInstanceTest::CreateCompiledCallbackInstance(
    const std::shared_ptr<runtime::MTSRuntime>& runtime, bool nested_target,
    const char* key) {
  auto instance = CreateCompiledSpreadInstance(nested_target);
  const auto& entry = tasm->template_entries_[DEFAULT_ENTRY_NAME];
  entry->SetVm(runtime);
  auto& root =
      entry->template_bundle_.element_template_infos_["spread_template"]
          ->elements_[0];
  auto& target = nested_target ? root.children_[0] : root;
  target.attributes_ =
      std::make_shared<const TemplateAttributes>(TemplateAttributes{
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String(key),
                    lepus::Value(), 0},
          Attribute{ATTRIBUTE_BINDING_TYPE_STATIC, base::String("flatten"),
                    lepus::Value(true), 0},
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("data-label"),
                    lepus::Value(), 1}});
  return instance;
}

lepus::Value ElementTemplateInstanceTest::GestureDescriptor(
    std::initializer_list<int> ids, int callback_version, int entry_count) {
  auto gestures = lepus::CArray::Create();
  for (auto id : ids) {
    auto callbacks = lepus::CArray::Create();
    for (int index = 0; index < entry_count; ++index) {
      auto callback = lepus::Dictionary::Create();
      callback->SetValue("name",
                         lepus::Value(index == 0 ? "onUpdate" : "onEnd"));
      auto worklet = lepus::Dictionary::Create();
      worklet->SetValue("version", lepus::Value(callback_version + index));
      callback->SetValue("callback", lepus::Value(worklet));
      callbacks->emplace_back(lepus::Value(callback));
    }
    auto config = lepus::Dictionary::Create();
    config->SetValue("enabled", lepus::Value(callback_version != 0));
    auto relations = lepus::Dictionary::Create();
    for (const auto* key :
         {kGestureWaitFor, kGestureSimultaneous, kGestureContinueWith}) {
      auto relation_ids = lepus::CArray::Create();
      for (int index = 0; index < entry_count; ++index) {
        relation_ids->emplace_back(lepus::Value(id + 10 + index));
      }
      relations->SetValue(key, lepus::Value(relation_ids));
    }
    auto gesture = lepus::Dictionary::Create();
    gesture->SetValue("id", lepus::Value(id));
    gesture->SetValue("type", lepus::Value(static_cast<int>(GestureType::PAN)));
    gesture->SetValue("config", lepus::Value(config));
    gesture->SetValue("callbacks", lepus::Value(callbacks));
    gesture->SetValue("relationMap", lepus::Value(relations));
    gestures->emplace_back(lepus::Value(gesture));
  }
  auto descriptor = lepus::Dictionary::Create();
  descriptor->SetValue("type", lepus::Value("gesture"));
  descriptor->SetValue("value", lepus::Value(gestures));
  return lepus::Value(descriptor);
}

void ElementTemplateInstanceTest::CreateCSSEntry(const char* name,
                                                 const char* default_color,
                                                 const char* scoped_color) {
  auto entry = std::make_shared<TemplateEntry>();
  entry->SetName(name);
  auto styles = std::make_shared<CSSStyleSheetManager>(nullptr);
  for (int32_t css_id : {0, 42}) {
    auto fragment = std::make_unique<SharedCSSFragment>(css_id);
    fragment->SetEnableCSSSelector();
    auto token = fml::MakeRefCounted<CSSParseToken>(CSSParserConfigs{});
    const auto color = css_id == 0 ? default_color : scoped_color;
    token->raw_attributes_[kPropertyIDBackgroundColor] =
        CSSValue::MakePlainString(color);
    token->raw_attributes_[kPropertyIDColor] = CSSValue::MakePlainString(color);
    token->raw_attributes_[kPropertyIDFontWeight] =
        CSSValue::MakePlainString("700");
    css::CSSParserContext context;
    css::CSSTokenizer tokenizer(".css-owner");
    const auto tokens = tokenizer.TokenizeToEOF();
    css::CSSParserTokenRange range(tokens);
    auto selectors = css::CSSSelectorParser::ParseSelector(range, &context);
    const auto size = css::CSSSelectorParser::FlattenedSize(selectors);
    auto selector_array = std::make_unique<css::LynxCSSSelector[]>(size);
    css::CSSSelectorParser::AdoptSelectorVector(selectors, selector_array.get(),
                                                size);
    fragment->AddStyleRule(std::move(selector_array), token);
    styles->AddSharedCSSFragment(std::move(fragment));
  }
  entry->template_bundle_.css_style_manager_ = std::move(styles);
  for (bool scoped : {false, true}) {
    auto info = std::make_shared<ElementTemplateInfo>();
    info->exist_ = true;
    info->key_ = scoped ? "scoped" : "default";
    ElementInfo root;
    root.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
    root.class_selector_ = {base::String("css-owner")};
    if (scoped) {
      root.attributes_ =
          std::make_shared<const TemplateAttributes>(TemplateAttributes{
              Attribute{ATTRIBUTE_BINDING_TYPE_STATIC, base::String("css-id"),
                        lepus::Value(42), 0}});
    }
    ElementInfo text;
    text.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_TEXT;
    text.class_selector_ = {base::String("css-owner")};
    root.children_.emplace_back(std::move(text));
    ElementInfo slot;
    slot.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_SLOT;
    slot.slot_index_ = 0;
    root.children_.emplace_back(std::move(slot));
    info->elements_.emplace_back(std::move(root));
    entry->template_bundle_.element_template_infos_[info->key_] = info;
  }
  tasm->template_entries_[name] = std::move(entry);
}

fml::RefPtr<ElementTemplateInstance>
ElementTemplateInstanceTest::CreateCSSInstance(const char* entry, bool scoped) {
  auto instance = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  instance->SetTASM(tasm.get());
  instance->SetBundleUrl(base::String(entry));
  instance->SetTemplateKey(base::String(scoped ? "scoped" : "default"));
  return instance;
}

void ElementTemplateInstanceTest::ExpectCSSColor(
    ElementTemplateInstance* instance, uint32_t color) {
  auto root = instance->PeekMaterializedRoot();
  ASSERT_NE(root, nullptr);
  ASSERT_FALSE(root->children().empty());
  auto* text = root->children()[0].get();
  platform_impl_->Flush();
  // Background is not inherited: both nodes must match their own CSS rule.
  for (auto* node : {root.get(), text}) {
    const auto found = platform_impl_->node_map_.find(node->impl_id());
    ASSERT_NE(found, platform_impl_->node_map_.end());
    const auto& props = found->second->props_;
    const auto background = props.find("background-color");
    ASSERT_NE(background, props.end());
    EXPECT_EQ(background->second.UInt32(), color);
  }
  const auto& text_props =
      platform_impl_->node_map_.at(text->impl_id())->props_;
  const auto foreground = text_props.find("color");
  ASSERT_NE(foreground, text_props.end());
  EXPECT_EQ(foreground->second.UInt32(), color);
}

std::vector<fml::RefPtr<ElementTemplateInstance>>
ElementTemplateInstanceTest::CreateCompiledMountChain(size_t length) {
  auto entry = std::make_shared<TemplateEntry>();
  entry->SetName(DEFAULT_ENTRY_NAME);
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = entry;
  auto info = std::make_shared<ElementTemplateInfo>();
  info->exist_ = true;
  info->key_ = "mount_chain";
  ElementInfo root;
  root.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  root.attributes_ =
      std::make_shared<const TemplateAttributes>(TemplateAttributes{
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC,
                    base::String("main-thread:ref"), lepus::Value(), 0}});
  ElementInfo slot;
  slot.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_SLOT;
  slot.slot_index_ = 0;
  root.children_.emplace_back(std::move(slot));
  info->elements_.emplace_back(std::move(root));
  entry->template_bundle_.element_template_infos_[info->key_] = info;

  std::vector<fml::RefPtr<ElementTemplateInstance>> chain;
  for (size_t i = 0; i < length; ++i) {
    auto instance = fml::AdoptRef<ElementTemplateInstance>(
        new ElementTemplateInstance(manager));
    instance->SetTASM(tasm.get());
    instance->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
    instance->SetTemplateKey(base::String("mount_chain"));
    if (!chain.empty()) {
      chain.back()->InsertNodeIntoChildSlot(0, lepus::Value(instance),
                                            lepus::Value());
    }
    chain.emplace_back(std::move(instance));
  }
  return chain;
}

void ElementTemplateInstanceTest::
    ExpectPageFlushCompletesNestedAttrSlotMaterialization(
        bool query_before_insertion) {
  auto chain = CreateCompiledMountChain(5);
  // page -> outer -> inner -> parent -> middle -> leaf. The two lazy
  // ancestors keep middle outside the physical flush scope in the first pass.
  auto& outer = chain[0];
  auto& parent = chain[2];
  auto& middle = chain[3];
  auto& leaf = chain[4];
  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->SetUid(lepus::Value(0));
  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);
  if (!query_before_insertion) {
    page->InsertNodeIntoChildSlot(0, lepus::Value(outer), lepus::Value());
  }
  auto runtime = CreatePAPIRuntime();
  auto* ctx = runtime::MTSRuntime::ToQuickContext(runtime.get());
  for (const auto& instance : {middle, parent}) {
    lepus::Value args[] = {lepus::Value(instance), lepus::Value(0)};
    ASSERT_TRUE(RendererFunctions::FiberGetElementByAttrSlot(ctx, args, 2)
                    .IsRefCounted());
  }
  if (query_before_insertion) {
    page->InsertNodeIntoChildSlot(0, lepus::Value(outer), lepus::Value());
  }
  ASSERT_EQ(manager->pending_element_template_child_mounts_.size(), 2u);
  EXPECT_EQ(manager->pending_element_template_child_mounts_.front().get(),
            query_before_insertion ? middle.get() : page.get());
  EXPECT_EQ(leaf->PeekMaterializedRoot(), nullptr);
  EXPECT_FALSE(parent->HasPendingChildMounts());
  EXPECT_TRUE(middle->HasPendingChildMounts());

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page_root.get());

  auto expected_parent = page_root;
  for (const auto& instance : chain) {
    auto root = instance->PeekMaterializedRoot();
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->parent(), expected_parent.get());
    ASSERT_EQ(expected_parent->children().size(), 1u);
    EXPECT_EQ(expected_parent->children()[0].get(), root.get());
    EXPECT_FALSE(instance->HasPendingChildMounts());
    expected_parent = std::move(root);
  }
  EXPECT_FALSE(page->HasPendingChildMounts());
  EXPECT_TRUE(manager->pending_element_template_child_mounts_.empty());
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
