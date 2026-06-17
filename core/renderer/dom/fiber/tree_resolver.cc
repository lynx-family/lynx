// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/tree_resolver.h"

#include <algorithm>
#include <cstdint>
#include <string_view>

#include "base/include/log/logging.h"
#include "base/include/string/string_utils.h"
#include "core/base/thread/once_task.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/component_element.h"
#include "core/renderer/dom/fiber/fiber_element.h"
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
#include "core/renderer/events/events.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/trace/renderer_trace_event_def.h"
#include "core/renderer/utils/base/tasm_constants.h"

namespace lynx {
namespace tasm {

namespace {

constexpr const char* kElementStyle = "style";
constexpr const char* kElementId = "id";
constexpr std::string_view kBindEventPrefix = "bind";
constexpr std::string_view kCatchEventPrefix = "catch";
constexpr std::string_view kCaptureBindEventPrefix = "capture-bind";
constexpr std::string_view kCaptureCatchEventPrefix = "capture-catch";
constexpr std::string_view kGlobalBindEventPrefix = "global-bind";
constexpr std::string_view kDataAttributePrefix = "data-";
// Element Template dynamic/spread attributes intentionally parse native event
// prefixes that can appear in runtime input. Non-event main-thread:* keys fall
// through to normal attribute handling.
constexpr std::string_view kMainThreadEventPrefix = "main-thread:";

enum class TemplateAttributeApplyMode {
  kAll,
  kNonEventOnly,
  kEventOnly,
};

bool StartsWith(std::string_view value, std::string_view prefix) {
  return value.size() >= prefix.size() &&
         value.compare(0, prefix.size(), prefix) == 0;
}

bool ParseTemplateEventAttribute(std::string_view key, base::String* type,
                                 base::String* name) {
  if (StartsWith(key, kMainThreadEventPrefix)) {
    key.remove_prefix(kMainThreadEventPrefix.size());
  }

  auto parse_with_prefix = [key, type, name](std::string_view prefix,
                                             const char* event_type) {
    if (!StartsWith(key, prefix) || key.size() <= prefix.size()) {
      return false;
    }
    auto event_name = key.substr(prefix.size());
    *type = base::String(event_type);
    *name = base::String(event_name.data(), event_name.size());
    return true;
  };

  if (parse_with_prefix(kCaptureCatchEventPrefix, kEventCaptureCatch) ||
      parse_with_prefix(kCaptureBindEventPrefix, kEventCaptureBind) ||
      parse_with_prefix(kGlobalBindEventPrefix, kEventGlobalBind) ||
      parse_with_prefix(kCatchEventPrefix, kEventCatchEvent) ||
      parse_with_prefix(kBindEventPrefix, kEventBindEvent)) {
    return true;
  }

  return false;
}

bool ParseTemplateDataAttribute(std::string_view key, base::String* name) {
  if (!StartsWith(key, kDataAttributePrefix) ||
      key.size() <= kDataAttributePrefix.size()) {
    return false;
  }

  auto data_name = key.substr(kDataAttributePrefix.size());
  *name = base::String(data_name.data(), data_name.size());
  return true;
}

bool IsTemplateEventAttribute(const base::String& key) {
  base::String event_type;
  base::String event_name;
  return ParseTemplateEventAttribute(key.string_view(), &event_type,
                                     &event_name);
}

void RegisterSlotTarget(base::Vector<fml::RefPtr<FiberElement>>* targets,
                        int32_t slot_index,
                        const fml::RefPtr<FiberElement>& element) {
  if (targets == nullptr || slot_index < 0 || element == nullptr) {
    return;
  }
  auto target_index = static_cast<size_t>(slot_index);
  if (targets->size() <= target_index) {
    targets->resize(target_index + 1);
  }
  auto& target = (*targets)[target_index];
  DCHECK(target == nullptr || target.get() == element.get());
  if (target == nullptr) {
    target = element;
  }
}

void RegisterElementSlotMountPoint(base::Vector<ElementSlotMountPoint>* targets,
                                   int32_t slot_index,
                                   const fml::RefPtr<FiberElement>& parent,
                                   const fml::RefPtr<FiberElement>& ref_node) {
  if (targets == nullptr || slot_index < 0 || parent == nullptr) {
    return;
  }
  auto target_index = static_cast<size_t>(slot_index);
  if (targets->size() <= target_index) {
    targets->resize(target_index + 1);
  }
  auto& target = (*targets)[target_index];
  DCHECK(target.parent_ == nullptr || target.parent_.get() == parent.get());
  DCHECK(target.ref_node_ == nullptr || ref_node == nullptr ||
         target.ref_node_.get() == ref_node.get());
  if (target.parent_ == nullptr) {
    target.parent_ = parent;
    target.ref_node_ = ref_node;
  }
}

// TODO(songshourui.null): Unify this class application path with Render
// Functions when both paths can share the same attribute setter.
void ApplyTemplateClassAttribute(FiberElement* element,
                                 const lepus::Value& value) {
  element->RemoveAllClass();
  if (!value.IsString()) {
    return;
  }

  ClassList classes;
  base::SplitString(value.String().str(), ' ', true,
                    [&classes](const char* s, size_t length, int index) {
                      classes.emplace_back(s, length);
                      return true;
                    });
  if (!classes.empty()) {
    element->SetClasses(std::move(classes));
  }
}

// TODO(songshourui.null): Unify this style application path with Render
// Functions when both paths can share the same attribute setter.
void ApplyTemplateStyleAttribute(FiberElement* element,
                                 const lepus::Value& value) {
  element->RemoveAllInlineStyles();
  if (value.IsString()) {
    element->SetRawInlineStyles(base::String(value.String()));
    return;
  }
  if (!value.IsObject()) {
    return;
  }

  if (element->IsCSSInlineVariablesEnabled()) {
    element->data_model()->MoveAndClearCSSInlineVariables(nullptr);
  }
  tasm::ForEachLepusValue(value, [element](const lepus::Value& key,
                                           const lepus::Value& item_value) {
    auto key_string_view = key.StringView();
    if (CSSProperty::IsCustomProperty(
            key_string_view.data(),
            static_cast<uint32_t>(key_string_view.length()))) {
      if (element->IsCSSInlineVariablesEnabled()) {
        element->data_model()->UpdateCSSInlineVariables(
            key.String(), item_value.IsString()
                              ? item_value.String()
                              : base::String(item_value.ToString()));
      }
      return;
    }
    auto id =
        CSSProperty::GetPropertyID(base::CamelCaseToDashCase(key_string_view));
    if (CSSProperty::IsPropertyValid(id)) {
      element->SetStyle(id, item_value);
    }
  });
}

// TODO(songshourui.null): Unify this dispatch with Render Functions
// SetAttribute once it supports special attributes through the shared setter.
bool ApplySpecialTemplateAttribute(FiberElement* element,
                                   const base::String& key,
                                   const lepus::Value& value) {
  if (key == kElementClass) {
    ApplyTemplateClassAttribute(element, value);
    return true;
  }
  if (key == kElementStyle) {
    ApplyTemplateStyleAttribute(element, value);
    return true;
  }
  if (key == kElementId) {
    element->SetIdSelector(value.IsString() ? value.String() : base::String());
    return true;
  }

  if (key == kElementAttrCSSID) {
    element->SetCSSID(value.IsNumber() ? static_cast<int32_t>(value.Number())
                                       : kInvalidCssId);
    return true;
  }
  return false;
}

bool ApplyTemplateEventAttribute(FiberElement* element, const base::String& key,
                                 const lepus::Value& value) {
  base::String event_type;
  base::String event_name;
  if (!ParseTemplateEventAttribute(key.string_view(), &event_type,
                                   &event_name)) {
    return false;
  }

  // Element Template is only used by RL3, whose template event handlers are
  // always registered under the default entry context.
  element->FiberAddEvent(event_type, event_name, value, DEFAULT_ENTRY_NAME);
  return true;
}

bool ApplyTemplateListCallbackAttribute(FiberElement* element,
                                        const base::String& key,
                                        const lepus::Value& value) {
  if (!ListElement::IsTemplateCallbackAttribute(key)) {
    return false;
  }
  if (element != nullptr && element->is_list()) {
    static_cast<ListElement*>(element)->ApplyTemplateCallbackAttribute(key,
                                                                       value);
  }
  return true;
}

bool ApplyTemplateDataAttribute(FiberElement* element, const base::String& key,
                                const lepus::Value& value) {
  base::String data_name;
  if (!ParseTemplateDataAttribute(key.string_view(), &data_name)) {
    return false;
  }

  // Element Template receives raw RL3 attribute-array keys at runtime. Match
  // __AddDataset semantics: strip only "data-" and keep kebab-case unchanged.
  element->AddDataset(data_name, value);
  return true;
}

bool RemoveTemplateDataAttribute(FiberElement* element,
                                 const base::String& key) {
  base::String data_name;
  if (!ParseTemplateDataAttribute(key.string_view(), &data_name)) {
    return false;
  }

  element->RemoveDataset(data_name);
  return true;
}

void ApplyTemplateAttributeValue(FiberElement* element, const base::String& key,
                                 const lepus::Value& value,
                                 TemplateAttributeApplyMode mode) {
  if (IsTemplateEventAttribute(key)) {
    if (mode == TemplateAttributeApplyMode::kNonEventOnly) {
      return;
    }
    ApplyTemplateEventAttribute(element, key, value);
    return;
  }
  if (mode == TemplateAttributeApplyMode::kEventOnly) {
    return;
  }
  if (ApplySpecialTemplateAttribute(element, key, value)) {
    return;
  }
  if (ApplyTemplateListCallbackAttribute(element, key, value)) {
    return;
  }
  if (ApplyTemplateDataAttribute(element, key, value)) {
    return;
  }
  element->SetAttribute(key, value);
}

void ApplyTemplateSpreadAttributes(FiberElement* element,
                                   const lepus::Value& value,
                                   TemplateAttributeApplyMode mode) {
  if (element == nullptr || !value.IsObject()) {
    return;
  }

  tasm::ForEachLepusValue(
      value, [element, mode](const auto& key, const auto& item) {
        ApplyTemplateAttributeValue(element, key.String(), item, mode);
      });
}

lepus::Value ResolveAttributeSlotValue(const lepus::Value& attribute_slots,
                                       uint32_t slot_index) {
  if (!attribute_slots.IsArrayOrJSArray() ||
      slot_index >= static_cast<uint32_t>(attribute_slots.GetLength())) {
    return lepus::Value();
  }
  return attribute_slots.GetProperty(slot_index);
}

void ClearPreviousTemplateSpreadAttributes(
    FiberElement* element, const TemplateAttributes& template_attributes,
    const lepus::Value& previous_attribute_slots) {
  if (!previous_attribute_slots.IsArrayOrJSArray()) {
    return;
  }

  for (const auto& attr : template_attributes) {
    if (attr.type_ != ATTRIBUTE_BINDING_TYPE_SPREAD) {
      continue;
    }
    auto previous_value =
        ResolveAttributeSlotValue(previous_attribute_slots, attr.slot_index_);
    if (!previous_value.IsObject()) {
      continue;
    }
    tasm::ForEachLepusValue(
        previous_value, [element](const auto& key, const auto&) {
          if (RemoveTemplateDataAttribute(element, key.String())) {
            return;
          }
          // Re-apply previous spread keys with an empty value to clear keys
          // that disappeared from the current spread object. data-* is handled
          // above because __AddDataset preserves empty values.
          ApplyTemplateAttributeValue(element, key.String(), lepus::Value(),
                                      TemplateAttributeApplyMode::kAll);
        });
  }
}

void ApplyTemplateAttributesToElementInternal(
    FiberElement* element, const lepus::Value* previous_attribute_slots,
    const lepus::Value& attribute_slots, TemplateAttributeApplyMode mode) {
  if (element == nullptr || !element->HasTemplateAttributes()) {
    return;
  }

  const auto& template_attributes = element->template_attributes();
  const bool rebuild_static_attributes = previous_attribute_slots != nullptr;
  DCHECK(!rebuild_static_attributes ||
         mode == TemplateAttributeApplyMode::kAll);
  if (rebuild_static_attributes) {
    ClearPreviousTemplateSpreadAttributes(element, *template_attributes,
                                          *previous_attribute_slots);
  }
  bool has_applied_spread = false;
  for (const auto& attr : *template_attributes) {
    if (attr.type_ == ATTRIBUTE_BINDING_TYPE_SPREAD) {
      ApplyTemplateSpreadAttributes(
          element, ResolveAttributeSlotValue(attribute_slots, attr.slot_index_),
          mode);
      has_applied_spread = true;
      continue;
    }
    if (attr.type_ == ATTRIBUTE_BINDING_TYPE_STATIC) {
      if (has_applied_spread || rebuild_static_attributes) {
        ApplyTemplateAttributeValue(element, attr.key_, attr.value_, mode);
      }
      continue;
    }
    if (attr.type_ == ATTRIBUTE_BINDING_TYPE_DYNAMIC) {
      ApplyTemplateAttributeValue(
          element, attr.key_,
          ResolveAttributeSlotValue(attribute_slots, attr.slot_index_), mode);
      continue;
    }
  }
}

}  // namespace

void TreeResolver::ApplyTemplateAttributesToElement(
    FiberElement* element, const lepus::Value& attribute_slots) {
  ApplyTemplateAttributesToElementInternal(element, nullptr, attribute_slots,
                                           TemplateAttributeApplyMode::kAll);
}

void TreeResolver::ApplyTemplateAttributesToElement(
    FiberElement* element, const lepus::Value& previous_attribute_slots,
    const lepus::Value& attribute_slots) {
  ApplyTemplateAttributesToElementInternal(element, &previous_attribute_slots,
                                           attribute_slots,
                                           TemplateAttributeApplyMode::kAll);
}

void TreeResolver::ApplyTemplateNonEventAttributesToElement(
    FiberElement* element, const lepus::Value& attribute_slots) {
  ApplyTemplateAttributesToElementInternal(
      element, nullptr, attribute_slots,
      TemplateAttributeApplyMode::kNonEventOnly);
}

void TreeResolver::ApplyTemplateEventAttributesToElement(
    FiberElement* element, const lepus::Value& attribute_slots) {
  ApplyTemplateAttributesToElementInternal(
      element, nullptr, attribute_slots,
      TemplateAttributeApplyMode::kEventOnly);
}

void TreeResolver::ApplyStaticTemplateEventAttributesToElement(
    FiberElement* element) {
  if (element == nullptr || !element->HasTemplateAttributes()) {
    return;
  }

  for (const auto& attr : *element->template_attributes()) {
    if (attr.type_ != ATTRIBUTE_BINDING_TYPE_STATIC) {
      continue;
    }
    ApplyTemplateEventAttribute(element, attr.key_, attr.value_);
  }
}

void TreeResolver::NotifyNodeInserted(FiberElement* insertion_point,
                                      FiberElement* node) {
  // If the insertion_point IsDetached, no nedd to call NotifyNodeInserted
  // recursively.
  if (insertion_point->IsDetached()) {
    return;
  }

  node->InsertedInto(insertion_point);

  for (const auto& child : node->children()) {
    NotifyNodeInserted(insertion_point,
                       static_cast<FiberElement*>(child.get()));
  }
}

void TreeResolver::NotifyNodeRemoved(FiberElement* insertion_point,
                                     FiberElement* node) {
  // If the insertion_point IsDetached, no nedd to call NotifyNodeRemoved
  // recursively.
  if (insertion_point->IsDetached()) {
    return;
  }

  node->RemovedFrom(insertion_point);

  for (const auto& child : node->children()) {
    if (static_cast<FiberElement*>(child.get())->is_raw_text()) {
      continue;
    }
    NotifyNodeRemoved(insertion_point, static_cast<FiberElement*>(child.get()));
  }
}

FiberElement* TreeResolver::FindFirstChildOrSiblingAsRefNode(
    FiberElement* ref) {
  return ref ? static_cast<FiberElement*>(
                   ref->FindFirstNonWrapperChildOrSibling())
             : nullptr;
}

void TreeResolver::AttachChildToTargetParentForWrapper(FiberElement* parent,
                                                       FiberElement* child,
                                                       FiberElement* ref_node) {
  // ref is null, find the first none-wrapper ancestor's next sibling as ref!
  auto* temp_parent = parent;

  if (ref_node && ref_node->is_wrapper()) {
    // this API always return null or non-wrapper ref_node
    ref_node = FindFirstChildOrSiblingAsRefNode(ref_node);
  }
  DCHECK(!ref_node || !ref_node->is_wrapper());

  while (!ref_node && temp_parent && temp_parent->is_wrapper()) {
    ref_node = static_cast<FiberElement*>(temp_parent->next_render_sibling());

    if (ref_node && ref_node->EnableLayoutInElementMode()) {
      // If ref_node is virtual in element-mode, try to find the next
      // non-virtual sibling (virtual nodes are not inserted into the layout
      // tree for unknown reasons).
      ref_node = static_cast<FiberElement*>(
          ref_node->FindFirstNonVirtualRenderSibling());
    }

    if (ref_node && ref_node->is_wrapper()) {
      // try to find the wrapper's first non-wrapper child as ref
      ref_node = FindFirstChildOrSiblingAsRefNode(ref_node);
    }

    if (ref_node && !ref_node->is_wrapper()) {
      // break when found any non-wrapper ref_node
      break;
    }

    temp_parent = static_cast<FiberElement*>(temp_parent->render_parent());
  }

  FiberElement* real_parent =
      static_cast<FiberElement*>(parent->FindFirstNonWrapperRenderAncestor());

  AttachChildToTargetContainerRecursive(real_parent, child, ref_node);
}

std::pair<FiberElement*, int> TreeResolver::FindParentForChildForWrapper(
    FiberElement* parent, FiberElement* child, FiberElement* ref_node) {
  FiberElement* node = parent;

  if (!ref_node && !parent->is_wrapper()) {
    // ref is null & parent is none-wrapper, layout_index:-1 is to append the
    // end
    return {parent, -1};
  }

  int in_wrapper_index = 0;
  if (!ref_node && parent->is_wrapper()) {
    // append to wrapper, use parent as ref and then add self layout index in
    // parent
    in_wrapper_index = GetLayoutIndexForChildForWrapper(parent, child);
    ref_node = parent;
  }

  int layout_index = GetLayoutIndexForChildForWrapper(node, ref_node);

  if (layout_index == -1) {
    return {nullptr, layout_index};
  }
  while (node->is_wrapper()) {
    auto* p = static_cast<FiberElement*>(node->render_parent());
    if (!p) {
      return {nullptr, -1};
    }
    layout_index += static_cast<int>(GetLayoutIndexForChildForWrapper(p, node));
    node = p;
  }
  return {node, layout_index + in_wrapper_index};
}

int TreeResolver::GetLayoutIndexForChildForWrapper(FiberElement* parent,
                                                   FiberElement* child) {
  int index = 0;
  bool found = false;
  for (const auto& it : parent->children()) {
    auto* current = static_cast<FiberElement*>(it.get());
    if (child == current) {
      found = true;
      break;
    }
    index +=
        (current->is_wrapper() ? GetLayoutChildrenCountForWrapper(current) : 1);
  }
  if (!found) {
    LOGI("fiber element can not found for wrapper:" + parent->GetTag().str());
    // index:-1 means the child id was not a child of parent id
    return -1;
  }
  return index;
}

size_t TreeResolver::GetLayoutChildrenCountForWrapper(FiberElement* node) {
  size_t ret = 0;
  for (const auto& current : node->children()) {
    auto* current_fiber = static_cast<FiberElement*>(current.get());
    if (current_fiber->is_wrapper()) {
      ret += GetLayoutChildrenCountForWrapper(current_fiber);
    } else {
      ret++;
    }
  }
  return ret;
}

void TreeResolver::AttachChildToTargetContainerRecursive(FiberElement* parent,
                                                         FiberElement* child,
                                                         FiberElement* ref) {
  // in the mapped layout node tree, insert the wrapper node in front of its
  // first child real parent:
  // [node0,node1,[wrapper,wrapper-child0,wrapper-child1],node3....]

  DCHECK(!ref || !ref->is_wrapper());
  if (!child->is_wrapper()) {
    parent->InsertLayoutNode(child, ref);
    return;
  }

  // wrapper node should add subtree to parent recursively.
  auto* grand = static_cast<FiberElement*>(child->first_render_child());
  while (grand) {
    AttachChildToTargetContainerRecursive(parent, grand, ref);
    grand = static_cast<FiberElement*>(grand->next_render_sibling());
  }
}

FiberElement* TreeResolver::FindTheRealParent(FiberElement* node) {
  return node ? static_cast<FiberElement*>(
                    node->FindFirstNonWrapperRenderAncestor())
              : nullptr;
}

// for layout node
void TreeResolver::RemoveChildRecursively(FiberElement* parent,
                                          FiberElement* child) {
  if (!child->is_wrapper()) {
    parent->RemoveLayoutNode(child);
  } else {
    auto* grand = child->first_render_child();
    while (grand) {
      RemoveChildRecursively(parent, static_cast<FiberElement*>(grand));
      grand = grand->next_render_sibling();
    }
  }
}

void TreeResolver::RemoveFromParentForWrapperChild(FiberElement* parent,
                                                   FiberElement* child) {
  FiberElement* real_parent = FindTheRealParent(parent);
  if (real_parent->is_wrapper()) {
    LOGE(
        "[WrapperElement] parent maybe detached from the view tree, can not "
        "find real parent!");
    return;
  }

  RemoveChildRecursively(real_parent, child);
}

fml::RefPtr<lepus::Dictionary> TreeResolver::GetTemplateParts(
    const fml::RefPtr<FiberElement>& template_element) {
  DCHECK(template_element->IsTemplateElement());
  auto parts_map = lepus::Dictionary::Create();
  for (const auto& c : template_element->children()) {
    GetPartsRecursively(fml::static_ref_ptr_cast<FiberElement>(c), parts_map);
  }
  return parts_map;
}

fml::RefPtr<FiberElement> TreeResolver::CloneElements(
    const fml::RefPtr<FiberElement>& root,
    const std::shared_ptr<CSSStyleSheetManager>& style_manager,
    bool clone_resolved_props, CloningDepth cloning_depth) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TREE_RESOLVER_CLONE_ELEMENTS);

