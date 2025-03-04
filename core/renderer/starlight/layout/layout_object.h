// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_LAYOUT_LAYOUT_OBJECT_H_
#define CORE_RENDERER_STARLIGHT_LAYOUT_LAYOUT_OBJECT_H_

#include <assert.h>

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/include/position.h"
#include "core/renderer/starlight/event/layout_event_handler.h"
#include "core/renderer/starlight/layout/cache_manager.h"
#include "core/renderer/starlight/layout/container_node.h"
#include "core/renderer/starlight/layout/layout_global.h"
#include "core/renderer/starlight/style/layout_computed_style.h"
#include "core/renderer/starlight/types/layout_measurefunc.h"
#include "core/renderer/starlight/types/layout_types.h"

namespace lynx {
typedef starlight::LayoutObject SLNode;
typedef std::unordered_set<starlight::LayoutObject*> SLNodeSet;

namespace starlight {
class BoxInfo;
class LayoutAlgorithm;

enum class BoundType { kContent, kPadding, kBorder, kMargin };

class LayoutObject : public ContainerNode {
 public:
  BASE_EXPORT LayoutObject(
      const LayoutConfigs& config,
      const starlight::LayoutComputedStyle* init_style = nullptr);
  virtual ~LayoutObject();

  // exports
  BASE_EXPORT void SetSLRequestLayoutFunc(
      SLRequestLayoutFunc request_layout_func);

  // export for custom layout
  BASE_EXPORT void SetContext(void* context);
  BASE_EXPORT void* GetContext() const;
  BASE_EXPORT void SetSLMeasureFunc(SLMeasureFunc measure_func);
  BASE_EXPORT void SetSLAlignmentFunc(SLAlignmentFunc alignment_func);
  BASE_EXPORT void SetCanReuseLayoutWithSameSizeAsGivenConstraintFunc(
      SLCanReuseLayoutWithSameSizeAsGivenConstraintFunc layout_depends_ofunc);

  void Reset(LayoutObject* node);

  inline const LayoutConfigs& GetLayoutConfigs() const { return configs_; }

  inline void SetRoot(LayoutObject* root) { root_node_ = root; }
  inline const LayoutObject* GetRoot() const {
    return static_cast<const LayoutObject*>(root_node_);
  }
  inline void SetIsFixedBefore(bool is_fixed_before) {
    is_fixed_before_ = is_fixed_before;
  }
  inline bool IsFixedBefore() const { return is_fixed_before_; }
  inline bool IsFixed() const {
    return css_style_->GetPosition() == PositionType::kFixed;
  }
  inline bool IsFixedOrAbsolute() const {
    return css_style_->GetPosition() == PositionType::kFixed ||
           css_style_->GetPosition() == PositionType::kAbsolute;
  }
  inline bool IsNewFixed() const {
    return (css_style_->GetPosition() == PositionType::kFixed) &&
           configs_.enable_fixed_new_;
  }
  inline bool GetEnableFixedNew() const { return configs_.enable_fixed_new_; }

  SLMeasureFunc GetSLMeasureFunc() const;

  SLAlignmentFunc GetSLAlignmentFunc() const;

  FloatSize UpdateMeasureByPlatform(const Constraints& constraints,
                                    bool final_measure);
  void AlignmentByPlatform(float offset_top, float offset_left);

  void MarkDirtyRecursion();

  virtual void MarkDirtyAndRequestLayout(bool force = false);
  virtual void MarkDirty();
  void MarkChildrenDirtyWithoutTriggerLayout();
  BASE_EXPORT bool IsDirty();

  BASE_EXPORT void MarkUpdated();
  bool GetHasNewLayout() const;

  bool GetFinalMeasure() const { return final_measure_; }

  void SetLayoutStyle(LayoutComputedStyle* layout_style) {
    css_style_ = layout_style;
  }
  const LayoutComputedStyle* GetCSSStyle() const { return css_style_; }
  LayoutComputedStyle* GetCSSMutableStyle() { return css_style_; }

  inline AttributesMap& attr_map() { return attr_map_; }
  inline const AttributesMap& attr_map() const { return attr_map_; }

  const base::Position& measured_position() { return measured_position_; }

  // Called to check whether a previous layout can be reused for current
  // measure, when the current constraint is the same as the previous layout
  // result size, but the previous layout constraint is not the same as the
  // current constraint.
  bool CanReuseLayoutWithSameSizeAsGivenConstraint(bool is_horizontal) const;

  const LayoutResultForRendering& GetLayoutResult() const {
    return layout_result_;
  }
  bool IsList() const { return is_list_; }

