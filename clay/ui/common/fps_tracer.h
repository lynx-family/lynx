// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMMON_FPS_TRACER_H_
#define CLAY_UI_COMMON_FPS_TRACER_H_

#include <string>
#include <vector>

#include "base/include/fml/time/time_point.h"

namespace clay {

struct FpsRawMetrics {
  int max_refresh_rate = 0;
  int duration_ms = 0;
  int frames = 0;
  int fps = 0;

  int drop1_count = 0;
  int drop1_duration_ms = 0;
  int drop3_count = 0;
  int drop3_duration_ms = 0;
  int drop7_count = 0;
  int drop7_duration_ms = 0;
  int drop25_count = 0;
  int drop25_duration_ms = 0;
};

struct FluencyTracerConfig {
  std::string scene = "";
  std::string tag = "";
};

class FpsTracer {
 public:
  FpsTracer(const std::string scene, const std::string tag,
            int max_refresh_rate, int32_t session_timeout_s);
  ~FpsTracer();

  void Start();
  void Stop();
  void AddFrameTimeSample(int64_t frame_time_nanos);
  void GetFpsMetrics(FpsRawMetrics& metrics);
  const FluencyTracerConfig& GetConfig() { return config_; }

 private:
  int computeDropCount(int64_t cost_mills100, int frame_interval_ms100);

  FluencyTracerConfig config_;
  int max_refresh_rate_;
  double refresh_interval_ns_;
  fml::TimePoint start_time_;
  fml::TimePoint end_time_;
  std::vector<int64_t> frame_time_samples_;
};

}  // namespace clay

#endif  // CLAY_UI_COMMON_FPS_TRACER_H_
