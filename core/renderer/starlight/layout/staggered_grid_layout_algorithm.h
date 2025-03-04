// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_LAYOUT_STAGGERED_GRID_LAYOUT_ALGORITHM_H_
#define CORE_RENDERER_STARLIGHT_LAYOUT_STAGGERED_GRID_LAYOUT_ALGORITHM_H_

#include "core/renderer/starlight/layout/linear_layout_algorithm.h"

namespace lynx {
namespace starlight {

class StaggeredGridLayoutAlgorithm : public LinearLayoutAlgorithm {
 public:
  StaggeredGridLayoutAlgorithm(LayoutObject*);
  ~StaggeredGridLayoutAlgorithm() = default;

 protected:
  void DetermineContainerSize() override;
  void UpdateContainerSize() override;
  void UpdateChildSize(const size_t idx) override;

 private:
  bool isHeaderFooter(LayoutObject* item);

  int column_count_;
  double cross_axis_gap_;
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_LAYOUT_STAGGERED_GRID_LAYOUT_ALGORITHM_H_
