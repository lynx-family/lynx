// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/element_container.h"

#include <algorithm>
#include <cstddef>
#include <deque>

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

static int CompareElementOrder(Element* left, Element* right);

ElementContainer::ElementContainer(Element* element) : element_(element) {
  was_stacking_context_ = IsStackingContextNode();
  was_position_fixed_ = element->IsNewFixed();
  old_index_ = ZIndex();
}

ElementContainer::~ElementContainer() {
  if (!element_->will_destroy() && was_stacking_context_) {
    element_manager()->RemoveDirtyContext(this);
  }
  // Remove self from parent's children.
  if (parent_) {
    auto it =
        std::find(parent_->children_.begin(), parent_->children_.end(), this);
    if (it != parent_->children_.end()) parent_->children_.erase(it);
    parent_ = nullptr;
  }
  // Set children's parent to null.
  for (auto child : children_) {
    if (child) {
      child->parent_ = nullptr;
    }
  }
}

int ElementContainer::id() const { return element_->impl_id(); }

void ElementContainer::AddChild(ElementContainer* child, int index) {
  if (child->parent()) {
    child->RemoveFromParent(true);
  }
  children_.push_back(child);

  if (!child->element()->IsLayoutOnly()) {
    none_layout_only_children_size_++;
  }
  // If the index is equal to -1 should add to the last. The node could be
  // position: fixed or with z-index.
  if (index != -1) {
    index = index + static_cast<int>(negative_z_children_.size());
  }

  if (child->element_->IsNewFixed() && child->ZIndex() == 0) {
    int fixed_node_offset = 0;
    for (const ElementContainer* el : children_) {
      if (!el->element_->IsLayoutOnly() && el->ZIndex() == 0 &&
          !el->element_->is_fixed()) {
        fixed_node_offset++;
      }
    }

    int index_of_fixed = 0;
    auto it = element_manager()->fixed_node_list_.end();
    size_t left = 0, right = element_manager()->fixed_node_list_.size();
    size_t mid = right;
    while (left < right) {
      mid = left + (right - left) / 2;
      it = element_manager()->fixed_node_list_.begin();
      std::advance(it, mid);
      if (CompareElementOrder(child->element_, (*it)->element_) > 0) {
        left = mid + 1;
        std::advance(it, 1);
      } else {
        right = mid;
      }
    }
    index_of_fixed = static_cast<int>(left);
    element_manager()->fixed_node_list_.insert(it, child);
    index = fixed_node_offset + index_of_fixed;
  }

  child->parent_ = this;
  if ((child->ZIndex() != 0 || child->IsSticky()) && need_update_) {
    MarkDirty();
  }
  if (!child->element()->IsLayoutOnly()) {
    painting_context()->InsertPaintingNode(id(), child->id(), index);
  }
}

void ElementContainer::RemoveChild(ElementContainer* child) {
  auto it = std::find(children_.begin(), children_.end(), child);
  if (it != children_.end()) {
    children_.erase(it);
    if (child->element_->ZIndex() < 0) {
      auto z_it = std::find(negative_z_children_.begin(),
                            negative_z_children_.end(), child);
      if (z_it != negative_z_children_.end()) {
        negative_z_children_.erase(z_it);
      }
    }
    if ((child->element()->IsNewFixed() || child->was_position_fixed_) &&
        child->ZIndex() == 0) {
      element_->element_manager()->fixed_node_list_.remove(child);
    }
    if (!child->element_->IsLayoutOnly()) {
      none_layout_only_children_size_--;
    }
  }

  child->parent_ = nullptr;
  if (!need_update_) {
    return;
  }
  if (child->ZIndex() != 0) {
    // The stacking context need update
    MarkDirty();
  }
}

