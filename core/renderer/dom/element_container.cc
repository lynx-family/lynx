// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/element_container.h"

#include <algorithm>
#include <cstddef>
#include <deque>

#include "base/trace/native/trace_event.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/trace/renderer_trace_event_def.h"
#include "core/renderer/utils/prop_bundle_style_writer.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

ElementContainer::ElementContainer(Element* element)
    : BaseElementContainer(element) {}

ElementContainer::~ElementContainer() {
  if (!element()->will_destroy()) {
    if (NeedSortZChild()) {
      element_manager()->RemoveDirtyContext(this);
    } else if (was_stacking_context_) {
      // FIXME(linxs): to remove below code in next version!
      element_manager()->RemoveDirtyContext(this);
    }
  }
  // Remove self from parent's children.
  if (parent()) {
    auto it = std::find(element_container_parent()->children_.begin(),
                        element_container_parent()->children_.end(), this);
    if (it != element_container_parent()->children_.end())
      element_container_parent()->children_.erase(it);
    set_parent(nullptr);
  }
  // Set children's parent to null.
  for (auto child : children_) {
    if (child) {
      child->set_parent(nullptr);
    }
  }
}

void ElementContainer::CalcUIIndexForFixedNew(ElementContainer* child,
                                              int& index) {
  if (child->element()->IsNewFixed() && child->ZIndex() == 0) {
    int fixed_node_offset = 0;
    for (const ElementContainer* el : children_) {
      if (!el->element()->IsLayoutOnly() && el->ZIndex() == 0 &&
          !el->element()->is_fixed()) {
        fixed_node_offset++;
      }
    }

    auto it = element_manager()->fixed_node_list_.end();
    size_t left = 0, right = element_manager()->fixed_node_list_.size();
    size_t mid = right;
    while (left < right) {
      mid = left + (right - left) / 2;
      it = element_manager()->fixed_node_list_.begin();
      std::advance(it, mid);
      if (CompareElementOrder(child->element(), (*it)->element()) > 0) {
        left = mid + 1;
        std::advance(it, 1);
      } else {
        right = mid;
      }
    }
    element_manager()->fixed_node_list_.insert(it, child);

    int index_of_fixed_z0 = static_cast<int>(left);
    index = static_cast<int>(negative_z_children_.size()) + fixed_node_offset +
            index_of_fixed_z0;
  }
}

void ElementContainer::CalcUIIndexForFixedUnified(ElementContainer* child,
                                                  int& index) {
  if (child->element()->IsFixedUnified() && child->ZIndex() == 0) {
    int fixed_node_offset = 0;
    bool is_fiber_arch = child->element()->IsFiberArch();
    for (const ElementContainer* el : children_) {
      bool compare_result =
          is_fiber_arch
              ? (el->global_insertion_order_ < child->global_insertion_order_)
              : (el->element()->GlobalInsertionOrder() <
                 child->element()->GlobalInsertionOrder());
      if (!el->element()->IsLayoutOnly() && el->ZIndex() == 0 &&
          compare_result) {
        fixed_node_offset++;
      }
    }
    element_manager()->fixed_node_list_.emplace_back(child);
    index = fixed_node_offset;
  }
}

void ElementContainer::CalcUIIndexForFixed(ElementContainer* child,
                                           int& index) {
  if (child->element()->IsNewFixed()) {
    CalcUIIndexForFixedNew(child, index);
  } else if (child->element()->IsFixedUnified()) {
    CalcUIIndexForFixedUnified(child, index);
  }
}

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

  CalcUIIndexForFixed(child, index);

  child->set_parent(this);
  if ((child->ZIndex() != 0 || child->IsSticky()) && need_update_) {
    MarkDirtyState(kNeedSortZChild);
  }
  if (!child->element()->IsLayoutOnly()) {
    painting_context()->InsertPaintingNode(id(), child->id(), index);
  }
}

