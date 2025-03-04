// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/starlight/style/grid_data.h"

#include "core/renderer/starlight/style/default_layout_style.h"

namespace lynx {
namespace starlight {

GridData::GridData()
    : grid_template_columns_min_track_sizing_function_(
          DefaultLayoutStyle::SL_DEFAULT_GRID_TRACK()),
      grid_template_columns_max_track_sizing_function_(
          DefaultLayoutStyle::SL_DEFAULT_GRID_TRACK()),
      grid_template_rows_min_track_sizing_function_(
          DefaultLayoutStyle::SL_DEFAULT_GRID_TRACK()),
      grid_template_rows_max_track_sizing_function_(
          DefaultLayoutStyle::SL_DEFAULT_GRID_TRACK()),
      grid_auto_columns_min_track_sizing_function_(
          DefaultLayoutStyle::SL_DEFAULT_GRID_TRACK()),
      grid_auto_columns_max_track_sizing_function_(
          DefaultLayoutStyle::SL_DEFAULT_GRID_TRACK()),
      grid_auto_rows_min_track_sizing_function_(
          DefaultLayoutStyle::SL_DEFAULT_GRID_TRACK()),
      grid_auto_rows_max_track_sizing_function_(
          DefaultLayoutStyle::SL_DEFAULT_GRID_TRACK()),
      grid_column_gap_(DefaultLayoutStyle::SL_DEFAULT_GRID_GAP()),
      grid_row_gap_(DefaultLayoutStyle::SL_DEFAULT_GRID_GAP()),
      justify_items_(DefaultLayoutStyle::SL_DEFAULT_JUSTIFY_ITEMS),
      grid_auto_flow_(DefaultLayoutStyle::SL_DEFAULT_GRID_AUTO_FLOW),
      grid_row_span_(DefaultLayoutStyle::SL_DEFAULT_GRID_SPAN),
      grid_column_span_(DefaultLayoutStyle::SL_DEFAULT_GRID_SPAN),
      grid_column_end_(DefaultLayoutStyle::SL_DEFAULT_GRID_ITEM_POSITION),
      grid_column_start_(DefaultLayoutStyle::SL_DEFAULT_GRID_ITEM_POSITION),
      grid_row_end_(DefaultLayoutStyle::SL_DEFAULT_GRID_ITEM_POSITION),
      grid_row_start_(DefaultLayoutStyle::SL_DEFAULT_GRID_ITEM_POSITION),
      justify_self_(DefaultLayoutStyle::SL_DEFAULT_JUSTIFY_SELF) {}

GridData::GridData(const GridData& data)
    : grid_template_columns_min_track_sizing_function_(
          data.grid_template_columns_min_track_sizing_function_),
      grid_template_columns_max_track_sizing_function_(
          data.grid_template_columns_max_track_sizing_function_),
      grid_template_rows_min_track_sizing_function_(
          data.grid_template_rows_min_track_sizing_function_),
      grid_template_rows_max_track_sizing_function_(
          data.grid_template_rows_max_track_sizing_function_),
      grid_auto_columns_min_track_sizing_function_(
          data.grid_auto_columns_min_track_sizing_function_),
      grid_auto_columns_max_track_sizing_function_(
          data.grid_auto_columns_max_track_sizing_function_),
      grid_auto_rows_min_track_sizing_function_(
          data.grid_auto_rows_min_track_sizing_function_),
      grid_auto_rows_max_track_sizing_function_(
          data.grid_auto_rows_max_track_sizing_function_),
      grid_column_gap_(data.grid_column_gap_),
      grid_row_gap_(data.grid_row_gap_),
      justify_items_(data.justify_items_),
      grid_auto_flow_(data.grid_auto_flow_),
      grid_row_span_(data.grid_row_span_),
      grid_column_span_(data.grid_column_span_),
      grid_column_end_(data.grid_column_end_),
      grid_column_start_(data.grid_column_start_),
      grid_row_end_(data.grid_row_end_),
      grid_row_start_(data.grid_row_start_),
      justify_self_(data.justify_self_) {}

void GridData::Reset() {
  grid_template_columns_min_track_sizing_function_ =
      DefaultLayoutStyle::SL_DEFAULT_GRID_TRACK();
  grid_template_columns_max_track_sizing_function_ =
      DefaultLayoutStyle::SL_DEFAULT_GRID_TRACK();
  grid_template_rows_min_track_sizing_function_ =
      DefaultLayoutStyle::SL_DEFAULT_GRID_TRACK();
  grid_template_rows_max_track_sizing_function_ =
      DefaultLayoutStyle::SL_DEFAULT_GRID_TRACK();
  grid_auto_columns_min_track_sizing_function_ =
      DefaultLayoutStyle::SL_DEFAULT_GRID_TRACK();
  grid_auto_columns_max_track_sizing_function_ =
      DefaultLayoutStyle::SL_DEFAULT_GRID_TRACK();
  grid_auto_rows_min_track_sizing_function_ =
      DefaultLayoutStyle::SL_DEFAULT_GRID_TRACK();
  grid_auto_rows_max_track_sizing_function_ =
      DefaultLayoutStyle::SL_DEFAULT_GRID_TRACK();
  grid_column_gap_ = DefaultLayoutStyle::SL_DEFAULT_GRID_GAP();
  grid_row_gap_ = DefaultLayoutStyle::SL_DEFAULT_GRID_GAP();
  justify_items_ = DefaultLayoutStyle::SL_DEFAULT_JUSTIFY_ITEMS;
  grid_auto_flow_ = DefaultLayoutStyle::SL_DEFAULT_GRID_AUTO_FLOW;
  grid_row_span_ = DefaultLayoutStyle::SL_DEFAULT_GRID_SPAN;
  grid_column_span_ = DefaultLayoutStyle::SL_DEFAULT_GRID_SPAN;
  grid_column_end_ = DefaultLayoutStyle::SL_DEFAULT_GRID_ITEM_POSITION;
  grid_column_start_ = DefaultLayoutStyle::SL_DEFAULT_GRID_ITEM_POSITION;
  grid_row_end_ = DefaultLayoutStyle::SL_DEFAULT_GRID_ITEM_POSITION;
  grid_row_start_ = DefaultLayoutStyle::SL_DEFAULT_GRID_ITEM_POSITION;
  justify_self_ = DefaultLayoutStyle::SL_DEFAULT_JUSTIFY_SELF;
}

}  // namespace starlight
}  // namespace lynx