void ElementContainer::RemoveFromParent(bool is_move) {
  if (!parent_) return;
  if (!element()->IsLayoutOnly()) {
    painting_context()->RemovePaintingNode(parent_->id(), id(), 0, is_move);
  } else {
    // Layout only node remove children from parent recursively.
    if (element_->is_radon_element()) {
      for (int i = static_cast<int>(element_->GetChildCount()) - 1; i >= 0;
           --i) {
        Element* child = element_->GetChildAt(i);
        child->element_container()->RemoveFromParent(is_move);
      }
    } else {
      // fiber element;
      auto* child = static_cast<FiberElement*>(element())->first_render_child();
      while (child) {
        child->element_container()->RemoveFromParent(is_move);
        child = child->next_render_sibling();
      }
    }
  }
  parent_->RemoveChild(this);
}

void ElementContainer::Destroy() {
  // Layout only destroy recursively, the z-index child may has been destroyed
  if (!element()->IsLayoutOnly()) {
    painting_context()->DestroyPaintingNode(parent() ? parent()->id() : -1,
                                            id(), 0);
  } else {
    if (element_->is_radon_element()) {
      // fiber element's layout only children handle Destroy in self Destructor
      for (int i = static_cast<int>(element_->GetChildCount()) - 1; i >= 0;
           --i) {
        element_->GetChildAt(i)->element_container()->Destroy();
      }
    }
  }
  if (parent()) {
    parent()->RemoveChild(this);
  }
}

void ElementContainer::RemoveSelf(bool destroy) {
  if (!parent_) return;

  if (destroy) {
    Destroy();
  } else {
    RemoveFromParent(false);
  }
}

void ElementContainer::InsertSelf() {
  if (!parent_ && element()->parent()) {
    element()->parent()->element_container()->AttachChildToTargetContainer(
        element(), element()->next_render_sibling());
  }
}

PaintingContext* ElementContainer::painting_context() {
  return element()->painting_context();
}

std::pair<ElementContainer*, int> ElementContainer::FindParentForChild(
    Element* child) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ElementContainer::FindParentForChild");
  Element* node = element_;
  size_t ui_index = element_->GetUIIndexForChild(child);
  while (node->IsLayoutOnly()) {
    Element* parent = node->parent();
    if (!parent) {
      return {nullptr, -1};
    }
    ui_index += static_cast<int>(parent->GetUIIndexForChild(node));
    node = parent;
  }
  return {node->element_container(), ui_index};
}

void AttachChildToTargetContainerRecursive(ElementContainer* parent,
                                           Element* child, int& index) {
  if (child->ZIndex() != 0 || child->IsNewFixed()) {
    if (child->IsNewFixed()) {
      // fixed node should attach to page root.
      parent =
          parent->element()->element_manager()->root()->element_container();
    }
    auto ui_parent = parent->EnclosingStackingContextNode();
    ui_parent->AddChild(child->element_container(), -1);
    return;
  }
  // In the case that a scroll-view has a child, which is a wrapper element and
  // has a layout-only child view. If add wrapper element to scroll-view,
  // wrapper element should not create native view, but if add the layout-only
  // child to scroll-view, the child should create native view.
  if (!parent->element()->CanHasLayoutOnlyChildren() && child->IsLayoutOnly() &&
      !(child->is_fiber_element() &&
        static_cast<FiberElement*>(child)->is_wrapper()) &&
      !child->is_virtual()) {
    child->TransitionToNativeView();
  }
  parent->AddChild(child->element_container(), index);
  if (!child->IsLayoutOnly()) {
    ++index;
    return;
  }
  // Layout only node should add subtree to parent recursively.
  if (parent->element()->is_radon_element()) {
    for (size_t i = 0; i < child->GetChildCount(); ++i) {
      Element* grand_child = child->GetChildAt(i);
      AttachChildToTargetContainerRecursive(parent, grand_child, index);
    }
  } else {
    auto* grand = static_cast<FiberElement*>(child)->first_render_child();
    while (grand) {
      AttachChildToTargetContainerRecursive(parent, grand, index);
      grand = grand->next_render_sibling();
    }
  }
}

void ElementContainer::ReInsertChildForLayoutOnlyTransition(Element* child,
                                                            int& index) {
  if (!child->element_container()) {
    LOGE("re-insert the wrong element!");
    return;
  }
  AttachChildToTargetContainerRecursive(this, child, index);

  child->MarkFrameChanged();
  child->element_container()->UpdateLayout(child->left(), child->top(), true);
}

