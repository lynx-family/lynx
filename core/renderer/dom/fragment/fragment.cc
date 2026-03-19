// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/fragment.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/include/closure.h"
#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/transforms/transform_operations.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/event/platform_event_bundle.h"
#include "core/renderer/dom/fragment/fragment_behavior.h"
#include "core/renderer/dom/fragment/rounded_rectangle.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/ui_wrapper/painting/platform_renderer_impl.h"
#include "core/style/transform/matrix44.h"

namespace lynx {
namespace tasm {

// Init value for the draw node capacity.
const int32_t Fragment::kDefaultDrawNodeCapacity = 1;

Fragment::Fragment(Element* element) : BaseElementContainer(element) {}

Fragment* Fragment::fragment_parent() const {
  return static_cast<Fragment*>(parent());
}

void Fragment::CreateLayerIfNeeded(const fml::RefPtr<PropBundle>& init_data) {
  if (element()->is_wrapper() || has_platform_renderer_) {
    // If the fragment has a platform renderer, it means that the fragment
    // is already layerized.
    return;
  }

  if ((element()->is_view() || element()->is_text() || element()->is_image()) &&
      element()->TendToFlatten()) {
    // If the fragment is a view, text, or image, and it tends to flatten,
    // then it does not need to be layerized.
    return;
  }

  if (element()->IsShadowNodeVirtual()) {
    // If the fragment is a virtual shadow node, then it does not need to be
    // layerized.
    return;
  }

  if (behavior_ == nullptr) {
    // If the fragment does not have a behavior, then it does not need to be
    // layerized.
    LOGE("Fragment " << element()->GetTag().str()
                     << " does not have a behavior.");
    return;
  }

  // TODO(zhongyr): abstract one behavior for layerize.
  behavior_->CreatePlatformRenderer(init_data);
  has_platform_renderer_ = true;
}

void Fragment::UpdatePlatformExtraBundle(PlatformExtraBundle* bundle) {
  if (!has_platform_renderer_) {
    return;
  }

  painting_context()->UpdatePlatformExtraBundle(id(), bundle);
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
  InvalidateForRedraw();

  ResetDirtyState(kNeedSortZChild);
  ResetDirtyState(kNeedSortFixedChild);
}

void Fragment::CreatePaintingNode(
    bool is_flatten, const fml::RefPtr<PropBundle>& painting_data) {
  set_old_z_index(element()->ZIndex());
  set_was_stacking_context(element()->IsStackingContextNode());
  set_was_position_fixed(element()->is_fixed());
  InvalidateForRedraw();
  element()->SetupFragmentBehavior(this);
  CreateLayerIfNeeded(painting_data);
}

void Fragment::UpdatePaintingNode(
    bool tend_to_flatten, const fml::RefPtr<PropBundle>& painting_data) {
  if (behavior_) {
    behavior_->OnAttributeUpdate(painting_data);
  }
  if (has_platform_renderer_) {
    painting_context()->UpdatePaintingNode(id(), tend_to_flatten,
                                           painting_data);
  }
}

void Fragment::InsertListItemPaintingNode(int32_t child_id) {
  if (behavior_ == nullptr) {
    LOGE(
        "Fragment::InsertListItemPaintingNode failed since behavior_ is null.");
    return;
  }
  // TODO(songshourui.null): impl this method later.
}

void Fragment::RemoveListItemPaintingNode(int32_t child_id) {
  if (behavior_ == nullptr) {
    LOGE(
        "Fragment::RemoveListItemPaintingNode failed since behavior_ is null.");
    return;
  }
  // TODO(songshourui.null): impl this method later.
}

void Fragment::UpdateContentOffsetForListContainer(float content_size,
                                                   float delta_x, float delta_y,
                                                   bool is_init_scroll_offset,
                                                   bool from_layout) {
  if (behavior_ == nullptr) {
    LOGE(
        "Fragment::UpdateContentOffsetForListContainer failed since behavior_ "
        "is null.");
    return;
  }
  // TODO(songshourui.null): impl this method later.
}

void Fragment::UpdateLayout(
    LayoutResultForRendering layout_result_for_rendering) {
  InvalidateForRedraw();
  layout_info_.layout_result = std::move(layout_result_for_rendering);
  UpdateBorderRadiusAccordingToLayoutInfo();
}

void Fragment::SetBehavior(std::unique_ptr<FragmentBehavior> behavior) {
  behavior_ = std::move(behavior);
}

void Fragment::OnElementDestroying() {
  if (behavior_) {
    behavior_->OnElementDestroying();
  }
}

int32_t Fragment::DefineBorderBox(DisplayListBuilder& display_list_builder) {
  return box_recorder_.GetIndexOfBoxModel(BoxModelType::kBoxModelTypeBorder,
                                          LayoutResult(), display_list_builder);
}

int32_t Fragment::DefinePaddingBox(DisplayListBuilder& display_list_builder) {
  return box_recorder_.GetIndexOfBoxModel(BoxModelType::kBoxModelTypePadding,
                                          LayoutResult(), display_list_builder);
}

int32_t Fragment::DefineContentBox(DisplayListBuilder& display_list_builder) {
  return box_recorder_.GetIndexOfBoxModel(BoxModelType::kBoxModelTypeContent,
                                          LayoutResult(), display_list_builder);
}

void Fragment::SetTextBundle(intptr_t bundle) {
  if (behavior_ == nullptr) {
    LOGE("Fragment::SetTextBundle failed since behavior_ is null.");
    return;
  }
  behavior_->SetTextBundle(bundle);
}

void Fragment::DrawBorder(DisplayListBuilder& display_list_builder) {
  if (!element()->computed_css_style()->HasBorder()) {
    return;
  }

  const auto& border = element()
                           ->computed_css_style()
                           ->GetLayoutComputedStyle()
                           ->surround_data_.border_data_;
  display_list_builder.Border(DefineBorderBox(display_list_builder),
                              DefinePaddingBox(display_list_builder), *border);
}

void Fragment::DrawBackground(DisplayListBuilder& display_list_builder) {
  if (!element()->computed_css_style()->GetBackgroundData()) {
    return;
  }
  const auto& background_data =
      element()->computed_css_style()->GetBackgroundData();
  int32_t clip_index = -1;
  starlight::BackgroundClipType clip_type =
      starlight::BackgroundClipType::kBorderBox;
  if (background_data->image_data &&
      !background_data->image_data->clip.empty()) {
    clip_type = background_data->image_data->clip.back();
  }
  switch (clip_type) {
    case starlight::BackgroundClipType::kPaddingBox:
      clip_index = DefinePaddingBox(display_list_builder);
      break;
    case starlight::BackgroundClipType::kBorderBox:
      clip_index = DefineBorderBox(display_list_builder);
      break;
    case starlight::BackgroundClipType::kContentBox:
      clip_index = DefineContentBox(display_list_builder);
      break;
    default:
      clip_index = DefineBorderBox(display_list_builder);
      break;
  }
  display_list_builder.Fill(background_data->color, clip_index);

  if (background_data->image_data) {
    const auto& image_data = background_data->image_data;
    if (image_data->image.IsArray()) {
      starlight::BackgroundOriginType origin_type =
          starlight::BackgroundOriginType::kPaddingBox;
      if (!image_data->origin.empty()) {
        origin_type = image_data->origin.back();
      }
      int32_t origin_index = -1;
      switch (origin_type) {
        case starlight::BackgroundOriginType::kPaddingBox:
          origin_index = DefinePaddingBox(display_list_builder);
          break;
        case starlight::BackgroundOriginType::kBorderBox:
          origin_index = DefineBorderBox(display_list_builder);
          break;
        case starlight::BackgroundOriginType::kContentBox:
          origin_index = DefineContentBox(display_list_builder);
          break;
        default:
          origin_index = DefinePaddingBox(display_list_builder);
          break;
      }

      auto array = image_data->image.Array();
      for (size_t i = 0; i + 1 < array->size(); i += 2) {
        size_t image_index = i / 2;
        auto type =
            static_cast<starlight::BackgroundImageType>(array->get(i).Number());
        if (type == starlight::BackgroundImageType::kLinearGradient) {
          auto gradient_arr = array->get(i + 1).Array();
          // gradient_arr: [angle, colors, stops, side_or_corner]
          float angle = static_cast<float>(gradient_arr->get(0).Number());
          auto colors_arr = gradient_arr->get(1).Array();
          auto stops_arr = gradient_arr->get(2).Array();

          base::Vector<uint32_t> colors;
          colors.reserve(colors_arr->size());
          for (size_t j = 0; j < colors_arr->size(); ++j) {
            colors.push_back(
                static_cast<uint32_t>(colors_arr->get(j).Number()));
          }

          base::Vector<float> stops;
          stops.reserve(stops_arr->size());
          for (size_t j = 0; j < stops_arr->size(); ++j) {
            stops.push_back(static_cast<float>(stops_arr->get(j).Number()) /
                            100.0f);
          }

          starlight::BackgroundRepeatType repeat_x =
              starlight::BackgroundRepeatType::kRepeat;
          starlight::BackgroundRepeatType repeat_y =
              starlight::BackgroundRepeatType::kRepeat;
          if (image_index * 2 < image_data->repeat.size()) {
            repeat_x = image_data->repeat[image_index * 2];
          }
          if (image_index * 2 + 1 < image_data->repeat.size()) {
            repeat_y = image_data->repeat[image_index * 2 + 1];
          }

          display_list_builder.LinearGradient(
              angle, colors, stops, origin_index, clip_index,
              static_cast<int32_t>(repeat_x), static_cast<int32_t>(repeat_y));
        }
      }
    }
  }
}

void Fragment::DrawTransform(DisplayListBuilder& display_list_builder) {
  if (!element()->computed_css_style()->TransformChanged()) {
    return;
  }

  transforms::Matrix44 final_matrix;
  if (!element()->computed_css_style()->HasTransform()) {
    display_list_builder.Transform(final_matrix);
    // Transform is reset to identity matrix.
    return;
  }

  transforms::TransformOperations transform_ops(
      layout_info_.layout_result,
      *element()->computed_css_style()->GetTransformData());
  transforms::Matrix44 matrix =
      transform_ops.ApplyRemaining(0, layout_info_.layout_result);

  float origin_x = 0.5f * layout_info_.layout_result.size_.width_;
  float origin_y = 0.5f * layout_info_.layout_result.size_.height_;
  if (element()->computed_css_style()->HasTransformOrigin()) {
    const auto& origin_data =
        *element()->computed_css_style()->GetTransformOriginData();
    origin_x =
        starlight::NLengthToLayoutUnit(
            origin_data.x,
            starlight::LayoutUnit(layout_info_.layout_result.size_.width_))
            .ToFloat();
    origin_y =
        starlight::NLengthToLayoutUnit(
            origin_data.y,
            starlight::LayoutUnit(layout_info_.layout_result.size_.height_))
            .ToFloat();
  }

  final_matrix.preTranslate(origin_x, origin_y, 0.0f);
  final_matrix.preConcat(matrix);
  final_matrix.preTranslate(-origin_x, -origin_y, 0.0f);
  display_list_builder.Transform(final_matrix);
}

void Fragment::DrawOpacity(DisplayListBuilder& display_list_builder) {
  if (!element()->computed_css_style()->OpacityChanged()) {
    return;
  }

  auto opacity = element()->computed_css_style()->GetOpacity();
  display_list_builder.Opacity(opacity);
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
  auto border_left_width =
      layout_info_.layout_result.border_[starlight::Direction::kLeft];
  auto border_top_width =
      layout_info_.layout_result.border_[starlight::Direction::kTop];
  auto border_right_width =
      layout_info_.layout_result.border_[starlight::Direction::kRight];
  auto border_bottom_width =
      layout_info_.layout_result.border_[starlight::Direction::kBottom];

  rect.SetX(border_left_width);
  rect.SetY(border_top_width);
  rect.SetWidth(std::max(layout_info_.layout_result.size_.width_ -
                             border_left_width - border_right_width,
                         0.f));
  rect.SetHeight(std::max(layout_info_.layout_result.size_.height_ -
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

    starlight::LayoutUnit width(layout_info_.layout_result.size_.width_);
    starlight::LayoutUnit height(layout_info_.layout_result.size_.height_);
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

void Fragment::SetEventProp(PlatformEventPropName name,
                            const lepus::Value& value) {
  if (name == PlatformEventPropName::kUnknown) {
    return;
  }
  event_props_.insert_or_assign(name, value);
}

void Fragment::ClearEventProps() {
  if (event_props_.empty()) {
    return;
  }
  event_props_.clear();
}

void Fragment::AddEventName(PlatformEventName name) {
  if (name == PlatformEventName::kUnknown) {
    return;
  }
  for (const auto& item : event_names_) {
    if (item == name) {
      return;
    }
  }
  event_names_.push_back(name);
}

void Fragment::ClearEventNames() {
  if (event_names_.empty()) {
    return;
  }
  event_names_.clear();
}

void Fragment::MarkHasExposureEventIfNeeded() const {
  auto* manager = element_manager();
  if (manager->NeedReconstructEventTargetTreeForExposure()) {
    return;
  }
  bool need_mark = false;
  for (const auto& name : event_names_) {
    if (name == PlatformEventName::kUIAppear ||
        name == PlatformEventName::kUIDisappear) {
      need_mark = true;
      break;
    }
  }
  if (!need_mark) {
    for (const auto& it : event_props_) {
      const auto prop_name = it.first;
      const auto& prop_value = it.second;
      if (prop_name == PlatformEventPropName::kExposureId) {
        if (prop_value.IsString() && !prop_value.StdString().empty()) {
          need_mark = true;
          break;
        }
        continue;
      }
    }
  }
  if (need_mark) {
    manager->MarkNeedReconstructEventTargetTreeForExposure();
  }
}

void Fragment::OnDraw(DisplayListBuilder& display_list_builder) {
  MarkHasExposureEventIfNeeded();

  // FIXME(zhongyr): find a way to avoid this redraw issue
  // if (NeedRedraw()) {
  DrawFull(display_list_builder);
  //} else {
  //  DispatchUpdateDisplayList();
  //}

  if (NeedUpdateSubtreeProperty()) {
    DrawTransform(display_list_builder);
    DrawOpacity(display_list_builder);
  }

  ClearPaintDirtyState();
}

void Fragment::DrawFull(DisplayListBuilder& display_list_builder) {
  if (element()->IsShadowNodeVirtual()) {
    // No contents to be rendered for virtual shadow nodes.
    return;
  }

  if (element()->is_wrapper()) {
    DrawChildren(display_list_builder);
    return;
  }

  box_recorder_.Reset();
  display_list_builder.Begin(id(), layout_info_.layout_result.offset_.X(),
                             layout_info_.layout_result.offset_.Y(),
                             layout_info_.layout_result.size_.width_,
                             layout_info_.layout_result.size_.height_);

  painting_context()->impl()->CastToNativeCtx()->UpdatePlatformEventBundle(
      id(), PlatformEventBundle(event_props_, event_names_));

  DrawBackground(display_list_builder);
  DrawBorder(display_list_builder);
  DrawClip(display_list_builder);

  if (behavior_) {
    behavior_->OnDraw(display_list_builder);
  }

  DrawChildren(display_list_builder);

  display_list_builder.End();
}

void Fragment::DrawChildren(DisplayListBuilder& display_list_builder) {
  for (const auto& child : children_) {
    child->Draw(display_list_builder);
  }
}

void Fragment::ReconstructEventTargetTreeForExposure() const {
  if (id() != kRootId) {
    return;
  }
  auto* manager = element_manager();
  if (!manager->NeedReconstructEventTargetTreeForExposure()) {
    return;
  }

  painting_context()
      ->impl()
      ->CastToNativeCtx()
      ->ReconstructEventTargetTreeRecursively();
  manager->ResetNeedReconstructEventTargetTreeForExposure();
}

void Fragment::Draw() {
  // XXX: Maybe this part could run parallely with parent displayList
  // generation. The shared totally different context.

  //  Collect own displayList.
  DisplayListBuilder builder{render_offset_[0], render_offset_[1]};

  if (draw_node_capacity_ > 0) {
    builder.Reserve(draw_node_capacity_);
  }

  OnDraw(builder);

  CheckRootIfNeedClipBounds(builder);

  painting_context()->impl()->CastToNativeCtx()->UpdateDisplayList(
      id(), builder.Build());

  ReconstructEventTargetTreeForExposure();
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

bool Fragment::HasUIPrimitive() const { return has_platform_renderer_; }

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

  InvalidateForRedraw();

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
    LOGI("Skip Fragment RemoveSelf: parent is nullptr");
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
    InvalidateForRedraw();
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

void Fragment::UpdateLayout(float left, float top, bool transition_view) {
  layout_info_.layout_result.offset_.SetX(left);
  layout_info_.layout_result.offset_.SetY(top);
  UpdateRenderOffsetRecursively(0, 0, this);
}

void Fragment::CheckRootIfNeedClipBounds(
    DisplayListBuilder& display_list_builder) {
  if (element()->computed_css_style()->IsOverflowHidden()) {
    display_list_builder.MarkRootNeedClipBounds();
  }
}

void Fragment::UpdateBorderRadiusAccordingToLayoutInfo() {
  if (element()->computed_css_style()->HasBorderRadius()) {
    const auto& border = element()
                             ->computed_css_style()
                             ->GetLayoutComputedStyle()
                             ->surround_data_.border_data_;
    starlight::LayoutUnit width(layout_info_.layout_result.size_.width_);
    starlight::LayoutUnit height(layout_info_.layout_result.size_.height_);

    BorderRadiusInfo border_radius_info{
        .x_top_left =
            starlight::NLengthToLayoutUnit(border->radius_x_top_left, width)
                .ToFloat(),
        .y_top_left =
            starlight::NLengthToLayoutUnit(border->radius_y_top_left, height)
                .ToFloat(),
        .x_top_right =
            starlight::NLengthToLayoutUnit(border->radius_x_top_right, width)
                .ToFloat(),
        .y_top_right =
            starlight::NLengthToLayoutUnit(border->radius_y_top_right, height)
                .ToFloat(),
        .x_bottom_right =
            starlight::NLengthToLayoutUnit(border->radius_x_bottom_right, width)
                .ToFloat(),
        .y_bottom_right = starlight::NLengthToLayoutUnit(
                              border->radius_y_bottom_right, height)
                              .ToFloat(),
        .x_bottom_left =
            starlight::NLengthToLayoutUnit(border->radius_x_bottom_left, width)
                .ToFloat(),
        .y_bottom_left =
            starlight::NLengthToLayoutUnit(border->radius_y_bottom_left, height)
                .ToFloat(),
    };
    layout_info_.border_radius_info = std::move(border_radius_info);
  } else {
    layout_info_.border_radius_info = std::nullopt;
  }
}

void Fragment::UpdateRenderOffsetRecursively(float left, float top,
                                             Fragment* root) {
  float child_offset_x = left + layout_info_.layout_result.offset_.X();
  float child_offset_y = top + layout_info_.layout_result.offset_.Y();
  if (has_platform_renderer_) {
    render_offset_[0] = left;
    render_offset_[1] = top;

    child_offset_x = 0;
    child_offset_y = 0;

    draw_node_capacity_ = kDefaultDrawNodeCapacity;
  } else if (root != nullptr) {
    root->draw_node_capacity_++;
  }

  if (behavior_) {
    behavior_->OnUpdateLayout(layout_info_);
  }

  for (auto* child : children_) {
    child->UpdateRenderOffsetRecursively(child_offset_x, child_offset_y,
                                         has_platform_renderer_ ? this : root);
  }
}

void Fragment::DispatchUpdateDisplayList() {
  for (auto* child : children_) {
    if (child->has_platform_renderer_) {
      child->Draw();
    } else {
      child->DispatchUpdateDisplayList();
    }
  }
}

}  // namespace tasm
}  // namespace lynx