  fml::RefPtr<FiberElement> res = root->CloneElement(clone_resolved_props);
  res->AttachToElementManager(root->element_manager(), style_manager, false);

  if (cloning_depth == CloningDepth::kSingle) {
    return res;
  }

  // construct children
  for (const auto& c : root->children()) {
    auto* child = static_cast<FiberElement*>(c.get());
    if (cloning_depth == CloningDepth::kTemplateScope &&
        child->IsTemplateElement()) {
      continue;
    }
    res->InsertNode(CloneElements(fml::static_ref_ptr_cast<FiberElement>(c),
                                  style_manager, clone_resolved_props,
                                  cloning_depth));
  }

  return res;
}

base::Vector<fml::RefPtr<FiberElement>> TreeResolver::FromTemplateInfo(
    const ElementTemplateInfo& info) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TREE_RESOLVER_FROM_TEMPLATE_INFO);
  base::Vector<fml::RefPtr<FiberElement>> res;
  for (const auto& element_info : info.elements_) {
    auto element_node = FromElementInfo(-1, element_info, nullptr);
    element_node->MarkTemplateElement();
    res.emplace_back(std::move(element_node));
  }
  return res;
}

GeneratedElementsResult TreeResolver::GenerateElementsFromTemplateInfo(
    const ElementTemplateInfo& info) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TREE_RESOLVER_FROM_TEMPLATE_INFO);
  GeneratedElementsResult generated;
  if (info.elements_.empty()) {
    return generated;
  }
  DCHECK_EQ(info.elements_.size(), 1u);
  generated.result_ = FromElementInfo(-1, info.elements_.front(), &generated);
  if (generated.result_ != nullptr) {
    generated.result_->MarkTemplateElement();
  }
  return generated;
}

