// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/fiber/element_template_instance.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <type_traits>
#include <vector>

#include "core/public/pipeline_option.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/page_element.h"
#include "core/renderer/dom/fiber/tree_resolver.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/testing/fiber_element_test.h"
#include "core/renderer/dom/testing/fiber_mock_painting_context.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/template_entry.h"
#include "core/renderer/utils/base/element_template_info.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/runtime/lepus/bindings/renderer_functions.h"
#include "core/runtime/lepus/bytecode_generator.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/shell/runtime/mts/mts_runtime.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

namespace {
std::map<std::string, lepus::Value>* PaintingPropsFor(ElementManager* manager,
                                                      Element* element) {
  auto* painting_context = static_cast<FiberMockPaintingContext*>(
      manager->painting_context()->impl());
  auto node_it = painting_context->node_map_.find(element->impl_id());
  if (node_it == painting_context->node_map_.end()) {
    return nullptr;
  }
  return &node_it->second->props_;
}

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
  void OnElementDataModelSet(Element* ptr) override {}
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
        {lynx::devtool::DevToolFunction::InitForInspector, noop},
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
};

class DestructionTrackingElementTemplateInstance final
    : public ElementTemplateInstance {
 public:
  DestructionTrackingElementTemplateInstance(
      ElementManager* manager, std::shared_ptr<int> destruction_count)
      : ElementTemplateInstance(manager),
        destruction_count_(std::move(destruction_count)) {}

  ~DestructionTrackingElementTemplateInstance() override {
    ++*destruction_count_;
  }

 private:
  std::shared_ptr<int> destruction_count_;
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

class ElementTemplateInstanceTest : public FiberElementTest {};

TEST_P(ElementTemplateInstanceTest, UsesIndependentLepusRefType) {
  auto instance = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));

  EXPECT_EQ(instance->GetRefType(), lepus::RefType::kElementTemplate);
  static_assert(!std::is_base_of_v<Element, ElementTemplateInstance>);
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