void ElementContainer::RemoveChild(ElementContainer* child) {
  auto it = std::find(children_.begin(), children_.end(), child);
  if (it != children_.end()) {
    children_.erase(it);
    if (child->element()->ZIndex() < 0) {
      auto z_it = std::find(negative_z_children_.begin(),
                            negative_z_children_.end(), child);
      if (z_it != negative_z_children_.end()) {
        negative_z_children_.erase(z_it);
      }
    }
    if ((child->element()->IsFixedNewOrUnified() ||
         child->was_position_fixed_) &&
        child->ZIndex() == 0) {
      element()->element_manager()->fixed_node_list_.remove(child);
    }
    if (!child->element()->IsLayoutOnly()) {
      none_layout_only_children_size_--;
    }
  }
  child->set_parent(nullptr);
  if (!need_update_) {
    return;
  }
  if (child->ZIndex() != 0 ||
      (element_manager()->GetEnableNewSticky() && child->IsSticky())) {
    // The stacking context need update
    MarkDirtyState(kNeedSortZChild);
  }
}

void ElementContainer::RemoveFromParent(bool is_move) {
  if (!parent()) return;
  if (!element()->IsLayoutOnly()) {
    painting_context()->RemovePaintingNode(parent()->id(), id(), 0, is_move);
  } else {
    // Layout only node remove children from parent recursively.
    // fiber element;
    auto* child = static_cast<FiberElement*>(element())->first_render_child();
    while (child) {
      child->element_container_impl()->RemoveFromParent(is_move);
      child = child->next_render_sibling();
    }
  }
  element_container_parent()->RemoveChild(this);
}

void ElementContainer::Destroy() {
  // Layout only destroy recursively, the z-index child may has been destroyed
  if (!element()->IsLayoutOnly()) {
    painting_context()->DestroyPaintingNode(parent() ? parent()->id() : -1,
                                            id(), 0);
  }
  if (element_container_parent()) {
    element_container_parent()->RemoveChild(this);
  }
}

void ElementContainer::RemoveElementContainerAccordingToElement(Element* child,
                                                                bool destroy) {
  if (child == nullptr || child->element_container_impl() == nullptr) {
    return;
  }

  child->element_container_impl()->RemoveSelf(destroy);
}

void ElementContainer::RemoveSelf(bool destroy) {
  if (parent() == nullptr) {
    return;
  }

  if (destroy) {
    Destroy();
  } else {
    // When remove self from parent element container, attach state should be
    // considered. If internal element is still attached, it should be
    // considered as a move operation.
    bool is_move = element() == nullptr ? false : element()->IsAttached();
    RemoveFromParent(is_move);
  }
}

void ElementContainer::InsertSelf() {
  if (!parent() && element()->parent()) {
    Element* insertion_parent = element()->parent();
    if (element()->is_fixed() && !element()->IsFixedNewOrUnified() &&
        element_manager()->FixOldFixedInsertSelfUseRenderParent() &&
        element()->render_parent() != nullptr) {
      // For old fixed, the logical parent can differ from the UI parent.
      // When a layout-only fixed node transitions to a native view, it should
      // be re-inserted according to the render tree, otherwise it may be
      // attached back to its logical wrapper instead of the page root.
      // Note that old fixed is only a temporary compatibility state. Business
      // should migrate to the new fixed behavior as soon as possible instead of
      // relying on old fixed + layout-only transition semantics long-term.
      insertion_parent = element()->render_parent();
    }
    insertion_parent->element_container_impl()
        ->InsertElementContainerAccordingToElement(
            element(), element()->next_render_sibling());
  }
}

std::pair<ElementContainer*, int> ElementContainer::FindParentForChild(
    Element* child) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_CONTAINER_FIND_PARENT);
  Element* node = element();
  size_t ui_index = element()->GetUIIndexForChild(child);
  while (node->IsLayoutOnly()) {
    Element* parent = node->parent();
    if (!parent) {
      return {nullptr, -1};
    }
    ui_index += static_cast<int>(parent->GetUIIndexForChild(node));
    node = parent;
  }
  return {node->element_container_impl(), ui_index};
}

