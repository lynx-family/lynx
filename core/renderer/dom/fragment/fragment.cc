// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/fragment.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/include/closure.h"
#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/fragment_behavior.h"
#include "core/renderer/dom/fragment/rounded_rectangle.h"

namespace lynx {
namespace tasm {

Fragment::Fragment(Element* element) : BaseElementContainer(element) {}

Fragment* Fragment::fragment_parent() const {
  return static_cast<Fragment*>(parent());
}

void Fragment::CreateLayerIfNeeded() {
  // TODO(zhongyr): abstract one behavior for layerize.
  if ((!element()->TendToFlatten() && !has_platform_renderer_ && behavior_ &&
       !element()->IsShadowNodeVirtual()) ||
      element()->is_page()) {
    behavior_->CreatePlatformRenderer();
    has_platform_renderer_ = true;
  }
}

void Fragment::StyleChanged() {
  if (element() == nullptr) {
    return;
  }

  // There are four cases for z-index:
  // z.0. z-index does not change.
  // z.1. The value of z-index changes, but it is not 0 before or after.
  // z.2. z-index was 0 before and is not 0 now.
  // z.3. z-index was not 0 before and is 0 now.

  // There are three cases for the fixed property:
  // f.0. The fixed property does not change.
  // f.1. The fixed property was true before and is not true now.
  // f.2. The fixed property was not true before and is true now.

  // There are three cases for stacking context changes:
  // s.0. The stacking context does not change.
  // s.1. The stacking context was true before and is not true now.
  // s.2. The stacking context was not true before and is true now.

  // In summary, there are 4 * 3 * 3 = 36 cases in total.
  // Enumerating all cases is very costly. We found that we can implement it as
  // follows:
  // 1. First, determine if the parent needs to be changed based on the current
  // z-index and fixed state, and then only move the element itself.
  // 2. Then, based on the current and previous stacking context states,
  // determine whether to move the descendant stacking context fragment.
  if (element()->is_fixed() != was_position_fixed() ||
      element()->ZIndex() != old_z_index()) {
    auto* target_parent = fragment_parent();

    set_was_position_fixed(element()->is_fixed());
    set_old_z_index(element()->ZIndex());

    if (was_position_fixed()) {
      // If it is a fixed element, the parent should be the root fragment.
      target_parent = element_manager()->root()->fragment_impl();
    } else if (old_z_index() != 0) {
      // If z-index is not 0, the parent should be the nearest stacking
      // context fragment.
      target_parent = EnclosingStackingContextNode()->CastToFragment();
    } else {
      // If it is not fixed and z-index is 0, the parent should be the
      // fragment corresponding to the element's parent.
      target_parent = element()->parent()->fragment_impl();
    }

    // If the parent has changed, the element needs to be moved.
    if (target_parent != fragment_parent()) {
      fragment_parent()->RemoveChild(this);

      Element* ref = nullptr;
      if (old_z_index() != 0) {
        if (element()->next_render_sibling() != nullptr) {
          ref = element()->next_render_sibling();
        }
        // If the child is not fixed and z-index is 0, insert it to the first
        // reliable sibling.
        while (ref != nullptr && !ref->fragment_impl()->IsReliableSibling()) {
          ref = ref->next_render_sibling();
        }
      }
      target_parent->AddChildBefore(
          this, ref != nullptr ? ref->fragment_impl() : nullptr);
    }

    Fragment* fragment_from_element_parent =
        element()->parent()->fragment_impl();
    if (old_z_index() == 0) {
      fragment_from_element_parent->z_children_.erase(this);
    } else {
      fragment_from_element_parent->z_children_.insert(this);
    }
    if (!was_position_fixed()) {
      fragment_from_element_parent->fixed_children_.erase(this);
    } else {
      fragment_from_element_parent->fixed_children_.insert(this);
    }
    set_fragment_from_element_parent(old_z_index() != 0 || was_position_fixed()
                                         ? fragment_from_element_parent
                                         : nullptr);
  }

  if (element()->IsStackingContextNode() != was_stacking_context()) {
    // If the element's stacking context state changed, we should move the
    // descendants stacking context fragment to correct parent.

    set_was_stacking_context(element()->IsStackingContextNode());
    Fragment* target_parent =
        was_stacking_context()
            ? this
            : EnclosingStackingContextNode()->CastToFragment();
    MoveDirectStackingChildren(target_parent, this);
  }
}

void Fragment::UpdateZIndexList() {
  // If the element is a list and disable list platform implementation,
  // we should not update z-index list.
  if (element() != nullptr && element()->is_list() &&
      element()->DisableListPlatformImplementation()) {
    return;
  }

  // If do not need sort z-index child and fixed child, we should not update
  // z-index list.
  if (!NeedSortZChild() && !NeedSortFixedChild()) {
    return;
  }

  // Sorts the children based on their stacking context.
  // The sorting order is as follows:
  // 1. Nodes with a negative z-index, sorted by z-index.
  // 2. Nodes with a z-index of 0 that are not fixed, maintaining their relative
  //    order.
  // 3. Nodes with a z-index of 0 that are fixed, sorted by CompareElementOrder.
  // 4. Nodes with a positive z-index, sorted by z-index.
  // If the order of children changes after sorting, NeedRedraw is marked.
  auto get_group = [](const Fragment* fragment) {
    int z_index = fragment->old_z_index();
    if (z_index < 0) {
      return 0;  // Group 0: negative z-index
    }
    if (z_index > 0) {
      return 3;  // Group 3: positive z-index
    }
    // z-index is 0
    if (fragment->was_position_fixed()) {
      return 2;  // Group 2: fixed
    }
    return 1;  // Group 1: not fixed
  };

  auto comparator = [&](const Fragment* a, const Fragment* b) {
    int group_a = get_group(a);
    int group_b = get_group(b);

    if (group_a != group_b) {
      return group_a < group_b;
    }

    // Same group, sort within the group
    switch (group_a) {
      case 0:  // negative z-index
      case 3:  // positive z-index
        return a->old_z_index() < b->old_z_index();
      case 2:  // fixed, z-index 0
        return BaseElementContainer::CompareElementOrder(a->element(),
                                                         b->element()) < 0;
      case 1:  // not fixed, z-index 0
      default:
        return false;
    }
  };

  std::stable_sort(children_.begin(), children_.end(), comparator);
  MarkDirtyState(kNeedRedraw);

  ResetDirtyState(kNeedSortZChild);
  ResetDirtyState(kNeedSortFixedChild);
}

void Fragment::CreatePaintingNode(
    bool is_flatten, const fml::RefPtr<PropBundle>& painting_data) {
  set_old_z_index(element()->ZIndex());
  set_was_stacking_context(element()->IsStackingContextNode());
  set_was_position_fixed(element()->is_fixed());
  MarkDirtyState(kNeedRedraw);
  element()->SetupFragmentBehavior(this);
  CreateLayerIfNeeded();
}

void Fragment::UpdatePaintingNode(
    bool tend_to_flatten, const fml::RefPtr<PropBundle>& painting_data) {
  MarkDirtyState(kNeedRedraw);
  // TODO(zhongyr): handle update or tend to flatten here.
}

void Fragment::UpdateLayout(
    LayoutResultForRendering layout_result_for_rendering) {
  MarkDirtyState(kNeedRedraw);
  layout_result_for_rendering_ = std::move(layout_result_for_rendering);
  if (behavior_) {
    behavior_->OnUpdateLayout(layout_result_for_rendering_);
  }
}

void Fragment::SetBehavior(std::unique_ptr<FragmentBehavior> behavior) {
  behavior_ = std::move(behavior);
}

void Fragment::DrawBorder(DisplayListBuilder& display_list_builder) {
  if (!element()->computed_css_style()->HasBorder()) {
    return;
  }

  const auto& border = element()
                           ->computed_css_style()
                           ->GetLayoutComputedStyle()
                           ->surround_data_.border_data_;
  display_list_builder.Border(*border);
}

void Fragment::DrawBackground(DisplayListBuilder& display_list_builder) {
  if (!element()->computed_css_style()->GetBackgroundData()) {
    return;
  }
  display_list_builder.Fill(
      element()->computed_css_style()->GetBackgroundData()->color);
}

void Fragment::DrawClip(DisplayListBuilder& display_list_builder) {
  // If the element is overflowed, do not need draw clip.
  if (element()->computed_css_style()->IsOverflowXY()) {
    return;
  }

  // If the element has no children and is not a text node, do not need draw
  // clip.
  if (children_.empty() && !element()->is_text()) {
    return;
  }

  RoundedRectangle rect;
  auto border_left_width = layout_result_for_rendering_.border_.at(0);
  auto border_top_width = layout_result_for_rendering_.border_.at(1);
  auto border_right_width = layout_result_for_rendering_.border_.at(2);
  auto border_bottom_width = layout_result_for_rendering_.border_.at(3);

  rect.SetX(border_left_width);
  rect.SetY(border_top_width);
  rect.SetWidth(std::max(layout_result_for_rendering_.size_.width_ -
                             border_left_width - border_right_width,
                         0.f));
  rect.SetHeight(std::max(layout_result_for_rendering_.size_.height_ -
                              border_top_width - border_bottom_width,
                          0.f));

  // If `overflow: hidden` is set, choose clip path or clip rect based on
  // border radius. Use clip path when a border radius exists; otherwise
  // use clip rect. If the element overflows on X or Y, clip a rect using
  // bounds and border.
  if (element()->computed_css_style()->IsOverflowHidden() &&
      element()->computed_css_style()->HasBorderRadius()) {
    const auto& border = element()
                             ->computed_css_style()
                             ->GetLayoutComputedStyle()
                             ->surround_data_.border_data_;

    starlight::LayoutUnit width(layout_result_for_rendering_.size_.width_);
    starlight::LayoutUnit height(layout_result_for_rendering_.size_.height_);
    rect.SetRadiusXTopLeft(std::max(
        starlight::NLengthToLayoutUnit(border->radius_x_top_left, width)
                .ToFloat() -
            border_left_width,
        0.f));
    rect.SetRadiusXTopRight(std::max(
        starlight::NLengthToLayoutUnit(border->radius_x_top_right, width)
                .ToFloat() -
            border_right_width,
        0.f));
    rect.SetRadiusXBottomRight(std::max(
        starlight::NLengthToLayoutUnit(border->radius_x_bottom_right, width)
                .ToFloat() -
            border_right_width,
        0.f));
    rect.SetRadiusXBottomLeft(std::max(
        starlight::NLengthToLayoutUnit(border->radius_x_bottom_left, width)
                .ToFloat() -
            border_left_width,
        0.f));
    rect.SetRadiusYTopLeft(std::max(
        starlight::NLengthToLayoutUnit(border->radius_y_top_left, height)
                .ToFloat() -
            border_top_width,
        0.f));
    rect.SetRadiusYTopRight(std::max(
        starlight::NLengthToLayoutUnit(border->radius_y_top_right, height)
                .ToFloat() -
            border_top_width,
        0.f));
    rect.SetRadiusYBottomRight(std::max(
        starlight::NLengthToLayoutUnit(border->radius_y_bottom_right, height)
                .ToFloat() -
            border_bottom_width,
        0.f));
    rect.SetRadiusYBottomLeft(std::max(
        starlight::NLengthToLayoutUnit(border->radius_y_bottom_left, height)
                .ToFloat() -
            border_bottom_width,
        0.f));
  } else if (element()->computed_css_style()->IsOverflowX()) {
    // x -= screen width
    // width += 2 * screen width
    rect.SetX(rect.GetX() -
              element_manager()->GetLynxEnvConfig().ScreenWidth());
    rect.SetWidth(rect.GetWidth() +
                  2 * element_manager()->GetLynxEnvConfig().ScreenWidth());
  } else if (element()->computed_css_style()->IsOverflowY()) {
    // y -= screen height
    // height += 2 * screen height
    rect.SetY(rect.GetY() -
              element_manager()->GetLynxEnvConfig().ScreenHeight());
    rect.SetHeight(rect.GetHeight() +
                   2 * element_manager()->GetLynxEnvConfig().ScreenHeight());
  }

  display_list_builder.ClipRect(rect);
}

// A non-null fragment_parent() indicates that the fragment has been added to
// the fragment tree. A null fragment_from_element_parent() suggests that the
// fragment is neither fixed nor has a z-index other than 0. Together, these
// conditions imply that the fragment is a reliable sibling.
bool Fragment::IsReliableSibling() const {
  return fragment_parent() != nullptr &&
         fragment_from_element_parent() == nullptr;
}

void Fragment::OnDraw(DisplayListBuilder& display_list_builder) {
  if (element()->IsShadowNodeVirtual()) {
    // No contents to be rendererd.
    return;
  }

  display_list_builder.Begin(layout_result_for_rendering_.offset_.X(),
                             layout_result_for_rendering_.offset_.Y(),
                             layout_result_for_rendering_.size_.width_,
                             layout_result_for_rendering_.size_.height_);

  DrawBackground(display_list_builder);
  DrawBorder(display_list_builder);
  DrawClip(display_list_builder);

  if (behavior_) {
    behavior_->OnDraw(display_list_builder);
  }

  for (const auto& child : children_) {
    child->Draw(display_list_builder);
  }

  display_list_builder.End();
}

void Fragment::Draw() {
  // XXX: Maybe this part could run parallely with parent displayList
  // generation. The shared totally different context.

  //  Collect own displayList.
  DisplayListBuilder builder;

  OnDraw(builder);

  painting_context()->impl()->CastToNativeCtx()->UpdateDisplayList(
      id(), builder.Build());
}

void Fragment::Draw(DisplayListBuilder& display_list_builder) {
  if (has_platform_renderer_) {
    display_list_builder.DrawView(id());
    // The view got its own display list.
    Draw();
    return;
  }

  OnDraw(display_list_builder);
}

void Fragment::InsertElementContainerAccordingToElement(Element* child,
                                                        Element* ref) {
  if (child == nullptr || child->fragment_impl() == nullptr) {
    return;
  }

  if (child->fragment_impl()->was_position_fixed()) {
    // If the child is fixed, insert it to the root fragment.
    fixed_children_.insert(child->fragment_impl());
    child->fragment_impl()->set_fragment_from_element_parent(this);

    element_manager()->root()->fragment_impl()->AddChildBefore(
        child->fragment_impl(), nullptr);
    return;
  } else if (child->fragment_impl()->old_z_index() != 0) {
    // If the child is not fixed, insert it to the enclosing stacking context
    // node.
    z_children_.insert(child->fragment_impl());
    child->fragment_impl()->set_fragment_from_element_parent(this);

    auto* parent_stacking_context =
        EnclosingStackingContextNode()->CastToFragment();
    parent_stacking_context->AddChildBefore(child->fragment_impl(), nullptr);
    return;
  } else {
    // If the child is not fixed and z-index is 0, insert it to the first
    // reliable sibling.
    while (ref != nullptr && !ref->fragment_impl()->IsReliableSibling()) {
      ref = ref->next_render_sibling();
    }
    AddChildBefore(child->fragment_impl(),
                   ref ? ref->fragment_impl() : nullptr);
  }

  // Reinsert the child's descendants with fixed or z-index !=0 to the correct
  // parent.
  child->fragment_impl()->ReinsertDescendantsToCorrectParent();
}

void Fragment::RemoveElementContainerAccordingToElement(Element* child,
                                                        bool destroy) {
  if (child == nullptr || child->fragment_impl() == nullptr) {
    return;
  }

  child->fragment_impl()->RemoveSelf();

  // Remove the child's descendants with fixed or z-index !=0 from current
  // parent.
  child->fragment_impl()->RemoveDescendantsFromCurrentParent();
}

void Fragment::AddChildBefore(Fragment* child, Fragment* sibling) {
  if (child == nullptr) {
    return;
  }

  if (child->fragment_parent()) {
    child->fragment_parent()->RemoveChild(child);
  }

  // Mark self need redraw when insert child.
  MarkDirtyState(kNeedRedraw);

  if (child->was_position_fixed()) {
    // Mark need resort children.
    MarkDirtyState(kNeedSortFixedChild);
  }

  if (child->old_z_index() != 0) {
    // Mark need resort children.
    MarkDirtyState(kNeedSortZChild);
  }

  if (sibling == nullptr) {
    children_.emplace_back(child);
  } else {
    if (auto it = std::find(children_.begin(), children_.end(), sibling);
        it != children_.end()) {
      children_.insert(it, child);
    }
  }

  child->set_parent(this);

  // FIXME(songshourui): rebuild the layer tree according to DisplayList in
  // PlatformRenderer::OnUpdateDisplayList.
  if (child->has_platform_renderer_) {
    auto* render_layer = this;
    while (!render_layer->has_platform_renderer_ && render_layer->parent()) {
      render_layer = static_cast<Fragment*>(render_layer->parent());
    }
    if (render_layer->has_platform_renderer_) {
      painting_context()->InsertPaintingNode(render_layer->id(), child->id(),
                                             -1);
    }
  }
}

void Fragment::RemoveSelf() {
  // If the fragment_from_element_parent_ is not null, it means the
  // fragment is fixed or z-index != 0. Remove it from
  // fragment_from_element_parent_'s corresponding set.
  if (fragment_from_element_parent() != nullptr) {
    fragment_from_element_parent()->z_children_.erase(this);
    fragment_from_element_parent()->fixed_children_.erase(this);
    set_fragment_from_element_parent(nullptr);
  }

  if (fragment_parent() == nullptr) {
    LOGE("Fragment RemoveSelf Error: parent is nullptr");
    return;
  }

  fragment_parent()->RemoveChild(this);
}

void Fragment::RemoveChild(Fragment* child) {
  if (child->parent() != this) {
    LOGE("Fragment RemoveChild Error: child's parent is not this fragment");
  }

  child->set_parent(nullptr);

  auto it = std::find(children_.begin(), children_.end(), child);
  if (it != children_.end()) {
    children_.erase(it);

    // Mark self need redraw when remove child.
    MarkDirtyState(kNeedRedraw);
  }
}

void Fragment::ReinsertDescendantsToCorrectParent() {
  base::MoveOnlyClosure<void, Fragment*, bool> f =
      [&f, manager = element_manager()](Fragment* current, bool need_handle_z) {
        if (!current->fixed_children_.empty()) {
          for (auto* fixed_child : current->fixed_children_) {
            if (fixed_child->fragment_parent() == nullptr) {
              manager->root()->fragment_impl()->AddChildBefore(fixed_child,
                                                               nullptr);
              // Recursively reinsert the fixed child's descendants. but do not
              // handle z-index since fixed child must be stacking context node.
              f(fixed_child, false);
            }
          }
        }

        // If this is not stacking context node and root is not stacking context
        // node,
        // then we need insert z-children.
        bool need_handle_z_children =
            !current->was_stacking_context() && need_handle_z;
        if (need_handle_z_children) {
          for (auto* z_child : current->z_children_) {
            if (z_child->fragment_parent() == nullptr) {
              z_child->EnclosingStackingContextNode()
                  ->CastToFragment()
                  ->AddChildBefore(z_child, nullptr);
              // Recursively reinsert the z-child's descendants. but do not
              // handle z-index since z-child must be stacking context node.
              f(z_child, false);
            }
          }
        }

        for (auto* child : current->children_) {
          f(child, need_handle_z_children);
        }
      };

  f(this, !was_stacking_context());
}

void Fragment::RemoveDescendantsFromCurrentParent() {
  base::MoveOnlyClosure<void, Fragment*, bool> f = [&f](Fragment* current,
                                                        bool handle_z_child) {
    if (!current->fixed_children_.empty()) {
      for (auto* fixed_child : current->fixed_children_) {
        if (fixed_child->fragment_parent() != nullptr) {
          fixed_child->fragment_parent()->RemoveChild(fixed_child);
          // Recursively remove the fixed child's descendants. but do not
          // handle z-index since fixed child must be stacking context node.
          f(fixed_child, false);
        }
      }
    }

    // If this is not stacking context node and root is not stacking context
    // node,
    // then we need remove z-children.
    bool need_handle_z_children =
        !current->was_stacking_context() && handle_z_child;
    if (need_handle_z_children) {
      for (auto* z_child : current->z_children_) {
        if (z_child->fragment_parent() != nullptr) {
          z_child->fragment_parent()->RemoveChild(z_child);
          // Recursively remove the z-child's descendants. but do not
          // handle z-index since z-child must be stacking context node.
          f(z_child, false);
        }
      }
    }

    for (auto* child : current->children_) {
      f(child, need_handle_z_children);
    }
  };

  f(this, !was_stacking_context());
}

void Fragment::MoveDirectStackingChildren(Fragment* parent, Fragment* root) {
  for (auto* z_child : root->z_children_) {
    z_child->fragment_parent()->RemoveChild(z_child);
    parent->AddChildBefore(z_child, nullptr);
  }
  for (auto* child : root->children_) {
    MoveDirectStackingChildren(parent, child);
  }
}

}  // namespace tasm
}  // namespace lynx