TEST_P(ElementTemplateInstanceTest,
       SerializeElementTemplatePreservesSlotShapesAndFiltersNonETChildren) {
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
  root->SetRootAttributes(lepus::Value(attributes));

  auto serialized = root->Serialize();
  EXPECT_TRUE(serialized.IsObject());
  EXPECT_FALSE(serialized.GetProperty("templateKey").IsString());
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

TEST_P(ElementTemplateInstanceTest, CreateElementTemplatesRejectNonNumericUid) {
  auto lepus_ctx = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  ASSERT_TRUE(lepus_ctx);
  lepus_ctx->Initialize();
  lepus_ctx->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  auto* mts_ctx = runtime::MTSRuntime::ToQuickContext(lepus_ctx.get());
  ASSERT_TRUE(mts_ctx);

  lepus::Value compiled_args[] = {lepus::Value("root_template"), lepus::Value(),
                                  lepus::Value(), lepus::Value(),
                                  lepus::Value("invalid")};
  base::ErrorStorage::GetInstance().Reset();
  auto compiled =
      RendererFunctions::FiberCreateElementTemplate(mts_ctx, compiled_args, 5);
  EXPECT_TRUE(compiled.IsEmpty());
  ASSERT_NE(base::ErrorStorage::GetInstance().GetError(), nullptr);

  lepus::Value typed_args[] = {lepus::Value("view"), lepus::Value(),
                               lepus::Value(), lepus::Value("invalid")};
  base::ErrorStorage::GetInstance().Reset();
  auto typed = RendererFunctions::FiberCreateTypedElementTemplate(
      mts_ctx, typed_args, 4);
  EXPECT_TRUE(typed.IsEmpty());
  ASSERT_NE(base::ErrorStorage::GetInstance().GetError(), nullptr);
  base::ErrorStorage::GetInstance().Reset();
}

TEST_P(ElementTemplateInstanceTest, CreateElementTemplatesSkipEmptyOptions) {
  auto lepus_ctx = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  ASSERT_TRUE(lepus_ctx);
  lepus_ctx->Initialize();
  lepus_ctx->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  auto* mts_ctx = runtime::MTSRuntime::ToQuickContext(lepus_ctx.get());
  ASSERT_TRUE(mts_ctx);

  lepus::Value args[] = {lepus::Value("root_template"),
                         lepus::Value(),
                         lepus::Value(),
                         lepus::Value(),
                         lepus::Value(2),
                         lepus::Value(lepus::Dictionary::Create())};
  auto created_value =
      RendererFunctions::FiberCreateElementTemplate(mts_ctx, args, 6);

  ASSERT_TRUE(created_value.IsRefCounted());
  ASSERT_EQ(created_value.RefCounted()->GetRefType(),
            lepus::RefType::kElementTemplate);
  auto created_instance = fml::static_ref_ptr_cast<ElementTemplateInstance>(
                              created_value.RefCounted())
                              .strongify();
  ASSERT_NE(created_instance, nullptr);
  auto serialized = created_instance->Serialize();
  EXPECT_TRUE(serialized.GetProperty("options").IsEmpty());

  lepus::Value typed_args[] = {lepus::Value("view"), lepus::Value(),
                               lepus::Value(), lepus::Value(16),
                               lepus::Value(lepus::Dictionary::Create())};
  auto created_typed_value = RendererFunctions::FiberCreateTypedElementTemplate(
      mts_ctx, typed_args, 5);
  ASSERT_TRUE(created_typed_value.IsRefCounted());
  ASSERT_EQ(created_typed_value.RefCounted()->GetRefType(),
            lepus::RefType::kElementTemplate);
  auto created_typed_instance =
      fml::static_ref_ptr_cast<ElementTemplateInstance>(
          created_typed_value.RefCounted())
          .strongify();
  ASSERT_NE(created_typed_instance, nullptr);
  auto serialized_typed = created_typed_instance->Serialize();
  EXPECT_TRUE(serialized_typed.GetProperty("options").IsEmpty());
}

TEST_P(ElementTemplateInstanceTest,
       CreateCompiledElementTemplateRejectsArrayOptions) {
  auto lepus_ctx = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  ASSERT_TRUE(lepus_ctx);
  lepus_ctx->Initialize();
  lepus_ctx->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  auto* mts_ctx = runtime::MTSRuntime::ToQuickContext(lepus_ctx.get());
  ASSERT_TRUE(mts_ctx);

  lepus::Value args[] = {lepus::Value("root_template"),
                         lepus::Value(),
                         lepus::Value(),
                         lepus::Value(),
                         lepus::Value(2),
                         lepus::Value(lepus::CArray::Create())};
  auto created_value =
      RendererFunctions::FiberCreateElementTemplate(mts_ctx, args, 6);
  EXPECT_TRUE(created_value.IsEmpty());

  lepus::Value typed_args[] = {lepus::Value("view"), lepus::Value(),
                               lepus::Value(), lepus::Value(16),
                               lepus::Value(lepus::CArray::Create())};
  auto created_typed_value = RendererFunctions::FiberCreateTypedElementTemplate(
      mts_ctx, typed_args, 5);
  EXPECT_TRUE(created_typed_value.IsEmpty());
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateHandleRoundTripsThroughJavaScriptChildSlots) {
  auto lepus_ctx = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  ASSERT_TRUE(lepus_ctx);
  lepus_ctx->Initialize();
  lepus_ctx->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  auto* mts_ctx = runtime::MTSRuntime::ToQuickContext(lepus_ctx.get());
  ASSERT_TRUE(mts_ctx);

  lepus::Value child_args[] = {lepus::Value("view"), lepus::Value(),
                               lepus::Value(), lepus::Value(5)};
  auto child = RendererFunctions::FiberCreateTypedElementTemplate(
      mts_ctx, child_args, 4);
  ASSERT_TRUE(child.IsRefCounted());
  ASSERT_EQ(child.RefCounted()->GetRefType(), lepus::RefType::kElementTemplate);

  lepus_ctx->SetGlobalData("etChild", child);
  lepus::BytecodeGenerator::GenerateBytecode(lepus_ctx->GetMTSContext(),
                                             "let etChildSlots = []; "
                                             "etChildSlots[2] = [etChild]; "
                                             "let etTypedChildSlots = "
                                             "[[etChild]];",
                                             lepus_ctx->GetSdkVersion(), "");
  ASSERT_TRUE(lepus_ctx->Execute(nullptr));

  auto child_slots = lepus_ctx->GetGlobalData("etChildSlots");
  ASSERT_TRUE(child_slots.IsJSValue());
  lepus::Value parent_args[] = {lepus::Value("parent_template"), lepus::Value(),
                                lepus::Value(), child_slots, lepus::Value(7)};
  auto parent =
      RendererFunctions::FiberCreateElementTemplate(mts_ctx, parent_args, 5);
  ASSERT_TRUE(parent.IsRefCounted());
  ASSERT_EQ(parent.RefCounted()->GetRefType(),
            lepus::RefType::kElementTemplate);

  lepus::Value serialize_args[] = {parent};
  auto serialized = RendererFunctions::FiberSerializeElementTemplate(
      mts_ctx, serialize_args, 1);
  auto serialized_slots = serialized.GetProperty("childSlots");
  ASSERT_EQ(serialized_slots.GetLength(), 3);
  EXPECT_TRUE(serialized_slots.GetProperty(0).IsUndefined());
  EXPECT_TRUE(serialized_slots.GetProperty(1).IsUndefined());
  auto serialized_children = serialized_slots.GetProperty(2);
  ASSERT_EQ(serialized_children.GetLength(), 1);
  EXPECT_EQ(serialized_children.GetProperty(0).GetProperty("uid").Number(), 5);

  auto typed_child_slots = lepus_ctx->GetGlobalData("etTypedChildSlots");
  ASSERT_TRUE(typed_child_slots.IsJSValue());
  lepus::Value typed_parent_args[] = {lepus::Value("view"), lepus::Value(),
                                      typed_child_slots, lepus::Value(8)};
  auto typed_parent = RendererFunctions::FiberCreateTypedElementTemplate(
      mts_ctx, typed_parent_args, 4);
  ASSERT_TRUE(typed_parent.IsRefCounted());
  ASSERT_EQ(typed_parent.RefCounted()->GetRefType(),
            lepus::RefType::kElementTemplate);

  lepus::Value serialize_typed_args[] = {typed_parent};
  auto serialized_typed = RendererFunctions::FiberSerializeElementTemplate(
      mts_ctx, serialize_typed_args, 1);
  auto serialized_typed_slots = serialized_typed.GetProperty("childSlots");
  ASSERT_EQ(serialized_typed_slots.GetLength(), 1);
  auto serialized_typed_children = serialized_typed_slots.GetProperty(0);
  ASSERT_EQ(serialized_typed_children.GetLength(), 1);
  EXPECT_EQ(
      serialized_typed_children.GetProperty(0).GetProperty("uid").Number(), 5);
}

TEST_P(ElementTemplateInstanceTest,
       CreateElementTemplateChildSlotsIgnoreWrongKindLocally) {
  auto lepus_ctx = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  ASSERT_TRUE(lepus_ctx);
  lepus_ctx->Initialize();
  lepus_ctx->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  auto* mts_ctx = runtime::MTSRuntime::ToQuickContext(lepus_ctx.get());
  ASSERT_TRUE(mts_ctx);

  auto make_child = [this](int uid) {
    auto child = fml::AdoptRef<ElementTemplateInstance>(
        new ElementTemplateInstance(manager));
    child->SetTypedTag(base::String("raw-text"));
    child->SetUid(lepus::Value(uid));
    return child;
  };
  auto make_child_slots =
      [&](const fml::RefPtr<ElementTemplateInstance>& first,
          const fml::RefPtr<ElementTemplateInstance>& second) {
        auto slot_children = lepus::CArray::Create();
        slot_children->emplace_back(lepus::Value(first));
        slot_children->emplace_back(lepus::Value(manager->CreateFiberView()));
        slot_children->emplace_back(lepus::Value(second));
        auto child_slots = lepus::CArray::Create();
        child_slots->emplace_back(lepus::Value(slot_children));
        return child_slots;
      };

  auto compiled_first = make_child(1);
  auto compiled_second = make_child(2);
  auto compiled_slots = make_child_slots(compiled_first, compiled_second);
  base::ErrorStorage::GetInstance().Reset();
  lepus::Value args[] = {lepus::Value("root_template"), lepus::Value(),
                         lepus::Value(), lepus::Value(compiled_slots),
                         lepus::Value(10)};
  auto created_value =
      RendererFunctions::FiberCreateElementTemplate(mts_ctx, args, 5);
  ASSERT_TRUE(created_value.IsRefCounted());
  EXPECT_EQ(base::ErrorStorage::GetInstance().GetError(), nullptr);
  auto compiled = fml::static_ref_ptr_cast<ElementTemplateInstance>(
                      created_value.RefCounted())
                      .strongify();
  ASSERT_NE(compiled, nullptr);
  auto serialized_compiled_children =
      compiled->Serialize().GetProperty("childSlots").GetProperty(0);
  ASSERT_EQ(serialized_compiled_children.GetLength(), 2);
  EXPECT_EQ(
      serialized_compiled_children.GetProperty(0).GetProperty("uid").Number(),
      1);
  EXPECT_EQ(
      serialized_compiled_children.GetProperty(1).GetProperty("uid").Number(),
      2);

  auto typed_first = make_child(3);
  auto typed_second = make_child(4);
  auto typed_slots = make_child_slots(typed_first, typed_second);
  base::ErrorStorage::GetInstance().Reset();
  lepus::Value typed_args[] = {lepus::Value("view"), lepus::Value(),
                               lepus::Value(typed_slots), lepus::Value(16)};
  auto created_typed_value = RendererFunctions::FiberCreateTypedElementTemplate(
      mts_ctx, typed_args, 4);
  ASSERT_TRUE(created_typed_value.IsRefCounted());
  EXPECT_EQ(base::ErrorStorage::GetInstance().GetError(), nullptr);
  auto typed = fml::static_ref_ptr_cast<ElementTemplateInstance>(
                   created_typed_value.RefCounted())
                   .strongify();
  ASSERT_NE(typed, nullptr);
  auto serialized_typed_children =
      typed->Serialize().GetProperty("childSlots").GetProperty(0);
  ASSERT_EQ(serialized_typed_children.GetLength(), 2);
  EXPECT_EQ(
      serialized_typed_children.GetProperty(0).GetProperty("uid").Number(), 3);
  EXPECT_EQ(
      serialized_typed_children.GetProperty(1).GetProperty("uid").Number(), 4);
  base::ErrorStorage::GetInstance().Reset();
}

TEST_P(ElementTemplateInstanceTest,
       CreateElementTemplatePAPIsApplyDuplicateChildrenInOrder) {
  auto lepus_ctx = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  ASSERT_TRUE(lepus_ctx);
  lepus_ctx->Initialize();
  lepus_ctx->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  auto* mts_ctx = runtime::MTSRuntime::ToQuickContext(lepus_ctx.get());
  ASSERT_TRUE(mts_ctx);

  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTypedTag(base::String("raw-text"));
  auto duplicate_children = lepus::CArray::Create();
  duplicate_children->emplace_back(lepus::Value(child));
  auto duplicate_children_other_slot = lepus::CArray::Create();
  duplicate_children_other_slot->emplace_back(lepus::Value(child));
  auto duplicate_slots = lepus::CArray::Create();
  duplicate_slots->emplace_back(lepus::Value(duplicate_children));
  duplicate_slots->emplace_back(lepus::Value(duplicate_children_other_slot));

  base::ErrorStorage::GetInstance().Reset();
  lepus::Value compiled_args[] = {lepus::Value("root_template"), lepus::Value(),
                                  lepus::Value(), lepus::Value(duplicate_slots),
                                  lepus::Value(2)};
  auto compiled_value =
      RendererFunctions::FiberCreateElementTemplate(mts_ctx, compiled_args, 5);
  ASSERT_TRUE(compiled_value.IsRefCounted());
  EXPECT_EQ(base::ErrorStorage::GetInstance().GetError(), nullptr);
  auto compiled = fml::static_ref_ptr_cast<ElementTemplateInstance>(
                      compiled_value.RefCounted())
                      .strongify();
  ASSERT_NE(compiled, nullptr);
  auto compiled_slots = compiled->Serialize().GetProperty("childSlots");
  ASSERT_EQ(compiled_slots.GetLength(), 2);
  EXPECT_EQ(compiled_slots.GetProperty(0).GetLength(), 0);
  EXPECT_EQ(compiled_slots.GetProperty(1).GetLength(), 1);

  base::ErrorStorage::GetInstance().Reset();
  auto typed_duplicate_children = lepus::CArray::Create();
  typed_duplicate_children->emplace_back(lepus::Value(child));
  typed_duplicate_children->emplace_back(lepus::Value(child));
  auto typed_duplicate_slots = lepus::CArray::Create();
  typed_duplicate_slots->emplace_back(lepus::Value(typed_duplicate_children));
  lepus::Value typed_args[] = {lepus::Value("view"), lepus::Value(),
                               lepus::Value(typed_duplicate_slots),
                               lepus::Value(16)};
  auto typed_value = RendererFunctions::FiberCreateTypedElementTemplate(
      mts_ctx, typed_args, 4);
  ASSERT_TRUE(typed_value.IsRefCounted());
  EXPECT_EQ(base::ErrorStorage::GetInstance().GetError(), nullptr);
  auto typed = fml::static_ref_ptr_cast<ElementTemplateInstance>(
                   typed_value.RefCounted())
                   .strongify();
  ASSERT_NE(typed, nullptr);
  auto typed_slot = typed->Serialize().GetProperty("childSlots").GetProperty(0);
  EXPECT_EQ(typed_slot.GetLength(), 1);
  EXPECT_EQ(child->PeekMaterializedRoot(), nullptr);
  base::ErrorStorage::GetInstance().Reset();
}

TEST_P(ElementTemplateInstanceTest,
       CreateElementTemplatePreservesSparseSlotsWithoutPreflight) {
  auto lepus_ctx = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  ASSERT_TRUE(lepus_ctx);
  lepus_ctx->Initialize();
  lepus_ctx->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  auto* mts_ctx = runtime::MTSRuntime::ToQuickContext(lepus_ctx.get());
  ASSERT_TRUE(mts_ctx);

  lepus::Value sparse_slots(lepus::CArray::Create());
  sparse_slots.SetProperty(3, lepus::Value(lepus::CArray::Create()));
  ASSERT_EQ(sparse_slots.GetLength(), 4);
  ASSERT_TRUE(sparse_slots.GetProperty(0).IsNil() ||
              sparse_slots.GetProperty(0).IsUndefined() ||
              sparse_slots.GetProperty(0).IsEmpty());

  base::ErrorStorage::GetInstance().Reset();
  lepus::Value compiled_args[] = {lepus::Value("root_template"), lepus::Value(),
                                  lepus::Value(), sparse_slots,
                                  lepus::Value(2)};
  auto compiled_value =
      RendererFunctions::FiberCreateElementTemplate(mts_ctx, compiled_args, 5);
  ASSERT_TRUE(compiled_value.IsRefCounted());
  EXPECT_EQ(base::ErrorStorage::GetInstance().GetError(), nullptr);
  auto compiled_instance = fml::static_ref_ptr_cast<ElementTemplateInstance>(
                               compiled_value.RefCounted())
                               .strongify();
  ASSERT_NE(compiled_instance, nullptr);
  auto serialized_compiled_slots =
      compiled_instance->Serialize().GetProperty("childSlots");
  ASSERT_EQ(serialized_compiled_slots.GetLength(), 4);
  for (uint32_t slot_index = 0; slot_index < 3; ++slot_index) {
    EXPECT_FALSE(
        serialized_compiled_slots.GetProperty(slot_index).IsArrayOrJSArray());
  }
  EXPECT_TRUE(serialized_compiled_slots.GetProperty(3).IsArrayOrJSArray());
  EXPECT_EQ(serialized_compiled_slots.GetProperty(3).GetLength(), 0);

  base::ErrorStorage::GetInstance().Reset();
  lepus::Value typed_args[] = {lepus::Value("view"), lepus::Value(),
                               sparse_slots, lepus::Value(16)};
  auto typed_value = RendererFunctions::FiberCreateTypedElementTemplate(
      mts_ctx, typed_args, 4);
  ASSERT_TRUE(typed_value.IsRefCounted());
  EXPECT_EQ(base::ErrorStorage::GetInstance().GetError(), nullptr);
  auto typed_instance = fml::static_ref_ptr_cast<ElementTemplateInstance>(
                            typed_value.RefCounted())
                            .strongify();
  ASSERT_NE(typed_instance, nullptr);
  auto serialized_typed_slots =
      typed_instance->Serialize().GetProperty("childSlots");
  ASSERT_EQ(serialized_typed_slots.GetLength(), 4);
  for (uint32_t slot_index = 0; slot_index < 3; ++slot_index) {
    EXPECT_FALSE(
        serialized_typed_slots.GetProperty(slot_index).IsArrayOrJSArray());
  }
  EXPECT_TRUE(serialized_typed_slots.GetProperty(3).IsArrayOrJSArray());
  EXPECT_EQ(serialized_typed_slots.GetProperty(3).GetLength(), 0);

  base::ErrorStorage::GetInstance().Reset();
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
       CreateTypedElementTemplateIgnoresNonzeroChildSlotsLocally) {
  auto lepus_ctx = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  ASSERT_TRUE(lepus_ctx);
  lepus_ctx->Initialize();
  lepus_ctx->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  auto* mts_ctx = runtime::MTSRuntime::ToQuickContext(lepus_ctx.get());
  ASSERT_TRUE(mts_ctx);

  for (const char* tag : {"page", "view"}) {
    SCOPED_TRACE(tag);
    auto child = fml::AdoptRef<ElementTemplateInstance>(
        new ElementTemplateInstance(manager));
    child->SetTypedTag(base::String("raw-text"));
    child->SetUid(lepus::Value(1));

    auto unsupported_children = lepus::CArray::Create();
    unsupported_children->emplace_back(lepus::Value(child));
    auto child_slots = lepus::CArray::Create();
    child_slots->emplace_back(lepus::Value(lepus::CArray::Create()));
    child_slots->emplace_back(lepus::Value(unsupported_children));

    base::ErrorStorage::GetInstance().Reset();
    lepus::Value args[] = {lepus::Value(tag), lepus::Value(),
                           lepus::Value(child_slots), lepus::Value(16)};
    auto created_value =
        RendererFunctions::FiberCreateTypedElementTemplate(mts_ctx, args, 4);

    ASSERT_TRUE(created_value.IsRefCounted());
    EXPECT_EQ(base::ErrorStorage::GetInstance().GetError(), nullptr);
    auto created = fml::static_ref_ptr_cast<ElementTemplateInstance>(
                       created_value.RefCounted())
                       .strongify();
    ASSERT_NE(created, nullptr);
    auto serialized_slots = created->Serialize().GetProperty("childSlots");
    ASSERT_EQ(serialized_slots.GetLength(), 2);
    EXPECT_EQ(serialized_slots.GetProperty(0).GetLength(), 0);
    EXPECT_EQ(serialized_slots.GetProperty(1).GetLength(), 0);
    EXPECT_EQ(child->PeekMaterializedRoot(), nullptr);
    base::ErrorStorage::GetInstance().Reset();
  }
}

TEST_P(ElementTemplateInstanceTest,
       CreateTypedElementTemplateRejectsListWithoutFallbackState) {
  auto lepus_ctx = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  ASSERT_TRUE(lepus_ctx);
  lepus_ctx->Initialize();
  lepus_ctx->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  auto* mts_ctx = runtime::MTSRuntime::ToQuickContext(lepus_ctx.get());
  ASSERT_TRUE(mts_ctx);

  lepus::Value args[] = {lepus::Value("list"), lepus::Value(), lepus::Value(),
                         lepus::Value(0)};
  auto* original_root = manager->root();
  auto captured_create_count = platform_impl_->captured_create_tags_map_.size();
  base::ErrorStorage::GetInstance().Reset();
  auto created_value =
      RendererFunctions::FiberCreateTypedElementTemplate(mts_ctx, args, 4);

  EXPECT_TRUE(created_value.IsEmpty());
  ASSERT_NE(base::ErrorStorage::GetInstance().GetError(), nullptr);
  EXPECT_EQ(manager->root(), original_root);
  EXPECT_EQ(platform_impl_->captured_create_tags_map_.size(),
            captured_create_count);
  base::ErrorStorage::GetInstance().Reset();
}

TEST_P(ElementTemplateInstanceTest,
       CreateCompiledElementTemplateDoesNotMaterializeOnCreate) {
  auto lepus_ctx = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  ASSERT_TRUE(lepus_ctx);
  lepus_ctx->Initialize();
  lepus_ctx->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  auto* mts_ctx = runtime::MTSRuntime::ToQuickContext(lepus_ctx.get());
  ASSERT_TRUE(mts_ctx);

  lepus::Value args[] = {lepus::Value("root_template"), lepus::Value(),
                         lepus::Value(), lepus::Value(), lepus::Value(2)};
  auto created_value =
      RendererFunctions::FiberCreateElementTemplate(mts_ctx, args, 5);

  ASSERT_TRUE(created_value.IsRefCounted());
  ASSERT_EQ(created_value.RefCounted()->GetRefType(),
            lepus::RefType::kElementTemplate);
  auto created_instance = fml::static_ref_ptr_cast<ElementTemplateInstance>(
                              created_value.RefCounted())
                              .strongify();
  ASSERT_NE(created_instance, nullptr);
  EXPECT_EQ(created_instance->PeekMaterializedRoot(), nullptr);
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplatePageAttachPreparesAndResolveUsesFinalAttributeSlots) {
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
       CreateTypedPageTemplateMaterializesTypedChildrenEagerly) {
  auto lepus_ctx = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  ASSERT_TRUE(lepus_ctx);
  lepus_ctx->Initialize();
  lepus_ctx->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  auto* mts_ctx = runtime::MTSRuntime::ToQuickContext(lepus_ctx.get());
  ASSERT_TRUE(mts_ctx);

  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTypedTag(base::String("view"));
  child->SetUid(lepus::Value(1));

  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(child));
  auto child_slots = lepus::CArray::Create();
  child_slots->emplace_back(lepus::Value(slot_children));

  lepus::Value create_args[] = {lepus::Value("page"), lepus::Value(),
                                lepus::Value(child_slots), lepus::Value(0)};
  auto created_value = RendererFunctions::FiberCreateTypedElementTemplate(
      mts_ctx, create_args, 4);

  ASSERT_TRUE(created_value.IsRefCounted());
  auto page_instance = fml::static_ref_ptr_cast<ElementTemplateInstance>(
                           created_value.RefCounted())
                           .strongify();
  ASSERT_NE(page_instance, nullptr);
  auto page_root_ref = page_instance->PeekMaterializedRoot();
  ASSERT_NE(manager->root(), nullptr);
  ASSERT_NE(page_root_ref, nullptr);
  EXPECT_EQ(manager->root(), page_root_ref.get());
  EXPECT_TRUE(page_root_ref->is_page());
  auto* page_root = static_cast<PageElement*>(page_root_ref.get());
  EXPECT_EQ(page_root->component_id().str(), "0");
  EXPECT_EQ(page_root->GetComponentCSSID(), 0);
  EXPECT_EQ(page_root->style_sheet_manager(),
            tasm->style_sheet_manager(DEFAULT_ENTRY_NAME));
  auto child_root = child->PeekMaterializedRoot();
  ASSERT_NE(child_root, nullptr);
  ASSERT_EQ(page_root_ref->children().size(), 1u);
  EXPECT_EQ(page_root_ref->children()[0].get(), child_root.get());
  EXPECT_TRUE(child_root->IsTemplateElement());
  EXPECT_FALSE(page_instance->HasPendingChildMounts());
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
       CreateTypedPageTemplateNotifiesInspectorRoot) {
  if (!ENABLE_INSPECTOR) {
    GTEST_SKIP() << "Inspector callbacks are compiled out.";
  }
  auto observer = std::make_shared<RecordingInspectorElementObserver>();
  manager->SetInspectorElementObserver(observer);
  manager->dom_tree_enabled_ = true;

  auto lepus_ctx = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  ASSERT_TRUE(lepus_ctx);
  lepus_ctx->Initialize();
  lepus_ctx->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  auto* mts_ctx = runtime::MTSRuntime::ToQuickContext(lepus_ctx.get());
  ASSERT_TRUE(mts_ctx);

  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTypedTag(base::String("view"));
  child->SetUid(lepus::Value(1));
  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(child));
  auto child_slots = lepus::CArray::Create();
  child_slots->emplace_back(lepus::Value(slot_children));

  lepus::Value create_args[] = {lepus::Value("page"), lepus::Value(),
                                lepus::Value(child_slots), lepus::Value(0)};
  auto created_value = RendererFunctions::FiberCreateTypedElementTemplate(
      mts_ctx, create_args, 4);

  ASSERT_TRUE(created_value.IsRefCounted());
  auto page_instance = fml::static_ref_ptr_cast<ElementTemplateInstance>(
                           created_value.RefCounted())
                           .strongify();
  ASSERT_NE(page_instance, nullptr);
  ASSERT_NE(manager->root(), nullptr);
  auto child_root = child->PeekMaterializedRoot();
  ASSERT_NE(child_root, nullptr);
  ASSERT_EQ(page_instance->PeekMaterializedRoot()->children().size(), 1u);
  EXPECT_EQ(page_instance->PeekMaterializedRoot()->children()[0].get(),
            child_root.get());
  EXPECT_EQ(child_root->parent(), page_instance->PeekMaterializedRoot().get());
  EXPECT_TRUE(observer->removed_nodes.empty());
  if (ENABLE_INSPECTOR) {
    auto* page_root = page_instance->PeekMaterializedRoot().get();
    EXPECT_EQ(std::count(observer->added_nodes.begin(),
                         observer->added_nodes.end(), child_root.get()),
              1);
    EXPECT_EQ(std::count(observer->added_nodes.begin(),
                         observer->added_nodes.end(), page_root),
              1);
    auto child_it = std::find(observer->added_nodes.begin(),
                              observer->added_nodes.end(), child_root.get());
    ASSERT_NE(child_it, observer->added_nodes.end());
    ASSERT_EQ(observer->added_nodes.size(),
              observer->added_node_parents.size());
    auto child_index = std::distance(observer->added_nodes.begin(), child_it);
    EXPECT_EQ(observer->added_node_parents[child_index], page_root);
    EXPECT_EQ(manager->root(), page_root);
    EXPECT_TRUE(page_root->is_page());
  }
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateInspectorNotificationsFollowDirectMutation) {
  if (!ENABLE_INSPECTOR) {
    GTEST_SKIP() << "Inspector callbacks are compiled out.";
  }
  auto observer = std::make_shared<RecordingInspectorElementObserver>();
  manager->SetInspectorElementObserver(observer);
  manager->dom_tree_enabled_ = true;

  auto lepus_ctx = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  ASSERT_TRUE(lepus_ctx);
  lepus_ctx->Initialize();
  lepus_ctx->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  auto* mts_ctx = runtime::MTSRuntime::ToQuickContext(lepus_ctx.get());
  ASSERT_TRUE(mts_ctx);

  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTypedTag(base::String("view"));
  child->SetUid(lepus::Value(1));
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
  ASSERT_EQ(child_root->parent(), parent_root.get());
  observer->added_nodes.clear();
  observer->added_node_parents.clear();
  observer->removed_nodes.clear();
  observer->removed_node_parents.clear();

  lepus::Value remove_args[] = {lepus::Value(parent), lepus::Value(0),
                                lepus::Value(child)};
  RendererFunctions::FiberRemoveNodeFromElementTemplate(mts_ctx, remove_args,
                                                        3);
  ASSERT_EQ(observer->removed_nodes.size(), 1u);
  EXPECT_EQ(observer->removed_nodes[0], child_root.get());
  EXPECT_EQ(observer->removed_node_parents[0], parent_root.get());
  EXPECT_EQ(child_root->parent(), nullptr);

  observer->removed_nodes.clear();
  observer->removed_node_parents.clear();
  lepus::Value insert_args[] = {lepus::Value(parent), lepus::Value(0),
                                lepus::Value(child), lepus::Value()};
  RendererFunctions::FiberInsertNodeToElementTemplate(mts_ctx, insert_args, 4);
  ASSERT_EQ(observer->added_nodes.size(), 1u);
  EXPECT_EQ(observer->added_nodes[0], child_root.get());
  EXPECT_EQ(observer->added_node_parents[0], parent_root.get());
  EXPECT_EQ(child_root->parent(), parent_root.get());

  observer->added_nodes.clear();
  observer->added_node_parents.clear();
  RendererFunctions::FiberRemoveNodeFromElementTemplate(mts_ctx, remove_args,
                                                        3);
  RendererFunctions::FiberInsertNodeToElementTemplate(mts_ctx, insert_args, 4);
  ASSERT_EQ(observer->removed_nodes.size(), 1u);
  EXPECT_EQ(observer->removed_nodes[0], child_root.get());
  EXPECT_EQ(observer->removed_node_parents[0], parent_root.get());
  ASSERT_EQ(observer->added_nodes.size(), 1u);
  EXPECT_EQ(observer->added_nodes[0], child_root.get());
  EXPECT_EQ(observer->added_node_parents[0], parent_root.get());
  EXPECT_EQ(child_root->parent(), parent_root.get());
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplatePendingMountCancelsBeforeDrain) {
  auto parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  parent->SetTypedTag(base::String("view"));
  auto parent_root = parent->GetRoot();
  ASSERT_NE(parent_root, nullptr);

  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTASM(tasm.get());
  child->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  child->SetTemplateKey(base::String("missing_template"));
  parent->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());

  ASSERT_TRUE(parent->HasPendingChildMounts());

  parent->RemoveNodeFromChildSlot(0, lepus::Value(child));
  manager->DrainPendingElementTemplateChildMounts(parent_root.get());

  EXPECT_FALSE(parent->HasPendingChildMounts());
  EXPECT_EQ(child->PeekMaterializedRoot(), nullptr);
}

