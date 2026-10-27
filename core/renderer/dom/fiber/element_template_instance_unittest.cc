// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/fiber/element_template_instance.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <type_traits>
#include <vector>

#include "core/event/event_dispatcher.h"
#include "core/event/touch_event.h"
#include "core/public/pipeline_option.h"
#include "core/renderer/css/css_style_sheet_manager.h"
#include "core/renderer/css/ng/parser/css_tokenizer.h"
#include "core/renderer/css/ng/selector/css_parser_context.h"
#include "core/renderer/css/ng/selector/css_selector_parser.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/dom/fiber/list_item_scheduler_adapter.h"
#include "core/renderer/dom/fiber/page_element.h"
#include "core/renderer/dom/fiber/tree_resolver.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/testing/fiber_element_test.h"
#include "core/renderer/events/closure_event_listener.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/template_entry.h"
#include "core/renderer/utils/base/element_template_info.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/runtime/common/bindings/event/message_event.h"
#include "core/runtime/lepus/bindings/renderer.h"
#include "core/runtime/lepus/bindings/renderer_functions.h"
#include "core/runtime/lepus/bytecode_generator.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/shell/runtime/mts/mts_runtime.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "core/value_wrapper/value_wrapper_utils.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

namespace {

class RecordingInspectorElementObserver final
    : public InspectorElementObserver {
 public:
  void OnDocumentUpdated() override {}
  void OnElementNodeAdded(Element* ptr) override {
    added_nodes.push_back(ptr);
    added_node_parents.push_back(ptr != nullptr ? ptr->parent() : nullptr);
  }
  void OnElementNodeRemoved(Element* ptr) override {
    removed_nodes.push_back(ptr);
    removed_node_parents.push_back(ptr != nullptr ? ptr->parent() : nullptr);
  }
  void OnCharacterDataModified(Element* ptr) override {}
  void OnElementDataModelSet(Element* ptr) override {
    modified_nodes.push_back(ptr);
    const auto& attributes = ptr->data_model()->attributes();
    auto text = attributes.find(base::String("text"));
    modified_texts.push_back(text == attributes.end() ? lepus::Value()
                                                      : text->second);
  }
  void OnElementManagerWillDestroy() override {}
  void OnCSSStyleSheetAdded(Element* ptr) override {}
  void OnComponentUselessUpdate(const std::string& component_name,
                                const lepus::Value& properties) override {}
  void OnSetNativeProps(Element* ptr, const std::string& name,
                        const std::string& value, bool is_style) override {}
  void OnCSSMediaQueryResultChanged() override {}

  std::map<lynx::devtool::DevToolFunction,
           std::function<void(const base::any&)>>
  GetDevToolFunction() override {
    auto noop = [](const base::any&) {};
    return {
        {lynx::devtool::DevToolFunction::InitForInspector,
         [this](const base::any& data) {
           auto* node = std::get<0>(base::any_cast<std::tuple<Element*>>(data));
           node->set_inspector_attribute(
               std::make_unique<InspectorAttribute>());
           initialized_nodes.push_back(node);
         }},
        {lynx::devtool::DevToolFunction::InitPlugForInspector, noop},
        {lynx::devtool::DevToolFunction::InitStyleValueElement, noop},
        {lynx::devtool::DevToolFunction::InitStyleRoot, noop},
        {lynx::devtool::DevToolFunction::SetDocElement, noop},
        {lynx::devtool::DevToolFunction::SetStyleValueElement, noop},
        {lynx::devtool::DevToolFunction::SetStyleRoot, noop},
    };
  }

  std::vector<Element*> added_nodes;
  std::vector<Element*> added_node_parents;
  std::vector<Element*> removed_nodes;
  std::vector<Element*> removed_node_parents;
  std::vector<Element*> modified_nodes;
  std::vector<Element*> initialized_nodes;
  std::vector<lepus::Value> modified_texts;
};

const lepus::Value* DatasetValue(const Element* element,
                                 const base::String& key) {
  auto it = element->data_model_->dataset().find(key);
  if (it == element->data_model_->dataset().end()) {
    return nullptr;
  }
  return &it->second;
}
}  // namespace

class ElementTemplateInstanceTest : public FiberElementTest {
 protected:
  std::shared_ptr<runtime::MTSRuntime> CreatePAPIRuntime() {
    auto runtime = runtime::MTSRuntime::CreateContext(
        runtime::ContextType::LepusNGContextType);
    runtime->Initialize();
    runtime->SetGlobalData(
        BASE_STATIC_STRING(tasm::kTemplateAssembler),
        lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
    return runtime;
  }

  fml::RefPtr<ElementTemplateInstance> CreateCompiledSpreadInstance(
      bool nested_target = false) {
    auto entry = std::make_shared<TemplateEntry>();
    entry->SetName(DEFAULT_ENTRY_NAME);
    tasm->template_entries_[DEFAULT_ENTRY_NAME] = entry;

    auto info = std::make_shared<ElementTemplateInfo>();
    info->exist_ = true;
    info->key_ = "spread_template";
    auto root_info = ElementInfo();
    root_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
    root_info.attributes_ =
        std::make_shared<const TemplateAttributes>(TemplateAttributes{
            Attribute{ATTRIBUTE_BINDING_TYPE_SPREAD, base::String("spread"),
                      lepus::Value(), 0}});
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

  fml::RefPtr<ElementTemplateInstance> CreateCompiledCallbackInstance(
      const std::shared_ptr<runtime::MTSRuntime>& runtime, bool nested_target,
      const char* key = "main-thread:gesture") {
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
            Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC,
                      base::String("data-label"), lepus::Value(), 1}});
    return instance;
  }

  void CreateCSSEntry(const char* name, const char* default_color,
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
      token->raw_attributes_[kPropertyIDColor] =
          CSSValue::MakePlainString(color);
      token->raw_attributes_[kPropertyIDFontWeight] =
          CSSValue::MakePlainString("700");
      css::CSSParserContext context;
      css::CSSTokenizer tokenizer(".css-owner");
      const auto tokens = tokenizer.TokenizeToEOF();
      css::CSSParserTokenRange range(tokens);
      auto selectors = css::CSSSelectorParser::ParseSelector(range, &context);
      const auto size = css::CSSSelectorParser::FlattenedSize(selectors);
      auto selector_array = std::make_unique<css::LynxCSSSelector[]>(size);
      css::CSSSelectorParser::AdoptSelectorVector(selectors,
                                                  selector_array.get(), size);
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

  fml::RefPtr<ElementTemplateInstance> CreateCSSInstance(const char* entry,
                                                         bool scoped) {
    auto instance = fml::AdoptRef<ElementTemplateInstance>(
        new ElementTemplateInstance(manager));
    instance->SetTASM(tasm.get());
    instance->SetBundleUrl(base::String(entry));
    instance->SetTemplateKey(base::String(scoped ? "scoped" : "default"));
    return instance;
  }

  void ExpectCSSColor(ElementTemplateInstance* instance, uint32_t color) {
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

  std::vector<fml::RefPtr<ElementTemplateInstance>> CreateCompiledMountChain(
      size_t length) {
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

  void ExpectPageFlushCompletesNestedAttrSlotMaterialization(
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
};

TEST_P(ElementTemplateInstanceTest, CompiledCSSUsesOwningEntryAndRootScope) {
  CreateCSSEntry(DEFAULT_ENTRY_NAME, "white", "red");
  CreateCSSEntry("lazy", "yellow", "green");
  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  auto page_root = page->GetRoot();

  for (const auto* entry : {DEFAULT_ENTRY_NAME, "lazy"}) {
    for (bool scoped : {false, true}) {
      SCOPED_TRACE(entry);
      SCOPED_TRACE(scoped);
      auto instance = CreateCSSInstance(entry, scoped);
      page->InsertNodeIntoChildSlot(0, lepus::Value(instance), lepus::Value());
      auto options = std::make_shared<PipelineOptions>();
      manager->OnPatchFinish(options, page_root.get());
      const auto main_entry = std::string(entry) == DEFAULT_ENTRY_NAME;
      ExpectCSSColor(instance.get(), main_entry
                                         ? (scoped ? 0xffff0000 : 0xffffffff)
                                         : (scoped ? 0xff008000 : 0xffffff00));
    }
  }
}

TEST_P(ElementTemplateInstanceTest, CompiledCSSUsesLatestScopeAfterPrepare) {
  CreateCSSEntry(DEFAULT_ENTRY_NAME, "white", "red");
  CreateCSSEntry("lazy", "yellow", "green");
  auto& info = tasm->template_entries_["lazy"]
                   ->template_bundle_.element_template_infos_["default"];
  info->elements_[0].attributes_ = std::make_shared<const TemplateAttributes>(
      TemplateAttributes{Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC,
                                   base::String("css-id"), lepus::Value(), 0}});
  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  auto page_root = page->GetRoot();

  auto instance = CreateCSSInstance("lazy", false);
  instance->SetAttributeSlot(0, lepus::Value(0));
  instance->EnsureCreateElementTreeTaskScheduled();
  ASSERT_NE(instance->create_element_tree_task_, nullptr);
  instance->create_element_tree_task_->Run();
  ASSERT_EQ(instance->PeekMaterializedRoot(), nullptr);

  instance->SetAttributeSlot(0, lepus::Value(42));
  page->InsertNodeIntoChildSlot(0, lepus::Value(instance), lepus::Value());
  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page_root.get());
  ExpectCSSColor(instance.get(), 0xff008000);
}

TEST_P(ElementTemplateInstanceTest, CompiledCSSOwnershipSurvivesNestedRemount) {
  CreateCSSEntry(DEFAULT_ENTRY_NAME, "white", "red");
  CreateCSSEntry("lazy-parent", "blue", "green");
  CreateCSSEntry("lazy-child", "yellow", "cyan");
  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  auto page_root = page->GetRoot();
  auto parent = CreateCSSInstance("lazy-parent", true);
  auto child = CreateCSSInstance("lazy-child", false);
  auto main_child = CreateCSSInstance(DEFAULT_ENTRY_NAME, false);
  child->InsertNodeIntoChildSlot(0, lepus::Value(main_child), lepus::Value());
  parent->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());
  page->InsertNodeIntoChildSlot(0, lepus::Value(parent), lepus::Value());
  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page_root.get());
  ExpectCSSColor(parent.get(), 0xff008000);
  ExpectCSSColor(child.get(), 0xffffff00);
  ExpectCSSColor(main_child.get(), 0xffffffff);

  auto child_root = child->PeekMaterializedRoot();
  parent->RemoveNodeFromChildSlot(0, lepus::Value(child));
  options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page_root.get());
  page->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());
  options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page_root.get());
  EXPECT_EQ(child->PeekMaterializedRoot(), child_root);
  ExpectCSSColor(child.get(), 0xffffff00);
  ExpectCSSColor(main_child.get(), 0xffffffff);

  auto remounted = CreateCSSInstance("lazy-child", false);
  parent->InsertNodeIntoChildSlot(0, lepus::Value(remounted), lepus::Value());
  options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page_root.get());
  ExpectCSSColor(remounted.get(), 0xffffff00);
  ExpectCSSColor(parent.get(), 0xff008000);
}

TEST_P(ElementTemplateInstanceTest,
       CompiledPreparedObjectStyleRemainsVisibleToInspector) {
  if (!ENABLE_INSPECTOR) {
    GTEST_SKIP();
  }
  manager->SetInspectorElementObserver(
      std::make_shared<RecordingInspectorElementObserver>());
  manager->devtool_flag_ = true;
  manager->dom_tree_enabled_ = true;
  auto instance = CreateCompiledSpreadInstance();
  instance->EnsureCreateElementTreeTaskScheduled();
  ASSERT_NE(instance->create_element_tree_task_, nullptr);
  instance->create_element_tree_task_->Run();
  auto attributes = lepus::Dictionary::Create();
  auto style = lepus::Dictionary::Create();
  style->SetValue("width", lepus::Value("23px"));
  attributes->SetValue("style", lepus::Value(style));
  instance->SetAttributeSlot(0, lepus::Value(attributes));
  auto root = instance->GetRoot();
  ASSERT_NE(root, nullptr);
  const auto& styles = root->data_model()->inline_styles();
  const auto width = styles.find(kPropertyIDWidth);
  ASSERT_NE(width, styles.end());
  EXPECT_EQ(width->second.GetValue().Number(), 23);
}

TEST_P(ElementTemplateInstanceTest, ClassUpdatesInvalidateDescendantSelectors) {
  for (bool new_styling : {false, true}) {
    for (bool typed : {false, true}) {
      SCOPED_TRACE(new_styling);
      SCOPED_TRACE(typed);
      manager->config_->SetEnableStandardCSSSelector(true);
      manager->config_->SetEnableNewStylingPipeline(new_styling);
      manager->SetConfig(manager->config_);
      auto styles = std::make_shared<CSSStyleSheetManager>(nullptr);
      auto fragment = std::make_unique<SharedCSSFragment>(0);
      fragment->SetEnableCSSInvalidation();
      fragment->SetEnableCSSSelector();
      auto add_rule = [&](const char* selector, double opacity) {
        auto token = fml::MakeRefCounted<CSSParseToken>(CSSParserConfigs{});
        token->raw_attributes_[kPropertyIDOpacity] =
            CSSValue(opacity, CSSValuePattern::NUMBER);
        css::CSSParserContext context;
        css::CSSTokenizer tokenizer(selector);
        const auto tokens = tokenizer.TokenizeToEOF();
        css::CSSParserTokenRange range(tokens);
        auto selectors = css::CSSSelectorParser::ParseSelector(range, &context);
        const auto size = css::CSSSelectorParser::FlattenedSize(selectors);
        auto selector_array = std::make_unique<css::LynxCSSSelector[]>(size);
        css::CSSSelectorParser::AdoptSelectorVector(selectors,
                                                    selector_array.get(), size);
        fragment->AddStyleRule(std::move(selector_array), token);
      };
      add_rule(".child", 0.75);
      add_rule(".active .child", 0.5);
      styles->AddSharedCSSFragment(std::move(fragment));
      auto page = manager->CreateFiberPage("page", 0);
      page->set_style_sheet_manager(styles);
      manager->SetFiberPageElement(page);
      auto instance = CreateCompiledSpreadInstance();
      // Compiled roots use their owning entry's CSS, while typed nodes use page
      // CSS. Both paths must resolve the fixture's descendant rules.
      tasm->template_entries_.at(DEFAULT_ENTRY_NAME)
          ->template_bundle_.css_style_manager_ = styles;
      if (typed) {
        instance->SetTypedTag(base::String("view"));
      } else {
        auto info = tasm->template_entries_.at(DEFAULT_ENTRY_NAME)
                        ->template_bundle_.element_template_infos_.at(
                            "spread_template");
        info->elements_[0].attributes_ =
            std::make_shared<const TemplateAttributes>(TemplateAttributes{
                Attribute{ATTRIBUTE_BINDING_TYPE_STATIC, base::String("class"),
                          lepus::Value("active"), 0},
                Attribute{ATTRIBUTE_BINDING_TYPE_SPREAD, base::String("spread"),
                          lepus::Value(), 0}});
      }
      auto set_class = [&](const lepus::Value& value) {
        auto attributes = lepus::Dictionary::Create();
        attributes->SetValue("class", value);
        instance->SetAttributeSlot(0, lepus::Value(attributes));
      };
      set_class(lepus::Value("other"));
      auto root = instance->GetRoot();
      ASSERT_NE(root, nullptr);
      auto child = manager->CreateFiberNode("view");
      root->SetParentComponentUniqueIdForFiber(page->impl_id());
      child->SetParentComponentUniqueIdForFiber(page->impl_id());
      child->SetClass(base::String("child"));
      root->InsertNode(child);
      page->InsertNode(root);
      auto expect_opacity = [&](double expected) {
        SCOPED_TRACE(expected);
        page->FlushActionsAsRoot();
        platform_impl_->Flush();
        auto node = platform_impl_->node_map_.find(child->impl_id());
        ASSERT_NE(node, platform_impl_->node_map_.end());
        auto opacity = node->second->props_.find("opacity");
        ASSERT_NE(opacity, node->second->props_.end());
        EXPECT_DOUBLE_EQ(opacity->second.Number(), expected);
        ASSERT_FALSE(child->StyleDirty());
      };
      expect_opacity(0.75);
      set_class(lepus::Value("active other"));
      expect_opacity(0.5);
      set_class(lepus::Value("other"));
      expect_opacity(0.75);
      instance->SetAttributeSlot(0, lepus::Value(lepus::Dictionary::Create()));
      expect_opacity(typed ? 0.75 : 0.5);
      set_class(lepus::Value());
      expect_opacity(0.75);
      set_class(lepus::Value("active"));
      expect_opacity(0.5);
      set_class(lepus::Value(""));
      expect_opacity(0.75);
      instance->SetAttributeSlot(0, lepus::Value());
      expect_opacity(typed ? 0.75 : 0.5);
    }
  }
}

TEST_P(ElementTemplateInstanceTest, PAPIHandlesRoundTripThroughJSChildSlots) {
  auto runtime = CreatePAPIRuntime();
  auto* ctx = runtime::MTSRuntime::ToQuickContext(runtime.get());
  lepus::Value child_args[] = {lepus::Value("view"), lepus::Value(),
                               lepus::Value(), lepus::Value(1.5)};
  auto child =
      RendererFunctions::FiberCreateTypedElementTemplate(ctx, child_args, 4);
  ASSERT_TRUE(child.IsRefCounted());
  EXPECT_EQ(child.RefCounted()->GetRefType(), lepus::RefType::kElementTemplate);

  runtime->SetGlobalData("etChild", child);
  lepus::BytecodeGenerator::GenerateBytecode(
      runtime->GetMTSContext(),
      "let compiledSlots = []; compiledSlots[2] = [etChild]; "
      "let typedSlots = [[etChild]];",
      runtime->GetSdkVersion(), "");
  ASSERT_TRUE(runtime->Execute(nullptr));
  auto compiled_slots = runtime->GetGlobalData("compiledSlots");
  auto typed_slots = runtime->GetGlobalData("typedSlots");
  ASSERT_TRUE(compiled_slots.IsJSValue());
  ASSERT_TRUE(typed_slots.IsJSValue());

  auto attributes = lepus::CArray::Create();
  attributes->emplace_back(lepus::Value("initial"));
  auto options = lepus::Dictionary::Create();
  options->SetValue("enabled", lepus::Value(true));
  lepus::Value compiled_args[] = {lepus::Value("parent_template"),
                                  lepus::Value("bundle.js"),
                                  lepus::Value(attributes),
                                  compiled_slots,
                                  lepus::Value(7),
                                  lepus::Value(options)};
  auto compiled =
      RendererFunctions::FiberCreateElementTemplate(ctx, compiled_args, 6);
  ASSERT_TRUE(compiled.IsRefCounted());
  EXPECT_EQ(compiled.RefCounted()->GetRefType(),
            lepus::RefType::kElementTemplate);
  lepus::Value serialize_args[] = {compiled};
  auto serialized =
      RendererFunctions::FiberSerializeElementTemplate(ctx, serialize_args, 1);
  EXPECT_EQ(serialized.GetProperty("templateKey").StdString(),
            "parent_template");
  EXPECT_EQ(serialized.GetProperty("bundleUrl").StdString(), "bundle.js");
  EXPECT_EQ(serialized.GetProperty("uid").Number(), 7);
  EXPECT_EQ(serialized.GetProperty("attributeSlots").GetProperty(0).StdString(),
            "initial");
  EXPECT_TRUE(serialized.GetProperty("options").GetProperty("enabled").Bool());
  auto slots = serialized.GetProperty("childSlots");
  ASSERT_EQ(slots.GetLength(), 3);
  EXPECT_TRUE(slots.GetProperty(0).IsUndefined());
  EXPECT_TRUE(slots.GetProperty(1).IsUndefined());
  ASSERT_EQ(slots.GetProperty(2).GetLength(), 1);
  auto serialized_child = slots.GetProperty(2).GetProperty(0);
  EXPECT_EQ(serialized_child.GetProperty("tag").StdString(), "view");
  ASSERT_TRUE(serialized_child.GetProperty("uid").IsNumber());
  EXPECT_EQ(serialized_child.GetProperty("uid").Number(), 1.5);

  auto typed_attributes = lepus::Dictionary::Create();
  typed_attributes->SetValue("data-state", lepus::Value("initial"));
  lepus::Value typed_args[] = {lepus::Value("view"),
                               lepus::Value(typed_attributes), typed_slots,
                               lepus::Value(8), lepus::Value(options)};
  auto typed =
      RendererFunctions::FiberCreateTypedElementTemplate(ctx, typed_args, 5);
  ASSERT_TRUE(typed.IsRefCounted());
  EXPECT_EQ(typed.RefCounted()->GetRefType(), lepus::RefType::kElementTemplate);
  serialize_args[0] = typed;
  serialized =
      RendererFunctions::FiberSerializeElementTemplate(ctx, serialize_args, 1);
  EXPECT_EQ(serialized.GetProperty("tag").StdString(), "view");
  EXPECT_EQ(serialized.GetProperty("uid").Number(), 8);
  EXPECT_EQ(serialized.GetProperty("attributes")
                .GetProperty("data-state")
                .StdString(),
            "initial");
  EXPECT_TRUE(serialized.GetProperty("options").GetProperty("enabled").Bool());
  slots = serialized.GetProperty("childSlots");
  ASSERT_EQ(slots.GetLength(), 1);
  ASSERT_EQ(slots.GetProperty(0).GetLength(), 1);
  EXPECT_EQ(slots.GetProperty(0).GetProperty(0).GetProperty("uid").Number(),
            1.5);
}