  // Position is with respect to top-left of parent's padding bound by default
  void SetBorderBoundTopFromParentPaddingBound(float top);
  void SetBorderBoundLeftFromParentPaddingBound(float left);
  // Position is with respect to top-left of parent's padding bound by default
  void SetBorderBoundWidth(float width);
  void SetBorderBoundHeight(float height);
  void SetBaseline(float baseline);

  inline float GetBorderBoundTopFromParentPaddingBound() const {
    return offset_top_;
  }
  inline float GetBorderBoundLeftFromParentPaddingBound() const {
    return offset_left_;
  }
  LayoutObject* ParentLayoutObject() {
    return static_cast<LayoutObject*>(parent_);
  }
  const LayoutObject* ParentLayoutObject() const {
    return static_cast<const LayoutObject*>(parent_);
  }
  inline float GetBorderBoundWidth() const { return offset_width_; }
  inline float GetBorderBoundHeight() const { return offset_height_; }
  // If not have/calculate baseline, return border bound height
  inline float GetBaseline() const {
    return base::IsZero(offset_baseline_) ? offset_height_ : offset_baseline_;
  }
  // Get the distance between cross-start margin edge and its baseline. If not
  // have/calculate baseline, regard the bottom edge of border bound as its
  // baseline when calculating
  float GetOffsetFromTopMarginEdgeToBaseline() const;
  float GetContentBoundWidth() const;
  float GetContentBoundHeight() const;
  float GetMarginBoundWidth() const;
  float GetMarginBoundHeight() const;
  float GetPaddingBoundWidth() const;
  float GetPaddingBoundHeight() const;
  float GetBoundTypeWidth(BoundType type) const;
  float GetBoundTypeHeight(BoundType type) const;

  // Return the top/left offset of requested bound from specified container
  // bound.
  // param container: the specified container.
  // param bound_type: the type of bound that the offset is referred to.
  // param container_bound_type: type of container bound from which the offset
  // is calculated.
  //
  // For example, if input is (container, BoundType:kBorder,
  // BoundType:kContent). The functions will return the top/left offset from
  // container content bound to current border bound.
  // New fixed node's parent is its real parent of dom structure(not root).
  // However, in layout part, fixed nodes are layouted by root's algorithm.
  float GetBoundTopFrom(const LayoutObject* container, BoundType bound_type,
                        BoundType container_bound_type) const;
  float GetBoundLeftFrom(const LayoutObject* container, BoundType bound_type,
                         BoundType container_bound_type) const;

  // Set the top/left offset of requested bound from specified container bound.
  // param container: the specified container.
  // param value: the value of offset to set.
  // param bound_type: the type of bound that the offset is referred to.
  // param container_bound_type: type of container bound from which the offset
  // is calculated.
  //
  // For example, if input is (container, 30.f, BoundType:kBorder,
  // BoundType:kContent) The offset from container content bound to top/left of
  // current border will be set to 30.f
  void SetBoundTopFrom(const LayoutObject* container, float value,
                       BoundType bound_type, BoundType container_bound_type);
  void SetBoundLeftFrom(const LayoutObject* container, float value,
                        BoundType bound_type, BoundType container_bound_type);
  void SetBoundRightFrom(const LayoutObject* container, float value,
                         BoundType bound_type, BoundType container_bound_type);
  void SetBoundBottomFrom(const LayoutObject* container, float value,
                          BoundType bound_type, BoundType container_bound_type);

  void ClearCache();

#define LAYOUT_OBJECT_GET_RESULT(name) \
  BASE_EXPORT_FOR_DEVTOOL float GetLayout##name() const;
  LAYOUT_OBJECT_GET_RESULT(PaddingLeft)
  LAYOUT_OBJECT_GET_RESULT(PaddingTop)
  LAYOUT_OBJECT_GET_RESULT(PaddingRight)
  LAYOUT_OBJECT_GET_RESULT(PaddingBottom)
  LAYOUT_OBJECT_GET_RESULT(MarginLeft)
  LAYOUT_OBJECT_GET_RESULT(MarginRight)
  LAYOUT_OBJECT_GET_RESULT(MarginTop)
  LAYOUT_OBJECT_GET_RESULT(MarginBottom)
  LAYOUT_OBJECT_GET_RESULT(BorderLeftWidth)
  LAYOUT_OBJECT_GET_RESULT(BorderTopWidth)
  LAYOUT_OBJECT_GET_RESULT(BorderRightWidth)
  LAYOUT_OBJECT_GET_RESULT(BorderBottomWidth)
#undef LAYOUT_OBJECT_GET_RESULT

  BoxInfo* GetBoxInfo() const;

  float ClampExactWidth(float width) const;
  float ClampExactHeight(float height) const;