void ElementContainer::AttachChildToTargetContainerRecursive(
    ElementContainer* parent, Element* child, int& index) {
  if (child->ZIndex() != 0 || child->IsFixedNewOrUnified()) {
    if (child->IsFixedNewOrUnified()) {
      // fixed node should attach to page root.
      parent = parent->element()
                   ->element_manager()
                   ->root()
                   ->element_container_impl();
    }
    auto ui_parent =
        parent->EnclosingStackingContextNode()->CastToElementContainer();
    ui_parent->AddChild(child->element_container_impl(), -1);
    return;
  }
  if (element_manager()->GetEnableNewSticky() && child->is_sticky() && parent) {
    BaseElementContainer* scroll_container =
        parent->EnclosingScrollContainerNode();
    if (scroll_container) {
      scroll_container->CastToElementContainer()->AddChild(
          child->element_container_impl(), -1);
      return;
    }
  }
  // In the case that a scroll-view has a child, which is a wrapper element and
  // has a layout-only child view. If add wrapper element to scroll-view,
  // wrapper element should not create native view, but if add the layout-only
  // child to scroll-view, the child should create native view.
  if (!parent->element()->CanHasLayoutOnlyChildren() && child->IsLayoutOnly() &&
      !(static_cast<FiberElement*>(child)->is_wrapper()) &&
      !child->is_virtual()) {
    child->TransitionToNativeView();
  }
  parent->AddChild(child->element_container_impl(), index);
  if (!child->IsLayoutOnly()) {
    ++index;
    return;
  }
  // Layout only node should add subtree to parent recursively.
  auto* grand = static_cast<FiberElement*>(child)->first_render_child();
  while (grand) {
    AttachChildToTargetContainerRecursive(parent, grand, index);
    grand = grand->next_render_sibling();
  }
}

void ElementContainer::ReInsertChildForLayoutOnlyTransition(Element* child,
                                                            int& index) {
  if (!child->element_container_impl()) {
    LOGE("re-insert the wrong element!");
    return;
  }
  AttachChildToTargetContainerRecursive(this, child, index);

  child->MarkFrameChanged();
  child->element_container_impl()->UpdateLayout(child->left(), child->top(),
                                                true);
}

bool ElementContainer::HasUIPrimitive() const {
  return element() && !element()->CanBeLayoutOnly();
}

void ElementContainer::InsertElementContainerAccordingToElement(Element* child,
                                                                Element* ref) {
  if (child->IsFixedNewOrUnified()) {
    element_manager()->root()->element_container_impl()->AddChild(
        child->element_container_impl(), -1);
    return;
  }
  if (child->ZIndex() != 0) {
    auto* enclosing_stacking_node =
        EnclosingStackingContextNode()->CastToElementContainer();
    enclosing_stacking_node->AddChild(child->element_container_impl(), -1);
    return;
  }
  if (element_manager()->GetEnableNewSticky() && child->is_sticky()) {
    BaseElementContainer* scroll_container = EnclosingScrollContainerNode();
    if (scroll_container) {
      scroll_container->CastToElementContainer()->AddChild(
          child->element_container_impl(), -1);
      return;
    }
  }
  std::pair<ElementContainer*, int> result;
  result = FindParentAndIndexForChildForFiber(element(), child, ref);
  if (result.first) {
    int index = result.second;
    AttachChildToTargetContainerRecursive(result.first, child, index);
  }
}

ElementContainer::PlatformLayout ElementContainer::CalculatePlatformLayout(
    float left, float top) const {
  if (element()->IsFixedNewOrUnified()) {
    // new fixed node's parent should always be root node. And layout params are
    // calculated by starlight.
    left = element()->left();
    top = element()->top();
  } else if (element()->ZIndex() != 0 ||
             (element_manager()->GetEnableNewSticky() && IsSticky())) {
    // The z-index or sticky child's parent may be different from ui parent,
    // and need to add the offset of the position
    left = element()->left();
    top = element()->top();
    auto* ui_parent = parent();
    auto* parent = element()->render_parent();
    while (parent && ui_parent && ui_parent->element() != parent) {
      left += parent->left();
      top += parent->top();
      parent = parent->parent();
    }
  }
  PlatformLayout layout;
  layout.left = left;
  layout.top = top;
  layout.child_offset_left = left;
  layout.child_offset_top = top;
  if (!element()->IsLayoutOnly()) {
    layout.child_offset_left = 0;
    layout.child_offset_top = 0;
  }
  return layout;
}

ElementContainer::PlatformLayout
ElementContainer::CalculateCurrentPlatformLayout() const {
  float left = element()->left();
  float top = element()->top();
  if (!element()->IsFixedNewOrUnified() && element()->ZIndex() == 0) {
    auto* ui_parent = parent();
    auto* render_parent = element()->render_parent();
    while (render_parent && ui_parent &&
           ui_parent->element() != render_parent) {
      if (render_parent->IsLayoutOnly()) {
        left += render_parent->left();
        top += render_parent->top();
      }
      render_parent = render_parent->parent();
    }
  }
  return CalculatePlatformLayout(left, top);
}

