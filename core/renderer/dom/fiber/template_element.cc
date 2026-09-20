// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/template_element.h"

#include <functional>
#include <memory>
#include <utility>

#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/dom/fiber/tree_resolver.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/renderer/utils/value_utils.h"

namespace lynx {
namespace tasm {
namespace {

static constexpr const char kTemplateTag[] = "template";
static constexpr const char kTemplateRootAttributeSpread[] = "rootAttributes";
static constexpr const char kDefaultPageComponentId[] = "0";
static constexpr int32_t kDefaultPageCSSId = 0;
static constexpr uint32_t kTypedTemplateRootSlotIndex = 0;

fml::RefPtr<Element> ResolveInitialElementSlotChild(const lepus::Value& child) {
  if (!child.IsRefCounted()) {
    return nullptr;
  }

  auto ref_counted = child.RefCounted();
  if (ref_counted->GetRefType() != lepus::RefType::kElement) {
    return nullptr;
  }

  return fml::static_ref_ptr_cast<Element>(ref_counted);
}

lepus::Value CopyTemplateValueForStorage(const lepus::Value& value) {
  // Clone would deep-convert JS functions to empty values. Keep callables by
  // reference, while preserving snapshot semantics for ordinary attributes.
  return value.IsCallable() ? value : lepus::Value::Clone(value);
}

lepus::Value CopyTemplateObjectForStorage(const lepus::Value& value) {
  if (!value.IsObject()) {
    return lepus::Value();
  }

  auto object = lepus::Dictionary::Create();
  lepus::Value::ForEachLepusValue(
      value, [&object](const lepus::Value& key, const lepus::Value& item) {
        if (!key.IsString()) {
          return;
        }
        object->SetValue(key.String(), CopyTemplateValueForStorage(item));
      });
  return lepus::Value(std::move(object));
}

lepus::Value CreateRootAttributeSlots(const lepus::Value& root_attributes) {
  auto attribute_slots = lepus::CArray::Create();
  attribute_slots->emplace_back(
      root_attributes.IsObject() ? CopyTemplateObjectForStorage(root_attributes)
                                 : lepus::Value());
  return lepus::Value(std::move(attribute_slots));
}

SharedTemplateAttributes CreateRootSpreadTemplateAttributes();

template <typename Apply>
void ApplyRootTemplateAttributes(Element* root, Apply&& apply) {
  if (root == nullptr) {
    return;
  }
  auto compiled_attributes = root->template_attributes();
  root->SetTemplateAttributes(CreateRootSpreadTemplateAttributes());
  apply();
  root->SetTemplateAttributes(compiled_attributes);
}

void ApplyRootTemplateAttributes(Element* root,
                                 const lepus::Value& previous_root_attributes,
                                 const lepus::Value& root_attributes) {
  if (!previous_root_attributes.IsObject() && !root_attributes.IsObject()) {
    return;
  }
  ApplyRootTemplateAttributes(
      root, [root, &previous_root_attributes, &root_attributes]() {
        if (previous_root_attributes.IsObject()) {
          TreeResolver::ApplyTemplateAttributesToElement(
              root, CreateRootAttributeSlots(previous_root_attributes),
              CreateRootAttributeSlots(root_attributes));
          return;
        }
        TreeResolver::ApplyTemplateAttributesToElement(
            root, CreateRootAttributeSlots(root_attributes));
      });
}

SharedTemplateAttributes CreateRootSpreadTemplateAttributes() {
  return std::make_shared<const TemplateAttributes>(TemplateAttributes{
      Attribute{ATTRIBUTE_BINDING_TYPE_SPREAD,
                BASE_STATIC_STRING(kTemplateRootAttributeSpread),
                lepus::Value(), kTypedTemplateRootSlotIndex}});
}

fml::RefPtr<Element> CreateTypedRootElement(ElementManager* manager,
                                            TemplateAssembler* tasm,
                                            const base::String& tag) {
  if (tag.IsEqual(kElementPageTag)) {
    auto page = manager->CreateFiberPage(
        BASE_STATIC_STRING(kDefaultPageComponentId), kDefaultPageCSSId);
    if (tasm != nullptr) {
      page->set_style_sheet_manager(
          tasm->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME));
    }
    return page;
  }
  if (tag.IsEqual(kElementListTag)) {
    return manager->CreateFiberList(tasm, tag, lepus::Value(), lepus::Value(),
                                    lepus::Value());
  }
  return manager->CreateFiberElement(tag);
}

void PrepareInitialElementSlots(GeneratedElementsResult* generated,
                                const lepus::Value& element_slots) {
  if (!element_slots.IsArrayOrJSArray()) {
    return;
  }

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

}  // namespace

TemplateElement::TemplateElement(ElementManager* element_manager)
    : Element(element_manager, BASE_STATIC_STRING(kTemplateTag)) {
  MarkTemplateElement();
}

TemplateElement::~TemplateElement() = default;

void TemplateElement::SetTypedTag(const base::String& typed_tag) {
  typed_tag_ = typed_tag;
}

void TemplateElement::SetRootAttributes(const lepus::Value& attributes) {
  if (!attributes.IsObject() && !attributes.IsNil() &&
      !attributes.IsUndefined()) {
    return;
  }
  auto previous_root_attributes = root_attributes_;
  root_attributes_ = attributes.IsObject()
                         ? CopyTemplateObjectForStorage(attributes)
                         : lepus::Value();
  ApplyRootAttributes(previous_root_attributes);
}

void TemplateElement::SetElementSlots(const lepus::Value& element_slots) {
  element_slots_ = element_slots;
}

void TemplateElement::ResolveGeneratedElements() {
  if (IsActiveMaterialized()) {
    return;
  }

  InitTypedRoot();
  if (result_ == nullptr) {
    return;
  }
  GeneratedElementsResult generated;
  PrepareInitialElementSlots(&generated, element_slots_);
  prepared_element_slot_insertions_ =
      std::move(generated.prepared_element_slot_insertions_);
  ApplyRootAttributes(lepus::Value());
  ApplyInitialElementSlots();
}

void TemplateElement::InitTypedRoot() {
  if (!IsTypedTemplate() || result_ != nullptr) {
    return;
  }

  auto* manager = element_manager();
  if (manager == nullptr) {
    return;
  }

  result_ = CreateTypedRootElement(manager, tasm_, typed_tag_);
  if (result_ == nullptr) {
    return;
  }
  result_->MarkTemplateElement();
  // Element Template is currently only used by RL3 in page scope. Typed roots
  // are created outside the normal FiberCreate* APIs, so seed their component
  // scope from the page root before class style resolution.
  auto* root = manager->root();
  if (root != nullptr) {
    result_->SetParentComponentUniqueIdRecursively(root->impl_id());
  }

  element_slot_targets_.clear();
  element_slot_targets_.push_back(ElementSlotMountPoint{result_, nullptr});
}

void TemplateElement::ApplyRootAttributes(
    const lepus::Value& previous_root_attributes) {
  if (!IsActiveMaterialized() ||
      (!previous_root_attributes.IsObject() && !root_attributes_.IsObject())) {
    return;
  }

  ApplyRootTemplateAttributes(result_.get(), previous_root_attributes,
                              root_attributes_);
}

void TemplateElement::ApplyInitialElementSlots() {
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
    const fml::RefPtr<Element>& child) {
  if (mount_point.parent_ == nullptr || child == nullptr) {
    return;
  }
  if (mount_point.ref_node_ != nullptr) {
    mount_point.parent_->InsertNodeBefore(child, mount_point.ref_node_);
  } else {
    mount_point.parent_->InsertNode(child);
  }
}

fml::RefPtr<Element> TemplateElement::GetRoot() {
  ResolveGeneratedElements();

  EXEC_EXPR_FOR_INSPECTOR(
      auto* manager = element_manager();
      if (result_ != nullptr && manager != nullptr &&
          manager->GetDevToolFlag() && manager->IsDomTreeEnabled()) {
        std::function<void(Element*)> prepare_node_f =
            [manager, &prepare_node_f](Element* element) {
              manager->PrepareNodeForInspector(element);
              for (const auto& child : element->children()) {
                prepare_node_f(child.get());
              }
            };
        prepare_node_f(result_.get());
      });
  return result_;
}

}  // namespace tasm
}  // namespace lynx
