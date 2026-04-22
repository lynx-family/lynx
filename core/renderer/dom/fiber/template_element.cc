// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/template_element.h"

#include <functional>
#include <future>
#include <utility>

#include "base/include/log/logging.h"
#include "base/include/value/base_value.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/tree_resolver.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/template_entry.h"
#include "core/renderer/trace/renderer_trace_event_def.h"

namespace lynx {
namespace tasm {
namespace {

// These keys define the serialized Element Template payload consumed by
// FiberSerializeElementTemplate / FiberCreateElementTemplate.
static constexpr const char kTemplateTag[] = "template";
static constexpr const char kDefaultTemplateBundleUrl[] = "__Card__";
static constexpr const char kTemplateKey[] = "templateKey";
static constexpr const char kTemplateBundleUrl[] = "bundleUrl";
static constexpr const char kTemplateAttributeSlots[] = "attributeSlots";
static constexpr const char kTemplateElementSlots[] = "elementSlots";
static constexpr const char kTemplateUid[] = "uid";

fml::RefPtr<FiberElement> ResolveInitialElementSlotChild(
    const lepus::Value& child) {
  if (!child.IsRefCounted()) {
    return nullptr;
  }

  auto ref_counted = child.RefCounted();
  if (ref_counted->GetRefType() != lepus::RefType::kElement) {
    return nullptr;
  }

  return fml::static_ref_ptr_cast<FiberElement>(ref_counted);
}

void PrepareGeneratedElementsResult(GeneratedElementsResult* generated,
                                    const lepus::Value& attribute_slots,
                                    const lepus::Value& element_slots) {
  if (generated == nullptr) {
    return;
  }
  FiberElement* previous_element = nullptr;
  // Attribute slot values can be applied before the generated tree is attached.
  for (const auto& target : generated->attribute_slot_targets_) {
    auto* element = target.get();
    if (element == nullptr || element == previous_element) {
      continue;
    }
    TreeResolver::ApplyTemplateAttributesToElement(element, attribute_slots);
    previous_element = element;
  }

  if (!element_slots.IsArrayOrJSArray()) {
    return;
  }

  // Resolve slot children early, but defer insertion until GetRoot consumes the
  // prepared tree on the main render path.
  for (size_t slot_index = 0;
       slot_index < static_cast<size_t>(element_slots.GetLength());
       ++slot_index) {
    auto slot_children =
        element_slots.GetProperty(static_cast<uint32_t>(slot_index));
    if (!slot_children.IsArrayOrJSArray()) {
      continue;
    }

    for (size_t child_index = 0;
         child_index < static_cast<size_t>(slot_children.GetLength());
         ++child_index) {
      auto child = ResolveInitialElementSlotChild(
          slot_children.GetProperty(static_cast<uint32_t>(child_index)));
      if (child == nullptr) {
        continue;
      }
      PreparedElementSlotInsertion insertion;
      insertion.slot_index_ = static_cast<uint32_t>(slot_index);
      insertion.child_ = std::move(child);
      generated->prepared_element_slot_insertions_.push_back(
          std::move(insertion));
    }
  }
}

GeneratedElementsResult GeneratePreparedElementsResult(
    TemplateEntry* entry, const base::String& template_key,
    const lepus::Value& attribute_slots, const lepus::Value& element_slots) {
  GeneratedElementsResult generated;
  if (entry != nullptr) {
    auto& info = entry->GetElementTemplateInfo(template_key.str());
    generated = TreeResolver::GenerateElementsFromTemplateInfo(info);
  }
  PrepareGeneratedElementsResult(&generated, attribute_slots, element_slots);
  return generated;
}

}  // namespace

TemplateElement::TemplateElement(ElementManager* element_manager)
    : FiberElement(element_manager, BASE_STATIC_STRING(kTemplateTag)),
      bundle_url_(BASE_STATIC_STRING(kDefaultTemplateBundleUrl)) {
  MarkTemplateElement();
}

TemplateElement::~TemplateElement() = default;

void TemplateElement::PrepareAsyncCreateElementTree() {
  if (result_ != nullptr || async_create_task_ != nullptr) {
    return;
  }
  auto* manager = element_manager();
  if (manager == nullptr) {
    return;
  }

  if (entry_ == nullptr && tasm_ != nullptr) {
    entry_ = tasm_->FindEntry(bundle_url_.str()).get();
  }

  async_create_task_ = CreateAsyncCreateElementTreeTask(entry_);
  manager->EnqueuePostMTSRenderTask(
      base::closure([task = async_create_task_]() { task->Run(); }));
}

base::OnceTaskRefptr<GeneratedElementsResult>
TemplateElement::CreateAsyncCreateElementTreeTask(TemplateEntry* entry) {
  std::promise<GeneratedElementsResult> promise;
  auto future = promise.get_future();
  auto template_key = template_key_;
  auto attribute_slots = attribute_slots_;
  auto element_slots = element_slots_;
  return fml::MakeRefCounted<base::OnceTask<GeneratedElementsResult>>(
      [entry, template_key = std::move(template_key),
       attribute_slots = std::move(attribute_slots),
       element_slots = std::move(element_slots),
       promise = std::move(promise)]() mutable {
        promise.set_value(GeneratePreparedElementsResult(
            entry, template_key, attribute_slots, element_slots));
      },
      std::move(future));
}

void TemplateElement::ResolveGeneratedElements() {
  if (result_ != nullptr) {
    return;
  }

  if (async_create_task_ == nullptr) {
    PrepareAsyncCreateElementTree();
    if (async_create_task_ == nullptr) {
      return;
    }
  }

  async_create_task_->Run();
  auto generated = async_create_task_->GetFuture().get();
  async_create_task_ = nullptr;
  result_ = std::move(generated.result_);
  attribute_slot_targets_ = std::move(generated.attribute_slot_targets_);
  element_slot_targets_ = std::move(generated.element_slot_targets_);
  prepared_element_slot_insertions_ =
      std::move(generated.prepared_element_slot_insertions_);

  // Attach generated elements and mount slot children only when the template is
  // actually materialized into the Fiber tree.
  InitGeneratedElementTree();
  ApplyInitialElementSlots();
}

void TemplateElement::InitGeneratedElementTree() {
  auto* manager = element_manager();
  if (result_ == nullptr || manager == nullptr || entry_ == nullptr) {
    return;
  }
  auto* root = manager->root();
  TreeResolver::InitElementTree(result_, root != nullptr ? root->impl_id() : -1,
                                manager, entry_->GetStyleSheetManager());
}

void TemplateElement::ApplyInitialElementSlots() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_ELEMENT_APPLY_INITIAL_ELEMENT_SLOTS,
              "template_key", template_key_.str(), "bundle_url",
              bundle_url_.str());
  for (const auto& insertion : prepared_element_slot_insertions_) {
    auto slot_index = static_cast<size_t>(insertion.slot_index_);
    if (slot_index >= element_slot_targets_.size()) {
      continue;
    }
    const auto& mount_point = element_slot_targets_[slot_index];
    if (mount_point.parent_ == nullptr || insertion.child_ == nullptr) {
      continue;
    }
    InsertInitialElementSlotChild(mount_point, insertion.child_);
  }
}