TEST_P(ElementTemplateInstanceTest,
       PendingElementTemplateParentIsNotRetainedByElementManager) {
  auto destruction_count = std::make_shared<int>(0);
  auto parent = fml::AdoptRef<DestructionTrackingElementTemplateInstance>(
      new DestructionTrackingElementTemplateInstance(manager,
                                                     destruction_count));
  parent->SetTypedTag(base::String("view"));
  auto parent_root = parent->GetRoot();
  ASSERT_NE(parent_root, nullptr);

  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTASM(tasm.get());
  child->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  child->SetTemplateKey(base::String("missing_template"));
  parent->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());

  ASSERT_TRUE(parent->HasPendingChildMounts());
  parent = nullptr;

  EXPECT_EQ(*destruction_count, 1);
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateHandlesReleaseAfterOwnerTeardown) {
  auto default_entry = std::make_shared<TemplateEntry>();
  default_entry->SetName(DEFAULT_ENTRY_NAME);
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = default_entry;

  auto template_info = std::make_shared<ElementTemplateInfo>();
  template_info->exist_ = true;
  template_info->key_ = "teardown_child";
  auto root_info = ElementInfo();
  root_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  template_info->elements_.emplace_back(std::move(root_info));
  default_entry->template_bundle_.element_template_infos_["teardown_child"] =
      std::move(template_info);

  auto destruction_count = std::make_shared<int>(0);
  auto child = fml::AdoptRef<DestructionTrackingElementTemplateInstance>(
      new DestructionTrackingElementTemplateInstance(manager,
                                                     destruction_count));
  child->SetTASM(tasm.get());
  child->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  child->SetTemplateKey(base::String("teardown_child"));
  auto page = fml::AdoptRef<DestructionTrackingElementTemplateInstance>(
      new DestructionTrackingElementTemplateInstance(manager,
                                                     destruction_count));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->SetUid(lepus::Value(0));
  page->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());

  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);
  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page_root.get());
  auto child_root = child->PeekMaterializedRoot();
  ASSERT_NE(child_root, nullptr);
  ASSERT_EQ(child_root->parent(), page_root.get());

  tasm->Destroy();
  EXPECT_TRUE(page_root->will_destroy());
  EXPECT_TRUE(child_root->will_destroy());

  EXPECT_EQ(*destruction_count, 0);
  page = nullptr;
  EXPECT_EQ(*destruction_count, 1);
  child = nullptr;
  EXPECT_EQ(*destruction_count, 2);
  page_root = nullptr;
  child_root = nullptr;
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
  child->SetRootAttributes(lepus::Value(initial_attributes));
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
  child->SetRootAttributes(lepus::Value(updated_attributes));
  EXPECT_FALSE(child->HasPendingChildMounts());
  ASSERT_EQ(DatasetValue(child_root.get(), "state")->StdString(), "reattached");

  parent->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());

  EXPECT_FALSE(child->HasPendingChildMounts());
  ASSERT_EQ(child_root->parent(), parent_root.get());
  ASSERT_NE(DatasetValue(child_root.get(), "state"), nullptr);
  EXPECT_EQ(DatasetValue(child_root.get(), "state")->StdString(), "reattached");
}