void TreeResolver::InitElementTree(
    fml::RefPtr<FiberElement>& element, int64_t pid, ElementManager* manager,
    const std::shared_ptr<CSSStyleSheetManager>& style_manager) {
  element->ApplyFunctionRecursive([manager, style_manager](FiberElement* e) {
    e->AttachToElementManager(manager, style_manager, false);
  });
  element->SetParentComponentUniqueIdRecursively(pid);
}

lepus::Value TreeResolver::InitElementTree(
    base::Vector<fml::RefPtr<FiberElement>>&& elements, int64_t pid,
    ElementManager* manager,
    const std::shared_ptr<CSSStyleSheetManager>& style_manager) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TREE_RESOLVER_INIT_ELEMENT_TREE);
  auto ary = lepus::CArray::Create();
  for (auto& element : elements) {
    element->ApplyFunctionRecursive([manager, style_manager](FiberElement* e) {
      e->AttachToElementManager(manager, style_manager, false);
    });
    element->SetParentComponentUniqueIdRecursively(pid);
    ary->emplace_back(element);
  }
  return lepus::Value(ary);
}

fml::RefPtr<FiberElement> TreeResolver::CloneElementRecursively(
    const FiberElement* element, bool clone_resolved_props) {
  fml::RefPtr<FiberElement> res = element->CloneElement(clone_resolved_props);

  // construct children
  for (const auto& c : element->children()) {
    res->InsertNode(CloneElementRecursively(static_cast<FiberElement*>(c.get()),
                                            clone_resolved_props));
  }

  return res;
}