void ElementContainer::AttachChildToTargetContainer(Element* child,
                                                    Element* ref) {
  if (child->IsNewFixed()) {
    element_manager()->root()->element_container()->AddChild(
        child->element_container(), -1);
    return;
  }
  if (child->ZIndex() != 0) {
    auto* enclosing_stacking_node = EnclosingStackingContextNode();
    if (enclosing_stacking_node) {
      enclosing_stacking_node->AddChild(child->element_container(), -1);
    } else {
      LOGE(
          "AttachChildToTargetContainer got error: enclosing_stacking_node is "
          "nullptr!");
    }
    return;
  }
  std::pair<ElementContainer*, int> result;
  if (element_->is_radon_element()) {
    result = FindParentForChild(child);
  } else {
    result = FindParentAndIndexForChildForFiber(element_, child, ref);
  }
  if (result.first) {
    int index = result.second;
    AttachChildToTargetContainerRecursive(result.first, child, index);
  }
}

// Calculate position for element and update it to impl layer.
void ElementContainer::UpdateLayout(float left, float top,
                                    bool transition_view) {
  // Self is updated or self position is changed because of parent's frame
  // changing.

  if (element_->IsNewFixed()) {
    // new fixed node's parent should always be root node. And layout params are
    // calculated by starlight.
    left = element_->left();
    top = element_->top();
  } else if (element_->ZIndex() != 0) {
    // The z-index child's parent may be different from ui parent,
    // and need to add the offset of the position
    left = element_->left();
    top = element_->top();
    auto* ui_parent = parent();
    auto* parent = element_->is_radon_element() ? element_->parent()
                                                : element_->render_parent();
    while (parent && ui_parent && ui_parent->element() != parent) {
      left += parent->left();
      top += parent->top();
      parent = parent->parent();
    }
  }
  bool need_update_impl =
      (!transition_view || is_layouted_) &&
      (element_->frame_changed() || left != last_left_ || top != last_top_);

  last_left_ = left;
  last_top_ = top;

  // The offset of child's position in its real parent's coordinator.
  float dx = left, dy = top;

  if (!element_->IsLayoutOnly()) {
    dx = 0;
    dy = 0;

    if (need_update_impl) {  // Update to impl layer

      element_->painting_context()->UpdateLayout(
          element_->impl_id(), left, top, element_->width(), element_->height(),
          element_->paddings().data(), element_->margins().data(),
          element_->borders().data(), nullptr,
          element_->is_sticky() ? element_->sticky_positions().data() : nullptr,
          element_->max_height(), element_->NodeIndex());
    }
    if (need_update_impl || props_changed_) {
      element_->painting_context()->OnNodeReady(element_->impl_id());
      props_changed_ = false;
    }
  }

  // If the element is list, and use c++ implementation, we will block it's
  // children invoking UpdateLayout() to flush layout info to platform, because
  // the left and top's value of child element is incorrect.
  if (!element_->DisableListPlatformImplementation()) {
    // Layout children
    if (element_->is_radon_element()) {
      for (size_t i = 0; i < element_->GetChildCount(); ++i) {
        Element* child = element_->GetChildAt(i);
        if (child->element_container()) {
          child->element_container()->UpdateLayout(
              child->left() + dx, child->top() + dy, transition_view);
        }
      }
    } else {
      // TDOO(linxs): need to uniform the usage for radonElement&FiberElement
      auto* child = static_cast<FiberElement*>(element_)->first_render_child();
      while (child) {
        if (child->element_container()) {
          child->element_container()->UpdateLayout(
              child->left() + dx, child->top() + dy, transition_view);
        }
        child = child->next_render_sibling();
      }
    }
  }
  element_->MarkUpdated();

  is_layouted_ = true;
}

