// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_LAYOUT_ROW_COLUMN_LAYOUT_ALGORITHM_H_
#define CORE_RENDERER_STARLIGHT_LAYOUT_ROW_COLUMN_LAYOUT_ALGORITHM_H_

#include "base/include/vector.h"
#include "core/renderer/starlight/layout/layout_algorithm.h"

namespace lynx {
namespace starlight {

// Compose Row/Column own their range-based measurement and placement. CSS
// size/mode constraints must be adapted before entering this algorithm. Sizes
// and positions retain floating-point layout units without pixel rounding.
class RowColumnLayoutAlgorithm final : public LayoutAlgorithm {
 public:
  explicit RowColumnLayoutAlgorithm(LayoutObject* container)
      : LayoutAlgorithm(container) {}

  void Alignment() override;
  void SetContainerBaseline() override;
  bool RequiresComposeConstraints() const override { return true; }

 protected:
  void Reset() override;
  void InitializeChildren(const SLNodeSet* fixed_node_set) override;
  void InitializeAlgorithmEnv() override;
  void SizeDeterminationByAlgorithm() override;
  void AlignInFlowItems() override;

 private:
  struct Child {
    FloatSize size;
    float main_position = 0;
  };

  FloatSize MeasureChildren();
  void MeasureChild(size_t index, float main_min, float main_max,
                    float cross_max);
  void ArrangeChildren();
  FlexAlignType CrossAlignment(const LayoutObject& child) const;
  float CrossPosition(size_t index) const;

  base::Vector<Child> children_;
  float spacing_ = 0;
  float main_layout_size_ = 0;
  float before_alignment_line_ = 0;
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_LAYOUT_ROW_COLUMN_LAYOUT_ALGORITHM_H_