void TemplateElement::InsertInitialElementSlotChild(
    const ElementSlotMountPoint& mount_point,
    const fml::RefPtr<FiberElement>& child) {
  if (mount_point.parent_ == nullptr || child == nullptr) {
    return;
  }
  if (mount_point.ref_node_ != nullptr) {
    mount_point.parent_->InsertNodeBefore(child, mount_point.ref_node_);
  } else {
    mount_point.parent_->InsertNode(child);
  }
}

lepus::Value TemplateElement::Serialize() const {
  auto serialized = lepus::Dictionary::Create();
  serialized->SetValue(BASE_STATIC_STRING(kTemplateKey), template_key_);
  serialized->SetValue(BASE_STATIC_STRING(kTemplateBundleUrl), bundle_url_);
  serialized->SetValue(BASE_STATIC_STRING(kTemplateAttributeSlots),
                       attribute_slots_);
  serialized->SetValue(BASE_STATIC_STRING(kTemplateElementSlots),
                       SerializeElementSlots());
  serialized->SetValue(BASE_STATIC_STRING(kTemplateUid), uid_);
  return lepus::Value(std::move(serialized));
}

lepus::Value TemplateElement::SerializeElementSlots() const {
  if (!element_slots_.IsArrayOrJSArray()) {
    return element_slots_;
  }

  auto serialized_slots = lepus::CArray::Create();
  serialized_slots->reserve(element_slots_.GetLength());
  for (size_t slot_index = 0;
       slot_index < static_cast<size_t>(element_slots_.GetLength());
       ++slot_index) {
    serialized_slots->emplace_back(SerializeElementSlotChildren(
        element_slots_.GetProperty(static_cast<uint32_t>(slot_index))));
  }
  return lepus::Value(std::move(serialized_slots));
}