void ElementContainer::UpdateLayoutWithoutChange() {
  if (props_changed_) {
    element_->painting_context()->OnNodeReady(element_->impl_id());
    props_changed_ = false;
  }
  if (element_->is_radon_element()) {
    for (size_t i = 0; i < element_->GetChildCount(); ++i) {
      Element* child = element_->GetChildAt(i);
      if (child->element_container()) {
        child->element_container()->UpdateLayoutWithoutChange();
      }
    }
  } else {
    // TDOO(linxs): need to uniform the usage for radonElement&FiberElement
    auto* child = static_cast<FiberElement*>(element_)->first_render_child();
    while (child) {
      child->element_container()->UpdateLayoutWithoutChange();
      child = child->next_render_sibling();
    }
  }
}

void ElementContainer::TransitionToNativeView(
    std::shared_ptr<PropBundle> prop_bundle) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ElementContainer::TransitionToNativeView");
  if (prop_bundle == nullptr) {
    return;
  }

  element_->element_manager()->DecreaseLayoutOnlyElementCount();
  element_->element_manager()->IncreaseLayoutOnlyTransitionCount();

  LOGI("[ElementContainer] TransitionToNativeView tag:"
       << element_->GetTag().str() << ",id:" << element_->impl_id());

  // Remove from current parent.
  RemoveFromParent(true);

  // Create LynxUI in impl layer.
  element_->set_is_layout_only(false);

  // Push painting related props into prop_bundle.
  prop_bundle->SetPropsByID(
      kPropertyIDOverflow,
      pub::ValueImplLepus(
          element()->computed_css_style()->GetValue(kPropertyIDOverflow)));

  element_->painting_context()->CreatePaintingNode(
      element_->impl_id(), element_->GetPlatformNodeTag().str(), prop_bundle,
      element_->TendToFlatten(), element_->NeedCreateNodeAsync(),
      element_->NodeIndex());

  // Insert children to this.
  InsertSelf();

  // Mark need update layout value to impl layer.
  element_->MarkFrameChanged();

  UpdateLayout(last_left_, last_top_, true);

  int ui_index = 0;
  if (element_->is_radon_element()) {
    for (size_t i = 0; i < element_->GetChildCount(); ++i) {
      Element* child = element_->GetChildAt(i);
      ReInsertChildForLayoutOnlyTransition(child, ui_index);
    }
  } else {
    auto* child = static_cast<FiberElement*>(element_)->first_render_child();
    while (child) {
      ReInsertChildForLayoutOnlyTransition(child, ui_index);
      child = child->next_render_sibling();
    }
  }

  // the updateLayout is not in LayoutContext flow, just flush patching
  // immediately. otherwise, the updateLayout may execute after followed
  // operation,such as Destroy.
  painting_context()->UpdateLayoutPatching();
}

void ElementContainer::MoveContainers(ElementContainer* old_parent,
                                      ElementContainer* new_parent) {
  if (!new_parent) return;
  if (old_parent == new_parent) return;

  RemoveFromParent(true);
  new_parent->AddChild(this, -1);
}

ElementContainer* ElementContainer::EnclosingStackingContextNode() {
  Element* current = element();
  for (; current != nullptr; current = current->parent()) {
    if (current->IsStackingContextNode()) return current->element_container();
  }
  // Unreachable code
  return nullptr;
}

void ElementContainer::MoveZChildrenRecursively(Element* element,
                                                ElementContainer* parent) {
  for (size_t i = 0; i < element->GetChildCount(); i++) {
    auto* child = element->GetChildAt(i);
    if (child->IsStackingContextNode()) {
      if (child->ZIndex() != 0) {
        child->element_container()->MoveContainers(
            child->element_container()->parent(), parent);
      }
    } else {
      MoveZChildrenRecursively(child, parent);
    }
  }
}

void ElementContainer::StyleChanged() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ElementContainer::StyleChanged");
  props_changed_ = true;
  if (element()->GetEnableZIndex()) {
    ZIndexChanged();
  }
  if (element()->GetEnableFixedNew()) {
    PositionFixedChanged();
  }
}