TEST_P(ElementTemplateInstanceTest, PAPIAttributesSnapshotJSObjects) {
  for (bool typed : {false, true}) {
    SCOPED_TRACE(typed);
    auto runtime = CreatePAPIRuntime();
    auto* ctx = runtime::MTSRuntime::ToQuickContext(runtime.get());
    lepus::BytecodeGenerator::GenerateBytecode(
        runtime->GetMTSContext(),
        "let callback = () => 42;"
        "let attributes = {nested: {values: [1]}, callback};"
        "let attributeSlots = [attributes];"
        "let mutate = () => { attributes.nested.values[0] = 2; "
        "attributes.callback = null; attributeSlots[0] = null; };",
        runtime->GetSdkVersion(), "");
    ASSERT_TRUE(runtime->Execute(nullptr));
    auto attributes = runtime->GetGlobalData("attributes");
    auto attribute_slots = runtime->GetGlobalData("attributeSlots");
    auto callback = runtime->GetGlobalData("callback");
    ASSERT_TRUE(attributes.IsJSValue());
    ASSERT_TRUE(attribute_slots.IsJSValue());
    ASSERT_TRUE(callback.IsCallable());

    lepus::Value instance;
    if (typed) {
      lepus::Value args[] = {lepus::Value("view"), attributes, lepus::Value(),
                             lepus::Value(1)};
      instance =
          RendererFunctions::FiberCreateTypedElementTemplate(ctx, args, 4);
    } else {
      lepus::Value args[] = {lepus::Value("logical_template"), lepus::Value(),
                             attribute_slots, lepus::Value(), lepus::Value(1)};
      instance = RendererFunctions::FiberCreateElementTemplate(ctx, args, 5);
    }
    ASSERT_TRUE(instance.IsRefCounted());
    runtime->CallClosure(runtime->GetGlobalData("mutate"));
    lepus::Value serialize_args[] = {instance};
    auto serialized = RendererFunctions::FiberSerializeElementTemplate(
        ctx, serialize_args, 1);
    auto stored = typed
                      ? serialized.GetProperty("attributes")
                      : serialized.GetProperty("attributeSlots").GetProperty(0);
    EXPECT_EQ(stored.GetProperty("nested")
                  .GetProperty("values")
                  .GetProperty(0)
                  .Number(),
              1);
    EXPECT_TRUE(stored.GetProperty("callback").IsEqual(callback));
    EXPECT_EQ(runtime->CallClosure(stored.GetProperty("callback")).Number(),
              42);

    attributes.SetProperty("callback", callback);
    lepus::Value update_args[] = {instance, lepus::Value(0), attributes};
    RendererFunctions::FiberSetAttributeOfElementTemplate(ctx, update_args, 3);
    attributes.GetProperty("nested").GetProperty("values").SetProperty(
        0, lepus::Value(3));
    attributes.SetProperty("callback", lepus::Value());
    serialized = RendererFunctions::FiberSerializeElementTemplate(
        ctx, serialize_args, 1);
    stored = typed ? serialized.GetProperty("attributes")
                   : serialized.GetProperty("attributeSlots").GetProperty(0);
    EXPECT_EQ(stored.GetProperty("nested")
                  .GetProperty("values")
                  .GetProperty(0)
                  .Number(),
              2);
    EXPECT_TRUE(stored.GetProperty("callback").IsEqual(callback));
    EXPECT_EQ(runtime->CallClosure(stored.GetProperty("callback")).Number(),
              42);
  }
}

TEST_P(ElementTemplateInstanceTest, PAPIMutationsUpdateSerializedLogicalSlots) {
  auto runtime = CreatePAPIRuntime();
  auto* ctx = runtime::MTSRuntime::ToQuickContext(runtime.get());
  lepus::Value child_args[] = {lepus::Value("view"), lepus::Value(),
                               lepus::Value(), lepus::Value(1)};
  auto first =
      RendererFunctions::FiberCreateTypedElementTemplate(ctx, child_args, 4);
  child_args[3] = lepus::Value(2);
  auto second =
      RendererFunctions::FiberCreateTypedElementTemplate(ctx, child_args, 4);
  ASSERT_TRUE(first.IsRefCounted());
  ASSERT_TRUE(second.IsRefCounted());
  auto children = lepus::CArray::Create();
  children->emplace_back(first);
  auto slots = lepus::CArray::Create();
  slots->emplace_back(lepus::Value(children));
  auto attributes = lepus::CArray::Create();
  attributes->emplace_back(lepus::Value("before"));
  lepus::Value create_args[] = {lepus::Value("logical_template"),
                                lepus::Value(), lepus::Value(attributes),
                                lepus::Value(slots), lepus::Value(3)};
  auto parent =
      RendererFunctions::FiberCreateElementTemplate(ctx, create_args, 5);
  ASSERT_TRUE(parent.IsRefCounted());

  lepus::Value update_args[] = {parent, lepus::Value(0), lepus::Value("after")};
  RendererFunctions::FiberSetAttributeOfElementTemplate(ctx, update_args, 3);
  lepus::Value insert_args[] = {parent, lepus::Value(0), second, first};
  RendererFunctions::FiberInsertNodeToElementTemplate(ctx, insert_args, 4);
  lepus::Value serialize_args[] = {parent};
  auto serialized =
      RendererFunctions::FiberSerializeElementTemplate(ctx, serialize_args, 1);
  EXPECT_EQ(serialized.GetProperty("attributeSlots").GetProperty(0).StdString(),
            "after");
  auto serialized_children =
      serialized.GetProperty("childSlots").GetProperty(0);
  ASSERT_EQ(serialized_children.GetLength(), 2);
  EXPECT_EQ(serialized_children.GetProperty(0).GetProperty("uid").Number(), 2);
  EXPECT_EQ(serialized_children.GetProperty(1).GetProperty("uid").Number(), 1);

  lepus::Value remove_args[] = {parent, lepus::Value(0), first};
  RendererFunctions::FiberRemoveNodeFromElementTemplate(ctx, remove_args, 3);
  serialized_children =
      RendererFunctions::FiberSerializeElementTemplate(ctx, serialize_args, 1)
          .GetProperty("childSlots")
          .GetProperty(0);
  ASSERT_EQ(serialized_children.GetLength(), 1);
  EXPECT_EQ(serialized_children.GetProperty(0).GetProperty("uid").Number(), 2);
}

TEST_P(ElementTemplateInstanceTest, PAPITypedPageAppliesAttributesAndChildren) {
  auto runtime = CreatePAPIRuntime();
  auto* ctx = runtime::MTSRuntime::ToQuickContext(runtime.get());
  auto observer = std::make_shared<RecordingInspectorElementObserver>();
  manager->SetInspectorElementObserver(observer);
  manager->dom_tree_enabled_ = true;

  auto attributes = lepus::Dictionary::Create();
  attributes->SetValue("data-old", lepus::Value("old"));
  lepus::Value child_args[] = {lepus::Value("view"), lepus::Value(attributes),
                               lepus::Value(), lepus::Value(1)};
  auto child =
      RendererFunctions::FiberCreateTypedElementTemplate(ctx, child_args, 4);
  ASSERT_TRUE(child.IsRefCounted());
  auto updated_attributes = lepus::Dictionary::Create();
  updated_attributes->SetValue("data-state", lepus::Value("before-mount"));
  lepus::Value update_args[] = {child, lepus::Value(0),
                                lepus::Value(updated_attributes)};
  RendererFunctions::FiberSetAttributeOfElementTemplate(ctx, update_args, 3);
  lepus::Value serialize_args[] = {child};
  auto serialized_attributes =
      RendererFunctions::FiberSerializeElementTemplate(ctx, serialize_args, 1)
          .GetProperty("attributes");
  EXPECT_TRUE(serialized_attributes.GetProperty("data-old").IsEmpty());
  EXPECT_EQ(serialized_attributes.GetProperty("data-state").StdString(),
            "before-mount");

  auto children = lepus::CArray::Create();
  children->emplace_back(child);
  auto slots = lepus::CArray::Create();
  slots->emplace_back(lepus::Value(children));
  lepus::Value page_args[] = {lepus::Value("page"), lepus::Value(),
                              lepus::Value(slots), lepus::Value(0)};
  auto page =
      RendererFunctions::FiberCreateTypedElementTemplate(ctx, page_args, 4);
  ASSERT_TRUE(page.IsRefCounted());
  auto* page_root = manager->root();
  ASSERT_NE(page_root, nullptr);
  EXPECT_TRUE(page_root->is_page());
  ASSERT_EQ(page_root->children().size(), 1u);
  auto* child_root = static_cast<Element*>(page_root->children()[0].get());
  EXPECT_TRUE(child_root->is_view());
  EXPECT_EQ(child_root->dataset().find("old"), child_root->dataset().end());
  auto state = child_root->dataset().find("state");
  ASSERT_NE(state, child_root->dataset().end());
  EXPECT_EQ(state->second.StdString(), "before-mount");
  if (ENABLE_INSPECTOR) {
    EXPECT_EQ(std::count(observer->added_nodes.begin(),
                         observer->added_nodes.end(), page_root),
              1);
  }

  auto mounted_attributes = lepus::Dictionary::Create();
  mounted_attributes->SetValue("data-state", lepus::Value("after-mount"));
  update_args[2] = lepus::Value(mounted_attributes);
  RendererFunctions::FiberSetAttributeOfElementTemplate(ctx, update_args, 3);
  state = child_root->dataset().find("state");
  ASSERT_NE(state, child_root->dataset().end());
  EXPECT_EQ(state->second.StdString(), "after-mount");

  lepus::Value remove_args[] = {page, lepus::Value(0), child};
  RendererFunctions::FiberRemoveNodeFromElementTemplate(ctx, remove_args, 3);
  EXPECT_TRUE(page_root->children().empty());
  lepus::Value insert_args[] = {page, lepus::Value(0), child, lepus::Value()};
  RendererFunctions::FiberInsertNodeToElementTemplate(ctx, insert_args, 4);
  ASSERT_EQ(page_root->children().size(), 1u);
  EXPECT_EQ(page_root->children()[0].get(), child_root);
}

TEST_P(ElementTemplateInstanceTest,
       PAPICreationRejectsInvalidUidAndArrayOptions) {
  auto runtime = CreatePAPIRuntime();
  auto* ctx = runtime::MTSRuntime::ToQuickContext(runtime.get());
  lepus::Value missing_uid_args[] = {lepus::Value("template"), lepus::Value(),
                                     lepus::Value(), lepus::Value()};
  for (int argc = 1; argc < 5; ++argc) {
    SCOPED_TRACE(argc);
    base::ErrorStorage::GetInstance().Reset();
    auto result = RendererFunctions::FiberCreateElementTemplate(
        ctx, missing_uid_args, argc);
    EXPECT_TRUE(result.IsEmpty());
    EXPECT_NE(base::ErrorStorage::GetInstance().GetError(), nullptr);
  }
  base::ErrorStorage::GetInstance().Reset();
  for (bool invalid_uid : {true, false}) {
    SCOPED_TRACE(invalid_uid ? "nonnumeric uid" : "array options");
    auto uid = invalid_uid ? lepus::Value("invalid") : lepus::Value(1);
    auto options =
        invalid_uid ? lepus::Value() : lepus::Value(lepus::CArray::Create());
    lepus::Value compiled_args[] = {lepus::Value("template"),
                                    lepus::Value(),
                                    lepus::Value(),
                                    lepus::Value(),
                                    uid,
                                    options};
    base::ErrorStorage::GetInstance().Reset();
    auto compiled =
        RendererFunctions::FiberCreateElementTemplate(ctx, compiled_args, 6);
    EXPECT_TRUE(compiled.IsEmpty());
    EXPECT_NE(base::ErrorStorage::GetInstance().GetError(), nullptr);

    lepus::Value typed_args[] = {lepus::Value("view"), lepus::Value(),
                                 lepus::Value(), uid, options};
    base::ErrorStorage::GetInstance().Reset();
    auto typed =
        RendererFunctions::FiberCreateTypedElementTemplate(ctx, typed_args, 5);
    EXPECT_TRUE(typed.IsEmpty());
    EXPECT_NE(base::ErrorStorage::GetInstance().GetError(), nullptr);
    base::ErrorStorage::GetInstance().Reset();
  }
}

TEST_P(ElementTemplateInstanceTest, PAPITypedCreationRejectsList) {
  auto runtime = CreatePAPIRuntime();
  auto* ctx = runtime::MTSRuntime::ToQuickContext(runtime.get());
  lepus::Value args[] = {lepus::Value("list"), lepus::Value(), lepus::Value(),
                         lepus::Value(0)};
  base::ErrorStorage::GetInstance().Reset();
  auto result =
      RendererFunctions::FiberCreateTypedElementTemplate(ctx, args, 4);
  EXPECT_TRUE(result.IsEmpty());
  EXPECT_NE(base::ErrorStorage::GetInstance().GetError(), nullptr);
  base::ErrorStorage::GetInstance().Reset();
}

TEST_P(ElementTemplateInstanceTest,
       PAPIMutationsRejectOrdinaryElementChildren) {
  auto runtime = CreatePAPIRuntime();
  auto* ctx = runtime::MTSRuntime::ToQuickContext(runtime.get());
  lepus::Value create_args[] = {lepus::Value("view"), lepus::Value(),
                                lepus::Value(), lepus::Value(1)};
  auto parent =
      RendererFunctions::FiberCreateTypedElementTemplate(ctx, create_args, 4);
  create_args[3] = lepus::Value(2);
  auto child =
      RendererFunctions::FiberCreateTypedElementTemplate(ctx, create_args, 4);
  ASSERT_TRUE(parent.IsRefCounted());
  ASSERT_TRUE(child.IsRefCounted());
  lepus::Value ordinary(manager->CreateFiberView());
  lepus::Value insert_args[] = {parent, lepus::Value(0), ordinary,
                                lepus::Value()};
  base::ErrorStorage::GetInstance().Reset();
  RendererFunctions::FiberInsertNodeToElementTemplate(ctx, insert_args, 4);
  EXPECT_NE(base::ErrorStorage::GetInstance().GetError(), nullptr);

  insert_args[2] = child;
  insert_args[3] = ordinary;
  base::ErrorStorage::GetInstance().Reset();
  RendererFunctions::FiberInsertNodeToElementTemplate(ctx, insert_args, 4);
  EXPECT_NE(base::ErrorStorage::GetInstance().GetError(), nullptr);

  lepus::Value remove_args[] = {parent, lepus::Value(0), ordinary};
  base::ErrorStorage::GetInstance().Reset();
  RendererFunctions::FiberRemoveNodeFromElementTemplate(ctx, remove_args, 3);
  EXPECT_NE(base::ErrorStorage::GetInstance().GetError(), nullptr);
  base::ErrorStorage::GetInstance().Reset();
}

TEST_P(ElementTemplateInstanceTest, UsesIndependentLepusRefType) {
  auto instance = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));

  EXPECT_EQ(instance->GetRefType(), lepus::RefType::kElementTemplate);
  static_assert(!std::is_base_of_v<Element, ElementTemplateInstance>);
}

TEST_P(ElementTemplateInstanceTest,
       PAPIAttributeSlotQueryReturnsDistinctNestedElements) {
  auto entry = std::make_shared<TemplateEntry>();
  entry->SetName(DEFAULT_ENTRY_NAME);
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = entry;
  auto info = std::make_shared<ElementTemplateInfo>();
  info->exist_ = true;
  info->key_ = "ref_targets";
  ElementInfo root_info;
  root_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  for (uint32_t slot : {0u, 2u}) {
    ElementInfo child_info;
    child_info.tag_enum_ = slot == 0 ? ElementBuiltInTagEnum::ELEMENT_TEXT
                                     : ElementBuiltInTagEnum::ELEMENT_VIEW;
    child_info.attributes_ =
        std::make_shared<const TemplateAttributes>(TemplateAttributes{
            Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC,
                      base::String("main-thread:ref"), lepus::Value(), slot}});
    root_info.children_.emplace_back(std::move(child_info));
  }
  info->elements_.emplace_back(std::move(root_info));
  entry->template_bundle_.element_template_infos_["ref_targets"] = info;
  auto instance = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  instance->SetTASM(tasm.get());
  instance->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  instance->SetTemplateKey(base::String("ref_targets"));

  auto runtime = CreatePAPIRuntime();
  auto* ctx = runtime::MTSRuntime::ToQuickContext(runtime.get());
  auto query = [&](uint32_t slot) {
    lepus::Value args[] = {lepus::Value(instance), lepus::Value(slot)};
    return RendererFunctions::FiberGetElementByAttrSlot(ctx, args, 2);
  };
  auto first_target = query(0);
  ASSERT_TRUE(first_target.IsRefCounted());
  EXPECT_TRUE(instance->materialization_requested_);
  auto root = instance->PeekMaterializedRoot();
  ASSERT_NE(root, nullptr);
  ASSERT_EQ(root->children().size(), 2u);
  for (uint32_t slot : {0u, 2u}) {
    auto target = query(slot);
    ASSERT_TRUE(target.IsRefCounted());
    EXPECT_EQ(target.RefCounted()->GetRefType(), lepus::RefType::kElement);
    EXPECT_EQ(target.RefCounted().get(), root->children()[slot / 2].get());
    EXPECT_NE(target.RefCounted().get(), root.get());
    EXPECT_EQ(query(slot).RefCounted().get(), target.RefCounted().get());
    lepus::Value args[] = {target, lepus::Value("accessibility-label"),
                           lepus::Value(slot == 0 ? "text" : "view")};
    RendererFunctions::FiberSetAttribute(ctx, args, 3);
    const auto& attributes =
        root->children()[slot / 2]->data_model()->attributes();
    auto written = attributes.find(base::String("accessibility-label"));
    ASSERT_NE(written, attributes.end());
    EXPECT_EQ(written->second.StdString(), slot == 0 ? "text" : "view");
  }
  EXPECT_TRUE(query(1).IsNil());
  EXPECT_TRUE(query(3).IsNil());

  Renderer::RegisterBuiltin(runtime.get(), ArchOption::FIBER_ARCH);
  runtime->SetGlobalData("et", lepus::Value(instance));
  lepus::BytecodeGenerator::GenerateBytecode(
      runtime->GetMTSContext(),
      "let slotTarget = __GetElementByAttrSlot(et, 2);"
      "let missingTarget = __GetElementByAttrSlot(et, 1) === null;",
      runtime->GetSdkVersion(), "");
  ASSERT_TRUE(runtime->Execute(nullptr));
  auto global_target = runtime->GetGlobalData("slotTarget");
  ASSERT_TRUE(global_target.IsRefCounted());
  EXPECT_EQ(global_target.RefCounted().get(), root->children()[1].get());
  EXPECT_TRUE(runtime->GetGlobalData("missingTarget").Bool());
}

