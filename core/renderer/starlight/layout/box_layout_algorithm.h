// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_LAYOUT_BOX_LAYOUT_ALGORITHM_H_
#define CORE_RENDERER_STARLIGHT_LAYOUT_BOX_LAYOUT_ALGORITHM_H_

#include "core/renderer/starlight/layout/layout_algorithm.h"

namespace lynx {
namespace starlight {

class BoxLayoutAlgorithm : public LayoutAlgorithm {
 public:
  explicit BoxLayoutAlgorithm(LayoutObject* container);

  void SetContainerBaseline() override;

 protected:
  void Reset() override;
  void InitializeAlgorithmEnv() override;
  void SizeDeterminationByAlgorithm() override;
  void MeasureAbsoluteAndFixed() override;
  void AlignInFlowItems() override;

 private:
  void MeasureChildren();
  void MeasureMatchParentSizeChildren();
  void DetermineContainerSize();
  DimensionValue<float> PropagatedMinConstraintsForChild(
      const LayoutObject& child) const;
  bool MatchesParentSize(const LayoutObject* item) const;
  float ResolveAutoSize(Dimension dimension) const;
  float ResolveAlignmentOffset(LayoutObject* item, Dimension dimension) const;
  float AvailableContentSize(Dimension dimension) const;
  float ItemMarginBoundSize(LayoutObject* item, Dimension dimension) const;

  float max_child_margin_bound_width_{0.f};
  float max_child_margin_bound_height_{0.f};
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_LAYOUT_BOX_LAYOUT_ALGORITHM_H_