TEST_P(ElementTemplateInstanceTest,
       DetachedPendingCompiledChildMountWakesOnReattach) {
  auto default_entry = std::make_shared<TemplateEntry>();
  default_entry->SetName(DEFAULT_ENTRY_NAME);
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = default_entry;

  auto template_info = std::make_shared<ElementTemplateInfo>();
  template_info->exist_ = true;
  template_info->key_ = "compiled_child";
  auto root_info = ElementInfo();
  root_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  template_info->elements_.emplace_back(std::move(root_info));
  default_entry->template_bundle_.element_template_infos_["compiled_child"] =
      std::move(template_info);

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
  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTASM(tasm.get());
  child->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  child->SetTemplateKey(base::String("compiled_child"));
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
  auto default_entry = std::make_shared<TemplateEntry>();
  default_entry->SetName(DEFAULT_ENTRY_NAME);
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = default_entry;

  auto template_info = std::make_shared<ElementTemplateInfo>();
  template_info->exist_ = true;
  template_info->key_ = "compiled_child";
  auto root_info = ElementInfo();
  root_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  template_info->elements_.emplace_back(std::move(root_info));
  default_entry->template_bundle_.element_template_infos_["compiled_child"] =
      std::move(template_info);

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

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page_root.get());

  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTASM(tasm.get());
  child->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  child->SetTemplateKey(base::String("compiled_child"));
  source->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());
  destination->InsertNodeIntoChildSlot(0, lepus::Value(child),
                                       lepus::Value(sibling));

  ASSERT_TRUE(source->HasPendingChildMounts());
  ASSERT_TRUE(destination->HasPendingChildMounts());
  options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, source_root.get());
  EXPECT_FALSE(source->HasPendingChildMounts());
  EXPECT_TRUE(destination->HasPendingChildMounts());
  EXPECT_EQ(child->PeekMaterializedRoot(), nullptr);

  options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, destination_root.get());

  auto child_root = child->PeekMaterializedRoot();
  ASSERT_NE(child_root, nullptr);
  ASSERT_EQ(destination_root->children().size(), 2u);
  EXPECT_EQ(destination_root->children()[0].get(), child_root.get());
  EXPECT_EQ(destination_root->children()[1].get(), sibling_root.get());
  EXPECT_EQ(child_root->parent(), destination_root.get());
  EXPECT_TRUE(source_root->children().empty());
  EXPECT_FALSE(destination->HasPendingChildMounts());
}