TEST_P(ElementTemplateInstanceTest,
       PAPIAttributeSlotQueryMaterializesTypedRoot) {
  auto instance = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  instance->SetTypedTag(base::String("view"));
  auto runtime = CreatePAPIRuntime();
  auto* ctx = runtime::MTSRuntime::ToQuickContext(runtime.get());
  lepus::Value args[] = {lepus::Value(instance), lepus::Value(0)};
  auto target = RendererFunctions::FiberGetElementByAttrSlot(ctx, args, 2);
  auto root = instance->PeekMaterializedRoot();
  ASSERT_NE(root, nullptr);
  ASSERT_TRUE(target.IsRefCounted());
  EXPECT_EQ(target.RefCounted()->GetRefType(), lepus::RefType::kElement);
  EXPECT_EQ(target.RefCounted().get(), root.get());
  args[1] = lepus::Value(1);
  EXPECT_TRUE(
      RendererFunctions::FiberGetElementByAttrSlot(ctx, args, 2).IsNil());
}

TEST_P(ElementTemplateInstanceTest,
       PAPIAttributeSlotQueryRejectsInvalidHandleAndIndex) {
  auto instance = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  instance->SetTypedTag(base::String("view"));
  ASSERT_NE(instance->GetRoot(), nullptr);
  auto runtime = CreatePAPIRuntime();
  auto* ctx = runtime::MTSRuntime::ToQuickContext(runtime.get());
  for (const auto& index :
       {lepus::Value(-1), lepus::Value(0.5), lepus::Value("0"), lepus::Value(),
        lepus::Value(std::numeric_limits<double>::quiet_NaN()),
        lepus::Value(std::numeric_limits<double>::infinity()),
        lepus::Value(static_cast<double>(std::numeric_limits<uint32_t>::max()) +
                     1)}) {
    lepus::Value args[] = {lepus::Value(instance), index};
    EXPECT_TRUE(
        RendererFunctions::FiberGetElementByAttrSlot(ctx, args, 2).IsNil());
  }
  for (const auto& handle :
       {lepus::Value(), lepus::Value(manager->CreateFiberView())}) {
    lepus::Value args[] = {handle, lepus::Value(0)};
    EXPECT_TRUE(
        RendererFunctions::FiberGetElementByAttrSlot(ctx, args, 2).IsNil());
  }
}

TEST_P(ElementTemplateInstanceTest, PAPISlotMutationsRejectInvalidIndex) {
  auto parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  parent->SetTypedTag(base::String("view"));
  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTypedTag(base::String("view"));
  auto attributes = lepus::Dictionary::Create();
  attributes->SetValue("data-state", lepus::Value("unchanged"));
  parent->SetAttributeSlot(0, lepus::Value(attributes));
  auto changed_attributes = lepus::Dictionary::Create();
  changed_attributes->SetValue("data-state", lepus::Value("changed"));
  auto runtime = CreatePAPIRuntime();
  auto* ctx = runtime::MTSRuntime::ToQuickContext(runtime.get());

  for (const auto& index :
       {lepus::Value(-1), lepus::Value(0.5), lepus::Value("0"), lepus::Value(),
        lepus::Value(std::numeric_limits<double>::quiet_NaN()),
        lepus::Value(std::numeric_limits<double>::infinity()),
        lepus::Value(-std::numeric_limits<double>::infinity()),
        lepus::Value(static_cast<double>(std::numeric_limits<uint32_t>::max()) +
                     1)}) {
    SCOPED_TRACE(index.IsNumber() ? std::to_string(index.Number())
                                  : "nonnumeric");
    parent->SetAttributeSlot(0, lepus::Value(attributes));
    lepus::Value set_args[] = {lepus::Value(parent), index,
                               lepus::Value(changed_attributes)};
    RendererFunctions::FiberSetAttributeOfElementTemplate(ctx, set_args, 3);
    EXPECT_EQ(parent->attribute_slots_.GetProperty(0)
                  .GetProperty("data-state")
                  .StdString(),
              "unchanged");

    lepus::Value insert_args[] = {lepus::Value(parent), index,
                                  lepus::Value(child), lepus::Value()};
    RendererFunctions::FiberInsertNodeToElementTemplate(ctx, insert_args, 4);
    EXPECT_EQ(child->logical_parent_, nullptr);

    parent->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());
    lepus::Value remove_args[] = {lepus::Value(parent), index,
                                  lepus::Value(child)};
    RendererFunctions::FiberRemoveNodeFromElementTemplate(ctx, remove_args, 3);
    EXPECT_EQ(child->logical_parent_, parent.get());
    parent->RemoveNodeFromChildSlot(0, lepus::Value(child));
    base::ErrorStorage::GetInstance().Reset();
  }
}

TEST_P(ElementTemplateInstanceTest, PAPISlotIndexAcceptsUint32Boundaries) {
  auto runtime = CreatePAPIRuntime();
  auto* ctx = runtime::MTSRuntime::ToQuickContext(runtime.get());
  auto parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  parent->SetTypedTag(base::String("view"));
  auto attributes = lepus::Dictionary::Create();
  attributes->SetValue("data-state", lepus::Value("valid"));
  for (const auto& index : {lepus::Value(0), lepus::Value(-0.0)}) {
    lepus::Value set_args[] = {lepus::Value(parent), index,
                               lepus::Value(attributes)};
    RendererFunctions::FiberSetAttributeOfElementTemplate(ctx, set_args, 3);
    EXPECT_EQ(parent->attribute_slots_.GetProperty(0)
                  .GetProperty("data-state")
                  .StdString(),
              "valid");
  }
  // Exercise the upper bound without allocating a UINT32_MAX-sized slot array.
  lepus::Value insert_args[] = {
      lepus::Value(parent),
      lepus::Value(static_cast<double>(std::numeric_limits<uint32_t>::max())),
      lepus::Value(parent), lepus::Value()};
  base::ErrorStorage::GetInstance().Reset();
  RendererFunctions::FiberInsertNodeToElementTemplate(ctx, insert_args, 4);
  EXPECT_NE(base::ErrorStorage::GetInstance().GetError(), nullptr);
  base::ErrorStorage::GetInstance().Reset();
}

TEST_P(ElementTemplateInstanceTest, SerializeElementTemplateRecursively) {
  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTemplateKey(base::String("child_template"));
  child->SetBundleUrl(base::String("child_bundle.js"));

  auto child_attribute_slots = lepus::CArray::Create();
  child_attribute_slots->emplace_back(lepus::Value(true));
  child->SetAttributeSlots(lepus::Value(std::move(child_attribute_slots)));

  auto child_child_slots = lepus::CArray::Create();
  child_child_slots->emplace_back(lepus::Value(lepus::CArray::Create()));
  child->InitializeChildSlots(lepus::Value(std::move(child_child_slots)));

  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->SetTemplateKey(base::String("root_template"));
  root->SetBundleUrl(base::String("root_bundle.js"));

  auto root_attribute_slots = lepus::CArray::Create();
  root_attribute_slots->emplace_back(lepus::Value("slot_0"));
  root_attribute_slots->emplace_back(lepus::Value(42));
  root->SetAttributeSlots(lepus::Value(std::move(root_attribute_slots)));

  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(child));
  auto root_child_slots = lepus::CArray::Create();
  root_child_slots->emplace_back(lepus::Value(std::move(slot_children)));
  root->InitializeChildSlots(lepus::Value(std::move(root_child_slots)));

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

  auto serialized_child_slots = serialized.GetProperty("childSlots");
  EXPECT_TRUE(serialized_child_slots.IsArrayOrJSArray());
  ASSERT_EQ(serialized_child_slots.GetLength(), 1);
  auto serialized_slot_children = serialized_child_slots.GetProperty(0);
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
  EXPECT_TRUE(serialized_child.GetProperty("childSlots").IsArrayOrJSArray());
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateInitializationOwnsCallerSlotArrays) {
  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTypedTag(base::String("raw-text"));
  child->SetUid(lepus::Value(7));

  auto first_slot_children = lepus::CArray::Create();
  first_slot_children->emplace_back(lepus::Value(child));
  auto second_slot_children = lepus::CArray::Create();
  second_slot_children->emplace_back(lepus::Value(child));
  auto child_slots = lepus::CArray::Create();
  child_slots->emplace_back(lepus::Value(first_slot_children));
  child_slots->emplace_back(lepus::Value(second_slot_children));

  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->InitializeChildSlots(lepus::Value(child_slots));

  ASSERT_TRUE(first_slot_children->Erase(0, first_slot_children->size()));
  ASSERT_TRUE(second_slot_children->Erase(0, second_slot_children->size()));
  ASSERT_TRUE(child_slots->Erase(0, child_slots->size()));

  auto serialized_slots = root->Serialize().GetProperty("childSlots");
  ASSERT_TRUE(serialized_slots.IsArrayOrJSArray());
  ASSERT_EQ(serialized_slots.GetLength(), 2);
  ASSERT_TRUE(serialized_slots.GetProperty(0).IsArrayOrJSArray());
  EXPECT_EQ(serialized_slots.GetProperty(0).GetLength(), 0);
  ASSERT_TRUE(serialized_slots.GetProperty(1).IsArrayOrJSArray());
  ASSERT_EQ(serialized_slots.GetProperty(1).GetLength(), 1);
  EXPECT_EQ(serialized_slots.GetProperty(1)
                .GetProperty(0)
                .GetProperty("uid")
                .Number(),
            7);
}

TEST_P(
    ElementTemplateInstanceTest,
    ElementTemplateInitializationPreservesSlotShapesAndFiltersNonETChildren) {
  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->SetTemplateKey(base::String("root_template"));
  root->SetBundleUrl(base::String("root_bundle.js"));

  auto invalid_slot_children = lepus::CArray::Create();
  invalid_slot_children->emplace_back(lepus::Value(manager->CreateFiberView()));
  invalid_slot_children->emplace_back(lepus::Value(1));

  auto root_child_slots = lepus::CArray::Create();
  root_child_slots->emplace_back(
      lepus::Value(std::move(invalid_slot_children)));
  root_child_slots->emplace_back(lepus::Value("invalid_slot_shape"));
  root->InitializeChildSlots(lepus::Value(std::move(root_child_slots)));

  auto serialized = root->Serialize();
  EXPECT_TRUE(serialized.IsObject());

  auto serialized_child_slots = serialized.GetProperty("childSlots");
  EXPECT_TRUE(serialized_child_slots.IsArrayOrJSArray());
  ASSERT_EQ(serialized_child_slots.GetLength(), 2);
  EXPECT_TRUE(serialized_child_slots.GetProperty(0).IsArrayOrJSArray());
  EXPECT_EQ(serialized_child_slots.GetProperty(0).GetLength(), 0);
  EXPECT_EQ(serialized_child_slots.GetProperty(1).StdString(),
            "invalid_slot_shape");
}

TEST_P(ElementTemplateInstanceTest, SerializeTypedElementTemplate) {
  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTypedTag(base::String("raw-text"));
  child->SetUid(lepus::Value(1));

  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(child));
  auto child_slots = lepus::CArray::Create();
  child_slots->emplace_back(lepus::Value(slot_children));

  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->InitializeChildSlots(lepus::Value(child_slots));
  root->SetUid(lepus::Value(2));
  root->SetTypedTag(base::String("view"));
  auto attributes = lepus::Dictionary::Create();
  attributes->SetValue(base::String("data-test"), lepus::Value("root_attr"));
  root->SetAttributes(lepus::Value(attributes));

  auto serialized = root->Serialize();
  EXPECT_TRUE(serialized.IsObject());
  EXPECT_FALSE(serialized.GetProperty("templateKey").IsString());
  EXPECT_FALSE(serialized.Contains("attributeSlots"));
  EXPECT_EQ(serialized.GetProperty("tag").StdString(), "view");
  EXPECT_EQ(serialized.GetProperty("uid").Number(), 2);
  EXPECT_EQ(
      serialized.GetProperty("attributes").GetProperty("data-test").StdString(),
      "root_attr");

  auto serialized_slots = serialized.GetProperty("childSlots");
  EXPECT_TRUE(serialized_slots.IsArrayOrJSArray());
  ASSERT_EQ(serialized_slots.GetLength(), 1);
  auto serialized_child = serialized_slots.GetProperty(0).GetProperty(0);
  EXPECT_EQ(serialized_child.GetProperty("tag").StdString(), "raw-text");
  EXPECT_EQ(serialized_child.GetProperty("uid").Number(), 1);
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateUidPreservesRuntimeNumberValues) {
  auto instance = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  instance->SetTypedTag(base::String("view"));

  instance->SetUid(lepus::Value(1.5));
  EXPECT_EQ(instance->Serialize().GetProperty("uid").Number(), 1.5);

  instance->SetUid(lepus::Value(std::numeric_limits<double>::quiet_NaN()));
  EXPECT_TRUE(std::isnan(instance->Serialize().GetProperty("uid").Number()));
}

TEST_P(ElementTemplateInstanceTest,
       MovingMaterializedChildToSparseMountPointHoleDetachesSource) {
  auto default_entry = std::make_shared<TemplateEntry>();
  default_entry->SetName(DEFAULT_ENTRY_NAME);
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = default_entry;

  auto template_info = std::make_shared<ElementTemplateInfo>();
  template_info->exist_ = true;
  template_info->key_ = "sparse_slot";
  auto root_info = ElementInfo();
  root_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  auto child_slot_info = ElementInfo();
  child_slot_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_SLOT;
  child_slot_info.slot_index_ = 2;
  root_info.children_.emplace_back(std::move(child_slot_info));
  template_info->elements_.emplace_back(std::move(root_info));
  default_entry->template_bundle_.element_template_infos_["sparse_slot"] =
      std::move(template_info);

  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTypedTag(base::String("raw-text"));
  auto source = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  source->SetTypedTag(base::String("view"));
  source->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());
  auto source_root = source->GetRoot();
  auto child_root = child->PeekMaterializedRoot();
  ASSERT_NE(source_root, nullptr);
  ASSERT_NE(child_root, nullptr);
  ASSERT_EQ(child_root->parent(), source_root.get());

  auto destination = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  destination->SetTASM(tasm.get());
  destination->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  destination->SetTemplateKey(base::String("sparse_slot"));
  ASSERT_NE(destination->GetRoot(), nullptr);

  destination->InsertNodeIntoChildSlot(1, lepus::Value(child), lepus::Value());

  EXPECT_EQ(child_root->parent(), nullptr);
  EXPECT_TRUE(source_root->children().empty());
  EXPECT_EQ(destination->Serialize()
                .GetProperty("childSlots")
                .GetProperty(1)
                .GetLength(),
            1);

  destination->RemoveNodeFromChildSlot(1, lepus::Value(child));

  EXPECT_EQ(child_root->parent(), nullptr);
  EXPECT_EQ(destination->Serialize()
                .GetProperty("childSlots")
                .GetProperty(1)
                .GetLength(),
            0);
}

TEST_P(ElementTemplateInstanceTest,
       MaterializedElementTemplatesBoundPartsAndTemplateScopeClones) {
  auto part = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  part->SetTypedTag(base::String("view"));
  part->SetUid(lepus::Value(3));

  auto inner = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  inner->SetTypedTag(base::String("view"));
  inner->SetUid(lepus::Value(4));
  inner->InsertNodeIntoChildSlot(0, lepus::Value(part), lepus::Value());

  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->SetUid(lepus::Value(0));
  page->InsertNodeIntoChildSlot(0, lepus::Value(inner), lepus::Value());

  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);
  auto inner_root = inner->PeekMaterializedRoot();
  auto part_root = part->PeekMaterializedRoot();
  ASSERT_NE(inner_root, nullptr);
  ASSERT_NE(part_root, nullptr);
  ASSERT_TRUE(page_root->IsTemplateElement());
  ASSERT_TRUE(inner_root->IsTemplateElement());
  ASSERT_TRUE(part_root->IsTemplateElement());
  part_root->MarkPartElement(base::String("inner-part"));

  auto page_parts = TreeResolver::GetTemplateParts(page_root);
  EXPECT_FALSE(page_parts->GetValueOrNull("inner-part").has_value());
  auto inner_parts = TreeResolver::GetTemplateParts(inner_root);
  auto inner_part = inner_parts->GetValueOrNull("inner-part");
  ASSERT_TRUE(inner_part.has_value());
  ASSERT_TRUE(inner_part->IsRefCounted());
  EXPECT_EQ(inner_part->RefCounted().get(), part_root.get());

  auto scope_clone = TreeResolver::CloneElements(
      page_root, tasm->style_sheet_manager(DEFAULT_ENTRY_NAME), false,
      TreeResolver::CloningDepth::kTemplateScope);
  ASSERT_NE(scope_clone, nullptr);
  EXPECT_TRUE(scope_clone->children().empty());
}

TEST_P(ElementTemplateInstanceTest,
       DetachedMaterializedElementTemplateUpdatesAndReattachesDirectly) {
  auto initial_attributes = lepus::Dictionary::Create();
  initial_attributes->SetValue(base::String("data-state"),
                               lepus::Value("initial"));
  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTypedTag(base::String("view"));
  child->SetUid(lepus::Value(1));
  child->SetAttributes(lepus::Value(initial_attributes));
  auto parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  parent->SetTypedTag(base::String("view"));
  parent->SetUid(lepus::Value(5));
  parent->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());
  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->SetUid(lepus::Value(0));
  page->InsertNodeIntoChildSlot(0, lepus::Value(parent), lepus::Value());

  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);
  auto parent_root = parent->PeekMaterializedRoot();
  auto child_root = child->PeekMaterializedRoot();
  ASSERT_NE(parent_root, nullptr);
  ASSERT_NE(child_root, nullptr);
  ASSERT_EQ(DatasetValue(child_root.get(), "state")->StdString(), "initial");

  parent->RemoveNodeFromChildSlot(0, lepus::Value(child));
  ASSERT_EQ(child_root->parent(), nullptr);

  auto updated_attributes = lepus::Dictionary::Create();
  updated_attributes->SetValue(base::String("data-state"),
                               lepus::Value("reattached"));
  child->SetAttributes(lepus::Value(updated_attributes));
  ASSERT_EQ(DatasetValue(child_root.get(), "state")->StdString(), "reattached");

  parent->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());

  ASSERT_EQ(child_root->parent(), parent_root.get());
  ASSERT_NE(DatasetValue(child_root.get(), "state"), nullptr);
  EXPECT_EQ(DatasetValue(child_root.get(), "state")->StdString(), "reattached");
}