void ElementContainer::ZIndexChanged() {
  if (!parent() || !element()->parent() || element()->IsLayoutOnly()) return;
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ElementContainer::ZIndexChanged");
  auto* element_parent = element()->parent();
  bool is_stacking_context = IsStackingContextNode();
  auto* parent_stacking_context = parent()->EnclosingStackingContextNode();
  auto z = ZIndex();
  // The stacking context changed, need to move the z-index children
  if (was_stacking_context_ != is_stacking_context) {
    ElementContainer* new_parent =
        is_stacking_context ? this : parent_stacking_context;
    // The z-index elements may add to another stacking context
    MoveZChildrenRecursively(element(), new_parent);
    if (was_stacking_context_) {
      element_manager()->RemoveDirtyContext(this);
    }
    element()->MarkLayoutDirty();
    element()->MarkSubtreeNeedUpdate();
  }
  // If the state of z-index is 0 has changed, need to remount
  // Choose the parent container in the attach function
  if ((z == 0 && old_index_ != 0) || (old_index_ == 0 && z != 0)) {
    RemoveFromParent(true);
    // Use the parent of element to find the ui parent
    element_parent->element_container()->AttachChildToTargetContainer(
        element(), element()->next_render_sibling());
    parent_stacking_context->MarkDirty();
  } else if (old_index_ != z) {  // Just mark the stacking context is dirty
    parent_stacking_context->MarkDirty();
  }
  old_index_ = z;
  was_stacking_context_ = is_stacking_context;
}

int ElementContainer::ZIndex() const { return element_->ZIndex(); }

void ElementContainer::MarkDirty() {
  if (dirty_) return;
  dirty_ = true;
  has_z_child_ = true;
  element_manager()->InsertDirtyContext(this);
}

void ElementContainer::UpdateZIndexList() {
  if (!dirty_ || (element_ && element_->is_list() &&
                  element_->DisableListPlatformImplementation())) {
    return;
  }
  dirty_ = false;
  negative_z_children_.clear();
  decltype(this->negative_z_children_) z_list;
  for (const auto& child : children_) {
    if (child->ZIndex() != 0 || child->IsSticky()) {
      z_list.push_back(child);
    }
  }

  if (z_list.empty()) return;

  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ElementContainer::UpdateZIndexList");
  std::stable_sort(z_list.begin(), z_list.end(),
                   [](const auto& first, const auto& second) {
                     return first->ZIndex() < second->ZIndex();
                   });

  // Doesn't insert to dirty list again
  SetNeedUpdate(false);
  for (const auto& child : z_list) {
    // Append to the front of the children if the z-index is negative
    if (child->ZIndex() < 0) {
      AddChild(child, 0);
      negative_z_children_.push_back(child);
    } else {
      // Append to the end of the children
      AddChild(child, -1);
    }
  }
  SetNeedUpdate(true);
}

ElementManager* ElementContainer::element_manager() {
  return element()->element_manager();
}

bool ElementContainer::IsStackingContextNode() {
  return element()->IsStackingContextNode();
}

bool ElementContainer::IsSticky() { return element()->is_sticky(); }

//========helper function for get index for fiber ========
// static
std::pair<ElementContainer*, int>
ElementContainer::FindParentAndIndexForChildForFiber(Element* parent,
                                                     Element* child,
                                                     Element* ref) {
  auto* real_parent = parent;
  while (real_parent && real_parent->IsLayoutOnly()) {
    real_parent = real_parent->render_parent();
  }
  if (!real_parent) {
    return {nullptr, -1};
  }

  // We can skip index calculation if the target parent doesn't have any child
  // need adjust z order. And dirty_ context will sort its children. We don't
  // need to calculate the index here.
  bool should_skip_index_calculation =
      ((!real_parent->element_container()->HasZChild()) ||
       real_parent->element_container()->dirty_) &&
      !ref;

  int index = 0;
  if (should_skip_index_calculation) {
    index = real_parent->element_container()->none_layout_only_children_size_;
  } else {
    // insert to the middle, child is already inserted in Element, just use
    // child to get index
    index = GetUIIndexForChildForFiber(parent, child);
    while (parent->IsLayoutOnly()) {
      auto* up_parent = parent->render_parent();
      if (!up_parent) {
        return {nullptr, -1};
      }
      index += GetUIIndexForChildForFiber(up_parent, parent);
      parent = up_parent;
    }
  }

  return {real_parent->element_container(), index};
}