void TreeResolver::AttachRootToElementManager(
    fml::RefPtr<FiberElement>& root, ElementManager* element_manager,
    const std::shared_ptr<CSSStyleSheetManager>& style_manager,
    bool keep_element_id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TREE_RESOLVER_ATTACH_TO_ELEMENT_MANAGER);

  TreeResolver::AttachToElementManagerRecursively(
      *root, element_manager, style_manager, keep_element_id);
  element_manager->FiberAttachToInspectorRecursively(root.get());
}

void TreeResolver::AttachToElementManagerRecursively(
    FiberElement& element, ElementManager* manager,
    const std::shared_ptr<CSSStyleSheetManager>& style_manager,
    bool keep_element_id) {
  element.AttachToElementManager(manager, style_manager, keep_element_id);

  for (auto& child : element.children()) {
    AttachToElementManagerRecursively(*static_cast<FiberElement*>(child.get()),
                                      manager, style_manager, keep_element_id);
  }
}

void TreeResolver::GetPartsRecursively(
    const fml::RefPtr<FiberElement>& root,
    fml::RefPtr<lepus::Dictionary>& parts_map) {
  if (root->IsPartElement()) {
    parts_map->SetValue(root->GetPartID(), root);
  }
  if (root->IsTemplateElement()) {
    return;
  }
  for (const auto& c : root->children()) {
    GetPartsRecursively(fml::static_ref_ptr_cast<FiberElement>(c), parts_map);
  }
}