TEST_P(ElementTemplateInstanceTest,
       TypedPageRemoveAndInsertExposeBothDirectMutations) {
  auto observer = std::make_shared<RecordingInspectorElementObserver>();
  manager->SetInspectorElementObserver(observer);
  manager->dom_tree_enabled_ = true;

  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTypedTag(base::String("view"));
  child->SetUid(lepus::Value(1));

  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->SetUid(lepus::Value(0));
  page->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());

  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);
  auto child_root = child->PeekMaterializedRoot();
  ASSERT_NE(child_root, nullptr);
  ASSERT_EQ(page_root->children().size(), 1u);
  EXPECT_EQ(page_root->children()[0].get(), child_root.get());

  observer->removed_nodes.clear();
  observer->removed_node_parents.clear();
  observer->added_nodes.clear();
  observer->added_node_parents.clear();

  page->RemoveNodeFromChildSlot(0, lepus::Value(child));
  page->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());

  ASSERT_EQ(page_root->children().size(), 1u);
  EXPECT_EQ(page_root->children()[0].get(), child_root.get());
  if (ENABLE_INSPECTOR) {
    ASSERT_EQ(observer->removed_nodes.size(), 1u);
    EXPECT_EQ(observer->removed_nodes[0], child_root.get());
    EXPECT_EQ(observer->removed_node_parents[0], page_root.get());
    ASSERT_EQ(observer->added_nodes.size(), 1u);
    EXPECT_EQ(observer->added_nodes[0], child_root.get());
    EXPECT_EQ(observer->added_node_parents[0], page_root.get());
  }
}

TEST_P(ElementTemplateInstanceTest,
       TypedElementTemplateAppliesAttributesAsSpread) {
  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->SetTypedTag(base::String("view"));

  auto initial_attributes = lepus::Dictionary::Create();
  initial_attributes->SetValue(base::String("data-test"),
                               lepus::Value("before"));
  initial_attributes->SetValue(base::String("data-stale"),
                               lepus::Value("stale"));
  root->SetAttributes(lepus::Value(initial_attributes));
  EXPECT_EQ(root->PeekMaterializedRoot(), nullptr);

  auto updated_attributes = lepus::Dictionary::Create();
  updated_attributes->SetValue(base::String("data-test"),
                               lepus::Value("after"));
  updated_attributes->SetValue(base::String("data-added"),
                               lepus::Value("added"));
  updated_attributes->SetValue(base::String("bindtap"), lepus::Value("onTap"));
  root->SetAttributes(lepus::Value(updated_attributes));
  EXPECT_EQ(root->PeekMaterializedRoot(), nullptr);

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
  root->SetAttributeSlot(0, lepus::Value(reset_attributes));

  EXPECT_EQ(DatasetValue(resolved.get(), "test"), nullptr);
  added_data = DatasetValue(resolved.get(), "added");
  ASSERT_NE(added_data, nullptr);
  EXPECT_EQ(added_data->StdString(), "updated");
  EXPECT_EQ(resolved->event_map().count("tap"), 0u);

  root->SetAttributes(lepus::Value(lepus::Dictionary::Create()));
  EXPECT_EQ(DatasetValue(resolved.get(), "added"), nullptr);
  EXPECT_TRUE(root->Serialize().GetProperty("attributes").IsEmpty());

  root->SetAttributes(lepus::Value(updated_attributes));
  root->SetAttributes(lepus::Value());
  EXPECT_EQ(DatasetValue(resolved.get(), "test"), nullptr);
  EXPECT_EQ(DatasetValue(resolved.get(), "added"), nullptr);
  EXPECT_EQ(resolved->event_map().count("tap"), 0u);
  EXPECT_FALSE(root->Serialize().Contains("attributes"));
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateEntryPointsUseBaselineStoragePolicy) {
  auto nested = lepus::Dictionary::Create();
  nested->SetValue(base::String("value"), lepus::Value("before"));

  auto attributes = lepus::Dictionary::Create();
  attributes->SetValue(base::String("payload"), lepus::Value(nested));
  auto options = lepus::Dictionary::Create();
  options->SetValue(base::String("payload"), lepus::Value(nested));
  auto attribute_slots = lepus::CArray::Create();
  attribute_slots->emplace_back(lepus::Value(nested));

  auto typed = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  typed->SetTypedTag(base::String("view"));
  typed->SetAttributes(lepus::Value(attributes));
  typed->SetOptions(lepus::Value(options));

  auto compiled = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  compiled->SetTemplateKey(base::String("template"));
  compiled->SetAttributeSlots(lepus::Value(attribute_slots));
  compiled->SetAttributeSlot(1, lepus::Value(nested));

  nested->SetValue(base::String("value"), lepus::Value("after"));
  attributes->SetValue(base::String("late"), lepus::Value(true));
  options->SetValue(base::String("late"), lepus::Value(true));
  attribute_slots->set(0, lepus::Value("after"));

  auto typed_serialized = typed->Serialize();
  EXPECT_EQ(typed_serialized.GetProperty("attributes")
                .GetProperty("payload")
                .GetProperty("value")
                .StdString(),
            "before");
  EXPECT_FALSE(typed_serialized.GetProperty("attributes").Contains("late"));
  EXPECT_EQ(typed_serialized.GetProperty("options")
                .GetProperty("payload")
                .GetProperty("value")
                .StdString(),
            "after");
  EXPECT_TRUE(typed_serialized.GetProperty("options").Contains("late"));

  auto serialized_slots = compiled->Serialize().GetProperty("attributeSlots");
  EXPECT_EQ(serialized_slots.GetProperty(0).GetProperty("value").StdString(),
            "before");
  EXPECT_EQ(serialized_slots.GetProperty(1).GetProperty("value").StdString(),
            "before");
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateInstanceMaterializesCompiledRoot) {
  auto default_entry = std::make_shared<TemplateEntry>();
  default_entry->SetName(DEFAULT_ENTRY_NAME);
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = default_entry;

  auto template_info = std::make_shared<ElementTemplateInfo>();
  template_info->exist_ = true;
  template_info->key_ = "root_template";

  auto root_info = ElementInfo();
  root_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  root_info.attributes_ =
      std::make_shared<const TemplateAttributes>(TemplateAttributes{
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("data-test"),
                    lepus::Value(), 0}});

  auto slot_info = ElementInfo();
  slot_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_SLOT;
  slot_info.slot_index_ = 0;
  root_info.children_.emplace_back(std::move(slot_info));

  auto sentinel_info = ElementInfo();
  sentinel_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  sentinel_info.attrs_[base::String("id")] = lepus::Value("sentinel");
  root_info.children_.emplace_back(std::move(sentinel_info));

  template_info->elements_.emplace_back(std::move(root_info));
  default_entry->template_bundle_.element_template_infos_["root_template"] =
      std::move(template_info);

  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTypedTag(base::String("raw-text"));
  child->SetUid(lepus::Value(1));

  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(child));
  auto child_slots = lepus::CArray::Create();
  child_slots->emplace_back(lepus::Value(slot_children));

  auto attribute_slots = lepus::CArray::Create();
  attribute_slots->emplace_back(lepus::Value("compiled-value"));

  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->SetTASM(tasm.get());
  root->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  root->SetTemplateKey(base::String("root_template"));
  root->SetAttributeSlots(lepus::Value(attribute_slots));
  root->InitializeChildSlots(lepus::Value(child_slots));

  EXPECT_EQ(root->PeekMaterializedRoot(), nullptr);

  auto resolved = root->GetRoot();
  ASSERT_NE(resolved, nullptr);
  EXPECT_TRUE(resolved->is_view());
  EXPECT_TRUE(resolved->IsTemplateElement());
  auto* test_data = DatasetValue(resolved.get(), "test");
  ASSERT_NE(test_data, nullptr);
  EXPECT_EQ(test_data->StdString(), "compiled-value");

  auto child_root = child->PeekMaterializedRoot();
  ASSERT_NE(child_root, nullptr);
  ASSERT_EQ(resolved->children().size(), 2u);
  EXPECT_EQ(resolved->children()[0].get(), child_root.get());
  EXPECT_TRUE(child_root->is_raw_text());
  EXPECT_TRUE(child_root->IsTemplateElement());
  auto* sentinel = static_cast<Element*>(resolved->children()[1].get());
  ASSERT_NE(sentinel, nullptr);
  EXPECT_TRUE(sentinel->is_view());

  auto serialized = root->Serialize();
  EXPECT_EQ(serialized.GetProperty("childSlots")
                .GetProperty(0)
                .GetProperty(0)
                .GetProperty("uid")
                .Number(),
            1);
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateMaterializedUpdatesApplyDirectly) {
  auto page = manager->CreateFiberPage("page", 0);
  manager->SetFiberPageElement(page);

  auto default_entry = std::make_shared<TemplateEntry>();
  default_entry->SetName(DEFAULT_ENTRY_NAME);
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = default_entry;

  auto template_info = std::make_shared<ElementTemplateInfo>();
  template_info->exist_ = true;
  template_info->key_ = "root_template";

  auto root_info = ElementInfo();
  root_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  root_info.attributes_ =
      std::make_shared<const TemplateAttributes>(TemplateAttributes{
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("data-test"),
                    lepus::Value(), 0}});

  auto slot_info = ElementInfo();
  slot_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_SLOT;
  slot_info.slot_index_ = 0;
  root_info.children_.emplace_back(std::move(slot_info));

  auto sentinel_info = ElementInfo();
  sentinel_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  sentinel_info.attrs_[base::String("id")] = lepus::Value("sentinel");
  root_info.children_.emplace_back(std::move(sentinel_info));

  template_info->elements_.emplace_back(std::move(root_info));
  default_entry->template_bundle_.element_template_infos_["root_template"] =
      std::move(template_info);

  auto first = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  first->SetTypedTag(base::String("raw-text"));
  first->SetUid(lepus::Value(9));

  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(first));
  auto child_slots = lepus::CArray::Create();
  child_slots->emplace_back(lepus::Value(slot_children));

  auto attribute_slots = lepus::CArray::Create();
  attribute_slots->emplace_back(lepus::Value("initial-value"));

  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->SetTASM(tasm.get());
  root->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  root->SetTemplateKey(base::String("root_template"));
  root->SetAttributeSlots(lepus::Value(attribute_slots));
  root->InitializeChildSlots(lepus::Value(child_slots));

  auto resolved = root->GetRoot();
  ASSERT_NE(resolved, nullptr);
  auto first_root = first->PeekMaterializedRoot();
  ASSERT_NE(first_root, nullptr);
  ASSERT_EQ(resolved->children().size(), 2u);
  EXPECT_EQ(resolved->children()[0].get(), first_root.get());
  auto* test_data = DatasetValue(resolved.get(), "test");
  ASSERT_NE(test_data, nullptr);
  EXPECT_EQ(test_data->StdString(), "initial-value");

  auto second = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  second->SetTypedTag(base::String("raw-text"));
  second->SetUid(lepus::Value(10));
  auto third = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  third->SetTypedTag(base::String("raw-text"));
  third->SetUid(lepus::Value(11));
  auto fourth = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  fourth->SetTypedTag(base::String("raw-text"));
  fourth->SetUid(lepus::Value(12));

  root->SetAttributeSlot(0, lepus::Value("updated-value"));
  root->InsertNodeIntoChildSlot(0, lepus::Value(second), lepus::Value(first));
  root->InsertNodeIntoChildSlot(0, lepus::Value(third), lepus::Value(first));
  root->RemoveNodeFromChildSlot(0, lepus::Value(first));
  root->RemoveNodeFromChildSlot(0, lepus::Value(second));
  root->InsertNodeIntoChildSlot(0, lepus::Value(fourth), lepus::Value(third));

  EXPECT_FALSE(root->HasPendingChildMounts());
  test_data = DatasetValue(resolved.get(), "test");
  ASSERT_NE(test_data, nullptr);
  EXPECT_EQ(test_data->StdString(), "updated-value");
  auto fourth_root = fourth->PeekMaterializedRoot();
  ASSERT_NE(fourth_root, nullptr);
  auto third_root = third->PeekMaterializedRoot();
  ASSERT_NE(third_root, nullptr);
  ASSERT_EQ(resolved->children().size(), 3u);
  EXPECT_EQ(resolved->children()[0].get(), fourth_root.get());
  EXPECT_EQ(resolved->children()[1].get(), third_root.get());
  EXPECT_TRUE(fourth_root->is_raw_text());
  EXPECT_TRUE(fourth_root->IsTemplateElement());
  EXPECT_TRUE(third_root->is_raw_text());
  EXPECT_TRUE(third_root->IsTemplateElement());
  EXPECT_EQ(first_root->parent(), nullptr);
  auto second_root = second->PeekMaterializedRoot();
  ASSERT_NE(second_root, nullptr);
  EXPECT_EQ(second_root->parent(), nullptr);

  auto serialized = root->Serialize();
  auto serialized_slot_children =
      serialized.GetProperty("childSlots").GetProperty(0);
  ASSERT_TRUE(serialized_slot_children.IsArrayOrJSArray());
  ASSERT_EQ(serialized_slot_children.GetLength(), 2);
  EXPECT_EQ(serialized_slot_children.GetProperty(0).GetProperty("uid").Number(),
            12);
  EXPECT_EQ(serialized_slot_children.GetProperty(1).GetProperty("uid").Number(),
            11);
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateInsertNodeIntoChildSlotKeepsOtherSlots) {
  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->SetTemplateKey(base::String("root_template"));

  auto child_in_other_slot = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child_in_other_slot->SetTemplateKey(base::String("other_template"));
  auto child_to_insert = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child_to_insert->SetTemplateKey(base::String("inserted_template"));
  auto ref_node = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  ref_node->SetTemplateKey(base::String("ref_template"));

  auto first_slot_children = lepus::CArray::Create();
  first_slot_children->emplace_back(lepus::Value(child_in_other_slot));
  auto second_slot_children = lepus::CArray::Create();
  second_slot_children->emplace_back(lepus::Value(ref_node));
  auto child_slots = lepus::CArray::Create();
  child_slots->emplace_back(lepus::Value(first_slot_children));
  child_slots->emplace_back(lepus::Value(second_slot_children));
  root->InitializeChildSlots(lepus::Value(child_slots));

  root->InsertNodeIntoChildSlot(1, lepus::Value(child_to_insert),
                                lepus::Value(ref_node));

  auto serialized = root->Serialize();
  auto serialized_slots = serialized.GetProperty("childSlots");
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
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateInsertMovesAcrossSlotsAndParents) {
  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTemplateKey(base::String("child_template"));
  auto first_parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  first_parent->SetTemplateKey(base::String("first_parent"));
  auto second_parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  second_parent->SetTemplateKey(base::String("second_parent"));

  first_parent->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());
  first_parent->InsertNodeIntoChildSlot(1, lepus::Value(child), lepus::Value());

  auto first_parent_slots = first_parent->Serialize().GetProperty("childSlots");
  auto first_slot = first_parent_slots.GetProperty(0);
  auto second_slot = first_parent_slots.GetProperty(1);
  ASSERT_TRUE(first_slot.IsArrayOrJSArray());
  EXPECT_EQ(first_slot.GetLength(), 0);
  ASSERT_TRUE(second_slot.IsArrayOrJSArray());
  ASSERT_EQ(second_slot.GetLength(), 1);
  EXPECT_EQ(second_slot.GetProperty(0).GetProperty("templateKey").StdString(),
            "child_template");

  second_parent->InsertNodeIntoChildSlot(0, lepus::Value(child),
                                         lepus::Value());

  second_slot =
      first_parent->Serialize().GetProperty("childSlots").GetProperty(1);
  auto moved_slot =
      second_parent->Serialize().GetProperty("childSlots").GetProperty(0);
  ASSERT_TRUE(second_slot.IsArrayOrJSArray());
  EXPECT_EQ(second_slot.GetLength(), 0);
  ASSERT_TRUE(moved_slot.IsArrayOrJSArray());
  ASSERT_EQ(moved_slot.GetLength(), 1);
  EXPECT_EQ(moved_slot.GetProperty(0).GetProperty("templateKey").StdString(),
            "child_template");
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateSameSlotMovesConvergeBeforeAndAfterMaterialization) {
  auto first = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  first->SetTypedTag(base::String("raw-text"));
  first->SetUid(lepus::Value(9));
  auto second = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  second->SetTypedTag(base::String("raw-text"));
  second->SetUid(lepus::Value(10));
  auto third = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  third->SetTypedTag(base::String("raw-text"));
  third->SetUid(lepus::Value(11));

  auto parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  parent->SetTypedTag(base::String("view"));
  parent->SetUid(lepus::Value(5));
  parent->InsertNodeIntoChildSlot(0, lepus::Value(first), lepus::Value());
  parent->InsertNodeIntoChildSlot(0, lepus::Value(second), lepus::Value());
  parent->InsertNodeIntoChildSlot(0, lepus::Value(third), lepus::Value());

  parent->InsertNodeIntoChildSlot(0, lepus::Value(third), lepus::Value(first));
  parent->InsertNodeIntoChildSlot(0, lepus::Value(first), lepus::Value());
  EXPECT_EQ(parent->PeekMaterializedRoot(), nullptr);

  auto serialized_slot =
      parent->Serialize().GetProperty("childSlots").GetProperty(0);
  ASSERT_EQ(serialized_slot.GetLength(), 3);
  EXPECT_EQ(serialized_slot.GetProperty(0).GetProperty("uid").Number(), 11);
  EXPECT_EQ(serialized_slot.GetProperty(1).GetProperty("uid").Number(), 10);
  EXPECT_EQ(serialized_slot.GetProperty(2).GetProperty("uid").Number(), 9);

  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->SetUid(lepus::Value(0));
  page->InsertNodeIntoChildSlot(0, lepus::Value(parent), lepus::Value());

  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);
  auto parent_root = parent->PeekMaterializedRoot();
  auto first_root = first->PeekMaterializedRoot();
  auto second_root = second->PeekMaterializedRoot();
  auto third_root = third->PeekMaterializedRoot();
  ASSERT_NE(parent_root, nullptr);
  ASSERT_NE(first_root, nullptr);
  ASSERT_NE(second_root, nullptr);
  ASSERT_NE(third_root, nullptr);
  ASSERT_EQ(parent_root->children().size(), 3u);
  EXPECT_EQ(parent_root->children()[0].get(), third_root.get());
  EXPECT_EQ(parent_root->children()[1].get(), second_root.get());
  EXPECT_EQ(parent_root->children()[2].get(), first_root.get());

  parent->InsertNodeIntoChildSlot(0, lepus::Value(first), lepus::Value(third));
  parent->InsertNodeIntoChildSlot(0, lepus::Value(second), lepus::Value(first));

  ASSERT_EQ(parent_root->children().size(), 3u);
  EXPECT_EQ(parent_root->children()[0].get(), second_root.get());
  EXPECT_EQ(parent_root->children()[1].get(), first_root.get());
  EXPECT_EQ(parent_root->children()[2].get(), third_root.get());

  serialized_slot =
      parent->Serialize().GetProperty("childSlots").GetProperty(0);
  ASSERT_EQ(serialized_slot.GetLength(), 3);
  EXPECT_EQ(serialized_slot.GetProperty(0).GetProperty("uid").Number(), 10);
  EXPECT_EQ(serialized_slot.GetProperty(1).GetProperty("uid").Number(), 9);
  EXPECT_EQ(serialized_slot.GetProperty(2).GetProperty("uid").Number(), 11);

  auto before_self_reference = parent->Serialize();
  parent->InsertNodeIntoChildSlot(0, lepus::Value(first), lepus::Value(first));
  EXPECT_FALSE(parent->HasPendingChildMounts());
  EXPECT_EQ(parent->Serialize(), before_self_reference);
  ASSERT_EQ(parent_root->children().size(), 3u);
  EXPECT_EQ(parent_root->children()[0].get(), second_root.get());
  EXPECT_EQ(parent_root->children()[1].get(), first_root.get());
  EXPECT_EQ(parent_root->children()[2].get(), third_root.get());
}

