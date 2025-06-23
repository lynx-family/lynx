// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_LINE_HARMONY_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_LINE_HARMONY_H_

#include <native_drawing/drawing_text_typography.h>

#include <cinttypes>

namespace lynx {
namespace tasm {
namespace harmony {
class LineMetricsHarmony {
 public:
  LineMetricsHarmony(OH_Drawing_LineMetrics* metrics)
      : line_metrics_(metrics) {}
  double Ascent() const { return line_metrics_->ascender; }
  double Descent() const { return line_metrics_->descender; }
  double Height() const { return line_metrics_->height; }
  double Width() const { return line_metrics_->width; }
  double Left() const { return line_metrics_->x; }
  double Top() const { return line_metrics_->y; }
  double XHeight() const { return line_metrics_->xHeight; }
  size_t StartIndex() const { return line_metrics_->startIndex; }
  size_t EndIndex() const { return line_metrics_->endIndex; }

 private:
  OH_Drawing_LineMetrics* line_metrics_;
};

class LineMetricsList {
 public:
  LineMetricsList(OH_Drawing_LineMetrics* list)
      : line_metrics_list_(list), count_(OH_Drawing_LineMetricsGetSize(list)) {}
  ~LineMetricsList() { OH_Drawing_DestroyLineMetrics(line_metrics_list_); }

  const LineMetricsHarmony GetLineMetrics(size_t idx) const {
    return LineMetricsHarmony{line_metrics_list_ + idx};
  }

  size_t Size() const { return count_; }

 private:
  OH_Drawing_LineMetrics* line_metrics_list_;
  size_t count_;
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_LINE_HARMONY_H_