// static
int ElementContainer::GetUIIndexForChildForFiber(Element* parent,
                                                 Element* child) {
  auto* node = parent->first_render_child();
  int index = 0;
  bool found = false;

  while (node) {
    if (child == node) {
      found = true;
      break;
    }
    if (node->ZIndex() != 0 || node->IsNewFixed()) {
      node = node->next_render_sibling();
      continue;
    }
    index += (node->IsLayoutOnly() ? GetUIChildrenCountForFiber(node) : 1);
    node = node->next_render_sibling();
  }
  if (!found) {
    LOGE("element can not found:");
    DCHECK(false);
  }
  return index;
}

// static
int ElementContainer::GetUIChildrenCountForFiber(Element* parent) {
  int ret = 0;
  auto* child = parent->first_render_child();
  while (child) {
    if (child->IsLayoutOnly()) {
      ret += GetUIChildrenCountForFiber(child);
    } else if (child->ZIndex() == 0 && !child->IsNewFixed()) {
      ret++;
    }
    child = child->next_render_sibling();
  }
  return ret;
}

// When the position changes to fixed or changes from fixed to another, the node
// needs to be remounted to the correct position.
void ElementContainer::PositionFixedChanged() {
  if (!parent() || !element()->parent()) {
    return;
  }
  bool is_position_fixed = element()->is_fixed();
  if (was_position_fixed_ != is_position_fixed) {
    RemoveFromParent(true);
    element()->parent()->element_container()->AttachChildToTargetContainer(
        element());
  }
  was_position_fixed_ = is_position_fixed;
}

static Element const* FindCommonAncestor(Element const** left_mark,
                                         Element const** right_mark) {
  std::deque<const Element*> left_ancestors;
  std::deque<const Element*> right_ancestors;
  Element const* left = *left_mark;
  Element const* right = *right_mark;
  while (left != nullptr) {
    left_ancestors.emplace_front(left);
    left = left->parent();
  }
  while (right != nullptr) {
    right_ancestors.emplace_front(right);
    right = right->parent();
  }
  auto it_l = left_ancestors.begin();
  auto it_r = left_ancestors.begin();
  while (it_l != left_ancestors.end() && it_r != right_ancestors.end() &&
         *it_l == *it_r) {
    it_l++;
    it_r++;
  }
  if (it_l == left_ancestors.end() || it_r == right_ancestors.end()) {
    return nullptr;
  }
  *left_mark = *it_l;
  *right_mark = *it_r;
  return (*left_mark)->parent();
}

static int CompareElementOrder(Element* left, Element* right) {
  if (left == right) {
    return 0;
  }
  // left is right's ancestor
  const Element* temp = right;
  while (temp != nullptr) {
    if (temp->parent() == left) {
      // left is smaller
      return -1;
    }
    temp = temp->parent();
  }
  // right is left's ancestor
  temp = left;
  while (temp != nullptr) {
    if (temp->parent() == right) {
      return 1;
    }
    temp = temp->parent();
  }
  // find the common ancestor
  Element const* left_mark = left;
  Element const* right_mark = right;
  Element const* common_ancestor = FindCommonAncestor(&left_mark, &right_mark);
  // compare the order in the common ancestor
  if (common_ancestor) {
    int i = 0;
    size_t count = const_cast<Element*>(common_ancestor)->GetChildCount();
    Element* child = nullptr;
    while (i < static_cast<int>(count)) {
      child = const_cast<Element*>(common_ancestor)->GetChildAt(i);
      if (child == right_mark) {
        // left is after right
        return 1;
      } else if (child == left_mark) {
        // left is before right
        return -1;
      }
      i++;
    }
    return 0;
  }
  return 0;
}
}  // namespace tasm
}  // namespace lynx