TEST_P(
    ElementTemplateInstanceTest,
    ElementTemplateInitializationMovesOwnershipAndClearsParentOnDestruction) {
  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTemplateKey(base::String("child_template"));
  auto first_parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  first_parent->SetTemplateKey(base::String("first_parent"));

  auto first_children = lepus::CArray::Create();
  first_children->emplace_back(lepus::Value(child));
  auto first_slots = lepus::CArray::Create();
  first_slots->emplace_back(lepus::Value(first_children));
  first_parent->InitializeChildSlots(lepus::Value(first_slots));
  EXPECT_EQ(first_parent->Serialize()
                .GetProperty("childSlots")
                .GetProperty(0)
                .GetProperty(0)
                .GetProperty("templateKey")
                .StdString(),
            "child_template");

  auto second_parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  second_parent->SetTemplateKey(base::String("second_parent"));
  auto second_children = lepus::CArray::Create();
  second_children->emplace_back(lepus::Value(child));
  auto second_slots = lepus::CArray::Create();
  second_slots->emplace_back(lepus::Value(second_children));
  second_parent->InitializeChildSlots(lepus::Value(second_slots));

  auto detached_slot =
      first_parent->Serialize().GetProperty("childSlots").GetProperty(0);
  ASSERT_TRUE(detached_slot.IsArrayOrJSArray());
  EXPECT_EQ(detached_slot.GetLength(), 0);
  EXPECT_EQ(second_parent->Serialize()
                .GetProperty("childSlots")
                .GetProperty(0)
                .GetProperty(0)
                .GetProperty("templateKey")
                .StdString(),
            "child_template");

  second_parent = nullptr;
  auto third_parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  third_parent->SetTemplateKey(base::String("third_parent"));
  third_parent->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());
  EXPECT_EQ(third_parent->Serialize()
                .GetProperty("childSlots")
                .GetProperty(0)
                .GetProperty(0)
                .GetProperty("templateKey")
                .StdString(),
            "child_template");
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateInitializationLeavesDuplicateChildAtLastPosition) {
  auto moving_child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  moving_child->SetTemplateKey(base::String("moving_child"));
  auto first_parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  first_parent->SetTemplateKey(base::String("first_parent"));
  auto second_parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  second_parent->SetTemplateKey(base::String("second_parent"));

  auto first_children = lepus::CArray::Create();
  first_children->emplace_back(lepus::Value(moving_child));
  auto first_slots = lepus::CArray::Create();
  first_slots->emplace_back(lepus::Value(first_children));
  first_parent->InitializeChildSlots(lepus::Value(first_slots));

  auto duplicate_children = lepus::CArray::Create();
  duplicate_children->emplace_back(lepus::Value(moving_child));
  auto duplicate_children_other_slot = lepus::CArray::Create();
  duplicate_children_other_slot->emplace_back(lepus::Value(moving_child));
  auto duplicate_slots = lepus::CArray::Create();
  duplicate_slots->emplace_back(lepus::Value(duplicate_children));
  duplicate_slots->emplace_back(lepus::Value(duplicate_children_other_slot));

  second_parent->InitializeChildSlots(lepus::Value(duplicate_slots));
  auto first_slot =
      first_parent->Serialize().GetProperty("childSlots").GetProperty(0);
  auto second_slots = second_parent->Serialize().GetProperty("childSlots");
  auto second_first_slot = second_slots.GetProperty(0);
  auto second_last_slot = second_slots.GetProperty(1);
  ASSERT_TRUE(first_slot.IsArrayOrJSArray());
  ASSERT_TRUE(second_first_slot.IsArrayOrJSArray());
  ASSERT_TRUE(second_last_slot.IsArrayOrJSArray());
  EXPECT_EQ(first_slot.GetLength(), 0);
  EXPECT_EQ(second_first_slot.GetLength(), 0);
  ASSERT_EQ(second_last_slot.GetLength(), 1);
  EXPECT_EQ(
      second_last_slot.GetProperty(0).GetProperty("templateKey").StdString(),
      "moving_child");
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateInitializationAppliesNestedOwnershipMovesInOrder) {
  auto nested_child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  nested_child->SetTemplateKey(base::String("nested_child"));
  auto nested_parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  nested_parent->SetTemplateKey(base::String("nested_parent"));
  nested_parent->InsertNodeIntoChildSlot(0, lepus::Value(nested_child),
                                         lepus::Value());

  auto target = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  target->SetTemplateKey(base::String("target"));
  auto proposed_children = lepus::CArray::Create();
  proposed_children->emplace_back(lepus::Value(nested_parent));
  proposed_children->emplace_back(lepus::Value(nested_child));
  auto proposed_slots = lepus::CArray::Create();
  proposed_slots->emplace_back(lepus::Value(std::move(proposed_children)));

  target->InitializeChildSlots(lepus::Value(proposed_slots));
  auto target_children =
      target->Serialize().GetProperty("childSlots").GetProperty(0);
  ASSERT_TRUE(target_children.IsArrayOrJSArray());
  ASSERT_EQ(target_children.GetLength(), 2);
  EXPECT_EQ(
      target_children.GetProperty(0).GetProperty("templateKey").StdString(),
      "nested_parent");
  EXPECT_EQ(
      target_children.GetProperty(1).GetProperty("templateKey").StdString(),
      "nested_child");
  EXPECT_EQ(nested_parent->Serialize()
                .GetProperty("childSlots")
                .GetProperty(0)
                .GetLength(),
            0);
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateAdjacentChildSlotsPreserveDirectOrder) {
  auto default_entry = std::make_shared<TemplateEntry>();
  default_entry->SetName(DEFAULT_ENTRY_NAME);
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = default_entry;

  auto template_info = std::make_shared<ElementTemplateInfo>();
  template_info->exist_ = true;
  template_info->key_ = "adjacent_slots";
  auto root_info = ElementInfo();
  root_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  auto first_slot_info = ElementInfo();
  first_slot_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_SLOT;
  first_slot_info.slot_index_ = 0;
  root_info.children_.emplace_back(std::move(first_slot_info));
  auto second_slot_info = ElementInfo();
  second_slot_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_SLOT;
  second_slot_info.slot_index_ = 1;
  root_info.children_.emplace_back(std::move(second_slot_info));
  auto sentinel_info = ElementInfo();
  sentinel_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  root_info.children_.emplace_back(std::move(sentinel_info));
  template_info->elements_.emplace_back(std::move(root_info));
  default_entry->template_bundle_.element_template_infos_["adjacent_slots"] =
      std::move(template_info);

  auto first = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  first->SetTypedTag(base::String("view"));
  auto second = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  second->SetTypedTag(base::String("view"));
  auto third = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  third->SetTypedTag(base::String("view"));
  auto first_slot_children = lepus::CArray::Create();
  first_slot_children->emplace_back(lepus::Value(first));
  auto second_slot_children = lepus::CArray::Create();
  second_slot_children->emplace_back(lepus::Value(second));
  auto child_slots = lepus::CArray::Create();
  child_slots->emplace_back(lepus::Value(first_slot_children));
  child_slots->emplace_back(lepus::Value(second_slot_children));

  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->SetTASM(tasm.get());
  root->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  root->SetTemplateKey(base::String("adjacent_slots"));
  root->InitializeChildSlots(lepus::Value(child_slots));
  auto resolved = root->GetRoot();
  auto first_root = first->PeekMaterializedRoot();
  auto second_root = second->PeekMaterializedRoot();
  ASSERT_NE(resolved, nullptr);
  ASSERT_NE(first_root, nullptr);
  ASSERT_NE(second_root, nullptr);
  ASSERT_EQ(resolved->children().size(), 3u);
  EXPECT_EQ(resolved->children()[0].get(), first_root.get());
  EXPECT_EQ(resolved->children()[1].get(), second_root.get());

  root->InsertNodeIntoChildSlot(0, lepus::Value(third), lepus::Value());
  auto third_root = third->PeekMaterializedRoot();
  ASSERT_NE(third_root, nullptr);
  ASSERT_EQ(resolved->children().size(), 4u);
  EXPECT_EQ(resolved->children()[0].get(), first_root.get());
  EXPECT_EQ(resolved->children()[1].get(), third_root.get());
  EXPECT_EQ(resolved->children()[2].get(), second_root.get());

  root->InsertNodeIntoChildSlot(0, lepus::Value(second), lepus::Value(third));
  ASSERT_EQ(resolved->children().size(), 4u);
  EXPECT_EQ(resolved->children()[0].get(), first_root.get());
  EXPECT_EQ(resolved->children()[1].get(), second_root.get());
  EXPECT_EQ(resolved->children()[2].get(), third_root.get());
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateCrossSlotSwapAppliesInCallOrder) {
  auto page = manager->CreateFiberPage("page", 0);
  manager->SetFiberPageElement(page);

  auto observer = std::make_shared<RecordingInspectorElementObserver>();
  manager->SetInspectorElementObserver(observer);
  manager->dom_tree_enabled_ = true;

  auto default_entry = std::make_shared<TemplateEntry>();
  default_entry->SetName(DEFAULT_ENTRY_NAME);
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = default_entry;

  auto template_info = std::make_shared<ElementTemplateInfo>();
  template_info->exist_ = true;
  template_info->key_ = "root_template";

  auto root_info = ElementInfo();
  root_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  auto first_slot_info = ElementInfo();
  first_slot_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_SLOT;
  first_slot_info.slot_index_ = 0;
  root_info.children_.emplace_back(std::move(first_slot_info));
  auto first_sentinel_info = ElementInfo();
  first_sentinel_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  first_sentinel_info.attrs_[base::String("id")] =
      lepus::Value("first-sentinel");
  root_info.children_.emplace_back(std::move(first_sentinel_info));
  auto second_slot_info = ElementInfo();
  second_slot_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_SLOT;
  second_slot_info.slot_index_ = 1;
  root_info.children_.emplace_back(std::move(second_slot_info));
  auto second_sentinel_info = ElementInfo();
  second_sentinel_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  second_sentinel_info.attrs_[base::String("id")] =
      lepus::Value("second-sentinel");
  root_info.children_.emplace_back(std::move(second_sentinel_info));

  template_info->elements_.emplace_back(std::move(root_info));
  default_entry->template_bundle_.element_template_infos_["root_template"] =
      std::move(template_info);

  auto first = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  first->SetTypedTag(base::String("raw-text"));
  first->SetUid(lepus::Value(9));
  auto second = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  second->SetTypedTag(base::String("raw-text"));
  second->SetUid(lepus::Value(10));

  auto first_slot_children = lepus::CArray::Create();
  first_slot_children->emplace_back(lepus::Value(first));
  auto second_slot_children = lepus::CArray::Create();
  second_slot_children->emplace_back(lepus::Value(second));
  auto child_slots = lepus::CArray::Create();
  child_slots->emplace_back(lepus::Value(first_slot_children));
  child_slots->emplace_back(lepus::Value(second_slot_children));

  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->SetTASM(tasm.get());
  root->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  root->SetTemplateKey(base::String("root_template"));
  root->InitializeChildSlots(lepus::Value(child_slots));

  auto resolved = root->GetRoot();
  ASSERT_NE(resolved, nullptr);
  auto first_root = first->PeekMaterializedRoot();
  auto second_root = second->PeekMaterializedRoot();
  ASSERT_NE(first_root, nullptr);
  ASSERT_NE(second_root, nullptr);
  ASSERT_EQ(resolved->children().size(), 4u);
  EXPECT_EQ(resolved->children()[0].get(), first_root.get());
  EXPECT_EQ(resolved->children()[2].get(), second_root.get());

  observer->removed_nodes.clear();
  observer->removed_node_parents.clear();
  observer->added_nodes.clear();
  observer->added_node_parents.clear();

  root->InsertNodeIntoChildSlot(0, lepus::Value(second), lepus::Value());
  root->InsertNodeIntoChildSlot(1, lepus::Value(first), lepus::Value());

  ASSERT_EQ(resolved->children().size(), 4u);
  EXPECT_EQ(resolved->children()[0].get(), second_root.get());
  EXPECT_EQ(resolved->children()[2].get(), first_root.get());

  auto serialized_slots = root->Serialize().GetProperty("childSlots");
  ASSERT_EQ(serialized_slots.GetLength(), 2);
  EXPECT_EQ(serialized_slots.GetProperty(0)
                .GetProperty(0)
                .GetProperty("uid")
                .Number(),
            10);
  EXPECT_EQ(serialized_slots.GetProperty(1)
                .GetProperty(0)
                .GetProperty("uid")
                .Number(),
            9);

  if (ENABLE_INSPECTOR) {
    EXPECT_EQ(std::count(observer->removed_nodes.begin(),
                         observer->removed_nodes.end(), first_root.get()),
              1);
    EXPECT_EQ(std::count(observer->removed_nodes.begin(),
                         observer->removed_nodes.end(), second_root.get()),
              1);
    EXPECT_EQ(std::count(observer->added_nodes.begin(),
                         observer->added_nodes.end(), first_root.get()),
              1);
    EXPECT_EQ(std::count(observer->added_nodes.begin(),
                         observer->added_nodes.end(), second_root.get()),
              1);
    for (auto* parent_snapshot : observer->removed_node_parents) {
      EXPECT_EQ(parent_snapshot, resolved.get());
    }
    for (auto* parent_snapshot : observer->added_node_parents) {
      EXPECT_EQ(parent_snapshot, resolved.get());
    }
  }

  observer->removed_nodes.clear();
  observer->removed_node_parents.clear();
  observer->added_nodes.clear();
  observer->added_node_parents.clear();

  // Swap back with the inverse sequence of direct moves.
  root->InsertNodeIntoChildSlot(1, lepus::Value(second), lepus::Value());
  root->InsertNodeIntoChildSlot(0, lepus::Value(first), lepus::Value());

  ASSERT_EQ(resolved->children().size(), 4u);
  EXPECT_EQ(resolved->children()[0].get(), first_root.get());
  EXPECT_EQ(resolved->children()[2].get(), second_root.get());
  serialized_slots = root->Serialize().GetProperty("childSlots");
  EXPECT_EQ(serialized_slots.GetProperty(0)
                .GetProperty(0)
                .GetProperty("uid")
                .Number(),
            9);
  EXPECT_EQ(serialized_slots.GetProperty(1)
                .GetProperty(0)
                .GetProperty("uid")
                .Number(),
            10);
  if (ENABLE_INSPECTOR) {
    EXPECT_EQ(std::count(observer->removed_nodes.begin(),
                         observer->removed_nodes.end(), first_root.get()),
              1);
    EXPECT_EQ(std::count(observer->removed_nodes.begin(),
                         observer->removed_nodes.end(), second_root.get()),
              1);
    EXPECT_EQ(std::count(observer->added_nodes.begin(),
                         observer->added_nodes.end(), first_root.get()),
              1);
    EXPECT_EQ(std::count(observer->added_nodes.begin(),
                         observer->added_nodes.end(), second_root.get()),
              1);
    for (auto* parent_snapshot : observer->removed_node_parents) {
      EXPECT_EQ(parent_snapshot, resolved.get());
    }
    for (auto* parent_snapshot : observer->added_node_parents) {
      EXPECT_EQ(parent_snapshot, resolved.get());
    }
  }
}

TEST_P(ElementTemplateInstanceTest,
       RemovingAndMovingChildrenPreservesPendingCompiledMounts) {
  auto pending_child = CreateCompiledSpreadInstance();
  pending_child->SetAttributeSlot(0, lepus::Value(lepus::Dictionary::Create()));
  auto removed_child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  removed_child->SetTypedTag(base::String("view"));
  auto moved_child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  moved_child->SetTypedTag(base::String("view"));
  auto source = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  source->SetTypedTag(base::String("view"));
  source->InsertNodeIntoChildSlot(0, lepus::Value(pending_child),
                                  lepus::Value());
  source->InsertNodeIntoChildSlot(0, lepus::Value(removed_child),
                                  lepus::Value());
  source->InsertNodeIntoChildSlot(0, lepus::Value(moved_child), lepus::Value());
  auto destination = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  destination->SetTypedTag(base::String("view"));
  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->InsertNodeIntoChildSlot(0, lepus::Value(source), lepus::Value());
  page->InsertNodeIntoChildSlot(0, lepus::Value(destination), lepus::Value());
  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);
  auto source_root = source->PeekMaterializedRoot();
  auto destination_root = destination->PeekMaterializedRoot();
  ASSERT_NE(source_root, nullptr);
  ASSERT_NE(destination_root, nullptr);
  ASSERT_EQ(pending_child->PeekMaterializedRoot(), nullptr);

  source->RemoveNodeFromChildSlot(0, lepus::Value(removed_child));
  destination->InsertNodeIntoChildSlot(0, lepus::Value(moved_child),
                                       lepus::Value());
  manager->DrainPendingElementTemplateChildMounts(destination_root.get());
  EXPECT_EQ(pending_child->PeekMaterializedRoot(), nullptr);
  EXPECT_TRUE(source_root->children().empty());
  ASSERT_EQ(destination_root->children().size(), 1u);
  EXPECT_EQ(destination_root->children()[0].get(),
            moved_child->PeekMaterializedRoot().get());
  EXPECT_EQ(removed_child->PeekMaterializedRoot()->parent(), nullptr);

  manager->DrainPendingElementTemplateChildMounts(source_root.get());
  ASSERT_NE(pending_child->PeekMaterializedRoot(), nullptr);
  ASSERT_EQ(source_root->children().size(), 1u);
  EXPECT_EQ(source_root->children()[0].get(),
            pending_child->PeekMaterializedRoot().get());
  EXPECT_FALSE(source->HasPendingChildMounts());
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateCrossParentMoveNotifiesAfterDirectAttributeUpdate) {
  auto observer = std::make_shared<RecordingInspectorElementObserver>();
  manager->SetInspectorElementObserver(observer);
  manager->dom_tree_enabled_ = true;

  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTypedTag(base::String("raw-text"));
  child->SetUid(lepus::Value(1));

  auto source = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  source->SetTypedTag(base::String("view"));
  source->SetUid(lepus::Value(7));
  source->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());

  auto destination = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  destination->SetTypedTag(base::String("view"));
  destination->SetUid(lepus::Value(8));

  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->SetUid(lepus::Value(0));
  page->InsertNodeIntoChildSlot(0, lepus::Value(source), lepus::Value());
  page->InsertNodeIntoChildSlot(0, lepus::Value(destination), lepus::Value());

  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);
  auto source_root = source->PeekMaterializedRoot();
  auto destination_root = destination->PeekMaterializedRoot();
  auto child_root = child->PeekMaterializedRoot();
  ASSERT_NE(source_root, nullptr);
  ASSERT_NE(destination_root, nullptr);
  ASSERT_NE(child_root, nullptr);
  ASSERT_EQ(source_root->children().size(), 1u);
  EXPECT_EQ(source_root->children()[0].get(), child_root.get());
  EXPECT_TRUE(destination_root->children().empty());

  observer->removed_nodes.clear();
  observer->removed_node_parents.clear();
  observer->added_nodes.clear();
  observer->added_node_parents.clear();

  auto destination_attributes = lepus::Dictionary::Create();
  destination_attributes->SetValue(base::String("data-state"),
                                   lepus::Value("updated-first"));
  destination->SetAttributes(lepus::Value(destination_attributes));
  ASSERT_NE(DatasetValue(destination_root.get(), "state"), nullptr);
  EXPECT_EQ(DatasetValue(destination_root.get(), "state")->StdString(),
            "updated-first");
  destination->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());

  EXPECT_TRUE(source_root->children().empty());
  ASSERT_EQ(destination_root->children().size(), 1u);
  EXPECT_EQ(destination_root->children()[0].get(), child_root.get());
  EXPECT_EQ(child_root->parent(), destination_root.get());
  if (ENABLE_INSPECTOR) {
    ASSERT_EQ(observer->removed_nodes.size(), 1u);
    EXPECT_EQ(observer->removed_nodes[0], child_root.get());
    EXPECT_EQ(observer->removed_node_parents[0], source_root.get());
    ASSERT_EQ(observer->added_nodes.size(), 1u);
    EXPECT_EQ(observer->added_nodes[0], child_root.get());
    EXPECT_EQ(observer->added_node_parents[0], destination_root.get());
  }

  EXPECT_EQ(
      source->Serialize().GetProperty("childSlots").GetProperty(0).GetLength(),
      0);
  EXPECT_EQ(destination->Serialize()
                .GetProperty("childSlots")
                .GetProperty(0)
                .GetProperty(0)
                .GetProperty("uid")
                .Number(),
            1);
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateMaterializesStaticEventListeners) {
  manager->config_->SetEnableEventHandleRefactor(true);
  manager->SetConfig(manager->config_);
  tasm->page_config_ = manager->config_;

  auto default_entry = std::make_shared<TemplateEntry>();
  default_entry->SetName(DEFAULT_ENTRY_NAME);
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = default_entry;

  auto template_info = std::make_shared<ElementTemplateInfo>();
  template_info->exist_ = true;
  template_info->key_ = "root_template";

  auto target_info = ElementInfo();
  target_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  target_info.attributes_ =
      std::make_shared<const TemplateAttributes>(TemplateAttributes{
          Attribute{ATTRIBUTE_BINDING_TYPE_STATIC, base::String("bindtap"),
                    lepus::Value("onStaticTap"), 0}});
  template_info->elements_.emplace_back(std::move(target_info));
  default_entry->template_bundle_.element_template_infos_["root_template"] =
      std::move(template_info);

  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->SetTASM(tasm.get());
  root->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  root->SetTemplateKey(base::String("root_template"));

  auto resolved = root->GetRoot();

  ASSERT_NE(resolved, nullptr);
  EXPECT_EQ(resolved->element_manager(), manager);
  auto* listeners = resolved->GetEventListenerMap()->Find("tap");
  ASSERT_NE(listeners, nullptr);
  ASSERT_EQ(listeners->size(), 1u);
  EXPECT_FALSE(listeners->front()->GetOptions().IsCapture());
  EXPECT_FALSE(listeners->front()->GetOptions().IsCatch());
}