lepus::Value TemplateElement::SerializeElementSlotChildren(
    const lepus::Value& slot_children) const {
  if (!slot_children.IsArrayOrJSArray()) {
    return slot_children;
  }

  auto serialized_children = lepus::CArray::Create();
  serialized_children->reserve(slot_children.GetLength());
  for (size_t child_index = 0;
       child_index < static_cast<size_t>(slot_children.GetLength());
       ++child_index) {
    auto serialized_child = SerializeElementSlotChild(
        slot_children.GetProperty(static_cast<uint32_t>(child_index)));
    if (!serialized_child.IsEmpty() && !serialized_child.IsUndefined()) {
      serialized_children->emplace_back(std::move(serialized_child));
    }
  }
  return lepus::Value(std::move(serialized_children));
}

lepus::Value TemplateElement::SerializeElementSlotChild(
    const lepus::Value& child) const {
  if (!child.IsRefCounted()) {
    LOGE(
        "SerializeElementTemplate only supports TemplateElement children in "
        "elementSlots, but got non-refcounted child.");
    return lepus::Value();
  }

  auto ref_counted = child.RefCounted();
  if (ref_counted->GetRefType() != lepus::RefType::kElement) {
    LOGE(
        "SerializeElementTemplate only supports TemplateElement children in "
        "elementSlots.");
    return lepus::Value();
  }

  auto element =
      fml::static_ref_ptr_cast<FiberElement>(ref_counted).strongify();
  if (element == nullptr || !element->is_template()) {
    LOGE(
        "SerializeElementTemplate only supports TemplateElement children in "
        "elementSlots.");
    return lepus::Value();
  }
  auto template_element = fml::static_ref_ptr_cast<TemplateElement>(element);
  return template_element->Serialize();
}

fml::RefPtr<FiberElement> TemplateElement::GetRoot() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_ELEMENT_GET_ROOT, "template_key",
              template_key_.str(), "bundle_url", bundle_url_.str());
  ResolveGeneratedElements();

  EXEC_EXPR_FOR_INSPECTOR(
      auto* manager = element_manager();
      if (result_ != nullptr && manager != nullptr &&
          manager->GetDevToolFlag() && manager->IsDomTreeEnabled()) {
        std::function<void(FiberElement*)> prepare_node_f =
            [manager, &prepare_node_f](FiberElement* element) {
              manager->PrepareNodeForInspector(element);
              for (const auto& child : element->children()) {
                prepare_node_f(static_cast<FiberElement*>(child.get()));
              }
            };
        prepare_node_f(result_.get());
      });
  return result_;
}

}  // namespace tasm
}  // namespace lynx
