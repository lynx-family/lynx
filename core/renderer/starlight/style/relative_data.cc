// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/style/relative_data.h"

#include "core/renderer/starlight/style/default_layout_style.h"

namespace lynx {
namespace starlight {

RelativeData::RelativeData()
    : relative_id_(DefaultLayoutStyle::SL_DEFAULT_RELATIVE_ID),
      relative_align_top_(DefaultLayoutStyle::SL_DEFAULT_RELATIVE_ALIGN_TOP),
      relative_align_right_(
          DefaultLayoutStyle::SL_DEFAULT_RELATIVE_ALIGN_RIGHT),
      relative_align_bottom_(
          DefaultLayoutStyle::SL_DEFAULT_RELATIVE_ALIGN_BOTTOM),
      relative_align_left_(DefaultLayoutStyle::SL_DEFAULT_RELATIVE_ALIGN_LEFT),
      relative_top_of_(DefaultLayoutStyle::SL_DEFAULT_RELATIVE_TOP_OF),
      relative_right_of_(DefaultLayoutStyle::SL_DEFAULT_RELATIVE_RIGHT_OF),
      relative_bottom_of_(DefaultLayoutStyle::SL_DEFAULT_RELATIVE_BOTTOM_OF),
      relative_left_of_(DefaultLayoutStyle::SL_DEFAULT_RELATIVE_LEFT_OF),
      relative_layout_once_(
          DefaultLayoutStyle::SL_DEFAULT_RELATIVE_LAYOUT_ONCE),
      relative_center_(DefaultLayoutStyle::SL_DEFAULT_RELATIVE_CENTER) {}

RelativeData::RelativeData(const RelativeData& data)
    : relative_id_(data.relative_id_),
      relative_align_top_(data.relative_align_top_),
      relative_align_right_(data.relative_align_right_),
      relative_align_bottom_(data.relative_align_bottom_),
      relative_align_left_(data.relative_align_left_),
      relative_top_of_(data.relative_top_of_),
      relative_right_of_(data.relative_right_of_),
      relative_bottom_of_(data.relative_bottom_of_),
      relative_left_of_(data.relative_left_of_),
      relative_layout_once_(data.relative_layout_once_),
      relative_center_(data.relative_center_) {}

void RelativeData::RelativeData::Reset() {
  relative_id_ = DefaultLayoutStyle::SL_DEFAULT_RELATIVE_ID;
  relative_align_top_ = DefaultLayoutStyle::SL_DEFAULT_RELATIVE_ALIGN_TOP;
  relative_align_right_ = DefaultLayoutStyle::SL_DEFAULT_RELATIVE_ALIGN_RIGHT;
  relative_align_bottom_ = DefaultLayoutStyle::SL_DEFAULT_RELATIVE_ALIGN_BOTTOM;
  relative_align_left_ = DefaultLayoutStyle::SL_DEFAULT_RELATIVE_ALIGN_LEFT;
  relative_top_of_ = DefaultLayoutStyle::SL_DEFAULT_RELATIVE_TOP_OF;
  relative_right_of_ = DefaultLayoutStyle::SL_DEFAULT_RELATIVE_RIGHT_OF;
  relative_bottom_of_ = DefaultLayoutStyle::SL_DEFAULT_RELATIVE_BOTTOM_OF;
  relative_left_of_ = DefaultLayoutStyle::SL_DEFAULT_RELATIVE_LEFT_OF;
  relative_layout_once_ = DefaultLayoutStyle::SL_DEFAULT_RELATIVE_LAYOUT_ONCE;
  relative_center_ = DefaultLayoutStyle::SL_DEFAULT_RELATIVE_CENTER;
}

}  // namespace starlight
}  // namespace lynx