TEST_P(ElementTemplateInstanceTest,
       CompiledAttributeSlotsUpdatedAfterPrepareUseLatestState) {
  auto instance = CreateCompiledSpreadInstance();
  auto initial_slot = lepus::Dictionary::Create();
  initial_slot->SetValue(base::String("data-slot-stale"), lepus::Value("old"));
  instance->SetAttributeSlot(0, lepus::Value(initial_slot));
  // Finish detached preparation before updating the logical slot snapshot.
  instance->RequestMaterializationRecursively();
  instance->create_element_tree_task_->Run();

  auto latest_slot = lepus::Dictionary::Create();
  latest_slot->SetValue(base::String("data-slot-current"), lepus::Value("new"));
  instance->SetAttributeSlot(0, lepus::Value(latest_slot));

  auto root = instance->GetRoot();
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(DatasetValue(root.get(), "slot-stale"), nullptr);
  ASSERT_NE(DatasetValue(root.get(), "slot-current"), nullptr);
  EXPECT_EQ(DatasetValue(root.get(), "slot-current")->StdString(), "new");
}

TEST_P(ElementTemplateInstanceTest, CompiledSpreadUpdatesPreserveCallbacks) {
  auto lepus_runtime = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  lepus_runtime->Initialize();
  lepus_runtime->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  lepus::BytecodeGenerator::GenerateBytecode(
      lepus_runtime->GetMTSContext(),
      "let initialCallback = () => 1; let updatedCallback = () => 2;",
      lepus_runtime->GetSdkVersion(), "");
  ASSERT_TRUE(lepus_runtime->Execute(nullptr));
  auto initial_callback = lepus_runtime->GetGlobalData("initialCallback");
  auto updated_callback = lepus_runtime->GetGlobalData("updatedCallback");
  ASSERT_TRUE(initial_callback.IsCallable());
  ASSERT_TRUE(updated_callback.IsCallable());

  for (bool materialize_before_update : {false, true}) {
    SCOPED_TRACE(materialize_before_update);
    auto instance = CreateCompiledSpreadInstance();
    auto initial = lepus::Dictionary::Create();
    initial->SetValue(base::String("main-thread:bindtap"), initial_callback);
    auto slots = lepus::CArray::Create();
    slots->emplace_back(lepus::Value(initial));
    instance->SetAttributeSlots(lepus::Value(slots));
    if (materialize_before_update) {
      ASSERT_NE(instance->GetRoot(), nullptr);
    } else {
      instance->RequestMaterializationRecursively();
      instance->create_element_tree_task_->Run();
    }

    auto updated = lepus::Dictionary::Create();
    updated->SetValue(base::String("main-thread:bindtap"), updated_callback);
    instance->SetAttributeSlot(0, lepus::Value(updated));

    auto stored_callback = instance->Serialize()
                               .GetProperty("attributeSlots")
                               .GetProperty(0)
                               .GetProperty("main-thread:bindtap");
    ASSERT_TRUE(stored_callback.IsCallable());
    EXPECT_TRUE(stored_callback.IsEqual(updated_callback));

    auto root = instance->GetRoot();
    ASSERT_NE(root, nullptr);
    auto event = root->event_map().find("tap");
    ASSERT_NE(event, root->event_map().end());
    EXPECT_FALSE(event->second->is_js_event());
    auto handler = event->second->lepus_function();
    ASSERT_TRUE(handler.IsCallable());
    EXPECT_TRUE(handler.IsEqual(updated_callback));
    EXPECT_EQ(lepus_runtime->CallClosure(handler).Number(), 2);
  }
}

TEST_P(ElementTemplateInstanceTest,
       MaterializedCompiledSpreadClearsRemovedKeys) {
  auto instance = CreateCompiledSpreadInstance();
  auto initial = lepus::Dictionary::Create();
  initial->SetValue(base::String("data-stale"), lepus::Value("old"));
  initial->SetValue(base::String("data-current"), lepus::Value("before"));
  instance->SetAttributeSlot(0, lepus::Value(initial));

  auto root = instance->GetRoot();
  ASSERT_NE(root, nullptr);
  ASSERT_NE(DatasetValue(root.get(), "stale"), nullptr);
  ASSERT_NE(DatasetValue(root.get(), "current"), nullptr);
  EXPECT_EQ(DatasetValue(root.get(), "current")->StdString(), "before");

  auto updated = lepus::Dictionary::Create();
  updated->SetValue(base::String("data-current"), lepus::Value("after"));
  instance->SetAttributeSlot(0, lepus::Value(updated));

  EXPECT_EQ(DatasetValue(root.get(), "stale"), nullptr);
  ASSERT_NE(DatasetValue(root.get(), "current"), nullptr);
  EXPECT_EQ(DatasetValue(root.get(), "current")->StdString(), "after");

  instance->SetAttributeSlot(0, lepus::Value(lepus::Dictionary::Create()));
  EXPECT_EQ(DatasetValue(root.get(), "current"), nullptr);
}

TEST_P(ElementTemplateInstanceTest,
       PageFlushMountsCompiledSubtreeWithLatestAttributes) {
  manager->config_->SetEnableEventHandleRefactor(true);
  manager->SetConfig(manager->config_);
  tasm->page_config_ = manager->config_;

  auto default_entry = std::make_shared<TemplateEntry>();
  default_entry->SetName(DEFAULT_ENTRY_NAME);
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = default_entry;

  auto template_info = std::make_shared<ElementTemplateInfo>();
  template_info->exist_ = true;
  template_info->key_ = "prepared_child";
  auto root_info = ElementInfo();
  root_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  root_info.attributes_ =
      std::make_shared<const TemplateAttributes>(TemplateAttributes{
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("data-test"),
                    lepus::Value(), 0},
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("bindtap"),
                    lepus::Value(), 1},
          Attribute{ATTRIBUTE_BINDING_TYPE_SPREAD, base::String("spread"),
                    lepus::Value(), 2}});
  auto child_slot_info = ElementInfo();
  child_slot_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_SLOT;
  child_slot_info.slot_index_ = 0;
  root_info.children_.emplace_back(std::move(child_slot_info));
  template_info->elements_.emplace_back(std::move(root_info));
  default_entry->template_bundle_.element_template_infos_["prepared_child"] =
      std::move(template_info);

  auto grandchild_template_info = std::make_shared<ElementTemplateInfo>();
  grandchild_template_info->exist_ = true;
  grandchild_template_info->key_ = "prepared_grandchild";
  auto grandchild_root_info = ElementInfo();
  grandchild_root_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  grandchild_template_info->elements_.emplace_back(
      std::move(grandchild_root_info));
  default_entry->template_bundle_
      .element_template_infos_["prepared_grandchild"] =
      std::move(grandchild_template_info);

  auto grandchild = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  grandchild->SetTASM(tasm.get());
  grandchild->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  grandchild->SetTemplateKey(base::String("prepared_grandchild"));

  auto initial_attribute_slots = lepus::CArray::Create();
  initial_attribute_slots->emplace_back(lepus::Value("before"));
  initial_attribute_slots->emplace_back(lepus::Value("onBeforeTap"));
  auto initial_spread = lepus::Dictionary::Create();
  initial_spread->SetValue("data-old", lepus::Value("old-value"));
  initial_attribute_slots->emplace_back(lepus::Value(initial_spread));
  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTASM(tasm.get());
  child->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  child->SetTemplateKey(base::String("prepared_child"));
  child->SetAttributeSlots(lepus::Value(initial_attribute_slots));
  auto child_slot_children = lepus::CArray::Create();
  child_slot_children->emplace_back(lepus::Value(grandchild));
  auto child_slots = lepus::CArray::Create();
  child_slots->emplace_back(lepus::Value(child_slot_children));
  child->InitializeChildSlots(lepus::Value(child_slots));

  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->SetUid(lepus::Value(0));
  page->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());

  EXPECT_EQ(child->PeekMaterializedRoot(), nullptr);
  EXPECT_EQ(grandchild->PeekMaterializedRoot(), nullptr);

  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);
  EXPECT_TRUE(page_root->children().empty());
  EXPECT_EQ(child->PeekMaterializedRoot(), nullptr);
  EXPECT_EQ(grandchild->PeekMaterializedRoot(), nullptr);

  child->SetAttributeSlot(0, lepus::Value("after"));
  child->SetAttributeSlot(1, lepus::Value("onAfterTap"));
  auto final_spread = lepus::Dictionary::Create();
  final_spread->SetValue("data-new", lepus::Value("new-value"));
  child->SetAttributeSlot(2, lepus::Value(final_spread));
  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page_root.get());

  auto child_root = child->PeekMaterializedRoot();
  ASSERT_NE(child_root, nullptr);
  auto grandchild_root = grandchild->PeekMaterializedRoot();
  ASSERT_NE(grandchild_root, nullptr);
  ASSERT_EQ(page_root->children().size(), 1u);
  EXPECT_EQ(page_root->children()[0].get(), child_root.get());
  ASSERT_EQ(child_root->children().size(), 1u);
  EXPECT_EQ(child_root->children()[0].get(), grandchild_root.get());
  auto* test_data = DatasetValue(child_root.get(), "test");
  ASSERT_NE(test_data, nullptr);
  EXPECT_EQ(test_data->StdString(), "after");
  EXPECT_EQ(DatasetValue(child_root.get(), "old"), nullptr);
  auto* new_data = DatasetValue(child_root.get(), "new");
  ASSERT_NE(new_data, nullptr);
  EXPECT_EQ(new_data->StdString(), "new-value");
  auto event_iter = child_root->event_map().find("tap");
  ASSERT_NE(event_iter, child_root->event_map().end());
  EXPECT_EQ(event_iter->second->function(), "onAfterTap");
  auto* listeners = child_root->GetEventListenerMap()->Find("tap");
  ASSERT_NE(listeners, nullptr);
  EXPECT_EQ(listeners->size(), 1u);
}

TEST_P(ElementTemplateInstanceTest,
       PageFlushCompletesNestedAttrSlotMaterialization) {
  ExpectPageFlushCompletesNestedAttrSlotMaterialization(false);
}

TEST_P(ElementTemplateInstanceTest,
       PageFlushCompletesNestedAttrSlotMaterializationBeforeInsertion) {
  ExpectPageFlushCompletesNestedAttrSlotMaterialization(true);
}

TEST_P(ElementTemplateInstanceTest,
       AttrSlotMaterializedPendingDescendantsRespectFlushScope) {
  auto detached = CreateCompiledMountChain(3);
  auto attached = CreateCompiledMountChain(3);
  auto page_root = manager->CreateFiberPage("page", 0);
  manager->SetFiberPageElement(page_root);
  auto scope_root = manager->CreateFiberView();
  page_root->InsertNode(scope_root);
  auto runtime = CreatePAPIRuntime();
  auto* ctx = runtime::MTSRuntime::ToQuickContext(runtime.get());
  for (const auto& instance :
       {detached[1], detached[0], attached[1], attached[0]}) {
    lepus::Value args[] = {lepus::Value(instance), lepus::Value(0)};
    ASSERT_TRUE(RendererFunctions::FiberGetElementByAttrSlot(ctx, args, 2)
                    .IsRefCounted());
  }
  page_root->InsertNode(attached[0]->PeekMaterializedRoot());
  ASSERT_EQ(manager->pending_element_template_child_mounts_.size(), 2u);

  // Neither subtree belongs to this scope, including the one attached elsewhere
  // on the same page. Repeated drains must return without resolving either
  // leaf.
  for (int i = 0; i < 2; ++i) {
    manager->DrainPendingElementTemplateChildMounts(scope_root.get());
    EXPECT_EQ(detached[2]->PeekMaterializedRoot(), nullptr);
    EXPECT_EQ(attached[2]->PeekMaterializedRoot(), nullptr);
    EXPECT_TRUE(detached[1]->HasPendingChildMounts());
    EXPECT_TRUE(attached[1]->HasPendingChildMounts());
    EXPECT_EQ(manager->pending_element_template_child_mounts_.size(), 2u);
  }

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page_root.get());
  ASSERT_NE(attached[2]->PeekMaterializedRoot(), nullptr);
  EXPECT_EQ(attached[2]->PeekMaterializedRoot()->parent(),
            attached[1]->PeekMaterializedRoot().get());
  EXPECT_FALSE(attached[1]->HasPendingChildMounts());
  EXPECT_EQ(detached[2]->PeekMaterializedRoot(), nullptr);
  EXPECT_EQ(manager->pending_element_template_child_mounts_.size(), 1u);

  scope_root->InsertNode(detached[0]->PeekMaterializedRoot());
  options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page_root.get());
  ASSERT_NE(detached[2]->PeekMaterializedRoot(), nullptr);
  EXPECT_EQ(detached[2]->PeekMaterializedRoot()->parent(),
            detached[1]->PeekMaterializedRoot().get());
  EXPECT_FALSE(detached[1]->HasPendingChildMounts());
  EXPECT_TRUE(manager->pending_element_template_child_mounts_.empty());
}

TEST_P(ElementTemplateInstanceTest,
       DetachedPendingCompiledChildMountWakesOnReattach) {
  auto child = CreateCompiledSpreadInstance();

  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->SetUid(lepus::Value(0));
  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);

  auto parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  parent->SetTypedTag(base::String("view"));
  auto parent_root = parent->GetRoot();
  ASSERT_NE(parent_root, nullptr);
  parent->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());

  ASSERT_TRUE(parent->HasPendingChildMounts());
  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page_root.get());

  EXPECT_TRUE(parent->HasPendingChildMounts());
  EXPECT_EQ(child->PeekMaterializedRoot(), nullptr);

  page->InsertNodeIntoChildSlot(0, lepus::Value(parent), lepus::Value());
  EXPECT_EQ(parent_root->parent(), page_root.get());

  options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page_root.get());

  auto child_root = child->PeekMaterializedRoot();
  ASSERT_NE(child_root, nullptr);
  EXPECT_EQ(child_root->parent(), parent_root.get());
  EXPECT_FALSE(parent->HasPendingChildMounts());
}

TEST_P(ElementTemplateInstanceTest,
       PendingCompiledChildMoveUsesFinalParentAndScopedDrain) {
  auto child = CreateCompiledSpreadInstance();

  auto source = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  source->SetTypedTag(base::String("view"));
  auto destination = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  destination->SetTypedTag(base::String("view"));
  auto sibling = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  sibling->SetTypedTag(base::String("view"));
  destination->InsertNodeIntoChildSlot(0, lepus::Value(sibling),
                                       lepus::Value());

  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->SetUid(lepus::Value(0));
  page->InsertNodeIntoChildSlot(0, lepus::Value(source), lepus::Value());
  page->InsertNodeIntoChildSlot(0, lepus::Value(destination), lepus::Value());
  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);
  auto source_root = source->PeekMaterializedRoot();
  auto destination_root = destination->PeekMaterializedRoot();
  auto sibling_root = sibling->PeekMaterializedRoot();
  ASSERT_NE(source_root, nullptr);
  ASSERT_NE(destination_root, nullptr);
  ASSERT_NE(sibling_root, nullptr);

  page_root->FlushActionsAsRoot();

  source->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());
  destination->InsertNodeIntoChildSlot(0, lepus::Value(child),
                                       lepus::Value(sibling));

  ASSERT_TRUE(source->HasPendingChildMounts());
  ASSERT_TRUE(destination->HasPendingChildMounts());
  source_root->FlushActionsAsRoot();
  EXPECT_FALSE(source->HasPendingChildMounts());
  EXPECT_TRUE(destination->HasPendingChildMounts());
  EXPECT_EQ(child->PeekMaterializedRoot(), nullptr);

  destination_root->FlushActionsAsRoot();

  auto child_root = child->PeekMaterializedRoot();
  ASSERT_NE(child_root, nullptr);
  ASSERT_EQ(destination_root->children().size(), 2u);
  EXPECT_EQ(destination_root->children()[0].get(), child_root.get());
  EXPECT_EQ(destination_root->children()[1].get(), sibling_root.get());
  EXPECT_EQ(child_root->parent(), destination_root.get());
  EXPECT_TRUE(source_root->children().empty());
  EXPECT_FALSE(destination->HasPendingChildMounts());
}