// Calculate position for element and update it to impl layer.
void ElementContainer::UpdateLayout(float left, float top,
                                    bool transition_view) {
  // Self is updated or self position is changed because of parent's frame
  // changing.
  auto layout = CalculatePlatformLayout(left, top);
  left = layout.left;
  top = layout.top;
  bool need_update_impl = (!transition_view || is_layouted_) &&
                          (element()->frame_changed() || left != last_left_ ||
                           top != last_top_ || ShouldUpdateStickyRange());
  last_left_ = left;
  last_top_ = top;
  if (!element()->IsLayoutOnly()) {
    if (need_update_impl) {  // Update to impl layer
      painting_context()->UpdateLayout(
          element()->impl_id(), left, top, element()->width(),
          element()->height(), element()->paddings().data(),
          element()->margins().data(), element()->borders().data(), nullptr,
          GetStickyPositionIfNeeded(), element()->max_height(),
          element()->NodeIndex());
    }
    if (need_update_impl || props_changed_) {
      painting_context()->OnNodeReady(element()->impl_id());
      props_changed_ = false;
    }
  }

  // If the element is list, and use c++ implementation, we will block it's
  // children invoking UpdateLayout() to flush layout info to platform, because
  // the left and top's value of child element is incorrect.
  if (!element()->DisableListPlatformImplementation()) {
    // Layout children
    auto* child = static_cast<FiberElement*>(element())->first_render_child();
    while (child) {
      if (child->element_container_impl()) {
        child->element_container_impl()->UpdateLayout(
            child->left() + layout.child_offset_left,
            child->top() + layout.child_offset_top, transition_view);
      }
      child = child->next_render_sibling();
    }
  }
  element()->MarkUpdated();

  is_layouted_ = true;
}

const float* ElementContainer::GetStickyPositionIfNeeded() {
  bool is_sticky = IsSticky();
  if (!is_sticky) {
    return nullptr;
  }
  if (!element_manager()->GetEnableNewSticky()) {
    return element()->sticky_positions()->data();
  }
  // For the new sticky path, position: sticky without left/top/right/bottom
  // should not flush sticky info to platform.
  if (is_sticky && element()->sticky_positions().has_value()) {
    /*
     * The Element render tree describes the logical parent-child relationship,
     * while the ElementContainer tree describes how native platform views are
     * actually mounted. These two trees can differ.
     * Therefore, the sticky range cannot be calculated only from the render
     * parent chain. We need to skip wrapper elements to find the real parent
     * element, then accumulate positions through the ElementContainer chain for
     * both the sticky node and its real parent.
     *
     * case 1: the sticky node is a direct child of the scroll element. The
     * Element tree and ElementContainer tree have the same visible hierarchy.
     *
     *   Element tree:
     *     ScrollElement A (is stacking context)
     *       ViewElement B
     *       ViewElement C
     *       StickyElement D
     *
     *   ElementContainer tree:
     *     ScrollContainer A
     *       ViewContainer B
     *       ViewContainer C
     *       StickyContainer D
     *
     * case 2: wrappers are skipped and StickyElement E's real parent is
     * ViewElement B, but StickyContainer E is mounted under ScrollContainer A.
     *
     *   Element tree:
     *     ScrollElement A (is stacking context)
     *       ViewElement B
     *         WrapperElement C
     *           WrapperElement D
     *             StickyElement E
     *
     *   ElementContainer tree:
     *     ScrollContainer A
     *       ViewContainer B
     *       StickyContainer E
     *
     * case 3: wrappers are also skipped, but ViewElement B is a stacking
     * context, so StickyContainer E stays under ViewContainer B.
     *
     *   Element tree:
     *     ScrollElement A (is stacking context)
     *       ViewElement B (is stacking context)
     *         WrapperElement C
     *           WrapperElement D
     *             StickyElement E
     *
     *   ElementContainer tree:
     *     ScrollContainer A
     *       ViewContainer B
     *         StickyContainer E
     */
    float self_relative_left = 0.f;
    float self_relative_top = 0.f;
    float parent_relative_left = 0.f;
    float parent_relative_top = 0.f;
    bool valid_self_position = false;
    bool valid_parent_position = false;
    // Find the sticky scroller from the ElementContainer tree, because the
    // sticky range is calculated in native mounting coordinates. The Element
    // tree may still have a scroll-view ancestor after the sticky node is
    // reparented, for example by fixed or stacking-context behavior.
    BaseElementContainer* parent_container = parent();
    BaseElementContainer* sticky_scroller_container = nullptr;
    while (parent_container && parent_container->element()) {
      if (parent_container->element()->is_scroll_view()) {
        sticky_scroller_container = parent_container;
        break;
      }
      parent_container = parent_container->parent();
    }
    // Find the first non-wrapper render parent from the Element tree.
    Element* real_parent_element = nullptr;
    if (element()->render_parent()) {
      real_parent_element =
          element()->render_parent()->FindFirstNonWrapperRenderAncestor();
    }
    if (sticky_scroller_container && real_parent_element) {
      BaseElementContainer* current_container = this;
      while (current_container &&
             current_container != sticky_scroller_container) {
        self_relative_left +=
            current_container->CastToElementContainer()->last_left_;
        self_relative_top +=
            current_container->CastToElementContainer()->last_top_;
        current_container = current_container->parent();
      }
      valid_self_position = current_container == sticky_scroller_container;
      if (real_parent_element &&
          (current_container = real_parent_element->element_container_impl())) {
        while (current_container &&
               current_container != sticky_scroller_container) {
          parent_relative_left +=
              current_container->CastToElementContainer()->last_left_;
          parent_relative_top +=
              current_container->CastToElementContainer()->last_top_;
          current_container = current_container->parent();
        }
        valid_parent_position = current_container == sticky_scroller_container;
      }
    }
    if (!valid_self_position || !valid_parent_position) {
      // New sticky needs reliable self and parent positions relative to the
      // sticky scroller. If either position cannot be resolved, do not flush
      // stale range data; platform will clear the sticky state.
      return nullptr;
    }
    if (real_parent_element != sticky_scroller_container->element()) {
      (*element()->sticky_positions())[4] = real_parent_element->width();
      (*element()->sticky_positions())[5] = real_parent_element->height();
    } else {
      (*element()->sticky_positions())[4] = -1.f;
      (*element()->sticky_positions())[5] = -1.f;
    }
    (*element()->sticky_positions())[6] = self_relative_left;
    (*element()->sticky_positions())[7] = self_relative_top;
    (*element()->sticky_positions())[8] = parent_relative_left;
    (*element()->sticky_positions())[9] = parent_relative_top;
    return element()->sticky_positions()->data();
  }
  return nullptr;
}