TEST_P(ElementTemplateInstanceTest,
       MaterializedElementTemplateAttributesUpdateBeforeAnyFlush) {
  auto first = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  first->SetTypedTag(base::String("view"));
  first->SetUid(lepus::Value(9));
  auto second = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  second->SetTypedTag(base::String("view"));
  second->SetUid(lepus::Value(10));
  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->SetUid(lepus::Value(0));
  page->InsertNodeIntoChildSlot(0, lepus::Value(first), lepus::Value());
  page->InsertNodeIntoChildSlot(0, lepus::Value(second), lepus::Value());

  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);
  auto first_root = first->PeekMaterializedRoot();
  auto second_root = second->PeekMaterializedRoot();
  ASSERT_NE(first_root, nullptr);
  ASSERT_NE(second_root, nullptr);

  auto updated_attributes = lepus::Dictionary::Create();
  updated_attributes->SetValue(base::String("data-state"),
                               lepus::Value("updated"));
  second->SetRootAttributes(lepus::Value(updated_attributes));
  EXPECT_FALSE(second->HasPendingChildMounts());
  ASSERT_NE(DatasetValue(second_root.get(), "state"), nullptr);
  EXPECT_EQ(DatasetValue(second_root.get(), "state")->StdString(), "updated");
}

TEST_P(ElementTemplateInstanceTest,
       InsertTypedPageTemplateChildUpdatesDirectly) {
  auto lepus_ctx = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  ASSERT_TRUE(lepus_ctx);
  lepus_ctx->Initialize();
  lepus_ctx->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  auto* mts_ctx = runtime::MTSRuntime::ToQuickContext(lepus_ctx.get());
  ASSERT_TRUE(mts_ctx);

  lepus::Value create_args[] = {lepus::Value("page"), lepus::Value(),
                                lepus::Value(), lepus::Value(0)};
  auto created_value = RendererFunctions::FiberCreateTypedElementTemplate(
      mts_ctx, create_args, 4);
  ASSERT_TRUE(created_value.IsRefCounted());
  auto page_instance = fml::static_ref_ptr_cast<ElementTemplateInstance>(
                           created_value.RefCounted())
                           .strongify();
  ASSERT_NE(page_instance, nullptr);
  ASSERT_NE(manager->root(), nullptr);
  auto page_root = page_instance->PeekMaterializedRoot();
  ASSERT_NE(page_root, nullptr);

  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTypedTag(base::String("view"));
  child->SetUid(lepus::Value(1));

  lepus::Value insert_args[] = {lepus::Value(page_instance), lepus::Value(0),
                                lepus::Value(child), lepus::Value()};
  RendererFunctions::FiberInsertNodeToElementTemplate(mts_ctx, insert_args, 4);

  auto child_root = child->PeekMaterializedRoot();
  ASSERT_NE(child_root, nullptr);
  ASSERT_EQ(page_root->children().size(), 1u);
  EXPECT_EQ(page_root->children()[0].get(), child_root.get());
  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options);
  ASSERT_EQ(page_root->children().size(), 1u);
  EXPECT_EQ(page_root->children()[0].get(), child_root.get());
  EXPECT_TRUE(static_cast<Element*>(page_root->children()[0].get())
                  ->IsTemplateElement());
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
       TypedElementTemplateAppliesRootAttributesAsSpread) {
  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
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

  EXPECT_FALSE(root->HasPendingChildMounts());
  EXPECT_EQ(DatasetValue(resolved.get(), "test"), nullptr);
  added_data = DatasetValue(resolved.get(), "added");
  ASSERT_NE(added_data, nullptr);
  EXPECT_EQ(added_data->StdString(), "updated");
  EXPECT_EQ(resolved->event_map().count("tap"), 0u);

  root->SetRootAttributes(lepus::Value(lepus::Dictionary::Create()));
  EXPECT_FALSE(root->HasPendingChildMounts());
  EXPECT_EQ(DatasetValue(resolved.get(), "added"), nullptr);
  EXPECT_TRUE(root->Serialize().GetProperty("attributes").IsEmpty());
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateDirectAttributesReachPaintingOnOrdinaryFlush) {
  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->SetUid(lepus::Value(0));

  auto initial_attributes = lepus::Dictionary::Create();
  initial_attributes->SetValue(base::String("data-recording"),
                               lepus::Value("initial"));
  page->SetRootAttributes(lepus::Value(std::move(initial_attributes)));

  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);
  page_root->FlushActionsAsRoot();
  platform_impl_->Flush();
  auto* painting_props = PaintingPropsFor(manager, page_root.get());
  ASSERT_NE(painting_props, nullptr);
  ASSERT_NE(painting_props->find("dataset"), painting_props->end());
  EXPECT_EQ(painting_props->at("dataset").GetProperty("recording").StdString(),
            "initial");

  auto root_attributes = lepus::Dictionary::Create();
  root_attributes->SetValue(base::String("data-recording"),
                            lepus::Value("root-update"));
  page->SetRootAttributes(lepus::Value(std::move(root_attributes)));
  ASSERT_NE(DatasetValue(page_root.get(), "recording"), nullptr);
  EXPECT_EQ(DatasetValue(page_root.get(), "recording")->StdString(),
            "root-update");
  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page_root.get());
  platform_impl_->Flush();
  EXPECT_EQ(painting_props->at("dataset").GetProperty("recording").StdString(),
            "root-update");

  auto slot_attributes = lepus::Dictionary::Create();
  slot_attributes->SetValue(base::String("data-recording"),
                            lepus::Value("slot-update"));
  lepus::Value slot_args[] = {lepus::Value(page), lepus::Value(0),
                              lepus::Value(std::move(slot_attributes))};
  RendererFunctions::FiberSetAttributeOfElementTemplate(nullptr, slot_args, 3);
  ASSERT_NE(DatasetValue(page_root.get(), "recording"), nullptr);
  EXPECT_EQ(DatasetValue(page_root.get(), "recording")->StdString(),
            "slot-update");
  options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page_root.get());
  platform_impl_->Flush();
  EXPECT_EQ(painting_props->at("dataset").GetProperty("recording").StdString(),
            "slot-update");
  EXPECT_FALSE(page->HasPendingChildMounts());
}

