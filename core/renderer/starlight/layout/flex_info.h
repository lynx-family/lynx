// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_LAYOUT_FLEX_INFO_H_
#define CORE_RENDERER_STARLIGHT_LAYOUT_FLEX_INFO_H_

#include "core/renderer/starlight/types/layout_types.h"

namespace lynx {
namespace starlight {
class LayoutObject;

struct LineInfo {
  LineInfo(int start, int end, float line_cross_size,
           float remaining_free_space, bool is_flex_grow)
      : start_(start),
        end_(end),
        line_cross_size_(line_cross_size),
        remaining_free_space_(remaining_free_space),
        baseline_(0.0f),
        is_flex_grow_(is_flex_grow) {}
  int start_;
  int end_;
  float line_cross_size_;
  float remaining_free_space_;  // internal
  float baseline_;
  bool is_flex_grow_;
};

class FlexInfo {
 public:
  friend class LayoutObject;
  FlexInfo(int flex_count)
      : flex_base_size_(flex_count, 0),
        hypothetical_main_size_(flex_count, 0),
        hypothetical_cross_size_(flex_count, 0),
        flex_main_size_(flex_count, 0),
        flex_cross_size_(flex_count, 0),
        apply_stretch_later_(flex_count, false),
        has_item_flex_grow_(0),
        has_item_flex_shrink_(0),
        main_gap_size_(0.f),
        cross_gap_size_(0.f) {}

  void Reset();

  InlineFloatArray flex_base_size_;
  InlineFloatArray hypothetical_main_size_;
  InlineFloatArray hypothetical_cross_size_;

  InlineFloatArray flex_main_size_;
  InlineFloatArray flex_cross_size_;
  InlineBoolArray apply_stretch_later_;

  base::InlineVector<LineInfo, 2> line_info_;

  unsigned has_item_flex_grow_ : 1;
  unsigned has_item_flex_shrink_ : 1;

  float main_gap_size_;
  float cross_gap_size_;
};
}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_LAYOUT_FLEX_INFO_H_
