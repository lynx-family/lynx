// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// Copyright 2021 The Lynx Authors. All rights reserved.

#ifndef CORE_RENDERER_STARLIGHT_STYLE_GRID_DATA_H_
#define CORE_RENDERER_STARLIGHT_STYLE_GRID_DATA_H_

#include <vector>

#include "base/include/fml/memory/ref_counted.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/starlight/types/nlength.h"

namespace lynx {
namespace starlight {

class GridData : public fml::RefCountedThreadSafeStorage {
 public:
  void ReleaseSelf() const override { delete this; }
  static fml::RefPtr<GridData> Create() {
    return fml::AdoptRef(new GridData());
  }
  fml::RefPtr<GridData> Copy() const {
    return fml::AdoptRef(new GridData(*this));
  }
  GridData();
  GridData(const GridData& data);
  ~GridData() = default;
  void Reset();

  // grid container
  std::vector<NLength> grid_template_columns_min_track_sizing_function_;
  std::vector<NLength> grid_template_columns_max_track_sizing_function_;
  std::vector<NLength> grid_template_rows_min_track_sizing_function_;
  std::vector<NLength> grid_template_rows_max_track_sizing_function_;
  std::vector<NLength> grid_auto_columns_min_track_sizing_function_;
  std::vector<NLength> grid_auto_columns_max_track_sizing_function_;
  std::vector<NLength> grid_auto_rows_min_track_sizing_function_;
  std::vector<NLength> grid_auto_rows_max_track_sizing_function_;

  NLength grid_column_gap_;
  NLength grid_row_gap_;
  JustifyType justify_items_;
  GridAutoFlowType grid_auto_flow_;

  // grid item
  int32_t grid_row_span_;
  int32_t grid_column_span_;
  int32_t grid_column_end_;
  int32_t grid_column_start_;
  int32_t grid_row_end_;
  int32_t grid_row_start_;
  JustifyType justify_self_;
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_STYLE_GRID_DATA_H_