TEST_P(ElementTemplateInstanceTest,
       RendererFunctionCreateTypedElementTemplate) {
  auto lepus_ctx = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  ASSERT_TRUE(lepus_ctx);
  lepus_ctx->Initialize();
  lepus_ctx->SetGlobalData(
      BASE_STATIC_STRING(tasm::kTemplateAssembler),
      lepus::Value(static_cast<runtime::MTSRuntime::Delegate*>(tasm.get())));
  auto* mts_ctx = runtime::MTSRuntime::ToQuickContext(lepus_ctx.get());
  ASSERT_TRUE(mts_ctx);

  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTypedTag(base::String("raw-text"));
  child->SetUid(lepus::Value(1));

  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(child));
  auto child_slots = lepus::CArray::Create();
  child_slots->emplace_back(lepus::Value(slot_children));

  auto attributes = lepus::Dictionary::Create();
  attributes->SetValue(base::String("data-test"), lepus::Value("root_attr"));
  lepus::Value args[] = {lepus::Value("view"), lepus::Value(attributes),
                         lepus::Value(child_slots), lepus::Value(2)};
  auto created_value =
      RendererFunctions::FiberCreateTypedElementTemplate(mts_ctx, args, 4);

  ASSERT_TRUE(created_value.IsRefCounted());
  auto typed_instance = fml::static_ref_ptr_cast<ElementTemplateInstance>(
                            created_value.RefCounted())
                            .strongify();
  ASSERT_NE(typed_instance, nullptr);
  EXPECT_EQ(typed_instance->PeekMaterializedRoot(), nullptr);

  auto updated_attributes = lepus::Dictionary::Create();
  updated_attributes->SetValue(base::String("data-test"),
                               lepus::Value("updated_attr"));
  updated_attributes->SetValue(base::String("data-added"),
                               lepus::Value("added_attr"));
  lepus::Value update_args[] = {lepus::Value(typed_instance), lepus::Value(0),
                                lepus::Value(updated_attributes)};
  RendererFunctions::FiberSetAttributeOfElementTemplate(nullptr, update_args,
                                                        3);
  EXPECT_EQ(typed_instance->PeekMaterializedRoot(), nullptr);

  auto serialized_before_resolve = typed_instance->Serialize();
  EXPECT_EQ(serialized_before_resolve.GetProperty("attributes")
                .GetProperty("data-test")
                .StdString(),
            "updated_attr");
  EXPECT_EQ(serialized_before_resolve.GetProperty("attributes")
                .GetProperty("data-added")
                .StdString(),
            "added_attr");

  auto ignored_attributes = lepus::Dictionary::Create();
  ignored_attributes->SetValue(base::String("data-test"),
                               lepus::Value("ignored_attr"));
  lepus::Value ignored_update_args[] = {lepus::Value(typed_instance),
                                        lepus::Value(1),
                                        lepus::Value(ignored_attributes)};
  base::ErrorStorage::GetInstance().Reset();
  RendererFunctions::FiberSetAttributeOfElementTemplate(nullptr,
                                                        ignored_update_args, 3);
  EXPECT_EQ(base::ErrorStorage::GetInstance().GetError(), nullptr);
  EXPECT_EQ(typed_instance->Serialize()
                .GetProperty("attributes")
                .GetProperty("data-test")
                .StdString(),
            "updated_attr");

  auto root = typed_instance->GetRoot();
  ASSERT_NE(root, nullptr);
  manager->DrainPendingElementTemplateChildMounts(root.get());
  EXPECT_TRUE(root->is_view());
  auto* test_data = DatasetValue(root.get(), "test");
  ASSERT_NE(test_data, nullptr);
  EXPECT_EQ(test_data->StdString(), "updated_attr");
  auto* added_data = DatasetValue(root.get(), "added");
  ASSERT_NE(added_data, nullptr);
  EXPECT_EQ(added_data->StdString(), "added_attr");
  EXPECT_EQ(root->data_model_->attributes().count("data-test"), 0u);
  ASSERT_EQ(root->children().size(), 1u);
  auto* mounted_child = static_cast<Element*>(root->children()[0].get());
  ASSERT_NE(mounted_child, nullptr);
  auto child_root = child->PeekMaterializedRoot();
  ASSERT_NE(child_root, nullptr);
  EXPECT_EQ(mounted_child, child_root.get());
  EXPECT_TRUE(mounted_child->is_raw_text());
  EXPECT_TRUE(mounted_child->IsTemplateElement());

  auto serialized = typed_instance->Serialize();
  EXPECT_EQ(serialized.GetProperty("tag").StdString(), "view");
  EXPECT_EQ(serialized.GetProperty("uid").Number(), 2);
  EXPECT_EQ(
      serialized.GetProperty("attributes").GetProperty("data-test").StdString(),
      "updated_attr");
  EXPECT_EQ(serialized.GetProperty("attributes")
                .GetProperty("data-added")
                .StdString(),
            "added_attr");
  EXPECT_EQ(serialized.GetProperty("childSlots")
                .GetProperty(0)
                .GetProperty(0)
                .GetProperty("tag")
                .StdString(),
            "raw-text");
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateDynamicAPIsCacheLogicalStateBeforeRoot) {
  auto first = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  first->SetTemplateKey(base::String("first_template"));

  auto second = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  second->SetTemplateKey(base::String("second_template"));

  auto attribute_slots = lepus::CArray::Create();
  attribute_slots->emplace_back(lepus::Value("old_value"));

  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(first));
  auto child_slots = lepus::CArray::Create();
  child_slots->emplace_back(lepus::Value(slot_children));

  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->SetTemplateKey(base::String("root_template"));
  root->SetAttributeSlots(lepus::Value(attribute_slots));
  root->InitializeChildSlots(lepus::Value(child_slots));

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

  EXPECT_EQ(root->PeekMaterializedRoot(), nullptr);

  auto serialized = root->Serialize();
  auto serialized_attribute_slots = serialized.GetProperty("attributeSlots");
  ASSERT_TRUE(serialized_attribute_slots.IsArrayOrJSArray());
  EXPECT_EQ(serialized_attribute_slots.GetProperty(0).StdString(), "new_value");

  auto serialized_child_slots = serialized.GetProperty("childSlots");
  ASSERT_TRUE(serialized_child_slots.IsArrayOrJSArray());
  auto serialized_slot_children = serialized_child_slots.GetProperty(0);
  ASSERT_TRUE(serialized_slot_children.IsArrayOrJSArray());
  ASSERT_EQ(serialized_slot_children.GetLength(), 1);
  EXPECT_EQ(serialized_slot_children.GetProperty(0)
                .GetProperty("templateKey")
                .StdString(),
            "second_template");
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateMutationAPIsUseNumericNonnegativeSlotBoundary) {
  auto instance = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  instance->SetTemplateKey(base::String("root_template"));
  auto attribute_slots = lepus::CArray::Create();
  attribute_slots->emplace_back(lepus::Value("stable"));
  instance->SetAttributeSlots(lepus::Value(attribute_slots));

  lepus::Value string_index_args[] = {lepus::Value(instance), lepus::Value("0"),
                                      lepus::Value("ignored")};
  RendererFunctions::FiberSetAttributeOfElementTemplate(nullptr,
                                                        string_index_args, 3);
  lepus::Value negative_index_args[] = {
      lepus::Value(instance), lepus::Value(-1), lepus::Value("ignored")};
  RendererFunctions::FiberSetAttributeOfElementTemplate(nullptr,
                                                        negative_index_args, 3);

  auto serialized_slots = instance->Serialize().GetProperty("attributeSlots");
  ASSERT_EQ(serialized_slots.GetLength(), 1);
  EXPECT_EQ(serialized_slots.GetProperty(0).StdString(), "stable");

  lepus::Value fractional_index_args[] = {
      lepus::Value(instance), lepus::Value(0.5), lepus::Value("updated")};
  RendererFunctions::FiberSetAttributeOfElementTemplate(
      nullptr, fractional_index_args, 3);
  serialized_slots = instance->Serialize().GetProperty("attributeSlots");
  ASSERT_EQ(serialized_slots.GetLength(), 1);
  EXPECT_EQ(serialized_slots.GetProperty(0).StdString(), "updated");
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
  typed->SetRootAttributes(lepus::Value(attributes));
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
       ElementTemplateUnavailableChildDoesNotBlockOtherSlotsOrRecovery) {
  auto default_entry = std::make_shared<TemplateEntry>();
  default_entry->SetName(DEFAULT_ENTRY_NAME);
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = default_entry;

  auto template_info = std::make_shared<ElementTemplateInfo>();
  template_info->exist_ = true;
  template_info->key_ = "two_slots";
  auto root_info = ElementInfo();
  root_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  for (int32_t slot_index = 0; slot_index < 2; ++slot_index) {
    auto slot_info = ElementInfo();
    slot_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_SLOT;
    slot_info.slot_index_ = slot_index;
    root_info.children_.emplace_back(std::move(slot_info));
    auto sentinel_info = ElementInfo();
    sentinel_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
    root_info.children_.emplace_back(std::move(sentinel_info));
  }
  template_info->elements_.emplace_back(std::move(root_info));
  default_entry->template_bundle_.element_template_infos_["two_slots"] =
      std::move(template_info);

  auto make_slots = [](const fml::RefPtr<ElementTemplateInstance>& first,
                       const fml::RefPtr<ElementTemplateInstance>& second) {
    auto first_children = lepus::CArray::Create();
    first_children->emplace_back(lepus::Value(first));
    auto second_children = lepus::CArray::Create();
    second_children->emplace_back(lepus::Value(second));
    auto slots = lepus::CArray::Create();
    slots->emplace_back(lepus::Value(std::move(first_children)));
    slots->emplace_back(lepus::Value(std::move(second_children)));
    return lepus::Value(std::move(slots));
  };

  auto old_first = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  old_first->SetTypedTag(base::String("raw-text"));
  auto old_second = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  old_second->SetTypedTag(base::String("raw-text"));
  auto parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  parent->SetTASM(tasm.get());
  parent->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  parent->SetTemplateKey(base::String("two_slots"));
  parent->InitializeChildSlots(make_slots(old_first, old_second));

  auto parent_root = parent->GetRoot();
  ASSERT_NE(parent_root, nullptr);
  manager->DrainPendingElementTemplateChildMounts(parent_root.get());
  auto old_first_root = old_first->PeekMaterializedRoot();
  auto old_second_root = old_second->PeekMaterializedRoot();
  ASSERT_NE(old_first_root, nullptr);
  ASSERT_NE(old_second_root, nullptr);
  EXPECT_EQ(old_first_root->parent(), parent_root.get());
  EXPECT_EQ(old_second_root->parent(), parent_root.get());

  auto unavailable_second = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  unavailable_second->SetTASM(tasm.get());
  unavailable_second->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  unavailable_second->SetTemplateKey(base::String("missing_template"));
  auto next_first = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  next_first->SetTypedTag(base::String("raw-text"));

  parent->RemoveNodeFromChildSlot(1, lepus::Value(old_second));
  parent->InsertNodeIntoChildSlot(1, lepus::Value(unavailable_second),
                                  lepus::Value());
  parent->RemoveNodeFromChildSlot(0, lepus::Value(old_first));
  parent->InsertNodeIntoChildSlot(0, lepus::Value(next_first), lepus::Value());
  manager->DrainPendingElementTemplateChildMounts(parent_root.get());

  auto next_first_root = next_first->PeekMaterializedRoot();
  ASSERT_NE(next_first_root, nullptr);
  EXPECT_EQ(next_first_root->parent(), parent_root.get());
  EXPECT_EQ(old_first_root->parent(), nullptr);
  EXPECT_EQ(old_second_root->parent(), nullptr);
  EXPECT_EQ(unavailable_second->PeekMaterializedRoot(), nullptr);
  EXPECT_TRUE(parent->HasPendingChildMounts());

  auto next_second = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  next_second->SetTypedTag(base::String("raw-text"));
  parent->RemoveNodeFromChildSlot(1, lepus::Value(unavailable_second));
  parent->InsertNodeIntoChildSlot(1, lepus::Value(next_second), lepus::Value());
  manager->DrainPendingElementTemplateChildMounts(parent_root.get());

  auto next_second_root = next_second->PeekMaterializedRoot();
  ASSERT_NE(next_second_root, nullptr);
  EXPECT_EQ(next_first_root->parent(), parent_root.get());
  EXPECT_EQ(next_second_root->parent(), parent_root.get());
  EXPECT_FALSE(parent->HasPendingChildMounts());
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateAPIsRejectOrdinaryChildSlotChildren) {
  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->SetTemplateKey(base::String("root_template"));

  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTemplateKey(base::String("child_template"));
  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(child));
  auto child_slots = lepus::CArray::Create();
  child_slots->emplace_back(lepus::Value(slot_children));
  root->InitializeChildSlots(lepus::Value(child_slots));

  auto ordinary_element = manager->CreateFiberView();
  lepus::Value insert_args[] = {lepus::Value(root), lepus::Value(0),
                                lepus::Value(ordinary_element),
                                lepus::Value(child)};
  base::ErrorStorage::GetInstance().Reset();
  RendererFunctions::FiberInsertNodeToElementTemplate(nullptr, insert_args, 4);
  ASSERT_NE(base::ErrorStorage::GetInstance().GetError(), nullptr);

  lepus::Value invalid_reference_args[] = {lepus::Value(root), lepus::Value(0),
                                           lepus::Value(child),
                                           lepus::Value(ordinary_element)};
  base::ErrorStorage::GetInstance().Reset();
  RendererFunctions::FiberInsertNodeToElementTemplate(
      nullptr, invalid_reference_args, 4);
  ASSERT_NE(base::ErrorStorage::GetInstance().GetError(), nullptr);

  lepus::Value remove_args[] = {lepus::Value(root), lepus::Value(0),
                                lepus::Value(ordinary_element)};
  base::ErrorStorage::GetInstance().Reset();
  RendererFunctions::FiberRemoveNodeFromElementTemplate(nullptr, remove_args,
                                                        3);
  ASSERT_NE(base::ErrorStorage::GetInstance().GetError(), nullptr);
  base::ErrorStorage::GetInstance().Reset();

  auto serialized_slot_children =
      root->Serialize().GetProperty("childSlots").GetProperty(0);
  ASSERT_TRUE(serialized_slot_children.IsArrayOrJSArray());
  ASSERT_EQ(serialized_slot_children.GetLength(), 1);
  EXPECT_EQ(serialized_slot_children.GetProperty(0)
                .GetProperty("templateKey")
                .StdString(),
            "child_template");
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
  manager->DrainPendingElementTemplateChildMounts(resolved.get());
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

  first_parent->RemoveNodeFromChildSlot(0, lepus::Value(child));
  first_parent->RemoveNodeFromChildSlot(1, lepus::Value(child));
  second_parent->RemoveNodeFromChildSlot(0, lepus::Value(child));
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
       ElementTemplateMaterializedParentDestructionDetachesRetainedChild) {
  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTypedTag(base::String("raw-text"));
  child->SetUid(lepus::Value(1));

  auto parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  parent->SetTypedTag(base::String("view"));
  parent->SetUid(lepus::Value(5));
  parent->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());

  auto parent_root = parent->GetRoot();
  ASSERT_NE(parent_root, nullptr);
  auto child_root = child->PeekMaterializedRoot();
  ASSERT_NE(child_root, nullptr);
  ASSERT_EQ(child_root->parent(), parent_root.get());

  parent_root = nullptr;
  parent = nullptr;

  ASSERT_EQ(child_root->parent(), nullptr);

  auto destination = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  destination->SetTypedTag(base::String("view"));
  destination->SetUid(lepus::Value(8));
  destination->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());

  auto destination_root = destination->GetRoot();
  ASSERT_NE(destination_root, nullptr);
  manager->DrainPendingElementTemplateChildMounts(destination_root.get());
  ASSERT_EQ(destination_root->children().size(), 1u);
  EXPECT_EQ(destination_root->children()[0].get(), child_root.get());
  EXPECT_EQ(child_root->parent(), destination_root.get());
  EXPECT_EQ(destination->Serialize()
                .GetProperty("childSlots")
                .GetProperty(0)
                .GetProperty(0)
                .GetProperty("uid")
                .Number(),
            1);
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
       ElementTemplateAdjacentChildSlotsPreservePendingMountOrder) {
  auto default_entry = std::make_shared<TemplateEntry>();
  default_entry->SetName(DEFAULT_ENTRY_NAME);
  tasm->template_entries_[DEFAULT_ENTRY_NAME] = default_entry;

  auto root_template_info = std::make_shared<ElementTemplateInfo>();
  root_template_info->exist_ = true;
  root_template_info->key_ = "adjacent_slots";
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
  root_template_info->elements_.emplace_back(std::move(root_info));
  default_entry->template_bundle_.element_template_infos_["adjacent_slots"] =
      std::move(root_template_info);

  auto child_template_info = std::make_shared<ElementTemplateInfo>();
  child_template_info->exist_ = true;
  child_template_info->key_ = "compiled_child";
  auto child_root_info = ElementInfo();
  child_root_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  child_template_info->elements_.emplace_back(std::move(child_root_info));
  default_entry->template_bundle_.element_template_infos_["compiled_child"] =
      std::move(child_template_info);

  auto compiled_child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  compiled_child->SetTASM(tasm.get());
  compiled_child->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  compiled_child->SetTemplateKey(base::String("compiled_child"));
  auto available_child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  available_child->SetTypedTag(base::String("view"));
  auto first_slot_children = lepus::CArray::Create();
  first_slot_children->emplace_back(lepus::Value(compiled_child));
  auto second_slot_children = lepus::CArray::Create();
  second_slot_children->emplace_back(lepus::Value(available_child));
  auto child_slots = lepus::CArray::Create();
  child_slots->emplace_back(lepus::Value(first_slot_children));
  child_slots->emplace_back(lepus::Value(second_slot_children));

  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->SetTASM(tasm.get());
  root->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  root->SetTemplateKey(base::String("adjacent_slots"));
  root->InitializeChildSlots(lepus::Value(child_slots));

  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->InsertNodeIntoChildSlot(0, lepus::Value(root), lepus::Value());
  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);
  ASSERT_TRUE(page->HasPendingChildMounts());
  EXPECT_EQ(root->PeekMaterializedRoot(), nullptr);
  EXPECT_EQ(compiled_child->PeekMaterializedRoot(), nullptr);

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page_root.get());

  auto resolved = root->PeekMaterializedRoot();
  auto compiled_root = compiled_child->PeekMaterializedRoot();
  auto available_root = available_child->PeekMaterializedRoot();
  ASSERT_NE(resolved, nullptr);
  ASSERT_NE(compiled_root, nullptr);
  ASSERT_NE(available_root, nullptr);
  ASSERT_EQ(page_root->children().size(), 1u);
  EXPECT_EQ(page_root->children()[0].get(), resolved.get());
  ASSERT_EQ(resolved->children().size(), 3u);
  EXPECT_EQ(resolved->children()[0].get(), compiled_root.get());
  EXPECT_EQ(resolved->children()[1].get(), available_root.get());
  EXPECT_FALSE(page->HasPendingChildMounts());
  EXPECT_FALSE(root->HasPendingChildMounts());
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
       ElementTemplateMaterializedCrossParentMoveConverges) {
  auto child = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  child->SetTypedTag(base::String("raw-text"));
  child->SetUid(lepus::Value(1));

  auto first_parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  first_parent->SetTypedTag(base::String("view"));
  first_parent->SetUid(lepus::Value(13));
  first_parent->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());

  auto second_parent = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  second_parent->SetTypedTag(base::String("view"));
  second_parent->SetUid(lepus::Value(14));

  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->SetUid(lepus::Value(0));
  page->InsertNodeIntoChildSlot(0, lepus::Value(first_parent), lepus::Value());
  page->InsertNodeIntoChildSlot(0, lepus::Value(second_parent), lepus::Value());

  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);
  auto first_root = first_parent->PeekMaterializedRoot();
  auto second_root = second_parent->PeekMaterializedRoot();
  auto child_root = child->PeekMaterializedRoot();
  ASSERT_NE(first_root, nullptr);
  ASSERT_NE(second_root, nullptr);
  ASSERT_NE(child_root, nullptr);
  ASSERT_EQ(first_root->children().size(), 1u);
  EXPECT_EQ(first_root->children()[0].get(), child_root.get());
  EXPECT_TRUE(second_root->children().empty());

  second_parent->InsertNodeIntoChildSlot(0, lepus::Value(child),
                                         lepus::Value());

  EXPECT_TRUE(first_root->children().empty());
  ASSERT_EQ(second_root->children().size(), 1u);
  EXPECT_EQ(second_root->children()[0].get(), child_root.get());
  EXPECT_EQ(first_parent->Serialize()
                .GetProperty("childSlots")
                .GetProperty(0)
                .GetLength(),
            0);
  EXPECT_EQ(second_parent->Serialize()
                .GetProperty("childSlots")
                .GetProperty(0)
                .GetProperty(0)
                .GetProperty("uid")
                .Number(),
            1);
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
  destination->SetRootAttributes(lepus::Value(destination_attributes));
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
       ElementTemplateDirectCrossParentMoveSurvivesScopedFlushes) {
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

  destination_root->FlushActionsAsRoot();
  EXPECT_EQ(child_root->parent(), destination_root.get());

  source->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());
  EXPECT_EQ(child_root->parent(), source_root.get());
  EXPECT_EQ(source_root->children().size(), 1u);
  EXPECT_TRUE(destination_root->children().empty());
  EXPECT_EQ(source->Serialize()
                .GetProperty("childSlots")
                .GetProperty(0)
                .GetProperty(0)
                .GetProperty("uid")
                .Number(),
            1);
  EXPECT_EQ(destination->Serialize()
                .GetProperty("childSlots")
                .GetProperty(0)
                .GetLength(),
            0);

  source_root->FlushActionsAsRoot();
  page_root->FlushActionsAsRoot();

  ASSERT_EQ(source_root->children().size(), 1u);
  EXPECT_EQ(source_root->children()[0].get(), child_root.get());
  EXPECT_TRUE(destination_root->children().empty());
  EXPECT_EQ(child_root->parent(), source_root.get());
  EXPECT_FALSE(source->HasPendingChildMounts());
  EXPECT_FALSE(destination->HasPendingChildMounts());

  if (ENABLE_INSPECTOR) {
    ASSERT_EQ(observer->removed_nodes.size(), 2u);
    ASSERT_EQ(observer->removed_node_parents.size(), 2u);
    EXPECT_EQ(observer->removed_nodes[0], child_root.get());
    EXPECT_EQ(observer->removed_nodes[1], child_root.get());
    EXPECT_EQ(observer->removed_node_parents[0], source_root.get());
    EXPECT_EQ(observer->removed_node_parents[1], destination_root.get());
    ASSERT_EQ(observer->added_nodes.size(), 2u);
    ASSERT_EQ(observer->added_node_parents.size(), 2u);
    EXPECT_EQ(observer->added_nodes[0], child_root.get());
    EXPECT_EQ(observer->added_nodes[1], child_root.get());
    EXPECT_EQ(observer->added_node_parents[0], destination_root.get());
    EXPECT_EQ(observer->added_node_parents[1], source_root.get());
  }
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateInitialMaterializationNotifiesMountedChildRemoval) {
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

  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->SetUid(lepus::Value(0));
  page->InsertNodeIntoChildSlot(0, lepus::Value(source), lepus::Value());

  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);
  auto source_root = source->PeekMaterializedRoot();
  auto child_root = child->PeekMaterializedRoot();
  ASSERT_NE(source_root, nullptr);
  ASSERT_NE(child_root, nullptr);
  ASSERT_EQ(source_root->children().size(), 1u);
  EXPECT_EQ(source_root->children()[0].get(), child_root.get());

  auto destination = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  destination->SetTypedTag(base::String("view"));
  destination->SetUid(lepus::Value(8));

  observer->removed_nodes.clear();
  observer->removed_node_parents.clear();
  observer->added_nodes.clear();
  observer->added_node_parents.clear();

  // Inserting the typed destination materializes it immediately; moving the
  // existing child then emits the ordinary remove/add sequence.
  page->InsertNodeIntoChildSlot(0, lepus::Value(destination), lepus::Value());
  destination->InsertNodeIntoChildSlot(0, lepus::Value(child), lepus::Value());

  auto destination_root = destination->PeekMaterializedRoot();
  ASSERT_NE(destination_root, nullptr);
  EXPECT_TRUE(source_root->children().empty());
  ASSERT_EQ(destination_root->children().size(), 1u);
  EXPECT_EQ(destination_root->children()[0].get(), child_root.get());
  EXPECT_EQ(child_root->parent(), destination_root.get());

  if (ENABLE_INSPECTOR) {
    ASSERT_EQ(observer->removed_nodes.size(), 1u);
    EXPECT_EQ(observer->removed_nodes[0], child_root.get());
    EXPECT_EQ(observer->removed_node_parents[0], source_root.get());
    ASSERT_EQ(observer->added_nodes.size(), 2u);
    ASSERT_EQ(observer->added_node_parents.size(), 2u);
    EXPECT_EQ(observer->added_nodes[0], destination_root.get());
    EXPECT_EQ(observer->added_nodes[1], child_root.get());
    EXPECT_EQ(observer->added_node_parents[0], page_root.get());
    EXPECT_EQ(observer->added_node_parents[1], destination_root.get());
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
       ElementTemplatePreRootMutationsMaterializeFinalState) {
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
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("data-test"),
                    lepus::Value(), 0},
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("bindtap"),
                    lepus::Value(), 1}});

  auto slot_info = ElementInfo();
  slot_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_SLOT;
  slot_info.slot_index_ = 0;
  target_info.children_.emplace_back(std::move(slot_info));

  auto sentinel_info = ElementInfo();
  sentinel_info.tag_enum_ = ElementBuiltInTagEnum::ELEMENT_VIEW;
  sentinel_info.attrs_[base::String("id")] = lepus::Value("sentinel");
  target_info.children_.emplace_back(std::move(sentinel_info));
  template_info->elements_.emplace_back(std::move(target_info));
  default_entry->template_bundle_.element_template_infos_["root_template"] =
      std::move(template_info);

  auto attribute_slots = lepus::CArray::Create();
  attribute_slots->emplace_back(lepus::Value("old_value"));
  attribute_slots->emplace_back(lepus::Value("onTap"));

  auto first = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  first->SetTypedTag(base::String("raw-text"));
  first->SetUid(lepus::Value(9));
  auto second = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  second->SetTypedTag(base::String("raw-text"));
  second->SetUid(lepus::Value(10));

  auto slot_children = lepus::CArray::Create();
  slot_children->emplace_back(lepus::Value(first));
  auto child_slots = lepus::CArray::Create();
  child_slots->emplace_back(lepus::Value(slot_children));

  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->SetTASM(tasm.get());
  root->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  root->SetTemplateKey(base::String("root_template"));
  root->SetAttributeSlots(lepus::Value(attribute_slots));
  root->InitializeChildSlots(lepus::Value(child_slots));

  root->SetAttributeSlot(0, lepus::Value("new_value"));
  root->InsertNodeIntoChildSlot(0, lepus::Value(second), lepus::Value(first));
  root->RemoveNodeFromChildSlot(0, lepus::Value(first));
  EXPECT_EQ(root->PeekMaterializedRoot(), nullptr);

  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->SetUid(lepus::Value(0));
  page->InsertNodeIntoChildSlot(0, lepus::Value(root), lepus::Value());
  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);
  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page_root.get());

  auto resolved = root->PeekMaterializedRoot();
  ASSERT_NE(resolved, nullptr);
  auto* test_data = DatasetValue(resolved.get(), "test");
  ASSERT_NE(test_data, nullptr);
  EXPECT_EQ(test_data->StdString(), "new_value");
  EXPECT_EQ(resolved->data_model_->attributes().count("data-test"), 0u);
  auto* listeners = resolved->GetEventListenerMap()->Find("tap");
  ASSERT_NE(listeners, nullptr);
  ASSERT_EQ(listeners->size(), 1u);
  auto second_root = second->PeekMaterializedRoot();
  ASSERT_NE(second_root, nullptr);
  ASSERT_EQ(resolved->children().size(), 2u);
  EXPECT_EQ(resolved->children()[0].get(), second_root.get());
  EXPECT_EQ(first->PeekMaterializedRoot(), nullptr);
}