TEST_P(ElementTemplateInstanceTest, FlushSkipsRemovedPendingCompiledChild) {
  auto child = CreateCompiledSpreadInstance();
  auto page_root = manager->CreateFiberPage("page", 0);
  auto parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  parent->SetTypedTag(base::String("view"));
  auto parent_root = parent->GetRoot();
  ASSERT_NE(parent_root, nullptr);
  page_root->InsertNode(parent_root);
  page_root->FlushActionsAsRoot();
  parent->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());
  ASSERT_TRUE(parent->HasPendingChildMounts());

  parent->RemoveNodeFromChildSlot(0, lepus::Value(child));
  parent_root->FlushActionsAsRoot();

  EXPECT_TRUE(parent_root->children().empty());
  EXPECT_EQ(child->PeekMaterializedRoot(), nullptr);
  EXPECT_FALSE(parent->HasPendingChildMounts());
}

TEST_P(ElementTemplateInstanceTest, FlushDoesNotRetainPendingTemplateOwner) {
  auto child = CreateCompiledSpreadInstance();
  auto page_root = manager->CreateFiberPage("page", 0);
  auto parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  parent->SetTypedTag(base::String("view"));
  auto parent_root = parent->GetRoot();
  ASSERT_NE(parent_root, nullptr);
  page_root->InsertNode(parent_root);
  page_root->FlushActionsAsRoot();
  parent->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());
  ASSERT_TRUE(parent->HasPendingChildMounts());

  auto weak_parent = parent->WeakFromThis();
  parent = nullptr;
  EXPECT_FALSE(weak_parent);
  parent_root->FlushActionsAsRoot();

  EXPECT_TRUE(parent_root->children().empty());
  EXPECT_EQ(child->PeekMaterializedRoot(), nullptr);
}

TEST_P(ElementTemplateInstanceTest, ListSchedulerFlushMountsCompiledChild) {
  auto page_root = manager->CreateFiberPage("page", 0);
  for (auto strategy :
       {list::BatchRenderStrategy::kAsyncResolveProperty,
        list::BatchRenderStrategy::kAsyncResolvePropertyAndElementTree}) {
    SCOPED_TRACE(static_cast<int>(strategy));
    auto child = CreateCompiledSpreadInstance();
    auto parent = fml::AdoptRef<ElementTemplateInstance>(
        new ElementTemplateInstance(manager));
    parent->SetTypedTag(base::String("view"));
    auto parent_root = parent->GetRoot();
    ASSERT_NE(parent_root, nullptr);
    page_root->InsertNode(parent_root);
    page_root->FlushActionsAsRoot();
    parent_root->CreateListItemScheduler(strategy, true);
    parent_root->RecursivelyMarkRenderRootElement(parent_root.get());
    parent->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());
    ASSERT_EQ(child->PeekMaterializedRoot(), nullptr);

    auto* scheduler = parent_root->GetSchedulerAdapter();
    auto& tasks = manager->ParallelResolveTreeTasks();
    scheduler->ResolveElementTree(tasks);
    while (!tasks.empty()) {
      tasks.front()->Run();
      tasks.front()->GetFuture().get()();
      tasks.pop_front();
    }

    auto child_root = child->PeekMaterializedRoot();
    ASSERT_NE(child_root, nullptr);
    ASSERT_EQ(parent_root->children().size(), 1u);
    EXPECT_EQ(parent_root->children()[0].get(), child_root.get());
    EXPECT_EQ(child_root->parent(), parent_root.get());
    EXPECT_FALSE(parent->HasPendingChildMounts());
  }
}

TEST_P(ElementTemplateInstanceTest,
       InspectorReceivesFinalTypedAndCompiledAttributeUpdates) {
  if (!ENABLE_INSPECTOR) {
    GTEST_SKIP();
  }
  auto observer = std::make_shared<RecordingInspectorElementObserver>();
  manager->SetInspectorElementObserver(observer);
  manager->devtool_flag_ = true;
  manager->dom_tree_enabled_ = true;

  auto typed = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  typed->SetTypedTag(base::String("raw-text"));
  auto compiled = CreateCompiledSpreadInstance(true);
  for (const auto& instance : {typed, compiled}) {
    auto attributes = lepus::Dictionary::Create();
    attributes->SetValue("text", lepus::Value("before"));
    auto style = lepus::Dictionary::Create();
    style->SetValue("width", lepus::Value("10px"));
    attributes->SetValue("style", lepus::Value(style));
    instance->SetAttributeSlot(0, lepus::Value(attributes));
    observer->modified_nodes.clear();
    instance->RequestMaterializationRecursively();
    EXPECT_TRUE(observer->modified_nodes.empty());
    auto root = instance->GetRoot();
    ASSERT_NE(root, nullptr);
    auto* target = instance == typed ? root.get() : root->children()[0].get();
    ASSERT_NE(root->inspector_attribute(), nullptr);
    EXPECT_TRUE(observer->modified_nodes.empty());
    EXPECT_NE(target->data_model()->inline_styles().find(kPropertyIDWidth),
              target->data_model()->inline_styles().end());

    const auto initialized_count = observer->initialized_nodes.size();
    auto* inspector = root->inspector_attribute();
    EXPECT_EQ(instance->GetRoot(), root);
    EXPECT_EQ(root->inspector_attribute(), inspector);
    EXPECT_EQ(observer->initialized_nodes.size(), initialized_count);

    attributes = lepus::Dictionary::Create();
    attributes->SetValue("text", lepus::Value("after"));
    instance->SetAttributeSlot(0, lepus::Value(attributes));
    ASSERT_EQ(observer->modified_nodes.size(), 1u);
    EXPECT_EQ(observer->modified_nodes.back(), target);
    EXPECT_EQ(observer->modified_texts.back().StdString(), "after");
    EXPECT_EQ(instance->PeekMaterializedRoot(), root);
    instance->SetAttributeSlot(0, lepus::Value());
    ASSERT_EQ(observer->modified_nodes.size(), 2u);
    EXPECT_EQ(observer->modified_nodes.back(), target);
    EXPECT_TRUE(observer->modified_texts.back().IsEmpty());
  }
}

TEST_P(ElementTemplateInstanceTest,
       InspectorStringStyleUsesExistingFlushNotification) {
  if (!ENABLE_INSPECTOR) {
    GTEST_SKIP();
  }
  auto observer = std::make_shared<RecordingInspectorElementObserver>();
  manager->SetInspectorElementObserver(observer);
  manager->devtool_flag_ = true;
  manager->dom_tree_enabled_ = true;
  auto instance = CreateCompiledSpreadInstance();
  auto root = instance->GetRoot();
  ASSERT_NE(root, nullptr);
  auto attributes = lepus::Dictionary::Create();
  attributes->SetValue("style", lepus::Value("width: 23px"));
  instance->SetAttributeSlot(0, lepus::Value(attributes));
  EXPECT_TRUE(observer->modified_nodes.empty());
  root->ProcessFullRawInlineStyle(nullptr);
  ASSERT_EQ(observer->modified_nodes.size(), 1u);
  EXPECT_EQ(observer->modified_nodes.back(), root.get());
  root->ProcessFullRawInlineStyle(nullptr);
  EXPECT_EQ(observer->modified_nodes.size(), 1u);
}

TEST_P(ElementTemplateInstanceTest,
       GestureValueConstructorPreservesCallableAndObjectCallbacks) {
  auto runtime = CreatePAPIRuntime();
  auto* context = runtime::MTSRuntime::ToQuickContext(runtime.get());
  ASSERT_NE(context, nullptr);
  lepus::Value value;
  constexpr char source[] = R"JS(({
    config: {enabled: true},
    callbacks: [
      {name: 'onUpdate', callback: {state: 0}},
      {name: 'onEnd', callback: function() { return 42; }},
      {name: 'invalid', callback: 42},
      {name: 42, callback: {}}
    ],
    relationMap: {waitFor: [11, 'ignored', 12],
      simultaneous: {first: 21, second: 22}, continueWith: []}
  }))JS";
  ASSERT_TRUE(context->EvalBuf(source, sizeof(source) - 1, value,
                               "gesture_constructor.js"));
  GestureDetectorImpl detector(1, GestureType::PAN, value,
                               value.GetProperty("relationMap"), runtime.get());
  EXPECT_EQ(detector.gesture_id(), 1u);
  EXPECT_EQ(detector.gesture_type(), GestureType::PAN);
  ASSERT_EQ(detector.gesture_callbacks().size(), 2u);
  const auto& object = detector.gesture_callbacks()[0];
  const auto& function = detector.gesture_callbacks()[1];
  EXPECT_EQ(object.name_, "onUpdate");
  EXPECT_EQ(function.name_, "onEnd");
  EXPECT_EQ(object.ctx_, runtime.get());
  EXPECT_EQ(function.ctx_, runtime.get());
  EXPECT_TRUE(object.lepus_object_.IsEqual(
      value.GetProperty("callbacks").GetProperty(0).GetProperty("callback")));
  EXPECT_TRUE(function.lepus_function_.IsCallable());
  EXPECT_TRUE(function.lepus_function_.IsEqual(
      value.GetProperty("callbacks").GetProperty(1).GetProperty("callback")));
  EXPECT_TRUE(function.lepus_object_.IsEmpty());
  EXPECT_EQ(detector.relation_map().at(kGestureWaitFor),
            std::vector<uint32_t>({11, 12}));
  auto simultaneous = detector.relation_map().at(kGestureSimultaneous);
  std::sort(simultaneous.begin(), simultaneous.end());
  EXPECT_EQ(simultaneous, std::vector<uint32_t>({21, 22}));
  EXPECT_TRUE(detector.relation_map().at(kGestureContinueWith).empty());
  value.GetProperty("config").SetProperty("enabled", lepus::Value(false));
  EXPECT_TRUE(
      detector.gesture_config_in_lepus_value().GetProperty("enabled").Bool());
}

TEST_P(ElementTemplateInstanceTest,
       GestureStoragePreservesLiveCallbacksAndIsolatesWorkerPreparation) {
  auto runtime = CreatePAPIRuntime();
  auto* context = runtime::MTSRuntime::ToQuickContext(runtime.get());
  ASSERT_NE(context, nullptr);
  auto instance = CreateCompiledCallbackInstance(runtime, true);
  lepus::Value descriptor;
  constexpr char source[] = R"JS(({
    type: 'gesture',
    value: [{id: 1, type: 0, config: {enabled: true},
      callbacks: [{name: 'onUpdate', callback: {state: 0}}],
      relationMap: {waitFor: [], simultaneous: [], continueWith: []}}]
  }))JS";
  ASSERT_TRUE(context->EvalBuf(source, sizeof(source) - 1, descriptor,
                               "gesture_storage.js"));
  const auto source_entry = descriptor.GetProperty("value").GetProperty(0);
  auto callback = source_entry.GetProperty("callbacks")
                      .GetProperty(0)
                      .GetProperty("callback");
  ASSERT_TRUE(callback.IsJSValue());
  auto slots = lepus::CArray::Create();
  slots->emplace_back(descriptor);
  instance->SetAttributeSlots(lepus::Value(slots));
  EXPECT_TRUE(instance->attribute_slots_.GetProperty(0).IsEqual(descriptor));
  auto stored_entry = instance->attribute_slots_.GetProperty(0)
                          .GetProperty("value")
                          .GetProperty(0);
  auto stored_callback = stored_entry.GetProperty("callbacks")
                             .GetProperty(0)
                             .GetProperty("callback");
  EXPECT_TRUE(stored_callback.IsEqual(callback));
  stored_callback.SetProperty("state", lepus::Value(7));
  EXPECT_EQ(callback.GetProperty("state").Number(), 7);
  GestureDetectorImpl detector(1, GestureType::PAN, stored_entry,
                               stored_entry.GetProperty("relationMap"),
                               runtime.get());
  source_entry.GetProperty("config").SetProperty("enabled",
                                                 lepus::Value(false));
  EXPECT_TRUE(
      detector.gesture_config_in_lepus_value().GetProperty("enabled").Bool());

  lepus::Value next_descriptor;
  constexpr char next_source[] = R"JS(({
    type: 'gesture',
    value: [{id: 1, type: 0, config: {enabled: false},
      callbacks: [{name: 'onUpdate', callback: {state: 7}}],
      relationMap: {waitFor: [], simultaneous: [], continueWith: []}}]
  }))JS";
  ASSERT_TRUE(context->EvalBuf(next_source, sizeof(next_source) - 1,
                               next_descriptor, "gesture_storage_next.js"));
  instance->SetAttributeSlot(0, next_descriptor);
  EXPECT_TRUE(
      instance->attribute_slots_.GetProperty(0).IsEqual(next_descriptor));
  stored_entry = instance->attribute_slots_.GetProperty(0)
                     .GetProperty("value")
                     .GetProperty(0);
  EXPECT_FALSE(
      stored_entry.GetProperty("config").GetProperty("enabled").Bool());
  auto task = instance->CreateElementTreeTask(
      tasm->template_entries_[DEFAULT_ENTRY_NAME].get());
  instance = nullptr;
  task->Run();
  auto prepared = task->GetFuture().get();
  EXPECT_TRUE(prepared.prepared_attribute_slots_.GetProperty(0).IsEmpty());
  ASSERT_NE(prepared.result_, nullptr);
  EXPECT_TRUE(prepared.result_->children()[0]->gesture_map().empty());
}

TEST_P(ElementTemplateInstanceTest,
       PAPIEventStoragePreservesContextIdentityAcrossDispatchAndUpdate) {
  manager->config_->SetEnableEventHandleRefactor(true);
  auto runtime = CreatePAPIRuntime();
  auto* context = runtime::MTSRuntime::ToQuickContext(runtime.get());
  ASSERT_NE(context, nullptr);
  auto instance =
      CreateCompiledCallbackInstance(runtime, true, "main-thread:bindtap");
  lepus::Value ignored;
  constexpr char source[] = R"JS(
    globalThis.originalContext = {state: {count: 0}, _jsFn: {cb: {_jsFnId: 7}}};
    globalThis.registry = new FinalizationRegistry(function() {});
    globalThis.descriptor = {type: 'worklet', value: originalContext};
    globalThis.runWorklet = function(ctx) {
      globalThis.sameContext = ctx === originalContext;
      registry.register(ctx._jsFn.cb, 1);
      globalThis.carrierAccepted = true;
      ctx.state.count++;
    };
  )JS";
  ASSERT_TRUE(context->EvalBuf(source, sizeof(source) - 1, ignored,
                               "event_storage.js"));
  auto descriptor = runtime->GetGlobalData("descriptor");
  auto original = runtime->GetGlobalData("originalContext");
  auto slots = lepus::CArray::Create();
  slots->emplace_back(descriptor);
  lepus::Value create_args[] = {lepus::Value("spread_template"), lepus::Value(),
                                lepus::Value(slots), lepus::Value(),
                                lepus::Value(1)};
  auto handle =
      RendererFunctions::FiberCreateElementTemplate(context, create_args, 5);
  ASSERT_TRUE(handle.IsRefCounted());
  instance =
      fml::static_ref_ptr_cast<ElementTemplateInstance>(handle.RefCounted())
          .strongify();
  ASSERT_NE(instance, nullptr);
  auto stored = instance->attribute_slots_.GetProperty(0);
  EXPECT_TRUE(stored.GetProperty("value").IsEqual(original));
  lepus::Value serialize_args[] = {handle};
  auto serialized = RendererFunctions::FiberSerializeElementTemplate(
      context, serialize_args, 1);
  auto message = fml::MakeRefCounted<runtime::MessageEvent>(
      runtime::ContextProxy::Type::kCoreContext,
      runtime::ContextProxy::Type::kJSContext,
      std::make_unique<pub::ValueImplLepus>(serialized));
  auto copied_message = runtime::MessageEvent::ShallowCopy(*message);
  auto copied_context =
      pub::ValueUtils::ConvertValueToLepusValue(*copied_message->message())
          .GetProperty("attributeSlots")
          .GetProperty(0)
          .GetProperty("value");
  EXPECT_FALSE(copied_context.IsJSValue());
  EXPECT_TRUE(instance->attribute_slots_.GetProperty(0)
                  .GetProperty("value")
                  .IsJSValue());
  original.GetProperty("state").SetProperty("count", lepus::Value(3));
  EXPECT_EQ(copied_context.GetProperty("state").GetProperty("count").Number(),
            0);
  EXPECT_TRUE(
      instance->attribute_slots_.GetProperty(0).GetProperty("value").IsEqual(
          original));
  original.GetProperty("state").SetProperty("count", lepus::Value(0));

  auto root = instance->GetRoot();
  ASSERT_NE(root, nullptr);
  auto target = root->children()[0];
  // Replacing the caller's wrapper after installation must not replace the
  // installed context on the element.
  descriptor.SetProperty("value", lepus::Value());
  root->MarkAttached();
  target->MarkAttached();
  for (int count : {1, 2}) {
    auto event = fml::MakeRefCounted<event::TouchEvent>("tap");
    event::EventDispatcher::DispatchEvent(*target, event);
    EXPECT_TRUE(runtime->GetGlobalData("sameContext").Bool());
    EXPECT_TRUE(runtime->GetGlobalData("carrierAccepted").Bool());
    EXPECT_EQ(original.GetProperty("state").GetProperty("count").Number(),
              count);
  }
  // Hydration updates the original context's state in place.
  constexpr char hydrate[] = "originalContext.state = {count: 10}";
  ASSERT_TRUE(context->EvalBuf(hydrate, sizeof(hydrate) - 1, ignored,
                               "event_hydration.js"));
  auto event = fml::MakeRefCounted<event::TouchEvent>("tap");
  event::EventDispatcher::DispatchEvent(*target, event);
  EXPECT_EQ(original.GetProperty("state").GetProperty("count").Number(), 11);

  descriptor.SetProperty("value", original);
  lepus::Value update_args[] = {handle, lepus::Value(0), descriptor};
  RendererFunctions::FiberSetAttributeOfElementTemplate(context, update_args,
                                                        3);
  auto installed = target->lepus_event_map().find("tap");
  ASSERT_NE(installed, target->lepus_event_map().end());
  EXPECT_TRUE(installed->second->lepus_object().IsEqual(original));
  target->FiberAddEvent("bindEvent", "tap", lepus::Value("backgroundHandler"),
                        DEFAULT_ENTRY_NAME);
  update_args[2] = lepus::Value();
  RendererFunctions::FiberSetAttributeOfElementTemplate(context, update_args,
                                                        3);
  installed = target->lepus_event_map().find("tap");
  ASSERT_NE(installed, target->lepus_event_map().end());
  EXPECT_TRUE(installed->second->lepus_object().IsEmpty());
  ASSERT_NE(target->event_map().find("tap"), target->event_map().end());
  EXPECT_EQ(target->event_map().find("tap")->second->function().str(),
            "backgroundHandler");
  auto listeners = target->GetEventListenerMap()->Find("tap");
  ASSERT_NE(listeners, nullptr);
  ASSERT_EQ(listeners->size(), 1u);
  EXPECT_EQ(static_cast<event::ClosureEventListener*>(listeners->front().get())
                ->closure_type(),
            event::ClosureEventListener::ClosureType::kJS);
  auto cleared_event = fml::MakeRefCounted<event::TouchEvent>("tap");
  event::EventDispatcher::DispatchEvent(*target, cleared_event);
  EXPECT_EQ(original.GetProperty("state").GetProperty("count").Number(), 11);
  instance = nullptr;
  handle = lepus::Value();
  root = nullptr;
  target = nullptr;
}