fml::RefPtr<FiberElement> TreeResolver::FromElementInfo(
    int64_t parent_component_id, const ElementInfo& info) {
  return FromElementInfo(parent_component_id, info, nullptr);
}

fml::RefPtr<FiberElement> TreeResolver::FromElementInfo(
    int64_t parent_component_id, const ElementInfo& info,
    GeneratedElementsResult* generated) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TREE_RESOLVER_FROM_ELEMENT_INFO);
  fml::RefPtr<FiberElement> res =
      ElementManager::StaticCreateFiberElement(info.tag_enum_, info.tag_);
  if (info.attributes_ != nullptr) {
    // Keep a shared read-only copy of the template attribute descriptors on
    // the runtime element so later flush steps can rebuild all template attrs.
    res->SetTemplateAttributes(info.attributes_);
  }

  if (generated != nullptr && info.tag_enum_ == ELEMENT_SLOT) {
    return nullptr;
  }

  if (res->is_component()) {
    auto* component = static_cast<ComponentElement*>(res.get());
    component->set_component_id(info.component_id_);
    component->set_component_name(info.component_name_);
    component->set_component_path(info.component_path_);
    component->SetCSSID(info.css_id_);
  }
  if (res->is_page()) {
    auto* page = static_cast<PageElement*>(res.get());
    page->set_component_id(info.component_id_);
    page->SetCSSID(info.css_id_);
  }

  if (info.tag_enum_ != ELEMENT_PAGE && parent_component_id > 0) {
    res->SetParentComponentUniqueIdForFiber(parent_component_id);
  }

  // set id selector
  if (!info.id_selector_.empty()) {
    res->SetIdSelector(info.id_selector_);
  }

  // set class selector
  for (const auto& class_name : info.class_selector_) {
    res->SetClass(class_name);
  }

  // set inline style
  for (const auto& pair : info.inline_styles_) {
    res->SetStyle(static_cast<CSSPropertyID>(pair.first),
                  lepus::Value(pair.second));
  }

  // set js event
  for (const auto& event : info.events_) {
    res->SetJSEventHandler(event.name_, event.type_, event.value_);
  }

  // set parsed style
  if (info.has_parser_style_) {
    res->SetParsedStyles(*info.parsed_styles_, info.config_);
  }

  // set attributes
  for (const auto& pair : info.attrs_) {
    res->SetAttribute(pair.first, pair.second);
  }

  if (info.attributes_ != nullptr) {
    bool has_static_template_event = false;
    for (const auto& attr : *info.attributes_) {
      if (attr.type_ == ATTRIBUTE_BINDING_TYPE_STATIC) {
        if (generated != nullptr && IsTemplateEventAttribute(attr.key_)) {
          has_static_template_event = true;
        }
        ApplyTemplateAttributeValue(res.get(), attr.key_, attr.value_,
                                    TemplateAttributeApplyMode::kAll);
        continue;
      }
      if (generated != nullptr &&
          (attr.type_ == ATTRIBUTE_BINDING_TYPE_DYNAMIC ||
           attr.type_ == ATTRIBUTE_BINDING_TYPE_SPREAD)) {
        // attrSlotIndex only needs to locate the affected element. Attribute
        // re-application will happen in a later flush-oriented patch.
        RegisterSlotTarget(&generated->attribute_slot_targets_,
                           static_cast<int32_t>(attr.slot_index_), res);
        if (attr.type_ == ATTRIBUTE_BINDING_TYPE_SPREAD ||
            IsTemplateEventAttribute(attr.key_)) {
          RegisterSlotTarget(&generated->event_attribute_slot_targets_,
                             static_cast<int32_t>(attr.slot_index_), res);
        }
      }
    }
    if (has_static_template_event) {
      generated->static_event_targets_.emplace_back(res);
    }
  }

  // set dataset
  if (!info.data_set_.IsEmpty()) {
    res->SetDataset(info.data_set_);
  }

  // construct children
  if (info.tag_enum_ == ELEMENT_COMPONENT || info.tag_enum_ == ELEMENT_PAGE) {
    parent_component_id = res->impl_id();
  }

  auto it = info.children_.begin();
  while (it != info.children_.end()) {
    base::Vector<int32_t> pending_slot_indices;
    while (it != info.children_.end() && it->tag_enum_ == ELEMENT_SLOT) {
      pending_slot_indices.push_back(it->slot_index_);
      ++it;
    }

    fml::RefPtr<FiberElement> materialized_child = nullptr;
    if (it != info.children_.end()) {
      materialized_child = FromElementInfo(parent_component_id, *it, generated);
      ++it;
    }

    if (generated != nullptr) {
      for (auto slot_index : pending_slot_indices) {
        RegisterElementSlotMountPoint(&generated->element_slot_targets_,
                                      slot_index, res, materialized_child);
      }
    }

    if (materialized_child != nullptr) {
      res->InsertNode(materialized_child);
    }
  }

  if (info.css_id_ != kInvalidCssId) {
    res->SetCSSID(info.css_id_);
  }

  // consume builtin attributes
  if (!info.builtin_attrs_.empty()) {
    for (const auto& [key, value] : info.builtin_attrs_) {
      res->SetBuiltinAttribute(key, value);
    }
  }

  if (info.config_.IsTable()) {
    if (info.config_.GetProperty(BASE_STATIC_STRING(kIsAsyncFlushRoot))
            .IsTrue()) {
      res->MarkAsyncFlushRoot(true);
    }
    res->SetConfig(lepus::Value::ShallowCopy(info.config_));
  }

  return res;
}

