// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/layout/layout_object.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>

#include "core/renderer/starlight/layout/box_info.h"
#include "core/renderer/starlight/layout/flex_layout_algorithm.h"
#include "core/renderer/starlight/layout/grid_layout_algorithm.h"
#include "core/renderer/starlight/layout/layout_algorithm.h"
#include "core/renderer/starlight/layout/linear_layout_algorithm.h"
#include "core/renderer/starlight/layout/property_resolving_utils.h"
#include "core/renderer/starlight/layout/relative_layout_algorithm.h"
#include "core/renderer/starlight/layout/staggered_grid_layout_algorithm.h"
#include "core/renderer/starlight/style/default_layout_style.h"
#include "core/renderer/starlight/style/layout_style_utils.h"
#include "core/renderer/starlight/types/layout_constraints.h"

namespace lynx {
namespace starlight {

namespace {
inline float GetBoundLeftOffsetFromPaddingBound(const LayoutObject& target,
                                                BoundType bound_type) {
  float result = 0.f;
  switch (bound_type) {
    case BoundType::kMargin:
      result -= target.GetLayoutMarginLeft();
    case BoundType::kBorder:
      result -= target.GetLayoutBorderLeftWidth();
      break;
    case BoundType::kContent:
      result = target.GetLayoutPaddingLeft();
      break;
    case BoundType::kPadding:
    default:
      break;
  }
  return result;
}

float GetBoundTopOffsetFromPaddingBound(const LayoutObject& target,
                                        BoundType bound_type) {
  float result = 0.f;
  switch (bound_type) {
    case BoundType::kMargin:
      result -= target.GetLayoutMarginTop();
    case BoundType::kBorder:
      result -= target.GetLayoutBorderTopWidth();
      break;
    case BoundType::kContent:
      result = target.GetLayoutPaddingTop();
      break;
    case BoundType::kPadding:
    default:
      break;
  }
  return result;
}
inline float GetBoundLeftOffsetFromBorderBound(const LayoutObject& target,
                                               BoundType bound_type) {
  return GetBoundLeftOffsetFromPaddingBound(target, bound_type) +
         target.GetLayoutBorderLeftWidth();
}

inline float GetBoundTopOffsetFromBorderBound(const LayoutObject& target,
                                              BoundType bound_type) {
  return GetBoundTopOffsetFromPaddingBound(target, bound_type) +
         target.GetLayoutBorderTopWidth();
}

}  // namespace

LayoutObject::LayoutObject(const LayoutConfigs& config,
                           const starlight::LayoutComputedStyle* init_style)
    : measure_func_(nullptr),
      request_layout_func_(nullptr),
      alignment_func_(nullptr),
      offset_top_(0),
      offset_left_(0),
      offset_width_(0),
      offset_height_(0),
      offset_baseline_(0),
      algorithm_(nullptr),
      pos_left_(0),
      pos_right_(0),
      pos_top_(0),
      pos_bottom_(0),
      css_style_(const_cast<starlight::LayoutComputedStyle*>(init_style)),
      is_dirty_(false),
      current_node_has_new_layout_(false),
      is_first_layout_(true),
      configs_(config) {
  box_info_ = std::make_unique<BoxInfo>();
}

LayoutObject::~LayoutObject() {
  if (algorithm_) delete algorithm_;
}

void LayoutObject::SetContext(void* context) { context_ = context; }
void* LayoutObject::GetContext() const { return context_; }

void LayoutObject::SetSLMeasureFunc(SLMeasureFunc measure_func) {
  if (!measure_func) {
    measure_func_ = nullptr;
    return;
  }
  measure_func_ = measure_func;
}

SLMeasureFunc LayoutObject::GetSLMeasureFunc() const { return measure_func_; }

void LayoutObject::SetSLRequestLayoutFunc(
    SLRequestLayoutFunc request_layout_func) {
  request_layout_func_ = request_layout_func;
}

void LayoutObject::SetSLAlignmentFunc(SLAlignmentFunc alignment_func) {
  if (!alignment_func) {
    alignment_func_ = nullptr;
    return;
  }
  alignment_func_ = alignment_func;
}

SLAlignmentFunc LayoutObject::GetSLAlignmentFunc() const {
  return alignment_func_;
}

void LayoutObject::RemoveAlgorithm() {
  if (algorithm_) {
    delete algorithm_;
    algorithm_ = nullptr;
  }
}

void LayoutObject::RemoveAlgorithmRecursive() {
  RemoveAlgorithm();
  LayoutObject* child = static_cast<LayoutObject*>(FirstChild());
  while (child) {
    child->RemoveAlgorithmRecursive();
    child = static_cast<LayoutObject*>(child->Next());
  }
}

void LayoutObject::RoundToPixelGrid(const float container_absolute_left,
                                    const float container_absolute_top,
                                    const float container_rounded_left,
                                    const float container_rounded_top,
                                    bool ancestors_have_new_layout) {
  const LayoutObject* container =
      IsNewFixed() ? GetRoot() : ParentLayoutObject();
  float absolute_left =
      container_absolute_left +
      GetBoundLeftFrom(container, BoundType::kBorder, BoundType::kBorder);
  float absolute_top =
      container_absolute_top +
      GetBoundTopFrom(container, BoundType::kBorder, BoundType::kBorder);
  bool layout_changed_since_root = ancestors_have_new_layout ||
                                   is_first_layout_ ||
                                   current_node_has_new_layout_;
  current_node_has_new_layout_ = false;

  // The top / left of list item is decided by platform layout, the top / left
  // here will never be used.
  // Reset top to 0 when scroll orientation is vertical and left to 0 when
  // horizontal, to achieve unified layout result.
  if (!GetEnableFixedNew() || (GetEnableFixedNew() && !IsFixed())) {
    if (parent() && ParentLayoutObject()->IsList()) {
      const LinearOrientationType scroll_orientation =
          ParentLayoutObject()
              ->GetCSSStyle()
              ->linear_data_->linear_orientation_;
      if (scroll_orientation == LinearOrientationType::kVertical ||
          scroll_orientation == LinearOrientationType::kVerticalReverse) {
        absolute_top = 0.f;
      } else {
        absolute_left = 0.f;
      }
    }
  } else if (GetRoot() && GetRoot()->IsList()) {
    // When in a fixed new process, the parent layout object of the fixed
    // node is not the root, the position should be relative to the root.
    const LinearOrientationType scroll_orientation =
        GetRoot()->GetCSSStyle()->linear_data_->linear_orientation_;
    if (scroll_orientation == LinearOrientationType::kVertical ||
        scroll_orientation == LinearOrientationType::kVerticalReverse) {
      absolute_top = 0.f;
    } else {
      absolute_left = 0.f;
    }
  }

  float physical_pixels_per_layout_unit =
      css_style_->PhysicalPixelsPerLayoutUnit();
  float rounded_absolute_top = LayoutStyleUtils::RoundValueToPixelGrid(
      absolute_top, physical_pixels_per_layout_unit);
  float rounded_absolute_left = LayoutStyleUtils::RoundValueToPixelGrid(
      absolute_left, physical_pixels_per_layout_unit);

  if (layout_changed_since_root) {
    const float absolute_right = absolute_left + offset_width_;
    const float absolute_bottom = absolute_top + offset_height_;

    float rounded_absolute_right = LayoutStyleUtils::RoundValueToPixelGrid(
        absolute_right, physical_pixels_per_layout_unit);
    float rounded_absolute_bottom = LayoutStyleUtils::RoundValueToPixelGrid(
        absolute_bottom, physical_pixels_per_layout_unit);

    LayoutResultForRendering new_layout_result;
    new_layout_result.offset_.SetX(rounded_absolute_left -
                                   container_rounded_left);
    new_layout_result.offset_.SetY(rounded_absolute_top -
                                   container_rounded_top);

    float rounded_width = LayoutStyleUtils::RoundValueToPixelGrid(
                              absolute_right, physical_pixels_per_layout_unit) -
                          rounded_absolute_left;
    float rounded_height =
        LayoutStyleUtils::RoundValueToPixelGrid(
            absolute_bottom, physical_pixels_per_layout_unit) -
        rounded_absolute_top;
    new_layout_result.size_.width_ = rounded_width;
    new_layout_result.size_.height_ = rounded_height;

    new_layout_result.border_[kLeft] = LayoutStyleUtils::RoundValueToPixelGrid(
        GetLayoutBorderLeftWidth(), physical_pixels_per_layout_unit);
    new_layout_result.border_[kRight] = LayoutStyleUtils::RoundValueToPixelGrid(
        GetLayoutBorderRightWidth(), physical_pixels_per_layout_unit);
    new_layout_result.border_[kTop] = LayoutStyleUtils::RoundValueToPixelGrid(
        GetLayoutBorderTopWidth(), physical_pixels_per_layout_unit);
    new_layout_result.border_[kBottom] =
        LayoutStyleUtils::RoundValueToPixelGrid(
            GetLayoutBorderBottomWidth(), physical_pixels_per_layout_unit);

    const float content_left = LayoutStyleUtils::RoundValueToPixelGrid(
        (absolute_left + GetLayoutPaddingLeft() + GetLayoutBorderLeftWidth()),
        physical_pixels_per_layout_unit);
    const float content_top = LayoutStyleUtils::RoundValueToPixelGrid(
        (absolute_top + GetLayoutPaddingTop() + GetLayoutBorderTopWidth()),
        physical_pixels_per_layout_unit);
    const float content_right = LayoutStyleUtils::RoundValueToPixelGrid(
        (absolute_right - GetLayoutPaddingRight() -
         GetLayoutBorderRightWidth()),
        physical_pixels_per_layout_unit);
    const float content_bottom = LayoutStyleUtils::RoundValueToPixelGrid(
        (absolute_bottom - GetLayoutPaddingBottom() -
         GetLayoutBorderBottomWidth()),
        physical_pixels_per_layout_unit);

    new_layout_result.padding_[kLeft] =
        content_left - rounded_absolute_left - new_layout_result.border_[kLeft];
    new_layout_result.padding_[kTop] =
        content_top - rounded_absolute_top - new_layout_result.border_[kTop];
    new_layout_result.padding_[kRight] = rounded_absolute_right -
                                         content_right -
                                         new_layout_result.border_[kRight];
    new_layout_result.padding_[kBottom] = rounded_absolute_bottom -
                                          content_bottom -
                                          new_layout_result.border_[kBottom];

    new_layout_result.margin_[kLeft] = GetLayoutMarginLeft();
    new_layout_result.margin_[kTop] = GetLayoutMarginTop();
    new_layout_result.margin_[kRight] = GetLayoutMarginRight();
    new_layout_result.margin_[kBottom] = GetLayoutMarginBottom();

    if (IsSticky()) {
      new_layout_result.sticky_pos_[kLeft] =
          LayoutStyleUtils::RoundValueToPixelGrid(
              pos_left() + container_absolute_left,
              physical_pixels_per_layout_unit) -
          container_rounded_left;
      new_layout_result.sticky_pos_[kRight] =
          rounded_absolute_right -
          LayoutStyleUtils::RoundValueToPixelGrid(
              absolute_right - pos_right(), physical_pixels_per_layout_unit);
      new_layout_result.sticky_pos_[kTop] =
          LayoutStyleUtils::RoundValueToPixelGrid(
              pos_top() + container_absolute_top,
              physical_pixels_per_layout_unit) -
          container_rounded_top;
      new_layout_result.sticky_pos_[kBottom] =
          rounded_absolute_bottom -
          LayoutStyleUtils::RoundValueToPixelGrid(
              absolute_bottom - pos_bottom(), physical_pixels_per_layout_unit);
    }

    // if is first layout or has new layout result or has MeasureFunc && dirty,
    // mark and continue visit child
    if (SetNewLayoutResult(new_layout_result) || is_first_layout_ ||
        (GetSLMeasureFunc() && IsDirty())) {
      MarkHasNewLayout();
    }
  }

  if (is_dirty_ || layout_changed_since_root) {
    LayoutObject* child = static_cast<LayoutObject*>(FirstChild());
    while (child) {
      if (child->IsNewFixed()) {
        child->RoundToPixelGrid(0.f, 0.f, 0.f, 0.f, true);
        child = static_cast<LayoutObject*>(child->Next());
        continue;
      }
      child->RoundToPixelGrid(absolute_left, absolute_top,
                              rounded_absolute_left, rounded_absolute_top,
                              layout_changed_since_root);
      child = static_cast<LayoutObject*>(child->Next());
    }
  }
}

bool LayoutObject::SetNewLayoutResult(LayoutResultForRendering new_result) {
  auto IsLayoutResultDiff = [](DirectionValue<float>& old_direction_values,
                               DirectionValue<float>& new_direction_values) {
    for (int i = 0; i < kDirectionCount; i++) {
      if (!base::FloatsEqual(old_direction_values[i],
                             new_direction_values[i])) {
        return true;
      }
    }

    return false;
  };
  if (!base::FloatsEqual(layout_result_.size_.width_,
                         new_result.size_.width_) ||
      !base::FloatsEqual(layout_result_.size_.height_,
                         new_result.size_.height_) ||
      !base::FloatsEqual(layout_result_.offset_.X(), new_result.offset_.X()) ||
      !base::FloatsEqual(layout_result_.offset_.Y(), new_result.offset_.Y()) ||
      IsLayoutResultDiff(layout_result_.padding_, new_result.padding_) ||
      IsLayoutResultDiff(layout_result_.border_, new_result.border_) ||
      IsLayoutResultDiff(layout_result_.margin_, new_result.margin_) ||
      IsLayoutResultDiff(layout_result_.sticky_pos_, new_result.sticky_pos_)) {
    layout_result_ = new_result;
    return true;
  }

  return false;
}

BoxInfo* LayoutObject::GetBoxInfo() const { return box_info_.get(); }

void LayoutObject::ReLayout(const SLNodeSet* fixed_node_set) {
  Constraints constraints;
  UpdateConstraintsForViewport(constraints);
  ReLayoutWithConstraints(constraints, fixed_node_set);
}

void LayoutObject::ReLayoutWithConstraints(Constraints& constraints,
                                           const SLNodeSet* fixed_node_set) {
  MarkDirty();
  box_info_->InitializeBoxInfo(constraints, *this, GetLayoutConfigs());
  MarkHasNewLayout();
  SendLayoutEvent(LayoutEventType::UpdateMeasureBegin);
  UpdateMeasure(constraints, true, fixed_node_set);
  SendLayoutEvent(LayoutEventType::UpdateMeasureEnd);
  SendLayoutEvent(LayoutEventType::UpdateAlignmentBegin);
  UpdateAlignment();
  SendLayoutEvent(LayoutEventType::UpdateAlignmentEnd);
  SendLayoutEvent(LayoutEventType::RemoveAlgorithmRecursiveBegin);
  RemoveAlgorithmRecursive();
  SendLayoutEvent(LayoutEventType::RemoveAlgorithmRecursiveEnd);
  SendLayoutEvent(LayoutEventType::RoundToPixelGridBegin);
  RoundToPixelGrid(offset_left_, offset_top_, 0.f, 0.f, false);
  SendLayoutEvent(LayoutEventType::RoundToPixelGridEnd);
}

void LayoutObject::SendLayoutEvent(LayoutEventType type,
                                   const LayoutEventData& data) {
  if (event_handler_) {
    event_handler_->OnLayoutEvent(this, type, data);
  }
}

void LayoutObject::UpdateConstraintsForViewport(Constraints& constraints) {
  // By default, measure mode is Indefinite
  SLMeasureMode width_mode = SLMeasureModeIndefinite,
                height_mode = SLMeasureModeIndefinite;
  offset_width_ = 0;
  offset_height_ = 0;

  LayoutUnit indefinite_unit;

  // if max-width/max-height is set, measure mode is AtMost
  auto max_width =
      NLengthToLayoutUnit(css_style_->GetMaxWidth(), indefinite_unit);
  auto max_height =
      NLengthToLayoutUnit(css_style_->GetMaxHeight(), indefinite_unit);
  if (max_width.IsDefinite() &&
      css_style_->GetMaxWidth() != DefaultLayoutStyle::SL_DEFAULT_MAX_WIDTH()) {
    offset_width_ = max_width.ToFloat();
    width_mode = SLMeasureModeAtMost;
  }
  if (max_height.IsDefinite() &&
      css_style_->GetMaxHeight() !=
          DefaultLayoutStyle::SL_DEFAULT_MAX_HEIGHT()) {
    offset_height_ = max_height.ToFloat();
    height_mode = SLMeasureModeAtMost;
  }

  // if width/height is set, measure mode is Definite
  auto width = NLengthToLayoutUnit(css_style_->GetWidth(), indefinite_unit);
  auto height = NLengthToLayoutUnit(css_style_->GetHeight(), indefinite_unit);
  if (width.IsDefinite()) {
    offset_width_ = width.ToFloat();
    width_mode = SLMeasureModeDefinite;
  }
  if (height.IsDefinite()) {
    offset_height_ = height.ToFloat();
    height_mode = SLMeasureModeDefinite;
  }

  // TODO(zhixuan): refactor it later....
  constraints[kHorizontal] = OneSideConstraint(offset_width_, width_mode);
  constraints[kVertical] = OneSideConstraint(offset_height_, height_mode);
}

void LayoutObject::MarkDirtyAndRequestLayout(bool force) {
  MarkDirtyInternal(true, force);
}

void LayoutObject::MarkDirty() { MarkDirtyInternal(false); }

void LayoutObject::ClearCache() {
  cache_manager_.ResetCache();
  cached_can_reuse_layout_result_[kVertical].reset();
  cached_can_reuse_layout_result_[kHorizontal].reset();
  inflow_sub_tree_in_sync_with_last_measurement_ = false;
}

void LayoutObject::MarkDirtyInternal(bool request_layout, bool force) {
  if (force || !IsDirty()) {
    is_dirty_ = true;
    if (request_layout && request_layout_func_) {
      request_layout_func_(context_);
    }
    ClearCache();
    LayoutObject* parent = static_cast<LayoutObject*>(parent_);
    if (parent != nullptr && (force || !parent->IsDirty())) {
      parent->MarkDirtyInternal(request_layout, force);
    }
  }
}

void LayoutObject::MarkChildrenDirtyWithoutTriggerLayout() {
  int child_size = GetChildCount();
  for (int i = 0; i < child_size; ++i) {
    auto* child = static_cast<LayoutObject*>(Find(i));
    child->MarkDirty();
  }
}

void LayoutObject::MarkDirtyWithoutResetCache() {
  if (!IsDirty()) {
    // This function is used within layout stage.
    // dirty function should not be triggered here
    is_dirty_ = true;
    LayoutObject* parent = static_cast<LayoutObject*>(parent_);
    if (parent != nullptr && !parent->IsDirty()) {
      parent->MarkDirtyWithoutResetCache();
    }
  }
}

bool LayoutObject::IsDirty() { return is_dirty_; }

void LayoutObject::MarkUpdated() {
  current_node_has_new_layout_ = false;
  is_dirty_ = false;
  is_first_layout_ = false;
}

void LayoutObject::MarkHasNewLayout() {
  current_node_has_new_layout_ = true;
  MarkDirtyWithoutResetCache();
}

bool LayoutObject::GetHasNewLayout() const {
  return current_node_has_new_layout_;
}

void LayoutObject::SetBorderBoundTopFromParentPaddingBound(float offset_top) {
  if (!base::FloatsEqual(offset_top_, offset_top)) {
    MarkHasNewLayout();
    offset_top_ = offset_top;
  }
}
void LayoutObject::SetBorderBoundLeftFromParentPaddingBound(float offset_left) {
  if (!base::FloatsEqual(offset_left_, offset_left)) {
    MarkHasNewLayout();
    offset_left_ = offset_left;
  }
}
void LayoutObject::SetBorderBoundWidth(float offset_width) {
  if (!base::FloatsEqual(offset_width_, offset_width)) {
    MarkHasNewLayout();
    offset_width_ = offset_width;
  }
}
void LayoutObject::SetBorderBoundHeight(float offset_height) {
  if (!base::FloatsEqual(offset_height_, offset_height)) {
    MarkHasNewLayout();
    offset_height_ = offset_height;
  }
}

void LayoutObject::SetBaseline(float offset_baseline) {
  if (!base::FloatsEqual(offset_baseline_, offset_baseline)) {
    MarkHasNewLayout();
    offset_baseline_ = offset_baseline;
  }
}

float LayoutObject::ClampExactHeight(float height) const {
  height = std::max(height, box_info_->min_size_[kVertical]);
  height = std::min(height, box_info_->max_size_[kVertical]);
  return std::max(GetPaddingAndBorderVertical(), height);
}

float LayoutObject::ClampExactWidth(float width) const {
  width = std::max(width, box_info_->min_size_[kHorizontal]);
  width = std::min(width, box_info_->max_size_[kHorizontal]);
  return std::max(GetPaddingAndBorderHorizontal(), width);
}

bool LayoutObject::FetchEarlyReturnResultForMeasure(
    const Constraints& constraints, bool is_trying, FloatSize& result) {
  if (!measure_func_ && !GetChildCount()) {
    // No need to early return for trivial leaf node
    return false;
  }

  const auto cache = cache_manager_.FindAvailableCacheEntry(constraints, *this);

  if (cache.cache_) {
    // Matching cache is found

    if (!is_trying &&
        ((!IsInflowSubTreeInSyncWithLastMeasurement() && GetChildCount()) ||
         !cache.is_cache_in_sync_with_current_state)) {
      // When not trying and current subtree is not in sync with the result
      // of given constraints, the subtree have to be re-layout
      // to make sure the whole subtree is in sync
      return false;
    } else {
      result.width_ = cache.cache_->border_bound_width_;
      result.height_ = cache.cache_->border_bound_height_;
      if (!GetChildCount()) {
        inflow_sub_tree_in_sync_with_last_measurement_ =
            cache.is_cache_in_sync_with_current_state;
      } else {
        inflow_sub_tree_in_sync_with_last_measurement_ =
            cache.is_cache_in_sync_with_current_state &&
            IsInflowSubTreeInSyncWithLastMeasurement();
      }
      DCHECK(is_trying || inflow_sub_tree_in_sync_with_last_measurement_);
      return true;
    }
  }

  if (constraints[kHorizontal].Mode() == SLMeasureModeDefinite &&
      constraints[kVertical].Mode() == SLMeasureModeDefinite && is_trying) {
    result.height_ = constraints[kVertical].Size();
    result.width_ = constraints[kHorizontal].Size();
    inflow_sub_tree_in_sync_with_last_measurement_ = false;
    return true;
  }
  return false;
}

bool LayoutObject::CanReuseLayoutWithSameSizeAsGivenConstraint(
    bool is_horizontal) const {
  if (BoxInfo().IsDependentOnPercentBase(is_horizontal)) {
    return false;
  }

  if (measure_func_) {
    if (!is_horizontal || GetLayoutConfigs().IsFullQuirksMode()) {
      const auto& min_size = is_horizontal ? css_style_->GetMinWidth()
                                           : css_style_->GetMinHeight();
      const auto& max_size = is_horizontal ? css_style_->GetMaxWidth()
                                           : css_style_->GetMaxHeight();
      if (min_size != DefaultLayoutStyle::SL_DEFAULT_MIN_WIDTH()) {
        return false;
      }
      if (max_size != DefaultLayoutStyle::SL_DEFAULT_MAX_WIDTH()) {
        return false;
      }
    }
    if (!CanReuseLayoutResultForCustomMeasureNode(is_horizontal)) {
      return false;
    }
  } else {
    // TODO(wangzhixuan.0821):review the following checks.
    const auto display = css_style_->GetDisplay(configs_, attr_map());
    if (display == DisplayType::kLinear || display == DisplayType::kFlex) {
      if (is_horizontal != css_style_->IsRow(configs_, attr_map())) {
        return false;
      }
    } else if (display == DisplayType::kRelative) {
      return false;
    }
  }
  constexpr std::array kHorizontalRelated = {
      &LayoutComputedStyle::GetWidth,
      &LayoutComputedStyle::GetMinWidth,
      &LayoutComputedStyle::GetMaxWidth,
      &LayoutComputedStyle::GetPaddingTop,
      &LayoutComputedStyle::GetPaddingLeft,
      &LayoutComputedStyle::GetPaddingBottom,
      &LayoutComputedStyle::GetPaddingRight,
      &LayoutComputedStyle::GetMarginTop,
      &LayoutComputedStyle::GetMarginLeft,
      &LayoutComputedStyle::GetMarginBottom,
      &LayoutComputedStyle::GetMarginRight,
  };
  constexpr std::array kVerticalRelated = {
      &LayoutComputedStyle::GetHeight,
      &LayoutComputedStyle::GetMinHeight,
      &LayoutComputedStyle::GetMaxHeight,
  };
  // TODO(wangzhixuan.0821):review the following checks.
  for (Node* node = FirstChild(); node != nullptr; node = node->Next()) {
    const auto* css = static_cast<LayoutObject*>(node)->GetCSSStyle();

    if (is_horizontal) {
      for (const auto& entry : kHorizontalRelated) {
        if ((css->*entry)().ContainsPercentage()) {
          return false;
        }
      }
    } else {
      for (const auto& entry : kVerticalRelated) {
        if ((css->*entry)().ContainsPercentage()) {
          return false;
        }
      }
    }
  }
  return true;
}

FloatSize LayoutObject::UpdateMeasureByPlatform(const Constraints& constraints,
                                                bool final_measure) {
  Constraints item_constraints =
      property_utils::GenerateDefaultConstraints(*this, constraints);
  box_info_->InitializeBoxInfo(item_constraints, *this, GetLayoutConfigs());
  FloatSize size = UpdateMeasure(item_constraints, final_measure);
  size.width_ += GetLayoutMarginLeft() + GetLayoutMarginRight();
  size.height_ += GetLayoutMarginTop() + GetLayoutMarginBottom();
  size.baseline_ = GetOffsetFromTopMarginEdgeToBaseline();
  return size;
}

void LayoutObject::AlignmentByPlatform(float offset_top, float offset_left) {
  const LayoutObject* container =
      IsNewFixed() ? GetRoot() : ParentLayoutObject();
  SetBoundLeftFrom(container, offset_left, BoundType::kMargin,
                   BoundType::kContent);
  SetBoundTopFrom(container, offset_top, BoundType::kMargin,
                  BoundType::kContent);
  UpdateAlignment();
}

FloatSize LayoutObject::UpdateMeasure(const Constraints& given_constraints,
                                      bool final_measure,
                                      const SLNodeSet* fixed_node_set) {
  Constraints constraints = given_constraints;
  property_utils::ApplyMinMaxToConstraints(constraints, *this);

  final_measure_ = final_measure;

  FloatSize result;
  if (FetchEarlyReturnResultForMeasure(constraints, !final_measure, result)) {
    result.baseline_ = GetBaseline();
    return result;
  }

  const auto ReturnCurrentSizeAndInsertToCache = [this, &constraints]() {
    FloatSize size;
    cache_manager_.InsertCacheEntry(constraints, GetBorderBoundWidth(),
                                    GetBorderBoundHeight());
    size.width_ = GetBorderBoundWidth();
    size.height_ = GetBorderBoundHeight();
    size.baseline_ = GetBaseline();
    return size;
  };

  if (measure_func_) {
    UpdateMeasureWithMeasureFunc(constraints, final_measure);
    inflow_sub_tree_in_sync_with_last_measurement_ = true;
    for (Node* node = FirstChild(); node != nullptr; node = node->Next()) {
      if (!static_cast<LayoutObject*>(node)
               ->IsInflowSubTreeInSyncWithLastMeasurement()) {
        inflow_sub_tree_in_sync_with_last_measurement_ = false;
        break;
      }
    }

    return ReturnCurrentSizeAndInsertToCache();
  }

  /* IF THE NODE HAS NO CHILD, WE DO NOT NEED TO CREATE THE LAYOUT ALGORITHM*/
  if (!GetChildCount()) {
    UpdateMeasureWithLeafNode(constraints);
    inflow_sub_tree_in_sync_with_last_measurement_ = true;
    return ReturnCurrentSizeAndInsertToCache();
  }

  if (!algorithm_) {
    const auto type = css_style_->GetDisplay(configs_, attr_map());
    if (type == DisplayType::kNone) {
      return ReturnCurrentSizeAndInsertToCache();
    }

    if (type == DisplayType::kFlex) {
      algorithm_ = new FlexLayoutAlgorithm(this);
    } else if (type == DisplayType::kLinear) {
      if (attr_map_.getColumnCount().has_value()) {
        algorithm_ = new StaggeredGridLayoutAlgorithm(this);
      } else {
        algorithm_ = new LinearLayoutAlgorithm(this);
      }
    } else if (type == DisplayType::kRelative) {
      // Because of starlight standalone, we can't use FeatureCounter's instance
      // directly, and sent event to layoutcontext instead.
      SendLayoutEvent(LayoutEventType::FeatureCountOnRelativeDisplay);
      algorithm_ = new RelativeLayoutAlgorithm(this);
    } else if (type == DisplayType::kGrid) {
      SendLayoutEvent(LayoutEventType::FeatureCountOnGridDisplay);
      algorithm_ = new GridLayoutAlgorithm(this);
    }

    DCHECK(algorithm_);

    algorithm_->Initialize(constraints, fixed_node_set);
  } else {
    algorithm_->Update(constraints);
    // TODO: Handling boxdata and flexinfo when using cache.
  }
  inflow_sub_tree_in_sync_with_last_measurement_ = true;

  FloatSize size = algorithm_->SizeDetermination();
  inflow_sub_tree_in_sync_with_last_measurement_ =
      algorithm_->IsInflowSubTreeInSync();

  SetBorderBoundWidth(size.width_);
  SetBorderBoundHeight(size.height_);

  algorithm_->SetContainerBaseline();

  return ReturnCurrentSizeAndInsertToCache();
}

void LayoutObject::UpdateMeasureWithMeasureFunc(const Constraints& constraints,
                                                bool final_measure) {
  // Adapter code will be a little bit dirty but fine. It is unavoidable
  // anyways.
  float width = 0.f, height = 0.f;
  if (!IsSLIndefiniteMode(constraints[kHorizontal].Mode())) {
    width = ClampExactWidth(constraints[kHorizontal].Size());
  }
  if (!IsSLIndefiniteMode(constraints[kVertical].Mode())) {
    height = ClampExactHeight(constraints[kVertical].Size());
  }

  SLMeasureMode width_mode = constraints[kHorizontal].Mode();
  SLMeasureMode height_mode = constraints[kVertical].Mode();

  float inner_width = std::max(GetInnerWidthFromBorderBoxWidth(width), 0.0f);
  float inner_height =
      std::max(GetInnerHeightFromBorderBoxHeight(height), 0.0f);

  // prevent width from being affected by float.
  if (base::FloatsEqual(std::ceil(inner_width), inner_width)) {
    inner_width = std::ceil(inner_width);
  }

  if (base::FloatsEqual(std::floor(inner_width), inner_width)) {
    inner_width = std::floor(inner_width);
  }

  Constraints inner_constraints;
  inner_constraints[kHorizontal] = OneSideConstraint(inner_width, width_mode);
  inner_constraints[kVertical] = OneSideConstraint(inner_height, height_mode);

  FloatSize size = measure_func_(context_, inner_constraints, final_measure);

  SetBaseline(size.baseline_);

  float physical_pixels_per_layout_unit =
      css_style_->PhysicalPixelsPerLayoutUnit();
  // To avoid unexpected line break
  if (width_mode == SLMeasureModeDefinite) {
    size.width_ = inner_width;
  } else {
    size.width_ = std::ceil(size.width_ * physical_pixels_per_layout_unit) /
                  physical_pixels_per_layout_unit;
  }
  if (height_mode == SLMeasureModeDefinite) {
    size.height_ = inner_height;
  } else {
    size.height_ = std::ceil(size.height_ * physical_pixels_per_layout_unit) /
                   physical_pixels_per_layout_unit;
  }

  float layout_width =
      ClampExactWidth(size.width_ + GetPaddingAndBorderHorizontal());
  float layout_height =
      ClampExactHeight(size.height_ + GetPaddingAndBorderVertical());
  inner_width = std::max(GetInnerWidthFromBorderBoxWidth(layout_width), 0.0f);
  inner_height =
      std::max(GetInnerHeightFromBorderBoxHeight(layout_height), 0.0f);

  // Fix like 'text-align: right' display when measure_func_ is affected by
  // min/max size
  if (!GetLayoutConfigs().IsFullQuirksMode() &&
      (base::FloatsLarger(inner_width, size.width_) ||
       base::FloatsLarger(inner_height, size.height_))) {
    inner_constraints[kHorizontal] = OneSideConstraint::Definite(inner_width);
    inner_constraints[kVertical] = OneSideConstraint::Definite(inner_height);
    measure_func_(context_, inner_constraints, final_measure);
  }

  SetBorderBoundWidth(layout_width);
  SetBorderBoundHeight(layout_height);

  /* NO LAYOUT ALGORITHM AND NO ALIGNMENT TO CALL, SO UP TO DATE HERE*/
  //  UpToDate();
}

void LayoutObject::UpdateMeasureWithLeafNode(const Constraints& constraints) {
  /*LAYOUT OBJECT WITH ZERO CHILD DOES NOT CALL FOR A LAYOUT ALGORITHM
   * IT CAN DETERMINE ITS SIZE IMMEDIATELY*/
  float width_to_set = IsSLDefiniteMode(constraints[kHorizontal].Mode())
                           ? constraints[kHorizontal].Size()
                           : 0;
  float height_to_set = IsSLDefiniteMode(constraints[kVertical].Mode())
                            ? constraints[kVertical].Size()
                            : 0;
  SetBorderBoundWidth(ClampExactWidth(width_to_set));
  SetBorderBoundHeight(ClampExactHeight(height_to_set));
}

void LayoutObject::UpdateAlignment() {
  double border_box_offset_left = GetBorderBoundLeftFromParentPaddingBound();
  double border_box_offset_top = GetBorderBoundTopFromParentPaddingBound();

  if (!measured_position_.Reset(border_box_offset_left, border_box_offset_top,
                                border_box_offset_left + offset_width_,
                                border_box_offset_top + offset_height_) &&
      !IsDirty()) {
    return;
  }
  if (alignment_func_) {
    alignment_func_(context_);
    return;
  }

  if (algorithm_) {
    algorithm_->Alignment();
  }
}

void LayoutObject::UpdateSize(float width, float height) {
  if (base::FloatsEqual(width, offset_width_) &&
      base::FloatsEqual(height, offset_height_))
    return;
  offset_width_ = width;
  offset_height_ = height;
  MarkHasNewLayout();
}

void LayoutObject::HideLayoutObject() {
  SetBorderBoundTopFromParentPaddingBound(0);
  SetBorderBoundWidth(0);
  SetBorderBoundHeight(0);
  SetBorderBoundLeftFromParentPaddingBound(0);
  measured_position_.Reset(0, 0, 0, 0);
  MarkHasNewLayout();
  for (int i = 0; i < GetChildCount(); ++i) {
    LayoutObject* child = static_cast<LayoutObject*>(Find(i));
    child->HideLayoutObject();
  }
  // When hiding layout, insert an empty cache with negative constraints
  // area, to mark the last cached measurement is not in sync with the current
  // state of the layout object.
  Constraints constraints;
  constraints[kHorizontal] = constraints[kVertical] =
      OneSideConstraint::Definite(-1.f);
  cache_manager_.InsertCacheEntry(constraints, 0.f, 0.f);
}

void LayoutObject::LayoutDisplayNone() { HideLayoutObject(); }

std::vector<double> LayoutObject::GetBoxModel() {
  std::vector<double> res;
  res.push_back(offset_width_ - GetLayoutPaddingLeft() -
                GetLayoutPaddingRight() - GetLayoutBorderLeftWidth() -
                GetLayoutBorderRightWidth());
  res.push_back(offset_height_ - GetLayoutPaddingTop() -
                GetLayoutPaddingBottom() - GetLayoutBorderTopWidth() -
                GetLayoutBorderBottomWidth());

  float temp_root_x = 0;
  float temp_root_y = 0;
  auto temp_parent = parent_;
  while (temp_parent != nullptr) {
    temp_root_x += static_cast<LayoutObject*>(temp_parent)
                       ->GetBorderBoundLeftFromParentPaddingBound();
    temp_root_y += static_cast<LayoutObject*>(temp_parent)
                       ->GetBorderBoundTopFromParentPaddingBound();
    temp_parent = static_cast<LayoutObject*>(temp_parent)->parent_;
  }
  // content
  res.push_back(temp_root_x + GetBorderBoundLeftFromParentPaddingBound() +
                GetLayoutPaddingLeft() + GetLayoutBorderLeftWidth());
  res.push_back(temp_root_y + GetBorderBoundTopFromParentPaddingBound() +
                GetLayoutPaddingTop() + GetLayoutBorderTopWidth());
  res.push_back(temp_root_x + GetBorderBoundLeftFromParentPaddingBound() +
                offset_width_ - GetLayoutPaddingRight() -
                GetLayoutBorderRightWidth());
  res.push_back(temp_root_y + GetBorderBoundTopFromParentPaddingBound() +
                GetLayoutPaddingTop() + GetLayoutBorderTopWidth());
  res.push_back(temp_root_x + GetBorderBoundLeftFromParentPaddingBound() +
                offset_width_ - GetLayoutPaddingRight() -
                GetLayoutBorderRightWidth());
  res.push_back(temp_root_y + GetBorderBoundTopFromParentPaddingBound() +
                offset_height_ - GetLayoutPaddingBottom() -
                GetLayoutBorderBottomWidth());
  res.push_back(temp_root_x + GetBorderBoundLeftFromParentPaddingBound() +
                GetLayoutPaddingLeft() + GetLayoutBorderLeftWidth());
  res.push_back(temp_root_y + GetBorderBoundTopFromParentPaddingBound() +
                offset_height_ - GetLayoutPaddingBottom() -
                GetLayoutBorderBottomWidth());

  // padding
  res.push_back(res[2] - GetLayoutPaddingLeft());
  res.push_back(res[3] - GetLayoutPaddingTop());
  res.push_back(res[4] + GetLayoutPaddingRight());
  res.push_back(res[5] - GetLayoutPaddingTop());
  res.push_back(res[6] + GetLayoutPaddingRight());
  res.push_back(res[7] + GetLayoutPaddingBottom());
  res.push_back(res[8] - GetLayoutPaddingLeft());
  res.push_back(res[9] + GetLayoutPaddingBottom());

  // border
  res.push_back(res[10] - GetLayoutBorderLeftWidth());
  res.push_back(res[11] - GetLayoutBorderTopWidth());
  res.push_back(res[12] + GetLayoutBorderRightWidth());
  res.push_back(res[13] - GetLayoutBorderTopWidth());
  res.push_back(res[14] + GetLayoutBorderRightWidth());
  res.push_back(res[15] + GetLayoutBorderBottomWidth());
  res.push_back(res[16] - GetLayoutBorderLeftWidth());
  res.push_back(res[17] + GetLayoutBorderBottomWidth());

  // margin
  res.push_back(res[18] - GetLayoutMarginLeft());
  res.push_back(res[19] - GetLayoutMarginTop());
  res.push_back(res[20] + GetLayoutMarginRight());
  res.push_back(res[21] - GetLayoutMarginTop());
  res.push_back(res[22] + GetLayoutMarginRight());
  res.push_back(res[23] + GetLayoutMarginBottom());
  res.push_back(res[24] - GetLayoutMarginLeft());
  res.push_back(res[25] + GetLayoutMarginBottom());

  return res;
}

float LayoutObject::GetInnerWidthFromBorderBoxWidth(float width) const {
  return width - GetPaddingAndBorderHorizontal();
}
float LayoutObject::GetInnerHeightFromBorderBoxHeight(float height) const {
  return height - GetPaddingAndBorderVertical();
}

float LayoutObject::GetOuterWidthFromBorderBoxWidth(float width) const {
  return width + GetLayoutMarginLeft() + GetLayoutMarginRight();
}

float LayoutObject::GetOuterHeightFromBorderBoxHeight(float height) const {
  return height + GetLayoutMarginTop() + GetLayoutMarginBottom();
}

float LayoutObject::GetPaddingAndBorderHorizontal() const {
  return GetLayoutPaddingLeft() + GetLayoutPaddingRight() +
         css_style_->GetBorderFinalWidthHorizontal();
}

float LayoutObject::GetPaddingAndBorderVertical() const {
  return GetLayoutPaddingTop() + GetLayoutPaddingBottom() +
         css_style_->GetBorderFinalWidthVertical();
}

float LayoutObject::GetBorderBoxWidthFromInnerWidth(float inner_width) const {
  return inner_width + GetPaddingAndBorderHorizontal();
}
float LayoutObject::GetBorderBoxHeightFromInnerHeight(
    float inner_height) const {
  return inner_height + GetPaddingAndBorderVertical();
}

void LayoutObject::Reset(LayoutObject* node) {
  // Remove all children. Need to set child's prev & next to nullptr
  while (FirstChild()) {
    RemoveChild(static_cast<ContainerNode*>(FirstChild()));
  }
  measured_position_.Reset(0, 0, 0, 0);
  SetSLMeasureFunc(nullptr);
  SetContext(nullptr);
  SetBorderBoundWidth(node->GetBorderBoundWidth());
  SetBorderBoundHeight(node->GetBorderBoundHeight());
  SetBorderBoundLeftFromParentPaddingBound(
      node->GetBorderBoundLeftFromParentPaddingBound());
  SetBorderBoundTopFromParentPaddingBound(
      node->GetBorderBoundTopFromParentPaddingBound());

  RemoveAlgorithm();
  css_style_->Reset();
  is_dirty_ = false;
}

float LayoutObject::GetLayoutPaddingLeft() const {
  return box_info_->padding_[kLeft];
}
float LayoutObject::GetLayoutPaddingTop() const {
  return box_info_->padding_[kTop];
}
float LayoutObject::GetLayoutPaddingRight() const {
  return box_info_->padding_[kRight];
}
float LayoutObject::GetLayoutPaddingBottom() const {
  return box_info_->padding_[kBottom];
}
float LayoutObject::GetLayoutMarginLeft() const {
  return box_info_->margin_[kLeft];
}
float LayoutObject::GetLayoutMarginTop() const {
  return box_info_->margin_[kTop];
}
float LayoutObject::GetLayoutMarginRight() const {
  return box_info_->margin_[kRight];
}
float LayoutObject::GetLayoutMarginBottom() const {
  return box_info_->margin_[kBottom];
}
float LayoutObject::GetLayoutBorderLeftWidth() const {
  return css_style_->GetBorderFinalLeftWidth();
}
float LayoutObject::GetLayoutBorderTopWidth() const {
  return css_style_->GetBorderFinalTopWidth();
}
float LayoutObject::GetLayoutBorderRightWidth() const {
  return css_style_->GetBorderFinalRightWidth();
}
float LayoutObject::GetLayoutBorderBottomWidth() const {
  return css_style_->GetBorderFinalBottomWidth();
}

float LayoutObject::GetContentBoundWidth() const {
  return GetBorderBoundWidth() - GetPaddingAndBorderHorizontal();
}
float LayoutObject::GetContentBoundHeight() const {
  return GetBorderBoundHeight() - GetPaddingAndBorderVertical();
}

float LayoutObject::GetMarginBoundWidth() const {
  return GetBorderBoundWidth() + GetLayoutMarginLeft() + GetLayoutMarginRight();
}
float LayoutObject::GetMarginBoundHeight() const {
  return GetBorderBoundHeight() + GetLayoutMarginTop() +
         GetLayoutMarginBottom();
}

float LayoutObject::GetPaddingBoundWidth() const {
  return GetBorderBoundWidth() - GetLayoutBorderLeftWidth() -
         GetLayoutBorderRightWidth();
}
float LayoutObject::GetPaddingBoundHeight() const {
  return GetBorderBoundHeight() - GetLayoutBorderTopWidth() -
         GetLayoutBorderBottomWidth();
}

float LayoutObject::GetBoundTypeWidth(BoundType type) const {
  switch (type) {
    case BoundType::kBorder:
      return GetBorderBoundWidth();
    case BoundType::kMargin:
      return GetMarginBoundWidth();
    case BoundType::kContent:
      return GetContentBoundWidth();
    case BoundType::kPadding:
      return GetPaddingBoundWidth();
  }

  return 0.f;
}

float LayoutObject::GetBoundTypeHeight(BoundType type) const {
  switch (type) {
    case BoundType::kBorder:
      return GetBorderBoundHeight();
    case BoundType::kMargin:
      return GetMarginBoundHeight();
    case BoundType::kContent:
      return GetContentBoundHeight();
    case BoundType::kPadding:
      return GetPaddingBoundHeight();
  }

  return 0.f;
}

float LayoutObject::GetOffsetFromTopMarginEdgeToBaseline() const {
  return GetLayoutMarginTop() +
         (base::IsZero(offset_baseline_)
              ? offset_height_
              : offset_baseline_ + GetLayoutBorderTopWidth() +
                    GetLayoutPaddingTop());
}

void LayoutObject::UpdatePositions(float left, float top, float right,
                                   float bottom) {
  pos_left_ = left;
  pos_top_ = top;
  pos_right_ = right;
  pos_bottom_ = bottom;
}

float LayoutObject::GetBoundLeftFrom(const LayoutObject* container,
                                     BoundType bound_type,
                                     BoundType container_bound_type) const {
  return offset_left_ + GetBoundLeftOffsetFromBorderBound(*this, bound_type) -
         (container ? GetBoundLeftOffsetFromPaddingBound(*container,
                                                         container_bound_type)
                    : 0);
}

float LayoutObject::GetBoundTopFrom(const LayoutObject* container,
                                    BoundType bound_type,
                                    BoundType container_bound_type) const {
  return offset_top_ + GetBoundTopOffsetFromBorderBound(*this, bound_type) -
         (container ? GetBoundTopOffsetFromPaddingBound(*container,
                                                        container_bound_type)
                    : 0);
}

void LayoutObject::SetBoundLeftFrom(const LayoutObject* container, float value,
                                    BoundType bound_type,
                                    BoundType container_bound_type) {
  SetBorderBoundLeftFromParentPaddingBound(
      value - GetBoundLeftOffsetFromBorderBound(*this, bound_type) +
      (container ? GetBoundLeftOffsetFromPaddingBound(*container,
                                                      container_bound_type)
                 : 0));
}

void LayoutObject::SetBoundTopFrom(const LayoutObject* container, float value,
                                   BoundType bound_type,
                                   BoundType container_bound_type) {
  SetBorderBoundTopFromParentPaddingBound(
      value - GetBoundTopOffsetFromBorderBound(*this, bound_type) +
      (container
           ? GetBoundTopOffsetFromPaddingBound(*container, container_bound_type)
           : 0));
}

void LayoutObject::SetBoundRightFrom(const LayoutObject* container, float value,
                                     BoundType bound_type,
                                     BoundType container_bound_type) {
  float left_offset =
      container ? (container->GetBoundTypeWidth(container_bound_type) -
                   GetBoundTypeWidth(bound_type) - value)
                : 0;
  SetBoundLeftFrom(container, left_offset, bound_type, container_bound_type);
}

void LayoutObject::SetBoundBottomFrom(const LayoutObject* container,
                                      float value, BoundType bound_type,
                                      BoundType container_bound_type) {
  float top_offset =
      container ? (container->GetBoundTypeHeight(container_bound_type) -
                   GetBoundTypeHeight(bound_type) - value)
                : 0;
  SetBoundTopFrom(container, top_offset, bound_type, container_bound_type);
}

void LayoutObject::SetCanReuseLayoutWithSameSizeAsGivenConstraintFunc(
    SLCanReuseLayoutWithSameSizeAsGivenConstraintFunc func) {
  can_reuse_layout_func_ = func;
  ClearCache();
}

bool LayoutObject::CanReuseLayoutResultForCustomMeasureNode(
    bool is_horizontal) const {
  if (!can_reuse_layout_func_) {
    return true;
  }
  Dimension dim = is_horizontal ? kHorizontal : kVertical;
  if (cached_can_reuse_layout_result_[dim].has_value()) {
    return *(cached_can_reuse_layout_result_[dim]);
  }
  cached_can_reuse_layout_result_[dim] =
      can_reuse_layout_func_(GetContext(), is_horizontal);
  return *(cached_can_reuse_layout_result_[dim]);
}
}  // namespace starlight
}  // namespace lynx
