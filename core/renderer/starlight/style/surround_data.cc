// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/style/surround_data.h"

#include "core/renderer/starlight/style/default_layout_style.h"

namespace lynx {
namespace starlight {

SurroundData::SurroundData()
    : left_(DefaultLayoutStyle::SL_DEFAULT_FOUR_POSITION()),
      right_(DefaultLayoutStyle::SL_DEFAULT_FOUR_POSITION()),
      top_(DefaultLayoutStyle::SL_DEFAULT_FOUR_POSITION()),
      bottom_(DefaultLayoutStyle::SL_DEFAULT_FOUR_POSITION()),
      margin_left_(DefaultLayoutStyle::SL_DEFAULT_MARGIN()),
      margin_right_(DefaultLayoutStyle::SL_DEFAULT_MARGIN()),
      margin_top_(DefaultLayoutStyle::SL_DEFAULT_MARGIN()),
      margin_bottom_(DefaultLayoutStyle::SL_DEFAULT_MARGIN()),
      padding_left_(DefaultLayoutStyle::SL_DEFAULT_PADDING()),
      padding_right_(DefaultLayoutStyle::SL_DEFAULT_PADDING()),
      padding_top_(DefaultLayoutStyle::SL_DEFAULT_PADDING()),
      padding_bottom_(DefaultLayoutStyle::SL_DEFAULT_PADDING()) {
  border_data_.reset();
}

void SurroundData::Reset() {
  left_ = DefaultLayoutStyle::SL_DEFAULT_FOUR_POSITION();
  right_ = DefaultLayoutStyle::SL_DEFAULT_FOUR_POSITION();
  top_ = DefaultLayoutStyle::SL_DEFAULT_FOUR_POSITION();
  bottom_ = DefaultLayoutStyle::SL_DEFAULT_FOUR_POSITION();

  margin_left_ = DefaultLayoutStyle::SL_DEFAULT_MARGIN();
  margin_right_ = DefaultLayoutStyle::SL_DEFAULT_MARGIN();
  margin_top_ = DefaultLayoutStyle::SL_DEFAULT_MARGIN();
  margin_bottom_ = DefaultLayoutStyle::SL_DEFAULT_MARGIN();

  padding_left_ = DefaultLayoutStyle::SL_DEFAULT_PADDING();
  padding_right_ = DefaultLayoutStyle::SL_DEFAULT_PADDING();
  padding_top_ = DefaultLayoutStyle::SL_DEFAULT_PADDING();
  padding_bottom_ = DefaultLayoutStyle::SL_DEFAULT_PADDING();

  border_data_.reset();
}

}  // namespace starlight
}  // namespace lynx