// TODO(ZHOUZHITA0): Merge with greedy threaded element resolution later
void TreeResolver::TraverseDom(FiberElement* root, uint32_t work_unit_size) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TREE_RESOLVER_TRAVERSE_DOM);
  std::list<FiberElement*> discovered;
  discovered.emplace_back(root);
  root->ApplyFunctionRecursive([](FiberElement* element) {
    if (element->ShouldResolveStyle()) {
      element->PrepareSelfForThreadedElementResolution();
    }
  });
  std::list<ParallelFlushReturn> engine_thread_tasks =
      TreeResolver::StyleTrees(discovered, work_unit_size);
  for (auto& reduce_task : engine_thread_tasks) {
    reduce_task();
  }
}

std::list<ParallelFlushReturn> TreeResolver::StyleTrees(
    std::list<FiberElement*>& discovered, uint32_t work_unit_size) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TREE_RESOLVER_STYLE_TREES);
  auto element_manager = discovered.front()->element_manager();
  auto optional_engine_thread_id = element_manager->GetCurrentEngineThreadId();
  auto is_engine_thread =
      optional_engine_thread_id.has_value() &&
      *optional_engine_thread_id == std::this_thread::get_id();
  auto local_queue_size =
      is_engine_thread ? kLocalQueueSizeInMainThread : kLocalQueueSizeInWorker;
  uint32_t node_remaining_at_current_depth =
      static_cast<uint32_t>(discovered.size());
  std::list<ParallelFlushReturn> resolve_returns;
  while (!discovered.empty()) {
    auto* target = discovered.front();
    discovered.pop_front();

    // FIXME(zhouzhitao): Level order traversing is not compatible with async
    // resolving api right now.
    if (target->ShouldResolveStyle()) {
      target->UpdateResolveStatus(FiberElement::AsyncResolveStatus::kResolving);
      target->MarkParallelFlushFlag(FiberElement::kFlagLevelOrderParallel);
      resolve_returns.emplace_back(target->PrepareForCreateOrUpdate());
      DCHECK((target->dirty() & ~FiberElement::kDirtyTree) == 0);
    }

    target->InvalidateChildrenIfNeeded();
    for (auto& child : target->children()) {
      auto* fiber_child = static_cast<FiberElement*>(child.get());
      if (target->NeedPropagateInheritedDirtyFlag(true)) {
        fiber_child->MarkDirtyLite(FiberElement::kDirtyPropagateInherited);
      }
      discovered.emplace_back(fiber_child);
    }

    node_remaining_at_current_depth--;
    uint32_t discovered_children = static_cast<uint32_t>(discovered.size()) -
                                   node_remaining_at_current_depth;
    if (discovered_children >= work_unit_size &&
        discovered.size() >= local_queue_size + work_unit_size) {
      // could use shared_ptr to avoid copy
      uint32_t kept_work = node_remaining_at_current_depth > local_queue_size
                               ? node_remaining_at_current_depth
                               : local_queue_size;
      std::list<FiberElement*> distribute_work_list;
      auto split_point = discovered.begin();
      std::advance(split_point, kept_work);

      distribute_work_list.splice(distribute_work_list.end(), discovered,
                                  split_point, discovered.end());
      TreeResolver::DistributeStyleTreesTask(std::move(distribute_work_list),
                                             work_unit_size);
    }

    if (node_remaining_at_current_depth == 0) {
      node_remaining_at_current_depth =
          static_cast<uint32_t>(discovered.size());
    }
  }
  return resolve_returns;
}

