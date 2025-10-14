// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_WINDOW_VIEWPORT_METRICS_H_
#define CLAY_UI_WINDOW_VIEWPORT_METRICS_H_

#include <ostream>
#include <vector>

namespace clay {

struct ViewportMetrics {
  ViewportMetrics();
  ViewportMetrics(double p_device_pixel_ratio, double p_physical_width,
                  double p_physical_height, double p_physical_touch_slop);
  ViewportMetrics(
      double p_device_pixel_ratio, double p_device_density_dpi,
      double p_physical_width, double p_physical_height,
      double p_physical_padding_top, double p_physical_padding_right,
      double p_physical_padding_bottom, double p_physical_padding_left,
      double p_physical_view_inset_top, double p_physical_view_inset_right,
      double p_physical_view_inset_bottom, double p_physical_view_inset_left,
      double p_physical_system_gesture_inset_top,
      double p_physical_system_gesture_inset_right,
      double p_physical_system_gesture_inset_bottom,
      double p_physical_system_gesture_inset_left, double p_physical_touch_slop,
      const std::vector<double>& p_physical_display_features_bounds,
      const std::vector<int>& p_physical_display_features_type,
      const std::vector<int>& p_physical_display_features_state,
      double p_physical_screen_width = 0, double p_physical_screen_height = 0,
      double p_font_scale = 1.0, bool p_night_node = false);

  double device_pixel_ratio = 1.0;
  double device_density_dpi = 0;
  double physical_width = 0;
  double physical_height = 0;
  double physical_padding_top = 0;
  double physical_padding_right = 0;
  double physical_padding_bottom = 0;
  double physical_padding_left = 0;
  double physical_view_inset_top = 0;
  double physical_view_inset_right = 0;
  double physical_view_inset_bottom = 0;
  double physical_view_inset_left = 0;
  double physical_system_gesture_inset_top = 0;
  double physical_system_gesture_inset_right = 0;
  double physical_system_gesture_inset_bottom = 0;
  double physical_system_gesture_inset_left = 0;
  double physical_touch_slop = -1.0;
  std::vector<double> physical_display_features_bounds;
  std::vector<int> physical_display_features_type;
  std::vector<int> physical_display_features_state;
  double physical_screen_width = 0;
  double physical_screen_height = 0;
  double font_scale = 1.0;
  bool night_mode = false;
};

bool NeedsToChangeSize(const ViewportMetrics& old_metrics,
                       const ViewportMetrics& new_metrics);
bool NeedsToChangeSystemParameters(const ViewportMetrics& old_metrics,
                                   const ViewportMetrics& new_metrics);
bool operator==(const ViewportMetrics& a, const ViewportMetrics& b);
std::ostream& operator<<(std::ostream& os, const ViewportMetrics& a);

}  // namespace clay

#endif  // CLAY_UI_WINDOW_VIEWPORT_METRICS_H_
