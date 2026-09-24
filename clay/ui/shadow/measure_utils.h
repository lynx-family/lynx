// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_MEASURE_UTILS_H_
#define CLAY_UI_SHADOW_MEASURE_UTILS_H_

#include <algorithm>
#include <cmath>

#include "clay/third_party/txt/src/txt/paragraph.h"

namespace clay {

class MeasureUtils {
 public:
  static bool isUndefined(float value) {
    return (value >= static_cast<float>(10E8) ||
            value <= static_cast<float>(-10E8));
  }
};

enum class TextUpdateFlag {
  // None means either there is no update or the text view's width/height
  // changed.
  // In this case, we don't need to re-create the text builder.
  kUpdateFlagNone = 0,
  kUpdateFlagStyle = 1,
  kUpdateFlagChildren = 1 << 1,
};

inline double GetMeasuredParagraphWidth(txt::Paragraph* paragraph,
                                        double layout_width,
                                        float layout_tolerance = 1.f) {
  if (!paragraph) {
    return 0;
  }
  double measured_width = paragraph->GetMaxIntrinsicWidth();
#if defined(CLAY_ENABLE_TTTEXT)
  const auto& line_metrics = paragraph->GetLineMetrics();
  if (!line_metrics.empty()) {
    double longest_line_width = 0;
    for (const auto& line_metric : line_metrics) {
      longest_line_width =
          std::max(longest_line_width, static_cast<double>(line_metric.width));
    }
    if (!std::isfinite(layout_width) ||
        longest_line_width + layout_tolerance < layout_width) {
      measured_width = longest_line_width;
    }
  }
#endif
  return measured_width;
}

}  // namespace clay

#endif  // CLAY_UI_SHADOW_MEASURE_UTILS_H_