bool ElementContainer::ShouldUpdateStickyRange() {
  // A sticky node needs to refresh its platform range when the render parent's
  // frame changes. The range should use render_parent size regardless of
  // whether render_parent is layout-only.
  if (element_manager()->GetEnableNewSticky() && IsSticky() &&
      element()->render_parent()) {
    Element* real_parent =
        element()->render_parent()->FindFirstNonWrapperRenderAncestor();
    return real_parent && real_parent->frame_changed();
  }
  return false;
}

void ElementContainer::UpdateLayoutWithoutChange() {
  if (props_changed_) {
    painting_context()->OnNodeReady(element()->impl_id());
    props_changed_ = false;
  }
  auto* child = static_cast<FiberElement*>(element())->first_render_child();
  while (child) {
    child->element_container_impl()->UpdateLayoutWithoutChange();
    child = child->next_render_sibling();
  }
}

void ElementContainer::TransitionToNativeView(
    fml::RefPtr<PropBundle> prop_bundle) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_CONTAINER_TRANSITION);
  if (prop_bundle == nullptr) {
    return;
  }

  element()->element_manager()->DecreaseLayoutOnlyElementCount();
  element()->element_manager()->IncreaseLayoutOnlyTransitionCount();

  LOGI("[ElementContainer] TransitionToNativeView tag:"
       << element()->GetTag().str() << ",id:" << element()->impl_id());

  // Remove from current parent.
  RemoveFromParent(true);

  // Create LynxUI in impl layer.
  element()->set_is_layout_only(false);

  // Push painting related props into prop_bundle.
  PropBundleStyleWriter::PushStyleToBundle(
      prop_bundle.get(), kPropertyIDOverflow, element()->computed_css_style());

  painting_context()->CreatePaintingNode(
      element()->impl_id(), element()->GetPlatformNodeTag().str(), prop_bundle,
      element()->TendToFlatten(), element()->NeedCreateNodeAsync(),
      element()->NodeIndex());

  // Insert children to this.
  InsertSelf();

  // Mark need update layout value to impl layer.
  element()->MarkFrameChanged();

  UpdateLayout(last_left_, last_top_, true);

  int ui_index = 0;
  auto* child = static_cast<FiberElement*>(element())->first_render_child();
  while (child) {
    ReInsertChildForLayoutOnlyTransition(child, ui_index);
    child = child->next_render_sibling();
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

void ElementContainer::MoveZChildrenRecursively(Element* element,
                                                ElementContainer* parent) {
  for (size_t i = 0; i < element->GetChildCount(); i++) {
    auto* child = element->GetChildAt(i);
    if (child->IsStackingContextNode()) {
      if (child->ZIndex() != 0 && !child->IsFixedNewOrUnified()) {
        child->element_container_impl()->MoveContainers(
            child->element_container_impl()->element_container_parent(),
            parent);
      }
    } else {
      MoveZChildrenRecursively(child, parent);
    }
  }
}

void ElementContainer::StyleChanged() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_CONTAINER_STYLE_CHANGED);
  props_changed_ = true;
  if (element()->GetEnableZIndex()) {
    ZIndexChanged();
  }
  if (element()->GetEnableFixedNew() || element()->IsFixedUnifiedEnabled()) {
    PositionFixedChanged();
  }
  if (element_manager()->GetEnableNewSticky()) {
    StickyChanged();
  }
}

