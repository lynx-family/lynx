// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/fragment.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>

#include "base/include/closure.h"
#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/transforms/transform_operations_helper.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/event/platform_event_bundle.h"
#include "core/renderer/dom/fragment/fragment_behavior.h"
#include "core/renderer/dom/fragment/rounded_rectangle.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/ui_wrapper/painting/native_painting_context.h"
#include "core/renderer/ui_wrapper/painting/platform_renderer_impl.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "gfx/geometry/matrix44.h"

namespace lynx {
namespace tasm {

// Init value for the draw node capacity.
const int32_t Fragment::kDefaultDrawNodeCapacity = 1;

Fragment::Fragment(Element* element) : BaseElementContainer(element) {}

Fragment* Fragment::fragment_parent() const {
  return static_cast<Fragment*>(parent());
}

bool Fragment::ShouldSyncLayoutOnlyToEventTarget() const {
  return element()->element_manager()->GetEnableLayoutOnlyEventThrough() &&
         element()->IsLayoutOnly();
}

bool Fragment::CreateLayerIfNeeded(const fml::RefPtr<PropBundle>& init_data) {
  if (element()->is_wrapper() || has_platform_renderer_) {
    // If the fragment has a platform renderer, it means that the fragment
    // is already layerized.
    return false;
  }

  const bool tends_to_flatten = element()->TendToFlatten();
  const bool can_flatten_without_platform_renderer =
      (!element()->is_page() &&
       !element()->is_direct_child_of_compatible_component() &&
       (element()->is_text() || element()->is_image() || element()->is_view() ||
        element()->is_component())) &&
      tends_to_flatten;
  if (can_flatten_without_platform_renderer) {
    // If the fragment is a view, text, image, or component, and it tends to
    // flatten, then it does not need to be layerized. The page must keep its
    // platform renderer because it is the root of the PlatformRenderer tree.
    return false;
  }

  if (element()->IsShadowNodeVirtual()) {
    // If the fragment is a virtual shadow node, then it does not need to be
    // layerized.
    return false;
  }

  if (behavior_ == nullptr) {
    // If the fragment does not have a behavior, then it does not need to be
    // layerized.
    LOGE("Fragment " << element()->GetTag().str()
                     << " does not have a behavior.");
    return false;
  }

  // TODO(zhongyr): abstract one behavior for layerize.
  Element* element_parent = element()->parent();
  PlatformRendererInitConfig init_config;
  init_config.fragment_parent_id =
      element_parent != nullptr ? element_parent->impl_id() : -1;
  init_config.is_direct_child_of_compatible_component =
      element()->is_direct_child_of_compatible_component();

  fml::RefPtr<PropBundle> actual_init_data = init_data;
  auto ensure_actual_init_data = [&actual_init_data, this]() {
    if (actual_init_data == nullptr) {
      bool use_map_buffer =
          element()->element_manager()->GetEnableUseMapBuffer();
      actual_init_data =
          element()
              ->element_manager()
              ->GetPropBundleCreator()
              ->CreatePropBundle(use_map_buffer,
                                 element()->EnableFragmentLayerRender());
    }
    return actual_init_data != nullptr;
  };
  if (ensure_actual_init_data()) {
    actual_init_data->SetProps(kTendsToFlattenInitDataKey, tends_to_flatten);
  }
  behavior_->CreatePlatformRenderer(actual_init_data, init_config);
  has_platform_renderer_ = true;
  InvalidateRestacking();
  return true;
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
  const bool previous_fixed = was_position_fixed();
  const int32_t previous_z_index = old_z_index();
  const bool current_fixed = element()->is_fixed();
  const int32_t current_z_index = element()->ZIndex();
  if (current_fixed != previous_fixed || current_z_index != previous_z_index) {
    if (current_fixed != previous_fixed) {
      // New/unified fixed offsets are page-root relative. The Fragment may
      // already be hoisted to the page for z-index, so a fixed transition can
      // change coordinate semantics without changing either its stacking
      // parent or its numeric local offset.
      Fragment* restacking_root = RestackingRoot();
      const bool collection_was_pending =
          restacking_root->needs_layout_offset_collection_;
      InvalidateLayoutOffsetCache();
      InvalidateRestacking();
      FinishIncrementalLayoutOffsetUpdate(restacking_root,
                                          collection_was_pending);
    }
    auto* target_parent = fragment_parent();

    if (current_fixed) {
      // If it is a fixed element, the parent should be the root fragment.
      target_parent = element_manager()->root()->fragment_impl();
    } else if (current_z_index != 0) {
      // If z-index is not 0, the parent should be the nearest stacking
      // context ancestor. Start from the Element parent: the current element
      // becomes a stacking context as soon as z-index changes and must never
      // select itself as its new parent.
      target_parent = ResolveEnclosingStackingContextParent();
    } else {
      // If it is not fixed and z-index is 0, the parent should be the
      // fragment corresponding to the element's parent.
      target_parent = element()->parent() != nullptr
                          ? element()->parent()->fragment_impl()
                          : nullptr;
    }
    if (target_parent == nullptr) {
      LOGE("Fragment style change has no valid stacking parent: " << id());
      return;
    }

    Fragment* previous_parent = fragment_parent();
    const bool keeps_stacking_parent = target_parent == previous_parent;
    size_t previous_paint_index = 0;
    if (keeps_stacking_parent) {
      // Batched insertions may have left one paint bucket pending. Normalize it
      // once before applying the common single-node z-index update locally.
      target_parent->UpdateZIndexList();
      previous_paint_index = target_parent->PaintOrderIndex(this);
    }
    if (previous_parent != nullptr) {
      previous_parent->RemoveFromPaintOrderBucket(this);
    }

    set_was_position_fixed(current_fixed);
    set_old_z_index(current_z_index);

    // Only a node returning to normal flow needs a render sibling. Hoisted
    // z/fixed nodes are appended to their stacking parent and sorted there.
    Element* ref = nullptr;
    if (!current_fixed && current_z_index == 0 &&
        element()->next_render_sibling() != nullptr) {
      ref = element()->next_render_sibling();
    }
    while (ref != nullptr &&
           (ref->fragment_impl() == nullptr ||
            !ref->fragment_impl()->IsReliableSibling() ||
            ref->fragment_impl()->fragment_parent() != target_parent)) {
      ref = ref->next_render_sibling();
    }

    if (!keeps_stacking_parent) {
      ReparentStackingNode(target_parent,
                           ref != nullptr ? ref->fragment_impl() : nullptr);
    } else {
      // Keep children_ in structural/document order. A single style update
      // only moves this node inside the relevant paint bucket.
      target_parent->InsertIntoPaintOrderBucket(this);
      if (previous_paint_index != target_parent->PaintOrderIndex(this)) {
        target_parent->InvalidateForRedraw();
      }
    }

    Fragment* fragment_from_element_parent =
        element()->parent() != nullptr ? element()->parent()->fragment_impl()
                                       : nullptr;
    if (fragment_from_element_parent != nullptr) {
      if (current_z_index == 0) {
        fragment_from_element_parent->z_children_.erase(this);
      } else {
        fragment_from_element_parent->z_children_.insert(this);
      }
      if (!current_fixed) {
        fragment_from_element_parent->fixed_children_.erase(this);
      } else {
        fragment_from_element_parent->fixed_children_.insert(this);
      }
    }
    set_fragment_from_element_parent(current_z_index != 0 || current_fixed
                                         ? fragment_from_element_parent
                                         : nullptr);
  }

  if (element()->IsStackingContextNode() != was_stacking_context()) {
    // If the element's stacking context state changed, we should move the
    // descendants stacking context fragment to correct parent.

    set_was_stacking_context(element()->IsStackingContextNode());
    Fragment* target_parent =
        was_stacking_context() ? this : ResolveEnclosingStackingContextParent();
    MoveDirectStackingChildren(target_parent, this);
  }
}

void Fragment::UpdateZIndexList() {
  // If the element is a list and disable list platform implementation,
  // we should not update z-index list.
  if (ShouldBypassPaintOrderBuckets()) {
    return;
  }

  if (paint_order_buckets_ == nullptr) {
    ResetDirtyState(kNeedSortZChild);
    ResetDirtyState(kNeedSortFixedChild);
    return;
  }

  if (!NeedSortZChild() && !NeedSortFixedChild() &&
      !paint_order_buckets_->negative_z_dirty &&
      !paint_order_buckets_->fixed_zero_dirty &&
      !paint_order_buckets_->positive_z_dirty) {
    return;
  }

  if (paint_order_buckets_->negative_z_dirty) {
    std::stable_sort(paint_order_buckets_->negative_z.begin(),
                     paint_order_buckets_->negative_z.end(), ZPaintOrderLess);
    paint_order_buckets_->negative_z_dirty = false;
  }
  if (paint_order_buckets_->positive_z_dirty) {
    std::stable_sort(paint_order_buckets_->positive_z.begin(),
                     paint_order_buckets_->positive_z.end(), ZPaintOrderLess);
    paint_order_buckets_->positive_z_dirty = false;
  }
  if (paint_order_buckets_->fixed_zero_dirty) {
    std::stable_sort(paint_order_buckets_->fixed_zero.begin(),
                     paint_order_buckets_->fixed_zero.end(), DocumentOrderLess);
    paint_order_buckets_->fixed_zero_dirty = false;
  }

  ResetDirtyState(kNeedSortZChild);
  ResetDirtyState(kNeedSortFixedChild);
}

Fragment::PaintOrderGroup Fragment::PaintGroupFor(const Fragment* child) {
  if (child->old_z_index() < 0) {
    return PaintOrderGroup::kNegativeZ;
  }
  if (child->old_z_index() > 0) {
    return PaintOrderGroup::kPositiveZ;
  }
  return child->was_position_fixed() ? PaintOrderGroup::kFixedZero
                                     : PaintOrderGroup::kNormalFlow;
}

bool Fragment::ZPaintOrderLess(const Fragment* left, const Fragment* right) {
  if (left->old_z_index() != right->old_z_index()) {
    return left->old_z_index() < right->old_z_index();
  }
  return DocumentOrderLess(left, right);
}

bool Fragment::DocumentOrderLess(const Fragment* left, const Fragment* right) {
  return BaseElementContainer::CompareElementOrder(left->element(),
                                                   right->element()) < 0;
}

bool Fragment::ShouldBypassPaintOrderBuckets() const {
  return element() != nullptr && element()->is_list() &&
         element()->DisableListPlatformImplementation();
}

void Fragment::AppendToPaintOrderBucket(Fragment* child) {
  if (PaintGroupFor(child) == PaintOrderGroup::kNormalFlow) {
    return;
  }
  if (paint_order_buckets_ == nullptr) {
    paint_order_buckets_ = std::make_unique<PaintOrderBuckets>();
  }
  switch (PaintGroupFor(child)) {
    case PaintOrderGroup::kNegativeZ:
      paint_order_buckets_->negative_z.emplace_back(child);
      paint_order_buckets_->negative_z_dirty = true;
      MarkDirtyState(kNeedSortZChild);
      break;
    case PaintOrderGroup::kFixedZero:
      paint_order_buckets_->fixed_zero.emplace_back(child);
      paint_order_buckets_->fixed_zero_dirty = true;
      MarkDirtyState(kNeedSortFixedChild);
      break;
    case PaintOrderGroup::kPositiveZ:
      paint_order_buckets_->positive_z.emplace_back(child);
      paint_order_buckets_->positive_z_dirty = true;
      MarkDirtyState(kNeedSortZChild);
      break;
    case PaintOrderGroup::kNormalFlow:
      break;
  }
}

void Fragment::InsertIntoPaintOrderBucket(Fragment* child) {
  if (PaintGroupFor(child) == PaintOrderGroup::kNormalFlow) {
    return;
  }
  if (paint_order_buckets_ == nullptr) {
    paint_order_buckets_ = std::make_unique<PaintOrderBuckets>();
  }
  PaintOrderBucket* bucket = nullptr;
  bool (*less)(const Fragment*, const Fragment*) = nullptr;
  switch (PaintGroupFor(child)) {
    case PaintOrderGroup::kNegativeZ:
      bucket = &paint_order_buckets_->negative_z;
      less = ZPaintOrderLess;
      break;
    case PaintOrderGroup::kFixedZero:
      bucket = &paint_order_buckets_->fixed_zero;
      less = DocumentOrderLess;
      break;
    case PaintOrderGroup::kPositiveZ:
      bucket = &paint_order_buckets_->positive_z;
      less = ZPaintOrderLess;
      break;
    case PaintOrderGroup::kNormalFlow:
      return;
  }
  bucket->insert(std::lower_bound(bucket->begin(), bucket->end(), child, less),
                 child);
}

void Fragment::RemoveFromPaintOrderBucket(Fragment* child) {
  if (paint_order_buckets_ == nullptr) {
    return;
  }
  PaintOrderBucket* bucket = nullptr;
  switch (PaintGroupFor(child)) {
    case PaintOrderGroup::kNegativeZ:
      bucket = &paint_order_buckets_->negative_z;
      break;
    case PaintOrderGroup::kFixedZero:
      bucket = &paint_order_buckets_->fixed_zero;
      break;
    case PaintOrderGroup::kPositiveZ:
      bucket = &paint_order_buckets_->positive_z;
      break;
    case PaintOrderGroup::kNormalFlow:
      return;
  }
  if (auto it = std::find(bucket->begin(), bucket->end(), child);
      it != bucket->end()) {
    bucket->erase(it);
  }
  if (paint_order_buckets_->negative_z.empty() &&
      paint_order_buckets_->fixed_zero.empty() &&
      paint_order_buckets_->positive_z.empty()) {
    paint_order_buckets_.reset();
  }
}

size_t Fragment::PaintOrderIndex(const Fragment* target) const {
  if (ShouldBypassPaintOrderBuckets()) {
    const auto it = std::find(children_.begin(), children_.end(), target);
    return it == children_.end()
               ? children_.size()
               : static_cast<size_t>(std::distance(children_.begin(), it));
  }
  size_t index = 0;
  auto find_in_bucket = [&](const PaintOrderBucket& bucket) {
    for (const auto* child : bucket) {
      if (child == target) {
        return true;
      }
      ++index;
    }
    return false;
  };
  if (paint_order_buckets_ != nullptr &&
      find_in_bucket(paint_order_buckets_->negative_z)) {
    return index;
  }
  for (const auto* child : children_) {
    if (PaintGroupFor(child) != PaintOrderGroup::kNormalFlow) {
      continue;
    }
    if (child == target) {
      return index;
    }
    ++index;
  }
  if (paint_order_buckets_ != nullptr) {
    if (find_in_bucket(paint_order_buckets_->fixed_zero) ||
        find_in_bucket(paint_order_buckets_->positive_z)) {
      return index;
    }
  }
  return children_.size();
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
  if (CreateLayerIfNeeded(painting_data)) {
    if (parent()) {
      parent()->InvalidateForRedraw();
    }
    InvalidateForRedraw();
  }
  MarkNodeReadyIfNeeded();
}

void Fragment::OnFirstScreen() {
  painting_context()->impl()->CastToNativeCtx()->OnFirstScreen();
}

void Fragment::OnNodeReady() {
  if (!ShouldSyncNativePlatformRenderer()) {
    pending_node_ready_ = false;
    return;
  }
  pending_node_ready_ = false;
  painting_context()->OnNodeReady(id());
}

void Fragment::FinishTasmOperation(
    const std::shared_ptr<PipelineOptions>& options) {
  painting_context()->impl()->CastToNativeCtx()->FinishTasmOperation(options);
}

void Fragment::FinishLayoutOperation(
    const std::shared_ptr<PipelineOptions>& options) {
  painting_context()->impl()->CastToNativeCtx()->FinishLayoutOperation(options);
}

void Fragment::UpdateLayout(
    LayoutResultForRendering layout_result_for_rendering) {
  const auto& old_layout = layout_info_.layout_result;
  const bool offset_changed =
      old_layout.offset_ != layout_result_for_rendering.offset_;
  const bool draw_geometry_changed =
      !layout_geometry_initialized_ ||
      old_layout.size_.width_ != layout_result_for_rendering.size_.width_ ||
      old_layout.size_.height_ != layout_result_for_rendering.size_.height_ ||
      old_layout.padding_ != layout_result_for_rendering.padding_ ||
      old_layout.border_ != layout_result_for_rendering.border_;

  if (draw_geometry_changed) {
    InvalidateForRedraw();
  }
  Fragment* restacking_root = nullptr;
  bool collection_was_pending = false;
  if (offset_changed || !layout_geometry_initialized_) {
    restacking_root = RestackingRoot();
    collection_was_pending = restacking_root->needs_layout_offset_collection_;
    InvalidateLayoutOffsetCache();
    InvalidateRestacking();
  }
  layout_info_.layout_result = std::move(layout_result_for_rendering);
  layout_geometry_initialized_ = true;
  if (restacking_root != nullptr) {
    FinishIncrementalLayoutOffsetUpdate(restacking_root,
                                        collection_was_pending);
  }
  UpdateBorderRadiusAccordingToLayoutInfo();
  MarkNodeReadyIfNeeded();
}

void Fragment::SetBehavior(std::unique_ptr<FragmentBehavior> behavior) {
  behavior_ = std::move(behavior);
}

bool Fragment::ShouldSyncNativePlatformRenderer() const {
  return element() != nullptr && has_platform_renderer_ &&
         behavior_ != nullptr &&
         (behavior_->GetType() == PlatformRendererType::kExtended ||
          element()->is_direct_child_of_compatible_component());
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

namespace {

// Background size keyword constants (matching BackgroundSizeType enum)
constexpr int BACKGROUND_SIZE_AUTO =
    -1 * static_cast<int>(starlight::BackgroundSizeType::kAuto);  // -32
constexpr int BACKGROUND_SIZE_COVER =
    -1 * static_cast<int>(starlight::BackgroundSizeType::kCover);  // -33
constexpr int BACKGROUND_SIZE_CONTAIN =
    -1 * static_cast<int>(starlight::BackgroundSizeType::kContain);  // -34

// Calculate background size dimensions
void CalculateBackgroundSize(
    const starlight::BackgroundData::BackgroundImageData& image_data,
    size_t image_index, float origin_width, float origin_height,
    float& out_width, float& out_height) {
  // Default to auto (use origin box dimensions for gradients)
  out_width = origin_width;
  out_height = origin_height;

  if (image_data.size.empty()) {
    return;
  }

  // Get size values for this image (cycle if needed)
  size_t size_index = (image_index * 2) % image_data.size.size();
  const starlight::NLength& size_x = image_data.size[size_index];
  const starlight::NLength& size_y =
      image_data.size[(size_index + 1) % image_data.size.size()];

  // Handle keywords: cover, contain, auto
  // These are encoded as NLength with specific raw values (negated
  // BackgroundSizeType)
  bool is_cover =
      !size_x.IsPercent() && size_x.GetRawValue() == BACKGROUND_SIZE_COVER;
  bool is_contain =
      !size_x.IsPercent() && size_x.GetRawValue() == BACKGROUND_SIZE_CONTAIN;

  if (is_cover || is_contain) {
    // For gradients, cover just fills the origin box
    out_width = origin_width;
    out_height = origin_height;
    return;
  }

  // Handle auto for width
  if (!size_x.IsPercent() && size_x.GetRawValue() == BACKGROUND_SIZE_AUTO) {
    // auto - use origin width for gradients
    out_width = origin_width;
  } else {
    out_width = size_x.IsPercent() ? size_x.GetRawValue() * origin_width / 100
                                   : size_x.GetRawValue();
  }

  // Handle auto for height
  if (!size_y.IsPercent() && size_y.GetRawValue() == BACKGROUND_SIZE_AUTO) {
    // auto - use origin height for gradients
    out_height = origin_height;
  } else {
    out_height = size_y.IsPercent() ? size_y.GetRawValue() * origin_height / 100
                                    : size_y.GetRawValue();
  }
}

// Calculate background position offsets
void CalculateBackgroundPosition(
    const starlight::BackgroundData::BackgroundImageData& image_data,
    size_t image_index, float origin_width, float origin_height,
    float image_width, float image_height, float& out_offset_x,
    float& out_offset_y) {
  // Default to top-left (0, 0)
  out_offset_x = 0;
  out_offset_y = 0;

  if (image_data.position.empty()) {
    return;
  }

  // Get position values for this image (cycle if needed)
  size_t pos_index = (image_index * 2) % image_data.position.size();
  const starlight::NLength& pos_x = image_data.position[pos_index];
  const starlight::NLength& pos_y =
      image_data.position[(pos_index + 1) % image_data.position.size()];

  float delta_width = origin_width - image_width;
  float delta_height = origin_height - image_height;

  // Handle position keywords encoded as percentages
  // Left/Top = 0%, Center = 50%, Right/Bottom = 100%
  if (!pos_x.IsPercent()) {
    out_offset_x = pos_x.GetRawValue();
  } else {
    out_offset_x = pos_x.GetRawValue() * delta_width / 100;
  }

  if (!pos_y.IsPercent()) {
    out_offset_y = pos_y.GetRawValue();
  } else {
    out_offset_y = pos_y.GetRawValue() * delta_height / 100;
  }
}

float ResolveLinearGradientAngle(float angle,
                                 starlight::LinearGradientDirection direction,
                                 float tiling_width, float tiling_height) {
  constexpr float kPi = 3.14159265358979323846f;
  switch (direction) {
    case starlight::LinearGradientDirection::kTop:
      return 0.f;
    case starlight::LinearGradientDirection::kNone:
    case starlight::LinearGradientDirection::kBottom:
      return 180.f;
    case starlight::LinearGradientDirection::kLeft:
      return 270.f;
    case starlight::LinearGradientDirection::kRight:
      return 90.f;
    case starlight::LinearGradientDirection::kTopRight:
    case starlight::LinearGradientDirection::kTopLeft:
    case starlight::LinearGradientDirection::kBottomRight:
    case starlight::LinearGradientDirection::kBottomLeft: {
      // A corner keyword uses the gradient-box diagonal perpendicular to the
      // gradient line, so its angle depends on the tiling box's aspect ratio.
      const float corner_angle =
          std::atan2(tiling_height, tiling_width) * 180.f / kPi;
      switch (direction) {
        case starlight::LinearGradientDirection::kTopRight:
          return corner_angle;
        case starlight::LinearGradientDirection::kTopLeft:
          return 360.f - corner_angle;
        case starlight::LinearGradientDirection::kBottomRight:
          return 180.f - corner_angle;
        case starlight::LinearGradientDirection::kBottomLeft:
          return 180.f + corner_angle;
        default:
          break;
      }
      break;
    }
    case starlight::LinearGradientDirection::kAngle:
      return angle;
  }
  return angle;
}

}  // namespace

fml::RefPtr<PaintImage> Fragment::GetOrCreateBackgroundImage(
    size_t image_index, const base::String& url, float image_width,
    float image_height) {
  if (background_image_resources_.size() <= image_index) {
    background_image_resources_.resize(image_index + 1);
  }

  auto& resource = background_image_resources_[image_index];
  if (resource.image && resource.url == url) {
    return resource.image;
  }

  if (image_width <= 0.f || image_height <= 0.f) {
    resource = BackgroundImageResource{};
    return nullptr;
  }

  auto* platform_impl = painting_context()->impl();
  if (!platform_impl) {
    resource = BackgroundImageResource{};
    return nullptr;
  }

  auto* native_context = platform_impl->CastToNativeCtx();
  if (!native_context) {
    resource = BackgroundImageResource{};
    return nullptr;
  }

  auto image = native_context->CreateImage(id(), url, ImagePaintInfo(),
                                           image_width, image_height, 0, true);
  if (!image) {
    resource = BackgroundImageResource{};
    return nullptr;
  }

  resource.url = url;
  resource.image = image;
  return resource.image;
}

void Fragment::ClearBackgroundImage(size_t image_index) {
  if (image_index < background_image_resources_.size()) {
    background_image_resources_[image_index] = BackgroundImageResource{};
  }
}

void Fragment::DrawBackground(DisplayListBuilder& display_list_builder) {
  if (!element()->computed_css_style()->GetBackgroundData()) {
    background_image_resources_.clear();
    return;
  }
  const auto& background_data =
      element()->computed_css_style()->GetBackgroundData();
  auto define_clip_index = [&](starlight::BackgroundClipType clip_type) {
    switch (clip_type) {
      case starlight::BackgroundClipType::kPaddingBox:
        return DefinePaddingBox(display_list_builder);
      case starlight::BackgroundClipType::kContentBox:
        return DefineContentBox(display_list_builder);
      case starlight::BackgroundClipType::kBorderBox:
      default:
        return DefineBorderBox(display_list_builder);
    }
  };

  starlight::BackgroundClipType clip_type =
      starlight::BackgroundClipType::kBorderBox;
  if (background_data->image_data &&
      !background_data->image_data->clip.empty()) {
    const auto& image_data = *background_data->image_data;
    if (image_data.image_count == 0) {
      // background-image defaults to one implicit none layer.
      clip_type = image_data.clip.front();
    } else {
      size_t bottom_layer_index = image_data.image_count - 1;
      clip_type = image_data.clip[bottom_layer_index % image_data.clip.size()];
    }
  }
  display_list_builder.Fill(background_data->color,
                            define_clip_index(clip_type));

  if (!background_data->image_data) {
    background_image_resources_.clear();
    return;
  }

  const auto& image_data = background_data->image_data;
  if (!image_data->image.IsArray()) {
    background_image_resources_.clear();
    return;
  }

  auto array = image_data->image.Array();
  background_image_resources_.resize(array->size() / 2);
  auto define_image_clip_index = [&](size_t image_index) {
    starlight::BackgroundClipType image_clip_type =
        starlight::BackgroundClipType::kBorderBox;
    if (!image_data->clip.empty()) {
      image_clip_type = image_data->clip[image_index % image_data->clip.size()];
    }
    return define_clip_index(image_clip_type);
  };

  for (size_t i = 0; i + 1 < array->size(); i += 2) {
    size_t i_image = i / 2;
    starlight::BackgroundOriginType origin_type =
        starlight::BackgroundOriginType::kPaddingBox;

    if (!image_data->origin.empty()) {
      size_t i_origin = i_image % image_data->origin.size();
      origin_type = image_data->origin[i_origin];
    }

    // Get origin box dimensions
    float origin_x, origin_y, origin_width, origin_height;

    switch (origin_type) {
      case starlight::BackgroundOriginType::kBorderBox:
        origin_x = layout_info_.GetBorderBoxX();
        origin_y = layout_info_.GetBorderBoxY();
        origin_width = layout_info_.GetBorderBoxWidth();
        origin_height = layout_info_.GetBorderBoxHeight();
        break;
      case starlight::BackgroundOriginType::kContentBox:
        origin_x = layout_info_.GetContentBoxX();
        origin_y = layout_info_.GetContentBoxY();
        origin_width = layout_info_.GetContentBoxWidth();
        origin_height = layout_info_.GetContentBoxHeight();
        break;
      default:
        origin_x = layout_info_.GetPaddingBoxX();
        origin_y = layout_info_.GetPaddingBoxY();
        origin_width = layout_info_.GetPaddingBoxWidth();
        origin_height = layout_info_.GetPaddingBoxHeight();
        break;
    }

    starlight::BackgroundRepeatType repeat_x =
        starlight::BackgroundRepeatType::kRepeat;
    starlight::BackgroundRepeatType repeat_y =
        starlight::BackgroundRepeatType::kRepeat;

    if (!image_data->repeat.empty()) {
      size_t i_repeat = i_image % (image_data->repeat.size() / 2);
      repeat_x = image_data->repeat[2 * i_repeat];
      repeat_y = image_data->repeat[2 * i_repeat + 1];
    }

    // Calculate tiling box based on size and position
    float tiling_width, tiling_height;
    CalculateBackgroundSize(*image_data, i_image, origin_width, origin_height,
                            tiling_width, tiling_height);

    float offset_x = .0f, offset_y = .0f;
    CalculateBackgroundPosition(*image_data, i_image, origin_width,
                                origin_height, tiling_width, tiling_height,
                                offset_x, offset_y);

    // Create tiling box rectangle
    RoundedRectangle tiling_rect;
    tiling_rect.SetX(origin_x + offset_x);
    tiling_rect.SetY(origin_y + offset_y);
    tiling_rect.SetWidth(std::max(0.f, tiling_width));
    tiling_rect.SetHeight(std::max(0.f, tiling_height));

    // Record tiling box and get its index
    int32_t tiling_index = -1;
    display_list_builder.RecordBoxModel(tiling_rect, tiling_index);

    auto type =
        static_cast<starlight::BackgroundImageType>(array->get(i).Number());
    switch (type) {
      case starlight::BackgroundImageType::kUrl: {
        if (!array->get(i + 1).IsString()) {
          ClearBackgroundImage(i_image);
          break;
        }
        auto image = GetOrCreateBackgroundImage(
            i_image, array->get(i + 1).String(), tiling_rect.GetWidth(),
            tiling_rect.GetHeight());
        if (!image) {
          break;
        }
        display_list_builder.BackgroundImage(
            image, tiling_index, define_image_clip_index(i_image),
            static_cast<int32_t>(repeat_x), static_cast<int32_t>(repeat_y));
        break;
      }
      case starlight::BackgroundImageType::kLinearGradient: {
        ClearBackgroundImage(i_image);
        auto gradient_arr = array->get(i + 1).Array();
        // gradient_arr: [angle, colors, stops, side_or_corner]
        float angle = static_cast<float>(gradient_arr->get(0).Number());
        if (gradient_arr->size() > 3) {
          const auto direction =
              static_cast<starlight::LinearGradientDirection>(
                  gradient_arr->get(3).Number());
          angle = ResolveLinearGradientAngle(angle, direction,
                                             tiling_rect.GetWidth(),
                                             tiling_rect.GetHeight());
        }
        auto colors_arr = gradient_arr->get(1).Array();
        auto stops_arr = gradient_arr->get(2).Array();

        base::Vector<uint32_t> colors;
        colors.reserve(colors_arr->size());
        for (size_t j = 0; j < colors_arr->size(); ++j) {
          colors.push_back(static_cast<uint32_t>(colors_arr->get(j).UInt32()));
        }

        base::Vector<float> stops;
        stops.reserve(stops_arr->size());
        for (size_t j = 0; j < stops_arr->size(); ++j) {
          stops.push_back(static_cast<float>(stops_arr->get(j).Number()) /
                          100.0f);
        }

        display_list_builder.LinearGradient(angle, colors, stops, tiling_index,
                                            define_image_clip_index(i_image),
                                            static_cast<int32_t>(repeat_x),
                                            static_cast<int32_t>(repeat_y));
        break;
      }
      default:
        ClearBackgroundImage(i_image);
        break;
    }
  }
}

// W3C CSS Backgrounds and Borders Module Level 3
// https://drafts.csswg.org/css-backgrounds/#shadow-shape
// Computes the outset-adjusted border radius dimension:
//   radius + spread * (1 - (1 - ratio)^3 * (1 - coverage^3))
// This reduces the effect of spread on corner shape when border-radius is
// small, ensuring continuity between round and sharp corners.
//
// When border-radius < spread (ratio<1), the term (1 - ratio)^3 interpolates
// between full spread adjustment (ratio=0) and no adjustment (ratio=1).
// When the corner occupies a small fraction of the element (coverage<1),
// the term (1 - coverage^3) reduces the spread effect proportionally.
// When ratio >= 1 (radius exceeds spread) or coverage > 1, the result is
// simply radius + spread (full adjustment).
float ComputeOutsetAdjustedRadius(float radius, float spread, float coverage) {
  if (spread == 0.f) {
    return radius;
  }
  if (spread < 0.f) {
    return std::max(radius + spread, 0.f);
  }
  if (radius > spread || coverage > 1.f) {
    return radius + spread;
  }
  float ratio = radius / spread;
  float one_minus_ratio = 1.f - ratio;
  float coverage_cubed = coverage * coverage * coverage;
  float one_minus_coverage_cubed = 1.f - coverage_cubed;
  return radius +
         spread * (1.f - one_minus_ratio * one_minus_ratio * one_minus_ratio *
                             one_minus_coverage_cubed);
}

namespace {

// Computes shadow radii per W3C CSS spec:
// - Inset: radii decrease by spread (floored at zero)
// - Outset: radii increase by spread, with adjusted-radius formula when
//   border-radius < spread to preserve corner sharpness.
RoundedRectangle ComputeShadowBox(const RoundedRectangle& base_box,
                                  float spread, float offset_x, float offset_y,
                                  bool is_inset) {
  RoundedRectangle shadow_box;
  const auto& rect = base_box.GetRect();

  float left, top, right, bottom;
  if (is_inset) {
    // Inset: contract inward by spread
    left = rect.X() + offset_x + spread;
    top = rect.Y() + offset_y + spread;
    right = rect.X() + rect.Width() + offset_x - spread;
    bottom = rect.Y() + rect.Height() + offset_y - spread;
  } else {
    // Outset: expand outward by spread
    left = rect.X() + offset_x - spread;
    top = rect.Y() + offset_y - spread;
    right = rect.X() + rect.Width() + offset_x + spread;
    bottom = rect.Y() + rect.Height() + offset_y + spread;
  }

  shadow_box.SetX(left);
  shadow_box.SetY(top);
  shadow_box.SetWidth(std::max(right - left, 0.f));
  shadow_box.SetHeight(std::max(bottom - top, 0.f));

  if (!base_box.HasRadius()) {
    return shadow_box;
  }

  float width = rect.Width();
  float height = rect.Height();

  // Per W3C CSS spec: inset shadows shrink with positive spread and grow with
  // negative spread; the latter uses the same outset-adjusted formula.
  auto apply_spread_radius = [&](float rx, float ry) {
    if (is_inset && spread > 0.f) {
      return std::make_pair(std::max(rx - spread, 0.f),
                            std::max(ry - spread, 0.f));
    }
    float outset_spread = (is_inset && spread < 0.f) ? -spread : spread;
    if (width <= 0.f || height <= 0.f) {
      return std::make_pair(std::max(rx + outset_spread, 0.f),
                            std::max(ry + outset_spread, 0.f));
    }
    float coverage = 2.f * std::min(rx / width, ry / height);
    if (!std::isfinite(coverage)) {
      coverage = 2.f;  // force fast-path in ComputeOutsetAdjustedRadius
    }
    float adjusted_x = ComputeOutsetAdjustedRadius(rx, outset_spread, coverage);
    float adjusted_y = ComputeOutsetAdjustedRadius(ry, outset_spread, coverage);
    return std::make_pair(adjusted_x, adjusted_y);
  };

  auto [tl_x, tl_y] = apply_spread_radius(base_box.GetRadiusXTopLeft(),
                                          base_box.GetRadiusYTopLeft());
  shadow_box.SetRadiusXTopLeft(tl_x);
  shadow_box.SetRadiusYTopLeft(tl_y);

  auto [tr_x, tr_y] = apply_spread_radius(base_box.GetRadiusXTopRight(),
                                          base_box.GetRadiusYTopRight());
  shadow_box.SetRadiusXTopRight(tr_x);
  shadow_box.SetRadiusYTopRight(tr_y);

  auto [br_x, br_y] = apply_spread_radius(base_box.GetRadiusXBottomRight(),
                                          base_box.GetRadiusYBottomRight());
  shadow_box.SetRadiusXBottomRight(br_x);
  shadow_box.SetRadiusYBottomRight(br_y);

  auto [bl_x, bl_y] = apply_spread_radius(base_box.GetRadiusXBottomLeft(),
                                          base_box.GetRadiusYBottomLeft());
  shadow_box.SetRadiusXBottomLeft(bl_x);
  shadow_box.SetRadiusYBottomLeft(bl_y);

  return shadow_box;
}

}  // namespace

void Fragment::DrawBoxShadow(DisplayListBuilder& display_list_builder) {
  const auto& box_shadow_data =
      element()->computed_css_style()->GetBoxShadowData();
  if (!box_shadow_data.has_value()) {
    return;
  }

  // CSS box-shadow list is specified front-to-back: the first shadow is on
  // top. To achieve this with painter's algorithm, draw the shadows in reverse
  // order so the first declared shadow is emitted last.
  for (auto it = box_shadow_data->rbegin(); it != box_shadow_data->rend();
       ++it) {
    const auto& shadow = *it;
    bool is_inset = shadow.option == starlight::ShadowOption::kInset;
    DisplayListBuilder::BoxShadowClipMode clip_mode =
        is_inset ? DisplayListBuilder::BoxShadowClipMode::kInset
                 : DisplayListBuilder::BoxShadowClipMode::kOutset;

    // Per W3C spec:
    // - Outset shadows use border-box as base shape
    // - Inset shadows use padding-box as base shape
    RoundedRectangle base_box;
    int32_t clip_box_index;
    if (is_inset) {
      base_box = layout_info_.GeneratePaddingRectangle();
      clip_box_index = DefinePaddingBox(display_list_builder);
    } else {
      base_box = layout_info_.GenerateBorderRectangle();
      clip_box_index = DefineBorderBox(display_list_builder);
    }

    // Compute shadow geometry (rect + radii) per W3C CSS spec
    RoundedRectangle shadow_box = ComputeShadowBox(
        base_box, shadow.spread, shadow.h_offset, shadow.v_offset, is_inset);

    // Skip if spread inverts the rect (inset only)
    if (is_inset &&
        (shadow_box.GetWidth() <= 0.f || shadow_box.GetHeight() <= 0.f)) {
      continue;
    }

    // Record shadow box to display list
    int32_t shadow_box_index = -1;
    display_list_builder.RecordBoxModel(shadow_box, shadow_box_index);

    // Emit BoxShadow operation with pre-computed shadow box
    display_list_builder.BoxShadow(shadow_box_index, clip_box_index,
                                   shadow.color, shadow.blur, clip_mode);
  }
}

void Fragment::DrawTransform(DisplayListBuilder& display_list_builder) {
  if (!element()->computed_css_style()->TransformChanged()) {
    return;
  }

  gfx::Matrix44 final_matrix;
  if (!element()->computed_css_style()->HasTransform()) {
    display_list_builder.Transform(final_matrix);
    // Transform is reset to identity matrix.
    return;
  }

  gfx::TransformOperations transform_ops =
      transforms::ConvertToGfxTransformOperations(
          *element()->computed_css_style()->GetTransformData(),
          layout_info_.layout_result.size_.width_,
          layout_info_.layout_result.size_.height_);
  gfx::Matrix44 matrix =
      transform_ops.ApplyRemaining(0, layout_info_.layout_result.size_.width_,
                                   layout_info_.layout_result.size_.height_);

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

void Fragment::DrawFilter(DisplayListBuilder& display_list_builder) {
  auto* style = element()->computed_css_style();
  if (!style->FilterChanged()) {
    return;
  }

  const auto& filter = style->GetFilterData();
  display_list_builder.Filter(filter ? *filter : starlight::FilterData());
}

void Fragment::DrawClip(DisplayListBuilder& display_list_builder) {
  if (element()->IsOverlay()) {
    // Overlay keeps a zero-sized layout box while its content is measured
    // against the screen, so clipping by the overlay box hides the content.
    return;
  }

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

namespace {

bool IsValidExposurePropValue(PlatformEventPropName name,
                              const lepus::Value& value) {
  if (name == PlatformEventPropName::kExposureId) {
    return value.IsString() || value.IsNumber();
  }
  if (name == PlatformEventPropName::kExposureScene) {
    return value.IsString();
  }
  return false;
}

}  // namespace

void Fragment::SetEventProp(PlatformEventPropName name,
                            const lepus::Value& value) {
  if (name == PlatformEventPropName::kUnknown) {
    return;
  }
  auto it = event_props_.find(name);
  if (!IsValidExposurePropValue(name, value) && it != event_props_.end() &&
      it->second.IsEqual(value)) {
    return;
  }
  event_props_.insert_or_assign(name, value);
  event_bundle_dirty_ = true;
}

void Fragment::ClearEventProps() {
  if (event_props_.empty()) {
    return;
  }
  event_props_.clear();
  event_bundle_dirty_ = true;
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
  event_bundle_dirty_ = true;
}

void Fragment::ClearEventNames() {
  if (event_names_.empty()) {
    return;
  }
  event_names_.clear();
  event_bundle_dirty_ = true;
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
  RestackIfNeeded();
  if (!stacking_geometry_.valid) {
    return;
  }
  MarkHasExposureEventIfNeeded();

  // Only a fragment backed by a platform layer can skip full draw when its
  // contents haven't changed and only update subtree properties (transform,
  // opacity, filter) instead. Fragments without a platform renderer have no
  // display list of their own and must always contribute to the parent layer's
  // display list via DrawFull.
  if (NeedRedraw() || !has_platform_renderer_) {
    DrawFull(display_list_builder);
  } else {
    DispatchUpdateDisplayList();
  }

  if (NeedUpdateSubtreeProperty()) {
    DrawTransform(display_list_builder);
    DrawOpacity(display_list_builder);
    DrawFilter(display_list_builder);
  }

  ClearPaintDirtyState();
}

void Fragment::DrawFull(DisplayListBuilder& display_list_builder) {
  RestackIfNeeded();
  if (!stacking_geometry_.valid) {
    return;
  }

  if (element()->IsShadowNodeVirtual() || element()->display_none()) {
    // No contents to be rendered for virtual shadow nodes.
    return;
  }

  if (element()->is_wrapper()) {
    DrawChildren(display_list_builder);
    return;
  }

  box_recorder_.Reset();
  const auto* computed_style = element()->computed_css_style();
  DCHECK(stacking_geometry_.valid);
  display_list_builder.Begin(
      id(),
      behavior_ == nullptr ? PlatformRendererType::kUnknown
                           : behavior_->GetType(),
      stacking_geometry_.paint_offset.X(), stacking_geometry_.paint_offset.Y(),
      layout_info_.layout_result.size_.width_,
      layout_info_.layout_result.size_.height_, computed_style->IsOverflowX(),
      computed_style->IsOverflowY(), ShouldSyncLayoutOnlyToEventTarget());

  if (event_bundle_dirty_) {
    painting_context()->impl()->CastToNativeCtx()->UpdatePlatformEventBundle(
        id(), PlatformEventBundle(event_props_, event_names_));
    event_bundle_dirty_ = false;
  }

  DrawBackground(display_list_builder);
  DrawBoxShadow(display_list_builder);
  DrawBorder(display_list_builder);
  DrawClip(display_list_builder);

  if (behavior_) {
    behavior_->OnDraw(display_list_builder);
  }

  DrawChildren(display_list_builder);

  display_list_builder.End();
}

void Fragment::MarkNodeReadyIfNeeded() {
  if (ShouldSyncNativePlatformRenderer()) {
    pending_node_ready_ = true;
  }
}

void Fragment::FlushPendingNodeReadyIfNeeded() {
  if (!pending_node_ready_) {
    return;
  }
  OnNodeReady();
}

void Fragment::DrawChildren(DisplayListBuilder& display_list_builder) {
  if (ShouldBypassPaintOrderBuckets()) {
    for (auto* child : children_) {
      child->Draw(display_list_builder);
    }
    return;
  }
  // ElementManager normally flushes dirty stacking contexts before drawing.
  // Keep direct/standalone Fragment draws correct as well.
  UpdateZIndexList();
  if (paint_order_buckets_ == nullptr) {
    for (auto* child : children_) {
      child->Draw(display_list_builder);
    }
    return;
  }

  for (auto* child : paint_order_buckets_->negative_z) {
    child->Draw(display_list_builder);
  }
  const size_t special_child_count = paint_order_buckets_->negative_z.size() +
                                     paint_order_buckets_->fixed_zero.size() +
                                     paint_order_buckets_->positive_z.size();
  if (special_child_count != children_.size()) {
    for (auto* child : children_) {
      if (PaintGroupFor(child) != PaintOrderGroup::kNormalFlow) {
        continue;
      }
      child->Draw(display_list_builder);
    }
  }
  for (auto* child : paint_order_buckets_->fixed_zero) {
    child->Draw(display_list_builder);
  }
  for (auto* child : paint_order_buckets_->positive_z) {
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
  RestackIfNeeded();
  if (!stacking_geometry_.valid) {
    return;
  }

  // XXX: Maybe this part could run parallely with parent displayList
  // generation. The shared totally different context.

  //  Collect own displayList.
  DCHECK(stacking_geometry_.valid);
  DisplayListBuilder builder{stacking_geometry_.platform_embedding_offset.X(),
                             stacking_geometry_.platform_embedding_offset.Y()};

  if (draw_node_capacity_ > 0) {
    builder.Reserve(draw_node_capacity_);
  }

  if (!element()->display_none()) {
    OnDraw(builder);

    CheckRootIfNeedClipBounds(builder);
  } else {
    // display:none: still emit a display list containing only this node's
    // Begin/End so the platform layer receives an update and clears any stale
    // content / sublayers / event-target state instead of keeping the previous
    // frame.
    const auto* computed_style = element()->computed_css_style();
    DCHECK(stacking_geometry_.valid);
    builder.Begin(id(),
                  behavior_ == nullptr ? PlatformRendererType::kUnknown
                                       : behavior_->GetType(),
                  stacking_geometry_.paint_offset.X(),
                  stacking_geometry_.paint_offset.Y(),
                  layout_info_.layout_result.size_.width_,
                  layout_info_.layout_result.size_.height_,
                  computed_style->IsOverflowX(), computed_style->IsOverflowY(),
                  ShouldSyncLayoutOnlyToEventTarget());
    builder.End();
  }

  painting_context()->impl()->CastToNativeCtx()->UpdateDisplayList(
      id(), builder.Build());

  ReconstructEventTargetTreeForExposure();
}

void Fragment::Draw(DisplayListBuilder& display_list_builder) {
  RestackIfNeeded();
  if (!stacking_geometry_.valid) {
    return;
  }

  if (has_platform_renderer_) {
    // A platform child is not drawn through the parent's nested Begin stack.
    // Pass its final local offset so DrawView can update the native child
    // position without changing the size owned by the child's display list.
    DCHECK(stacking_geometry_.valid);
    display_list_builder.DrawView(id(), stacking_geometry_.offset_to_parent.X(),
                                  stacking_geometry_.offset_to_parent.Y());
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

  if (sibling == nullptr) {
    // Hoisted z/fixed fragments frequently arrive without a structural
    // sibling. Keep children_ in document order independently of paint order.
    if (PaintGroupFor(child) == PaintOrderGroup::kNormalFlow ||
        children_.empty() || !DocumentOrderLess(child, children_.back())) {
      children_.emplace_back(child);
    } else {
      auto it = std::find_if(children_.begin(), children_.end(),
                             [child](const Fragment* current) {
                               return DocumentOrderLess(child, current);
                             });
      children_.insert(it, child);
    }
  } else {
    if (auto it = std::find(children_.begin(), children_.end(), sibling);
        it != children_.end()) {
      children_.insert(it, child);
    } else {
      // Keep the tree internally consistent even if a stale caller supplied a
      // sibling from another stacking parent. Recover document order without
      // coupling structural storage to paint sorting.
      if (children_.empty() || !DocumentOrderLess(child, children_.back())) {
        children_.emplace_back(child);
      } else {
        auto insertion =
            std::find_if(children_.begin(), children_.end(),
                         [child](const Fragment* current) {
                           return DocumentOrderLess(child, current);
                         });
        children_.insert(insertion, child);
      }
    }
  }

  child->set_parent(this);
  AppendToPaintOrderBucket(child);
  Element* layout_parent = child->element()->render_parent();
  const int32_t layout_parent_id =
      layout_parent != nullptr ? layout_parent->impl_id() : -1;
  if (child->layout_offset_valid_ &&
      child->layout_parent_id_for_cached_offset_ != layout_parent_id) {
    Fragment* restacking_root = child->RestackingRoot();
    const bool collection_was_pending =
        restacking_root->needs_layout_offset_collection_;
    child->InvalidateLayoutOffsetCache();
    child->FinishIncrementalLayoutOffsetUpdate(restacking_root,
                                               collection_was_pending);
  }
  InvalidateRestacking();
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

  RemoveFromPaintOrderBucket(child);
  child->set_parent(nullptr);

  auto it = std::find(children_.begin(), children_.end(), child);
  if (it != children_.end()) {
    children_.erase(it);

    // Mark self need redraw when remove child.
    InvalidateForRedraw();
    InvalidateRestacking();
  }
}

void Fragment::ReparentStackingNode(Fragment* target_parent,
                                    Fragment* sibling) {
  if (target_parent == nullptr) {
    LOGE("Fragment reparent rejected because target parent is null: " << id());
    return;
  }
  for (Fragment* ancestor = target_parent; ancestor != nullptr;
       ancestor = ancestor->fragment_parent()) {
    if (ancestor == this) {
      LOGE("Fragment reparent rejected because it would create a cycle: "
           << id());
      return;
    }
  }
  if (target_parent == fragment_parent()) {
    return;
  }

  if (fragment_parent() != nullptr) {
    fragment_parent()->RemoveChild(this);
  }
  target_parent->AddChildBefore(this, sibling);
}

void Fragment::ReinsertDescendantsToCorrectParent() {
  base::MoveOnlyClosure<void, Fragment*, bool> f =
      [&f, manager = element_manager()](Fragment* current, bool need_handle_z) {
        if (!current->fixed_children_.empty()) {
          for (auto* fixed_child : current->fixed_children_) {
            if (fixed_child->fragment_parent() == nullptr) {
              fixed_child->ReparentStackingNode(
                  manager->root()->fragment_impl(), nullptr);
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
              z_child->ReparentStackingNode(
                  z_child->ResolveEnclosingStackingContextParent(), nullptr);
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
  if (parent == nullptr || root == nullptr) {
    return;
  }

  // Reparenting a nested z child can erase it from an ancestor's children_.
  // Traverse snapshots so mutations never invalidate the active iteration.
  const auto children_snapshot = root->children_;
  // Reparenting does not change the logical parent's z_children_ set, so it is
  // safe and cheaper to iterate that set directly.
  for (auto* z_child : root->z_children_) {
    z_child->ReparentStackingNode(parent, nullptr);
  }

  for (auto* child : children_snapshot) {
    // Hoisted nodes are already represented by a z/fixed set, and an existing
    // stacking context owns its descendants independently.
    if (child->fragment_from_element_parent() != nullptr ||
        child->was_stacking_context()) {
      continue;
    }
    MoveDirectStackingChildren(parent, child);
  }
}

void Fragment::InvalidateForRedraw() {
  // A platform-backed fragment owns an independent display list. Rebuilding
  // ancestors above that paint root cannot change its contents and only causes
  // redundant display-list generation and platform invalidation.
  Fragment* current = this;
  while (current != nullptr) {
    if (current->NeedRedraw()) {
      return;
    }
    current->MarkDirtyState(kNeedRedraw);
    if (current->has_platform_renderer_) {
      return;
    }
    current = current->fragment_parent();
  }
}

void Fragment::InvalidateRestacking() {
  RestackingRoot()->needs_restacking_ = true;
}

void Fragment::InvalidateLayoutOffsetCache() {
  layout_offset_valid_ = false;
  RestackingRoot()->needs_layout_offset_collection_ = true;
}

Fragment* Fragment::RestackingRoot() {
  if (fragment_parent() == nullptr) {
    return this;
  }

  // Managed FragmentTrees have one fixed root: the page Element's fragment.
  // Resolve it through ElementManager in O(1) instead of walking the fragment
  // parent chain on every layout/style invalidation and draw entry.
  if (element()->fragment_impl() == this) {
    Element* root_element = element_manager()->root();
    Fragment* page_fragment =
        root_element != nullptr ? root_element->fragment_impl() : nullptr;
    if (page_fragment != nullptr) {
      DCHECK(page_fragment->fragment_parent() == nullptr);
      return page_fragment;
    }
  }

  // Unit tests and a few embedders construct independent FragmentTrees that
  // are not installed as Element containers. Keep that compatibility path out
  // of the managed-tree hot path.
  Fragment* root_fragment = this;
  while (root_fragment->fragment_parent() != nullptr) {
    root_fragment = root_fragment->fragment_parent();
  }
  return root_fragment;
}

void Fragment::MarkResolvedPaintRootDirty(Fragment* paint_root) {
  if (paint_root == nullptr) {
    return;
  }
  paint_root->MarkDirtyState(kNeedRedraw);
}

Fragment* Fragment::ResolveStackingGeometryParent() const {
  Fragment* resolved_parent = fragment_parent();
  if (!has_platform_renderer_) {
    return resolved_parent;
  }
  while (resolved_parent != nullptr &&
         !resolved_parent->has_platform_renderer_) {
    resolved_parent = resolved_parent->fragment_parent();
  }
  return resolved_parent;
}

Fragment* Fragment::ResolveEnclosingStackingContextParent() const {
  for (Element* ancestor = element() != nullptr ? element()->parent() : nullptr;
       ancestor != nullptr; ancestor = ancestor->parent()) {
    if (ancestor->IsStackingContextNode() &&
        ancestor->fragment_impl() != nullptr) {
      return ancestor->fragment_impl();
    }
  }
  LOGE("No stacking context ancestor found for fragment " << id());
  return nullptr;
}

void Fragment::CollectLayoutOffsetsToRoot(
    Element* current, base::geometry::FloatPoint parent_offset) {
  if (current == nullptr) {
    return;
  }

  Fragment* current_fragment = current->fragment_impl();
  base::geometry::FloatPoint local_offset(current->left(), current->top());
  if (current_fragment != nullptr) {
    local_offset = current_fragment->layout_info_.layout_result.offset_;
    // New/unified fixed layout results are already page-root relative even
    // though the Element remains under its logical parent in the render tree.
    current_fragment->layout_offset_to_root_ =
        current->IsFixedNewOrUnified() ? local_offset
                                       : parent_offset + local_offset;
    current_fragment->layout_parent_id_for_cached_offset_ =
        current->render_parent() != nullptr
            ? current->render_parent()->impl_id()
            : -1;
    current_fragment->layout_offset_valid_ = true;
    parent_offset = current_fragment->layout_offset_to_root_;
  } else {
    parent_offset = current->IsFixedNewOrUnified()
                        ? local_offset
                        : parent_offset + local_offset;
  }

  for (Element* child = current->first_render_child(); child != nullptr;
       child = child->next_render_sibling()) {
    CollectLayoutOffsetsToRoot(child, parent_offset);
  }
}

bool Fragment::CacheLayoutOffsetToRoot(
    base::geometry::FloatPoint parent_offset) {
  const base::geometry::FloatPoint local_offset =
      layout_info_.layout_result.offset_;
  const base::geometry::FloatPoint updated_offset =
      element()->IsFixedNewOrUnified() ? local_offset
                                       : parent_offset + local_offset;
  const bool changed =
      !layout_offset_valid_ || layout_offset_to_root_ != updated_offset;
  layout_offset_to_root_ = updated_offset;
  layout_parent_id_for_cached_offset_ =
      element()->render_parent() != nullptr
          ? element()->render_parent()->impl_id()
          : -1;
  layout_offset_valid_ = true;
  return changed;
}

bool Fragment::RefreshLayoutOffsetSubtree() {
  if (element()->fragment_impl() != this) {
    layout_offset_to_root_ = layout_info_.layout_result.offset_;
    layout_parent_id_for_cached_offset_ =
        element()->render_parent() != nullptr
            ? element()->render_parent()->impl_id()
            : -1;
    layout_offset_valid_ = true;
    return true;
  }

  base::geometry::FloatPoint parent_offset(0.f, 0.f);
  if (!element()->IsFixedNewOrUnified()) {
    Element* layout_parent = element()->render_parent();
    if (layout_parent != nullptr) {
      Fragment* parent_fragment = layout_parent->fragment_impl();
      if (parent_fragment == nullptr ||
          !parent_fragment->layout_offset_valid_) {
        return false;
      }
      parent_offset = parent_fragment->layout_offset_to_root_;
    }
  }
  CollectLayoutOffsetsToRoot(element(), parent_offset);
  return true;
}

void Fragment::FinishIncrementalLayoutOffsetUpdate(
    Fragment* restacking_root, bool collection_was_pending) {
  if (!collection_was_pending &&
      !restacking_root->updating_layout_offset_cache_ &&
      RefreshLayoutOffsetSubtree()) {
    restacking_root->needs_layout_offset_collection_ = false;
  }
}

void Fragment::FinishLayoutOffsetCollection() {
  DCHECK(fragment_parent() == nullptr);
  needs_layout_offset_collection_ = false;
}

bool Fragment::ResolveStackingGeometry(
    base::geometry::FloatPoint active_paint_offset, Fragment* active_paint_root,
    bool flush_node_ready,
    base::geometry::FloatPoint* child_active_paint_offset,
    Fragment** child_active_paint_root) {
  auto invalidate_unreachable_geometry = [this]() {
    if (!stacking_geometry_.valid) {
      return;
    }
    Fragment* previous_parent = stacking_geometry_.parent;
    Fragment* previous_paint_root = stacking_geometry_.paint_root;
    Fragment* previous_parent_paint_root =
        previous_parent != nullptr && previous_parent->stacking_geometry_.valid
            ? previous_parent->stacking_geometry_.paint_root
            : previous_parent;
    stacking_geometry_.valid = false;
    MarkResolvedPaintRootDirty(previous_paint_root);
    MarkResolvedPaintRootDirty(previous_parent_paint_root);
  };
  if (!layout_offset_valid_) {
    LOGE("Restacking failed: fragment " << id()
                                        << " is not reachable in LayoutTree");
    invalidate_unreachable_geometry();
    return false;
  }

  Fragment* resolved_parent = ResolveStackingGeometryParent();
  base::geometry::FloatPoint parent_offset_to_root(0.f, 0.f);
  if (resolved_parent != nullptr) {
    if (!resolved_parent->layout_offset_valid_) {
      LOGE("Restacking failed: geometry parent " << resolved_parent->id()
                                                 << " of fragment " << id()
                                                 << " is not reachable in "
                                                    "LayoutTree");
      invalidate_unreachable_geometry();
      return false;
    }
    parent_offset_to_root = resolved_parent->layout_offset_to_root_;
  }

  Fragment* current_paint_root =
      has_platform_renderer_
          ? this
          : (active_paint_root != nullptr ? active_paint_root : this);
  ResolvedStackingGeometry resolved{
      .parent = resolved_parent,
      .paint_root = current_paint_root,
      .offset_to_parent = layout_offset_to_root_ - parent_offset_to_root,
      .paint_offset = layout_offset_to_root_ - parent_offset_to_root,
      .platform_embedding_offset = base::geometry::FloatPoint(0.f, 0.f),
      .valid = true};
  if (has_platform_renderer_) {
    // Only flattened fragments on the current StackingTree path have already
    // translated the parent canvas when it reaches this fragment's DrawView.
    // The platform must cancel exactly that active paint translation. Layout
    // ancestors skipped by a hoist are not on this path and must not be
    // cancelled. This preserves the invariant:
    //   paint_offset + platform_embedding_offset == offset_to_parent.
    resolved.platform_embedding_offset = active_paint_offset;
    resolved.paint_offset =
        resolved.offset_to_parent - resolved.platform_embedding_offset;
  }
  const bool changed =
      !stacking_geometry_.valid ||
      stacking_geometry_.parent != resolved.parent ||
      stacking_geometry_.paint_root != resolved.paint_root ||
      stacking_geometry_.offset_to_parent != resolved.offset_to_parent ||
      stacking_geometry_.paint_offset != resolved.paint_offset ||
      stacking_geometry_.platform_embedding_offset !=
          resolved.platform_embedding_offset;
  if (changed) {
    Fragment* previous_paint_root = stacking_geometry_.paint_root;
    stacking_geometry_ = resolved;
    // This fragment's Begin changes. For platform-backed fragments the
    // embedding DrawView in the platform parent changes as well.
    MarkResolvedPaintRootDirty(previous_paint_root);
    MarkResolvedPaintRootDirty(current_paint_root);
    MarkResolvedPaintRootDirty(
        resolved_parent != nullptr && resolved_parent->stacking_geometry_.valid
            ? resolved_parent->stacking_geometry_.paint_root
            : resolved_parent);
    MarkNodeReadyIfNeeded();
  }
  if (flush_node_ready) {
    FlushPendingNodeReadyIfNeeded();
  }

  *child_active_paint_offset =
      has_platform_renderer_ ? base::geometry::FloatPoint(0.f, 0.f)
                             : active_paint_offset + resolved.paint_offset;
  *child_active_paint_root = has_platform_renderer_ ? this : active_paint_root;
  return true;
}

void Fragment::ResolveStackingGeometryRecursively(
    base::geometry::FloatPoint active_paint_offset, Fragment* active_paint_root,
    bool flush_node_ready) {
  base::geometry::FloatPoint child_active_paint_offset;
  Fragment* child_active_paint_root = nullptr;
  if (!ResolveStackingGeometry(active_paint_offset, active_paint_root,
                               flush_node_ready, &child_active_paint_offset,
                               &child_active_paint_root)) {
    return;
  }
  for (auto* child : children_) {
    child->ResolveStackingGeometryRecursively(
        child_active_paint_offset, child_active_paint_root, flush_node_ready);
  }
}

uint64_t Fragment::PrepareRestacking() {
  DCHECK(fragment_parent() == nullptr);
  ++restacking_generation_;
  if (restacking_generation_ == 0) {
    // Generation zero means "not collected" on every Fragment.
    ++restacking_generation_;
  }

  // A few embedders construct a Fragment directly instead of installing it as
  // the Element's container. Such a standalone fragment is its complete
  // layout tree, so resolve its one local edge without consulting an unrelated
  // Element-owned fragment.
  if (element()->fragment_impl() != this) {
    layout_offset_to_root_ = layout_info_.layout_result.offset_;
    layout_parent_id_for_cached_offset_ =
        element()->render_parent() != nullptr
            ? element()->render_parent()->impl_id()
            : -1;
    layout_offset_valid_ = true;
    needs_layout_offset_collection_ = false;
  } else if (needs_layout_offset_collection_) {
    // Direct Fragment updates and standalone draw entry points do not pass
    // through Element::UpdateLayoutInfoRecursively. Preserve correctness with
    // one fallback collection; normal page layout maintains these offsets
    // incrementally in its existing traversal.
    CollectLayoutOffsetsToRoot(element(), base::geometry::FloatPoint(0.f, 0.f));
    needs_layout_offset_collection_ = false;
  }
  return restacking_generation_;
}

void Fragment::RestackIfNeeded() {
  Fragment* root_fragment = RestackingRoot();
  if (root_fragment != this) {
    root_fragment->RestackIfNeeded();
    return;
  }
  if (!needs_restacking_) {
    return;
  }
  PrepareRestacking();
  // Clear before resolving so a re-entrant mutation schedules another pass
  // instead of being overwritten when this pass completes.
  needs_restacking_ = false;
  ResolveStackingGeometryRecursively(base::geometry::FloatPoint(0.f, 0.f),
                                     nullptr, true);
}

void Fragment::UpdateLayout(float left, float top, bool transition_view) {
  const base::geometry::FloatPoint updated_offset(left, top);
  if (layout_info_.layout_result.offset_ != updated_offset) {
    Fragment* restacking_root = RestackingRoot();
    const bool collection_was_pending =
        restacking_root->needs_layout_offset_collection_;
    layout_info_.layout_result.offset_ = updated_offset;
    InvalidateLayoutOffsetCache();
    InvalidateRestacking();
    FinishIncrementalLayoutOffsetUpdate(restacking_root,
                                        collection_was_pending);
  }
  // Page layout already performs a full FragmentTree traversal to synchronize
  // platform layout. Resolve stacking geometry in that same traversal instead
  // of adding a second first-screen FragmentTree pass.
  if (fragment_parent() == nullptr && needs_restacking_) {
    const uint64_t generation = PrepareRestacking();
    // Platform layout synchronization and behavior callbacks run during the
    // fused traversal. Preserve any re-entrant invalidation they trigger.
    needs_restacking_ = false;
    UpdateLayoutRecursively(this, generation);
  } else {
    // Transition/layout updates can start below the root. Preserve their
    // existing behavior by resolving the complete stacking tree first.
    RestackIfNeeded();
    UpdateLayoutRecursively(this);
  }
}

void Fragment::UpdateLayoutWithoutChange() {
  FlushPendingNodeReadyIfNeeded();
  for (auto* child : children_) {
    child->UpdateLayoutWithoutChange();
  }
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
    layout_info_.SetBorderRadiusInfo(std::move(border_radius_info));
  } else {
    layout_info_.border_radius_info = std::nullopt;
  }
}

void Fragment::UpdateLayoutRecursively(
    Fragment* draw_root, uint64_t restacking_generation,
    base::geometry::FloatPoint active_paint_offset,
    Fragment* active_paint_root) {
  base::geometry::FloatPoint child_active_paint_offset = active_paint_offset;
  Fragment* child_active_paint_root = active_paint_root;
  if (restacking_generation != 0 &&
      !ResolveStackingGeometry(active_paint_offset, active_paint_root, false,
                               &child_active_paint_offset,
                               &child_active_paint_root)) {
    restacking_generation = 0;
  }

  if (has_platform_renderer_) {
    draw_node_capacity_ = kDefaultDrawNodeCapacity;
    draw_root = this;

    if (ShouldSyncNativePlatformRenderer()) {
      painting_context()->UpdateLayout(
          id(), layout_info_.layout_result.offset_.X(),
          layout_info_.layout_result.offset_.Y(),
          layout_info_.layout_result.size_.width_,
          layout_info_.layout_result.size_.height_,
          layout_info_.layout_result.padding_.data(),
          layout_info_.layout_result.margin_.data(),
          layout_info_.layout_result.border_.data(), nullptr, nullptr, 0.f,
          element()->NodeIndex(), element()->display_none());
    }
  } else if (draw_root != nullptr) {
    draw_root->draw_node_capacity_++;
  }

  if (behavior_) {
    behavior_->OnUpdateLayout(layout_info_);
  }
  FlushPendingNodeReadyIfNeeded();

  for (auto* child : children_) {
    child->UpdateLayoutRecursively(draw_root, restacking_generation,
                                   child_active_paint_offset,
                                   child_active_paint_root);
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
