// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_LAYOUT_LINEAR_LAYOUT_ALGORITHM_H_
#define CORE_RENDERER_STARLIGHT_LAYOUT_LINEAR_LAYOUT_ALGORITHM_H_

#include "core/renderer/starlight/layout/layout_algorithm.h"

namespace lynx {
namespace starlight {
class LayoutObject;
class LayoutComputedStyle;

class LinearLayoutAlgorithm : public LayoutAlgorithm {
 public:
  LinearLayoutAlgorithm(LayoutObject*);

  void SizeDeterminationByAlgorithm() override;
  void AlignInFlowItems() override;

  BoxPositions GetAbsoluteOrFixedItemInitialPosition(
      LayoutObject* absolute_or_fixed_item) override;

  Position GetAbsoluteOrFixedItemCrossAxisPosition(
      LayoutObject* absolute_or_fixed_item);
  Position GetAbsoluteOrFixedItemMainAxisPosition(
      LayoutObject* absolute_or_fixed_item);
  void SetContainerBaseline() override;

 protected:
  void Reset() override;
  // Algorithm-1
  void DetermineItemSize();
  // Algorithm-2
  virtual void DetermineContainerSize();
  // Algorithm-3
  virtual void UpdateChildSize(const size_t idx);
  void UpdateChildSizeInternal(const size_t idx,
                               const Constraints& used_container_constraints);
  void CrossAxisAlignment(LayoutObject* item);

  void LayoutWeightedChildren(const InlineFloatArray& base_sizes);

  void InitializeAlgorithmEnv() override;

  virtual void UpdateContainerSize();

  void AfterResultBorderBoxSize() override;

  LinearLayoutGravityType GetComputedLinearLayoutGravity(
      const LayoutComputedStyle& style) const;
  LinearGravityType GetLogicLinearGravityType() const;

  bool IsLayoutGravityDefault(LinearLayoutGravityType layout_gravity) const;

  bool IsLayoutGravityAfter(LinearLayoutGravityType layout_gravity) const;

  bool IsLayoutGravityCenter(LinearLayoutGravityType layout_gravity) const;

  bool IsLayoutGravityFill(LinearLayoutGravityType layout_gravity) const;

  bool IsGravityAfter(LinearGravityType gravity) const;

  bool IsGravityCenter(LinearGravityType gravity) const;

  bool IsGravityPhysical(LinearGravityType gravity) const;

  void HandleScrollView();

  InlineFloatArray main_size_;
  InlineFloatArray cross_size_;
  float total_main_size_;
  float total_cross_size_;
  float remaining_size_;
  float baseline_;
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_LAYOUT_LINEAR_LAYOUT_ALGORITHM_H_
