// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/tree_resolver.h"

#include "base/include/log/logging.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/component_element.h"
#include "core/renderer/dom/fiber/fiber_element.h"
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
#include "core/renderer/template_assembler.h"

namespace lynx {
namespace tasm {

void TreeResolver::NotifyNodeInserted(FiberElement* insertion_point,
                                      FiberElement* node) {
  // If the insertion_point IsDetached, no nedd to call NotifyNodeInserted
  // recursively.
  if (insertion_point->IsDetached()) {
    return;
  }

  node->InsertedInto(insertion_point);

  for (const auto& child : node->children()) {
    NotifyNodeInserted(insertion_point, child.get());
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
    if (child->is_raw_text()) {
      continue;
    }
    NotifyNodeRemoved(insertion_point, child.get());
  }
}

FiberElement* TreeResolver::FindFirstChildOrSiblingAsRefNode(
    FiberElement* ref) {
  while (ref) {
    if (!ref->is_wrapper()) {
      return ref;
    }
    auto* first_child = static_cast<FiberElement*>(ref->first_render_child());

    if (first_child) {
      if (!first_child->is_wrapper()) {
        return first_child;
      }
      auto* ret = FindFirstChildOrSiblingAsRefNode(first_child);
      if (ret && !ret->is_wrapper()) {
        return ret;
      }
    }
    ref = static_cast<FiberElement*>(ref->next_render_sibling());
  }
  return ref;
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

  FiberElement* real_parent = parent;
  while (real_parent->is_wrapper()) {
    auto* p = static_cast<FiberElement*>(real_parent->render_parent());
    if (!p) {
      break;
    }
    real_parent = p;
  }

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
    auto* current = it.get();
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
  for (auto current : node->children()) {
    if (current->is_wrapper()) {
      ret += GetLayoutChildrenCountForWrapper(current.get());
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
  FiberElement* real_parent = node;
  while (real_parent->is_wrapper()) {
    auto* p = static_cast<FiberElement*>(real_parent->render_parent());
    if (!p) {
      break;
    }
    real_parent = p;
  }
  return real_parent;
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
    GetPartsRecursively(c, parts_map);
  }
  return parts_map;
}

fml::RefPtr<FiberElement> TreeResolver::CloneElements(
    const fml::RefPtr<FiberElement>& root,
    const std::shared_ptr<CSSStyleSheetManager>& style_manager,
    bool clone_resolved_props, CloningDepth cloning_depth) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TreeResolver::CloneElements");

  fml::RefPtr<FiberElement> res = root->CloneElement(clone_resolved_props);
  res->AttachToElementManager(root->element_manager(), style_manager, false);

  if (cloning_depth == CloningDepth::kSingle) {
    return res;
  }

  // construct children
  for (const auto& c : root->children()) {
    if (cloning_depth == CloningDepth::kTemplateScope &&
        c->IsTemplateElement()) {
      continue;
    }
    res->InsertNode(
        CloneElements(c, style_manager, clone_resolved_props, cloning_depth));
  }

  return res;
}

base::Vector<fml::RefPtr<FiberElement>> TreeResolver::FromTemplateInfo(
    const ElementTemplateInfo& info) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TreeResolver::FromTemplateInfo");
  base::Vector<fml::RefPtr<FiberElement>> res;
  for (const auto& element_info : info.elements_) {
    auto element_node = FromElementInfo(-1, element_info);
    element_node->MarkTemplateElement();
    res.emplace_back(std::move(element_node));
  }
  return res;
}

lepus::Value TreeResolver::InitElementTree(
    base::Vector<fml::RefPtr<FiberElement>>&& elements, int64_t pid,
    ElementManager* manager,
    const std::shared_ptr<CSSStyleSheetManager>& style_manager) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TreeResolver::InitElementTree");
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
    res->InsertNode(CloneElementRecursively(c.get(), clone_resolved_props));
  }

  return res;
}

void TreeResolver::AttachRootToElementManager(
    fml::RefPtr<FiberElement>& root, ElementManager* element_manager,
    const std::shared_ptr<CSSStyleSheetManager>& style_manager,
    bool keep_element_id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TreeResolver::AttachRootToElementManager");
  element_manager->SetFiberPageElement(
      fml::static_ref_ptr_cast<PageElement>(root));

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
    AttachToElementManagerRecursively(*child, manager, style_manager,
                                      keep_element_id);
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
    GetPartsRecursively(c, parts_map);
  }
}

fml::RefPtr<FiberElement> TreeResolver::FromElementInfo(
    int64_t parent_component_id, const ElementInfo& info) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TreeResolver::FromElementInfo");
  fml::RefPtr<FiberElement> res =
      ElementManager::StaticCreateFiberElement(info.tag_enum_, info.tag_);
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

  // set dataset
  if (!info.data_set_.IsEmpty()) {
    res->SetDataset(info.data_set_);
  }

  // construct children
  if (info.tag_enum_ == ELEMENT_COMPONENT || info.tag_enum_ == ELEMENT_PAGE) {
    parent_component_id = res->impl_id();
  }
  for (const auto& child : info.children_) {
    res->InsertNode(FromElementInfo(parent_component_id, child));
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
    res->SetConfig(lepus::Value::ShallowCopy(info.config_));
  }

  return res;
}

}  // namespace tasm
}  // namespace lynx