void ElementContainer::ZIndexChanged() {
  if (!parent() || !element()->parent() || element()->IsLayoutOnly()) return;
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_CONTAINER_Z_INDEX_CHANGED);
  Element* element_parent = element()->render_parent();
  bool is_stacking_context = IsStackingContextNode();
  auto* parent_stacking_context = element_container_parent()
                                      ->EnclosingStackingContextNode()
                                      ->CastToElementContainer();
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
  if ((z == 0 && old_z_index() != 0) || (old_z_index() == 0 && z != 0)) {
    RemoveFromParent(true);
    // Use the parent of element to find the ui parent
    element_parent->element_container_impl()
        ->InsertElementContainerAccordingToElement(
            element(), element()->next_render_sibling());
    parent_stacking_context->MarkDirtyState(kNeedSortZChild);
  } else if (old_z_index() != z) {  // Just mark the stacking context is dirty
    parent_stacking_context->MarkDirtyState(kNeedSortZChild);
  }
  set_old_z_index(z);
  set_was_stacking_context(is_stacking_context);
}

int ElementContainer::ZIndex() const { return element()->ZIndex(); }

void ElementContainer::UpdateZIndexList() {
  if (!NeedSortZChild() || (element() && element()->is_list() &&
                            element()->DisableListPlatformImplementation())) {
    return;
  }
  ResetDirtyState(kNeedSortZChild);
  negative_z_children_.clear();
  decltype(this->negative_z_children_) z_list;
  for (const auto& child : children_) {
    if (child->ZIndex() != 0 || child->IsSticky()) {
      z_list.push_back(child);
    }
  }

  if (z_list.empty()) return;

  TRACE_EVENT(LYNX_TRACE_CATEGORY, ELEMENT_CONTAINER_UPDATE_Z_INDEX_LIST);
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

bool ElementContainer::IsStackingContextNode() {
  return element()->IsStackingContextNode();
}

void ElementContainer::CreatePaintingNode(
    bool is_flatten, const fml::RefPtr<PropBundle>& painting_data) {
  set_was_stacking_context(IsStackingContextNode());
  set_was_position_fixed(element()->IsFixedNewOrUnified());
  set_old_z_index(ZIndex());
  was_sticky_ = IsSticky();
  if (element()->IsLayoutOnly()) {
    return;
  }
  painting_context()->CreatePaintingNode(
      element()->impl_id(), element()->GetPlatformNodeTag().str(),
      painting_data, is_flatten, element()->NeedCreateNodeAsync(),
      element()->NodeIndex());
}

void ElementContainer::UpdatePaintingNode(
    bool tend_to_flatten, const fml::RefPtr<PropBundle>& painting_data) {
  painting_context()->UpdatePaintingNode(element()->impl_id(), tend_to_flatten,
                                         painting_data);
}

void ElementContainer::InsertListItemPaintingNode(int32_t child_id) {
  painting_context()->InsertListItemPaintingNode(element()->impl_id(),
                                                 child_id);
}

void ElementContainer::RemoveListItemPaintingNode(int32_t child_id) {
  painting_context()->RemoveListItemPaintingNode(element()->impl_id(),
                                                 child_id);
}

void ElementContainer::UpdateContentOffsetForListContainer(
    float content_size, float delta_x, float delta_y,
    bool is_init_scroll_offset, bool from_layout) {
  painting_context()->UpdateContentOffsetForListContainer(
      element()->impl_id(), content_size, delta_x, delta_y,
      is_init_scroll_offset, from_layout);
}

bool ElementContainer::IsSticky() const { return element()->is_sticky(); }

void ElementContainer::StickyChanged() {
  if (!parent() || !element()->parent()) {
    return;
  }
  bool is_sticky = IsSticky();
  if (was_sticky_ != is_sticky) {
    RemoveFromParent(true);
    element()
        ->parent()
        ->element_container_impl()
        ->InsertElementContainerAccordingToElement(
            element(), element()->next_render_sibling());
    BaseElementContainer* scroll_container =
        element_container_parent()->EnclosingScrollContainerNode();
    if (scroll_container) {
      scroll_container->MarkDirtyState(kNeedSortZChild);
    }
  }
  was_sticky_ = is_sticky;
}

//========helper function for get index for fiber ========
// static
std::pair<ElementContainer*, int>
ElementContainer::FindParentAndIndexForChildForFiber(Element* parent,
                                                     Element* child,
                                                     Element* ref) {
  auto* real_parent = parent;
  // Traverse up the render tree to find the nearest ancestor that has a
  // corresponding UI node. Layout-only elements do not have UI nodes, so we
  // must attach to an ancestor. However, if we encounter a layout-only element
  // that is also fixed (unified behavior), it behaves as if attached to the
  // root, so we handle it specially.
  while (real_parent && real_parent->IsLayoutOnly() &&
         !real_parent->IsFixedUnifiedOnly()) {
    // For element container tree a quick check for determine if need to append
    // the container to the end(check ref is null) ref is null, find the first
    // none-wrapper ancestor's next sibling as ref! ref_node: null means to
    // append to the real parent!!
    // FIXME(linxs): try to optimize the logic for wrapper element
    if (ref == nullptr) {
      ref = real_parent->next_render_sibling();
    }
    real_parent = real_parent->render_parent();
  }

  if (real_parent && real_parent->IsLayoutOnly() &&
      real_parent->IsFixedUnifiedOnly()) {
    real_parent = real_parent->element_manager()->root();
  }

  if (!real_parent) {
    return {nullptr, -1};
  }

  // We can skip index calculation if the target parent doesn't have any child
  // need adjust z order. And dirty_ context will sort its children. We don't
  // need to calculate the index here.
  bool should_skip_index_calculation =
      (!real_parent->element_container_impl()->has_z_child()) && !ref;
  ;

  int index = 0;
  if (should_skip_index_calculation) {
    index =
        real_parent->element_container_impl()->none_layout_only_children_size_;
  } else {
    // Calculate the cumulative UI index.
    // Since the direct parent might be a layout-only element (which is skipped
    // in the UI hierarchy), the child's actual UI index is the sum of its
    // relative index in the layout-only parent plus that parent's relative
    // index in the next ancestor, recursively up to the 'real_parent'.

    // insert to the middle, child is already inserted in Element, just use
    // child to get index
    index = GetUIIndexForChildForFiber(parent, child);
    while (parent->IsLayoutOnly() && !parent->IsFixedUnifiedOnly()) {
      auto* up_parent = parent->render_parent();
      if (!up_parent) {
        return {nullptr, -1};
      }
      index += GetUIIndexForChildForFiber(up_parent, parent);
      parent = up_parent;
    }
    if (parent->IsLayoutOnly() && parent->IsFixedUnifiedOnly()) {
      index += GetUIIndexForChildForFiber(real_parent, parent);
    }
  }

  return {real_parent->element_container_impl(), index};
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
    if (node->ZIndex() != 0 || node->IsFixedNewOrUnified()) {
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
    } else if (child->ZIndex() == 0 && !child->IsFixedNewOrUnified()) {
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
    element()
        ->parent()
        ->element_container_impl()
        ->InsertElementContainerAccordingToElement(element());
  }
  was_position_fixed_ = is_position_fixed;
}
}  // namespace tasm
}  // namespace lynx
