// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_FONT_METRICS_HARMONY_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_FONT_METRICS_HARMONY_H_
namespace lynx {
namespace tasm {
namespace harmony {
class FontMetricsListHarmony {
 public:
  FontMetricsListHarmony(OH_Drawing_Font_Metrics* metrics_list, size_t count)
      : metrics_list_(metrics_list), count_(count) {}
  ~FontMetricsListHarmony() {
    OH_Drawing_TypographyDestroyLineFontMetrics(metrics_list_);
  }

 private:
  OH_Drawing_Font_Metrics* metrics_list_;
  size_t count_;
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_FONT_METRICS_HARMONY_H_