TEST_P(ElementTemplateInstanceTest,
       ElementTemplateStaticEventsSyncAfterAttach) {
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
       ElementTemplateInitialRootAttributesSplitAfterPrepare) {
  manager->config_->SetEnableEventHandleRefactor(true);
  manager->SetConfig(manager->config_);
  tasm->page_config_ = manager->config_;

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
                    lepus::Value(), 0},
          Attribute{ATTRIBUTE_BINDING_TYPE_DYNAMIC, base::String("bindtap"),
                    lepus::Value(), 1},
          Attribute{ATTRIBUTE_BINDING_TYPE_STATIC, base::String("bindblur"),
                    lepus::Value("onCompiledBlur"), 0}});
  template_info->elements_.emplace_back(std::move(root_info));
  default_entry->template_bundle_.element_template_infos_["root_template"] =
      std::move(template_info);

  auto attribute_slots = lepus::CArray::Create();
  attribute_slots->emplace_back(lepus::Value("compiled-value"));
  attribute_slots->emplace_back(lepus::Value("onTap"));

  auto old_root_attributes = lepus::Dictionary::Create();
  old_root_attributes->SetValue("data-root", lepus::Value("old-root"));
  old_root_attributes->SetValue("data-old", lepus::Value("old-value"));
  old_root_attributes->SetValue("bindfocus", lepus::Value("onOldFocus"));
  old_root_attributes->SetValue("bindblur", lepus::Value("onOldBlur"));

  auto root = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  root->SetTASM(tasm.get());
  root->SetBundleUrl(base::String(DEFAULT_ENTRY_NAME));
  root->SetTemplateKey(base::String("root_template"));
  root->SetAttributeSlots(lepus::Value(attribute_slots));
  root->SetRootAttributes(lepus::Value(old_root_attributes));

  auto page = fml::AdoptRef<ElementTemplateInstance>(
      new ElementTemplateInstance(manager));
  page->SetTASM(tasm.get());
  page->SetTypedTag(base::String("page"));
  page->SetUid(lepus::Value(0));
  page->InsertNodeIntoChildSlot(0, lepus::Value(root), lepus::Value());
  auto page_root = page->GetRoot();
  ASSERT_NE(page_root, nullptr);
  EXPECT_EQ(root->PeekMaterializedRoot(), nullptr);

  auto new_root_attributes = lepus::Dictionary::Create();
  new_root_attributes->SetValue("data-root", lepus::Value("new-root"));
  new_root_attributes->SetValue("bindfocus", lepus::Value("onNewFocus"));
  root->SetRootAttributes(lepus::Value(new_root_attributes));

  auto options = std::make_shared<PipelineOptions>();
  manager->OnPatchFinish(options, page_root.get());
  auto resolved = root->PeekMaterializedRoot();
  ASSERT_NE(resolved, nullptr);

  auto* compiled_data = DatasetValue(resolved.get(), "test");
  ASSERT_NE(compiled_data, nullptr);
  EXPECT_EQ(compiled_data->StdString(), "compiled-value");
  auto* root_data = DatasetValue(resolved.get(), "root");
  ASSERT_NE(root_data, nullptr);
  EXPECT_EQ(root_data->StdString(), "new-root");
  EXPECT_EQ(resolved->data_model_->dataset().count("old"), 0u);

  auto* tap_listeners = resolved->GetEventListenerMap()->Find("tap");
  ASSERT_NE(tap_listeners, nullptr);
  ASSERT_EQ(tap_listeners->size(), 1u);
  auto* focus_listeners = resolved->GetEventListenerMap()->Find("focus");
  ASSERT_NE(focus_listeners, nullptr);
  ASSERT_EQ(focus_listeners->size(), 1u);
  auto focus_iter = resolved->event_map().find("focus");
  ASSERT_NE(focus_iter, resolved->event_map().end());
  EXPECT_EQ(focus_iter->second->function(), "onNewFocus");
  auto blur_iter = resolved->event_map().find("blur");
  ASSERT_NE(blur_iter, resolved->event_map().end());
  EXPECT_EQ(blur_iter->second->function(), "onCompiledBlur");

  root->SetAttributeSlot(0, lepus::Value("updated-compiled-value"));
  EXPECT_FALSE(root->HasPendingChildMounts());
  compiled_data = DatasetValue(resolved.get(), "test");
  ASSERT_NE(compiled_data, nullptr);
  EXPECT_EQ(compiled_data->StdString(), "updated-compiled-value");
}

INSTANTIATE_TEST_SUITE_P(ElementTemplateInstanceTestModule,
                         ElementTemplateInstanceTest,
                         ::testing::ValuesIn(fiber_element_generation_params));

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
