// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/style/flex_data.h"

#include "core/renderer/starlight/style/default_layout_style.h"

namespace lynx {
namespace starlight {

FlexData::FlexData()
    : flex_grow_(DefaultLayoutStyle::SL_DEFAULT_FLEX_GROW),
      flex_shrink_(DefaultLayoutStyle::SL_DEFAULT_FLEX_SHRINK),
      flex_basis_(DefaultLayoutStyle::SL_DEFAULT_FLEX_BASIS()),
      flex_direction_(DefaultLayoutStyle::SL_DEFAULT_FLEX_DIRECTION),
      flex_wrap_(DefaultLayoutStyle::SL_DEFAULT_FLEX_WRAP),
      justify_content_(DefaultLayoutStyle::SL_DEFAULT_JUSTIFY_CONTENT),
      align_items_(DefaultLayoutStyle::SL_DEFAULT_ALIGN_ITEMS),
      align_self_(DefaultLayoutStyle::SL_DEFAULT_ALIGN_SELF),
      align_content_(DefaultLayoutStyle::SL_DEFAULT_ALIGN_CONTENT),
      order_(DefaultLayoutStyle::SL_DEFAULT_ORDER) {}

FlexData::FlexData(const FlexData& data)
    : flex_grow_(data.flex_grow_),
      flex_shrink_(data.flex_shrink_),
      flex_basis_(data.flex_basis_),
      flex_direction_(data.flex_direction_),
      flex_wrap_(data.flex_wrap_),
      justify_content_(data.justify_content_),
      align_items_(data.align_items_),
      align_self_(data.align_self_),
      align_content_(data.align_content_),
      order_(data.order_) {}

void FlexData::Reset() {
  flex_grow_ = DefaultLayoutStyle::SL_DEFAULT_FLEX_GROW;
  flex_shrink_ = DefaultLayoutStyle::SL_DEFAULT_FLEX_SHRINK;
  flex_basis_ = DefaultLayoutStyle::SL_DEFAULT_FLEX_BASIS();
  flex_direction_ = DefaultLayoutStyle::SL_DEFAULT_FLEX_DIRECTION;
  flex_wrap_ = DefaultLayoutStyle::SL_DEFAULT_FLEX_WRAP;
  justify_content_ = DefaultLayoutStyle::SL_DEFAULT_JUSTIFY_CONTENT;
  align_items_ = DefaultLayoutStyle::SL_DEFAULT_ALIGN_ITEMS;
  align_self_ = DefaultLayoutStyle::SL_DEFAULT_ALIGN_SELF;
  align_content_ = DefaultLayoutStyle::SL_DEFAULT_ALIGN_CONTENT;
  order_ = DefaultLayoutStyle::SL_DEFAULT_ORDER;
}

}  // namespace starlight
}  // namespace lynx