  float GetInnerWidthFromBorderBoxWidth(float width) const;
  float GetInnerHeightFromBorderBoxHeight(float height) const;
  float GetOuterWidthFromBorderBoxWidth(float width) const;
  float GetOuterHeightFromBorderBoxHeight(float height) const;

  float GetBorderBoxWidthFromInnerWidth(float inner_width) const;
  float GetBorderBoxHeightFromInnerHeight(float inner_height) const;

  BASE_EXPORT void ReLayout(const SLNodeSet* fixed_node_set = nullptr);

  void ReLayoutWithConstraints(Constraints& constraints,
                               const SLNodeSet* fixed_node_set = nullptr);

  void UpdateConstraintsForViewport(Constraints& constraints);
  virtual FloatSize UpdateMeasure(const Constraints& constraints,
                                  bool final_measure,
                                  const SLNodeSet* fixed_node_set = nullptr);
  virtual void UpdateAlignment();
  void LayoutDisplayNone();
  std::vector<double> GetBoxModel();

  inline float pos_left() const { return pos_left_; }
  inline float pos_right() const { return pos_right_; }
  inline float pos_top() const { return pos_top_; }
  inline float pos_bottom() const { return pos_bottom_; }

  bool IsSticky() const {
    return css_style_->GetPosition() == PositionType::kSticky;
  }
  void UpdatePositions(float left, float top, float right, float bottom);

  float GetPaddingAndBorderHorizontal() const;
  float GetPaddingAndBorderVertical() const;

  void MarkList() { is_list_ = true; }
  bool IsAbsoluteInContentBound() const {
    return configs_.is_absolute_in_content_bound_;
  }

  bool IsInflowSubTreeInSyncWithLastMeasurement() const {
    return inflow_sub_tree_in_sync_with_last_measurement_;
  }

  void SetTag(const std::string& tag) { tag_ = tag; }

  const std::string& GetTag() const { return tag_; }
  bool need_custom_layout() const { return measure_func_ != nullptr; }
  void SendLayoutEvent(LayoutEventType type,
                       const LayoutEventData& data = LayoutEventData());
  void SetEventHandler(LayoutEventHandler* handler) {
    event_handler_ = handler;
  }

 protected:
  void MarkDirtyWithoutResetCache();
  void MarkHasNewLayout();
  void MarkDirtyInternal(bool request_layout, bool force = false);
  bool SetNewLayoutResult(LayoutResultForRendering new_result);
  void HideLayoutObject();
  void UpdateSize(float width, float height);
  void RoundToPixelGrid(const float container_absolute_left,
                        const float container_absolute_top,
                        const float container_rounded_left,
                        const float container_rounded_top,
                        bool ancestors_have_new_layout);

  void UpdateMeasureWithMeasureFunc(const Constraints& constraints,
                                    bool final_measure);

  void UpdateMeasureWithLeafNode(const Constraints& constraints);

  void RemoveAlgorithm();
  void RemoveAlgorithmRecursive();
  bool CanReuseLayoutResultForCustomMeasureNode(bool is_horizontal) const;

  base::Position measured_position_;

  SLMeasureFunc measure_func_;
  SLRequestLayoutFunc request_layout_func_;
  SLAlignmentFunc alignment_func_;
  SLCanReuseLayoutWithSameSizeAsGivenConstraintFunc can_reuse_layout_func_ =
      nullptr;
  mutable DimensionValue<std::optional<bool>> cached_can_reuse_layout_result_;
  void* context_ = nullptr;

  float offset_top_;
  float offset_left_;
  float offset_width_;
  float offset_height_;
  // The distance from the top edge of the content bound to the baseline.
  float offset_baseline_;

  std::unique_ptr<BoxInfo> box_info_;
  LayoutAlgorithm* algorithm_;

  AttributesMap attr_map_;

  float pos_left_;
  float pos_right_;
  float pos_top_;
  float pos_bottom_;

  LayoutComputedStyle* css_style_;
  bool is_dirty_;
  bool current_node_has_new_layout_;
  bool is_first_layout_;

  bool final_measure_ = false;
  bool is_list_ = false;
  bool is_fixed_before_ = false;
  const LayoutConfigs configs_;
  LayoutObject* root_node_ = nullptr;

  std::string tag_;

  bool inflow_sub_tree_in_sync_with_last_measurement_ = false;

  bool FetchEarlyReturnResultForMeasure(const Constraints& constraints,
                                        bool is_trying, FloatSize& result);

  CacheManager cache_manager_;
  LayoutResultForRendering layout_result_;
  LayoutEventHandler* event_handler_ = nullptr;
};

}  // namespace starlight

}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_LAYOUT_LAYOUT_OBJECT_H_