void TreeResolver::DistributeStyleTreesTask(std::list<FiberElement*> discovered,
                                            uint32_t work_unit_size) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TREE_RESOLVER_DISTRIBUTE_STYLE_TREES_TASK);
  auto element_manager = discovered.front()->element_manager();
  while (!discovered.empty()) {
    size_t transfer_count =
        std::min(static_cast<size_t>(work_unit_size), discovered.size());
    auto end_iter = discovered.begin();
    std::advance(end_iter, transfer_count);

    std::list<FiberElement*> work_slice;
    work_slice.splice(work_slice.end(), discovered, discovered.begin(),
                      end_iter);

    element_manager->IncrementLevelOrderResolveTaskCounter();
    TreeResolver::DistributeOneChunkStyleTreesTask(std::move(work_slice),
                                                   work_unit_size);
  }
}

void TreeResolver::DistributeOneChunkStyleTreesTask(
    std::list<FiberElement*> discovered, uint32_t work_unit_size) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              TREE_RESOLVER_DISTRIBUTE_ONE_CHUNK_STYLE_TREES_TASK);
  auto element_manager = discovered.front()->element_manager();
  std::promise<std::list<ParallelFlushReturn>> promise;
  std::future<std::list<ParallelFlushReturn>> future = promise.get_future();

  auto task_info_ptr =
      fml::MakeRefCounted<base::OnceTask<std::list<ParallelFlushReturn>>>(
          [promise = std::move(promise), discovered = std::move(discovered),
           work_unit_size]() mutable {
            auto element_manager = discovered.front()->element_manager();
            auto style_trees_result =
                TreeResolver::StyleTrees(discovered, work_unit_size);
            element_manager->DecrementLevelOrderResolveTaskCounter();
            promise.set_value(std::move(style_trees_result));
          },
          std::move(future));

  base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
      [task_info_ptr]() { task_info_ptr->Run(); },
      base::ConcurrentTaskType::HIGH_PRIORITY);
  element_manager->EnqueueLevelOrderTask(std::move(task_info_ptr));
}

}  // namespace tasm
}  // namespace lynx