TEST_P(ElementTemplateInstanceTest,
       MainThreadNullPreservesNativeCallableRemovalInBothEventModes) {
  auto runtime = CreatePAPIRuntime();
  auto* context = runtime::MTSRuntime::ToQuickContext(runtime.get());
  lepus::Value callable;
  constexpr char source[] = "(function() {})";
  ASSERT_TRUE(context->EvalBuf(source, sizeof(source) - 1, callable,
                               "callable_removal.js"));
  for (bool refactored : {false, true}) {
    manager->config_->SetEnableEventHandleRefactor(refactored);
    for (const auto* key :
         {"main-thread:bindtap", "main-thread:global-bindtap"}) {
      auto instance = CreateCompiledCallbackInstance(runtime, false, key);
      instance->SetAttributeSlot(0, callable);
      auto root = instance->GetRoot();
      const auto& events = std::string(key) == "main-thread:global-bindtap"
                               ? root->global_bind_event_map()
                               : root->event_map();
      ASSERT_NE(events.find("tap"), events.end());
      EXPECT_TRUE(events.find("tap")->second->lepus_function().IsCallable());
      instance->SetAttributeSlot(0, lepus::Value());
      EXPECT_EQ(events.find("tap"), events.end());
      if (refactored) {
        auto listeners = root->GetEventListenerMap()->Find("tap");
        ASSERT_NE(listeners, nullptr);
        EXPECT_TRUE(listeners->empty());
      }
    }
  }
}

TEST_P(ElementTemplateInstanceTest,
       EventStorageExcludesLiveContextsFromDetachedPreparation) {
  auto runtime = CreatePAPIRuntime();
  auto* context = runtime::MTSRuntime::ToQuickContext(runtime.get());
  ASSERT_NE(context, nullptr);
  lepus::Value descriptor;
  constexpr char source[] = "({type: 'worklet', value: {state: {count: 0}}})";
  ASSERT_TRUE(context->EvalBuf(source, sizeof(source) - 1, descriptor,
                               "event_preparation.js"));
  for (bool initially_empty : {false, true}) {
    auto instance =
        CreateCompiledCallbackInstance(runtime, true, "main-thread:bindtap");
    auto slots = lepus::CArray::Create();
    slots->emplace_back(initially_empty ? lepus::Value() : descriptor);
    slots->emplace_back(lepus::Value("label"));
    instance->SetAttributeSlots(lepus::Value(slots));
    auto task = instance->CreateElementTreeTask(
        tasm->template_entries_[DEFAULT_ENTRY_NAME].get());
    instance->SetAttributeSlot(0, descriptor);
    EXPECT_TRUE(
        instance->attribute_slots_.GetProperty(0).GetProperty("value").IsEqual(
            descriptor.GetProperty("value")));
    instance = nullptr;
    task->Run();
    auto prepared = task->GetFuture().get();
    EXPECT_TRUE(prepared.prepared_attribute_slots_.GetProperty(0).IsEmpty());
    EXPECT_EQ(prepared.prepared_attribute_slots_.GetProperty(1).StdString(),
              "label");
    ASSERT_NE(prepared.result_, nullptr);
    EXPECT_TRUE(prepared.result_->children()[0]->event_map().empty());
  }
}

TEST_P(ElementTemplateInstanceTest,
       OrdinaryAttributeStorageDefersTemplateMetadataLookup) {
  auto runtime = CreatePAPIRuntime();
  auto* context = runtime::MTSRuntime::ToQuickContext(runtime.get());
  ASSERT_NE(context, nullptr);
  lepus::Value js_array;
  const char* array_source = "([[-180, -135], [135, 180]])";
  ASSERT_TRUE(context->EvalBuf(array_source, strlen(array_source), js_array,
                               "ordinary_array.js"));
  auto expected_array = lepus::Value(js_array).ToLepusValue(true);
  auto object = lepus::Dictionary::Create();
  object->SetValue("type", lepus::Value(42));
  object->SetValue("payload", lepus::Value("ordinary"));
  auto other_type = lepus::Dictionary::Create();
  other_type->SetValue("type", lepus::Value("ordinary"));
  for (const auto& [value, expected] :
       std::initializer_list<std::pair<lepus::Value, lepus::Value>>{
           {lepus::Value(), lepus::Value()},
           {lepus::Value(42), lepus::Value(42)},
           {lepus::Value("label"), lepus::Value("label")},
           {lepus::Value(object), lepus::Value(object)},
           {lepus::Value(other_type), lepus::Value(other_type)},
           {js_array, expected_array}}) {
    auto instance = CreateCompiledSpreadInstance();
    auto& infos = tasm->template_entries_[DEFAULT_ENTRY_NAME]
                      ->template_bundle_.element_template_infos_;
    // A cold bundle has no cached metadata until preparation requests it.
    infos.clear();
    auto slots = lepus::CArray::Create();
    slots->emplace_back(value);
    instance->SetAttributeSlots(lepus::Value(slots));
    EXPECT_TRUE(infos.empty());
    instance->SetAttributeSlot(0, value);
    EXPECT_TRUE(infos.empty());
    EXPECT_TRUE(instance->attribute_slots_.GetProperty(0).IsEqual(expected));
  }
}

TEST_P(ElementTemplateInstanceTest,
       CallbackMarkersRemainOrdinaryDataOutsideDeclaredSlots) {
  auto runtime = CreatePAPIRuntime();
  auto* context = runtime::MTSRuntime::ToQuickContext(runtime.get());
  ASSERT_NE(context, nullptr);
  auto data = [](const lepus::Value& descriptor) {
    const auto value = descriptor.GetProperty("value");
    return value.IsArrayOrJSArray() ? value.GetProperty(0).GetProperty("config")
                                    : value.GetProperty("state");
  };
  for (const auto* source : {
           R"JS(({type: 'gesture', custom: 'retained', value: [{
             id: 1, type: 0, config: {count: 1}, callbacks: [],
             relationMap: {waitFor: [], simultaneous: [], continueWith: []}
           }]}))JS",
           "({type: 'worklet', value: {state: {count: 1}}, custom: "
           "'retained'})"}) {
    for (int host_kind : {0, 1, 2}) {
      lepus::Value descriptor;
      ASSERT_TRUE(context->EvalBuf(source, strlen(source), descriptor,
                                   "ordinary_callback.js"));
      auto instance = host_kind == 2 ? fml::AdoptRef<ElementTemplateInstance>(
                                           new ElementTemplateInstance(manager))
                                     : CreateCompiledSpreadInstance();
      if (host_kind == 0) {
        auto& info =
            tasm->template_entries_[DEFAULT_ENTRY_NAME]
                ->template_bundle_.element_template_infos_["spread_template"];
        info->elements_[0].attributes_ =
            std::make_shared<const TemplateAttributes>(TemplateAttributes{
                Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC,
                          base::String("payload"), lepus::Value(), 0}});
      }
      if (host_kind == 2) {
        instance->SetTypedTag(base::String("view"));
        instance->SetAttributes(descriptor);
      } else {
        auto slots = lepus::CArray::Create();
        slots->emplace_back(descriptor);
        instance->SetAttributeSlots(lepus::Value(slots));
      }
      data(descriptor).SetProperty("count", lepus::Value(2));
      auto serialized = instance->Serialize();
      auto stored =
          host_kind == 2
              ? serialized.GetProperty("attributes")
              : serialized.GetProperty("attributeSlots").GetProperty(0);
      EXPECT_EQ(stored.GetProperty("custom").StdString(), "retained");
      EXPECT_EQ(stored.GetProperty("type").StdString(),
                descriptor.GetProperty("type").StdString());
      EXPECT_EQ(stored.GetProperty("value").GetLength(),
                descriptor.GetProperty("value").GetLength());
      EXPECT_EQ(data(stored).GetProperty("count").Number(), 1);
      instance->SetAttributeSlot(0, descriptor);
      data(descriptor).SetProperty("count", lepus::Value(3));
      serialized = instance->Serialize();
      stored = host_kind == 2
                   ? serialized.GetProperty("attributes")
                   : serialized.GetProperty("attributeSlots").GetProperty(0);
      EXPECT_EQ(stored.GetProperty("custom").StdString(), "retained");
      EXPECT_EQ(data(stored).GetProperty("count").Number(), 2);
      auto root = instance->GetRoot();
      ASSERT_NE(root, nullptr);
      EXPECT_TRUE(root->gesture_map().empty());
    }
  }
}

TEST_P(ElementTemplateInstanceTest,
       PAPIArrayAndCallableSlotsPreserveTypesAndMatchSnapshot) {
  auto runtime = CreatePAPIRuntime();
  auto* ctx = runtime::MTSRuntime::ToQuickContext(runtime.get());
  ASSERT_NE(ctx, nullptr);

  auto info = std::make_shared<ElementTemplateInfo>();
  info->key_ = "array_callable_template";
  ElementInfo root_info;
  root_info.tag_enum_ = ELEMENT_VIEW;
  root_info.tag_ = BASE_STATIC_STRING(kElementViewTag);
  root_info.id_selector_ = base::String("root");
  root_info.attributes_ =
      std::make_shared<const TemplateAttributes>(TemplateAttributes{
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC,
                    base::String("consume-slide-event"), lepus::Value(), 0},
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("data-items"),
                    lepus::Value(), 1},
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC,
                    base::String("main-thread:bindtap"), lepus::Value(), 2},
          Attribute{ATTRIBUTE_BINDING_TYPE_SPREAD, base::String("spread"),
                    lepus::Value(), 3}});
  info->elements_.push_back(std::move(root_info));
  tasm->template_entries_[DEFAULT_ENTRY_NAME]
      ->template_bundle_.element_template_infos_["array_callable_template"] =
      info;

  auto nested_lepus_ref = lepus::Dictionary::Create();
  nested_lepus_ref->SetValue("fromLepusRef", lepus::Value("initial"));
  runtime->SetGlobalData("nestedLepusRef", lepus::Value(nested_lepus_ref));

  lepus::Value harness;
  const char* source = R"JS((() => {
    const slideRanges = [[-180, -135], [135, 180]];
    const items = [1, {label: 'before'}, [2, 3], globalThis.nestedLepusRef];
    const callback = function onTapOrFocus() { return 7; };
    const spread = {
      'extra-ranges': [[-90, -45]],
      'data-spread-items': [{nested: [4, 5], ref: globalThis.nestedLepusRef}],
      'data-direct-ref': globalThis.nestedLepusRef,
      'main-thread:bindfocus': callback,
    };
    const nextRanges = [[-120, -60]];
    return {
      slideRanges,
      items,
      callback,
      spread,
      slots: [slideRanges, items, callback, spread],
      nextRanges,
      mutateInitial() {
        slideRanges[0][0] = 0;
        slideRanges.push([0, 45]);
        items[1].label = 'mutated';
        items[2].push(4);
        spread['extra-ranges'][0][0] = 0;
        spread['data-spread-items'][0].nested[0] = 99;
      },
      mutateUpdated() {
        nextRanges[0][0] = -10;
      },
    };
  })())JS";
  ASSERT_TRUE(
      ctx->EvalBuf(source, strlen(source), harness, "array_callable_slots.js"));

  lepus::Value create_view_args[] = {lepus::Value(0)};
  auto snapshot_view_val =
      RendererFunctions::FiberCreateView(ctx, create_view_args, 1);
  ASSERT_TRUE(snapshot_view_val.IsRefCounted());
  auto snapshot_view =
      fml::static_ref_ptr_cast<Element>(snapshot_view_val.RefCounted());
  lepus::Value set_slide_args[] = {snapshot_view_val,
                                   lepus::Value("consume-slide-event"),
                                   harness.GetProperty("slideRanges")};
  RendererFunctions::FiberSetAttribute(ctx, set_slide_args, 3);
  lepus::Value set_dataset_args[] = {snapshot_view_val, lepus::Value("items"),
                                     harness.GetProperty("items")};
  RendererFunctions::FiberAddDataset(ctx, set_dataset_args, 3);
  lepus::Value add_event_args[] = {snapshot_view_val, lepus::Value("bindEvent"),
                                   lepus::Value("tap"),
                                   harness.GetProperty("callback")};
  RendererFunctions::FiberAddEvent(ctx, add_event_args, 4);

  lepus::Value create_args[] = {
      lepus::Value("array_callable_template"), lepus::Value(DEFAULT_ENTRY_NAME),
      harness.GetProperty("slots"), lepus::Value(), lepus::Value(1)};
  auto template_value =
      RendererFunctions::FiberCreateElementTemplate(ctx, create_args, 5);
  ASSERT_TRUE(template_value.IsRefCounted());

  lepus::Value typed_args[] = {lepus::Value("view"),
                               harness.GetProperty("spread"), lepus::Value(),
                               lepus::Value(2)};
  auto typed_value =
      RendererFunctions::FiberCreateTypedElementTemplate(ctx, typed_args, 4);
  ASSERT_TRUE(typed_value.IsRefCounted());

  auto mutate_initial = harness.GetProperty("mutateInitial");
  EXPECT_TRUE(runtime->CallClosure(mutate_initial).IsUndefined());

  auto instance = fml::static_ref_ptr_cast<ElementTemplateInstance>(
      template_value.RefCounted());
  auto serialized_slots = instance->Serialize().GetProperty("attributeSlots");
  EXPECT_TRUE(serialized_slots.GetProperty(0).IsArray());
  EXPECT_TRUE(serialized_slots.GetProperty(1).IsArray());
  EXPECT_TRUE(serialized_slots.GetProperty(2).IsCallable());
  EXPECT_TRUE(
      serialized_slots.GetProperty(2).IsEqual(harness.GetProperty("callback")));
  EXPECT_TRUE(serialized_slots.GetProperty(3)
                  .GetProperty("main-thread:bindfocus")
                  .IsEqual(harness.GetProperty("callback")));

  // Detached worker preparation must strip both declared callback slots (2) and
  // 1st-level callables inside spread maps (3) while keeping non-callable
  // spread entries intact.
  auto task = instance->CreateElementTreeTask(
      tasm->template_entries_[DEFAULT_ENTRY_NAME].get());
  task->Run();
  auto prepared = task->GetFuture().get();
  EXPECT_TRUE(prepared.prepared_attribute_slots_.GetProperty(0).IsArray());
  EXPECT_TRUE(prepared.prepared_attribute_slots_.GetProperty(1).IsArray());
  EXPECT_TRUE(prepared.prepared_attribute_slots_.GetProperty(2).IsEmpty());
  auto prepared_spread = prepared.prepared_attribute_slots_.GetProperty(3);
  EXPECT_TRUE(prepared_spread.IsTable());
  EXPECT_TRUE(prepared_spread.GetProperty("main-thread:bindfocus").IsEmpty());
  EXPECT_TRUE(prepared_spread.GetProperty("extra-ranges").IsArray());
  EXPECT_EQ(prepared_spread.GetProperty("extra-ranges")
                .GetProperty(0)
                .GetProperty(0)
                .Number(),
            -90);

  auto attribute_value = [](const Element* element,
                            const char* key) -> lepus::Value {
    const auto& attributes = element->data_model()->attributes();
    const auto it = attributes.find(base::String(key));
    return it == attributes.end() ? lepus::Value() : it->second;
  };

  auto root = instance->GetRoot();
  ASSERT_NE(root, nullptr);
  const auto tap_event = root->event_map().find("tap");
  const auto snapshot_tap_event = snapshot_view->event_map().find("tap");
  ASSERT_NE(tap_event, root->event_map().end());
  ASSERT_NE(snapshot_tap_event, snapshot_view->event_map().end());
  EXPECT_TRUE(tap_event->second->lepus_function().IsEqual(
      snapshot_tap_event->second->lepus_function()));
  const auto focus_event = root->event_map().find("focus");
  ASSERT_NE(focus_event, root->event_map().end());
  EXPECT_TRUE(focus_event->second->lepus_function().IsEqual(
      harness.GetProperty("callback")));

  const auto slide_attr = attribute_value(root.get(), "consume-slide-event");
  const auto snapshot_slide_attr =
      attribute_value(snapshot_view.get(), "consume-slide-event");
  EXPECT_TRUE(slide_attr.IsArray());
  EXPECT_TRUE(slide_attr.IsEqual(snapshot_slide_attr));
  EXPECT_EQ(slide_attr.GetProperty(0).GetProperty(0).Number(), -180);

  const auto extra_ranges = attribute_value(root.get(), "extra-ranges");
  EXPECT_TRUE(extra_ranges.IsArray());
  EXPECT_EQ(extra_ranges.GetProperty(0).GetProperty(0).Number(), -90);

  const auto* items = DatasetValue(root.get(), base::String("items"));
  const auto* snapshot_items =
      DatasetValue(snapshot_view.get(), base::String("items"));
  ASSERT_NE(items, nullptr);
  ASSERT_NE(snapshot_items, nullptr);
  EXPECT_TRUE(items->IsArray());
  EXPECT_TRUE(items->IsEqual(*snapshot_items));
  // Element Template always deep-clones ordinary data on write, whereas
  // Snapshot's FiberAddDataset only deep-converts LepusRef when
  // GetEnableParallelElement() is true.
  nested_lepus_ref->SetValue("fromLepusRef", lepus::Value("mutated"));
  EXPECT_EQ(items->GetProperty(1).GetProperty("label").StdString(), "before");
  EXPECT_EQ(items->GetProperty(3).GetProperty("fromLepusRef").StdString(),
            "initial");
  EXPECT_EQ(
      snapshot_items->GetProperty(3).GetProperty("fromLepusRef").StdString(),
      manager->GetEnableParallelElement() ? "initial" : "mutated");
  const auto* spread_items =
      DatasetValue(root.get(), base::String("spread-items"));
  ASSERT_NE(spread_items, nullptr);
  EXPECT_EQ(spread_items->GetProperty(0)
                .GetProperty("nested")
                .GetProperty(0)
                .Number(),
            4);
  EXPECT_EQ(spread_items->GetProperty(0)
                .GetProperty("ref")
                .GetProperty("fromLepusRef")
                .StdString(),
            "initial");
  const auto* direct_ref = DatasetValue(root.get(), base::String("direct-ref"));
  ASSERT_NE(direct_ref, nullptr);
  EXPECT_TRUE(direct_ref->IsTable());
  EXPECT_EQ(direct_ref->GetProperty("fromLepusRef").StdString(), "initial");

  auto typed_instance = fml::static_ref_ptr_cast<ElementTemplateInstance>(
      typed_value.RefCounted());
  auto typed_root = typed_instance->GetRoot();
  ASSERT_NE(typed_root, nullptr);
  const auto typed_extra_ranges =
      attribute_value(typed_root.get(), "extra-ranges");
  EXPECT_TRUE(typed_extra_ranges.IsArray());
  EXPECT_EQ(typed_extra_ranges.GetProperty(0).GetProperty(0).Number(), -90);
  const auto typed_focus_event = typed_root->event_map().find("focus");
  ASSERT_NE(typed_focus_event, typed_root->event_map().end());
  EXPECT_TRUE(typed_focus_event->second->lepus_function().IsEqual(
      harness.GetProperty("callback")));

  lepus::Value update_args[] = {template_value, lepus::Value(0),
                                harness.GetProperty("nextRanges")};
  RendererFunctions::FiberSetAttributeOfElementTemplate(ctx, update_args, 3);
  auto mutate_updated = harness.GetProperty("mutateUpdated");
  EXPECT_TRUE(runtime->CallClosure(mutate_updated).IsUndefined());
  const auto updated_slide_attr =
      attribute_value(root.get(), "consume-slide-event");
  EXPECT_TRUE(updated_slide_attr.IsArray());
  EXPECT_EQ(updated_slide_attr.GetProperty(0).GetProperty(0).Number(), -120);
}

INSTANTIATE_TEST_SUITE_P(ElementTemplateInstanceTestModule,
                         ElementTemplateInstanceTest,
                         ::testing::ValuesIn(fiber_element_generation_params));

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
