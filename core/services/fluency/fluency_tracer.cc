// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/fluency/fluency_tracer.h"

#include <stdlib.h>

#include <memory>
#include <utility>

#include "core/renderer/utils/lynx_env.h"
#include "core/services/event_report/event_tracker.h"

namespace lynx {
namespace tasm {

std::atomic<bool> FluencyTracer::need_check_{true};
std::atomic<bool> FluencyTracer::force_enable_{false};
std::atomic<bool> FluencyTracer::enable_{false};

void FluencyTracer::SetForceEnable(bool b) {
  force_enable_.store(b, std::memory_order::memory_order_relaxed);
}

// FluencyTracer frequently retrieves settings. Therefore, for better
// performance, we persistently save the toggle values when there are no updates
// to the settings.
void FluencyTracer::SetNeedCheck() {
  need_check_.store(true, std::memory_order::memory_order_relaxed);
}

bool FluencyTracer::IsEnable() {
  if (force_enable_.load(std::memory_order::memory_order_relaxed)) {
    return true;
  }
  bool expected = true;
  if (need_check_.compare_exchange_weak(
          expected, false, std::memory_order::memory_order_relaxed,
          std::memory_order::memory_order_relaxed)) {
    bool enable = lynx::tasm::LynxEnv::GetInstance().GetBoolEnv(
        lynx::tasm::LynxEnv::Key::ENABLE_FLUENCY_TRACE, false);
    enable_.store(enable);
    return enable;
  }
  return enable_.load(std::memory_order::memory_order_relaxed);
}

void FluencyTracer::Trigger(int64_t time_stamp) {
  if (!IsEnable()) {
    return;
  }
  double total_dur = 0;
  if (start_timestamp_ == 0) {
    start_timestamp_ = time_stamp;
  } else if (((time_stamp - last_timestamp_) / 1.0e+9) > 10) {
    // If the timestamp of the current frame is 10 seconds away from the
    // previous frame's timestamp, which may be due to the user going into the
    // background. we should report this fluency metric immediately and reset
    // the tracer state.
    total_dur = (last_timestamp_ - start_timestamp_) / 1.0e+9;
    ReportFluency(total_dur);
    frames_dur_.clear();
    start_timestamp_ = time_stamp;
  } else if ((total_dur = (time_stamp - start_timestamp_) / 1.0e+9) > 30) {
    // compute every 30 seconds
    frames_dur_.push_back((time_stamp - last_timestamp_) / 1.0e+6);
    ReportFluency(total_dur);
    frames_dur_.clear();
    start_timestamp_ = time_stamp;
  } else {
    frames_dur_.push_back((time_stamp - last_timestamp_) / 1.0e+6);
  }
  last_timestamp_ = time_stamp;
}

void FluencyTracer::ReportFluency(double total_dur) {
  if (std::abs(total_dur) < 1e-9) {
    return;
  }
  double rate = 16.667;
  // If a single drawing exceeds 16.667ms, it is considered a frame drop.
  // drop1: The number of times a single drawing exceeds 16.667ms
  // drop3: The number of times a single drawing exceeds (16.667 * 3)ms
  // drop7: The number of times a single drawing exceeds (16.667 * 7)ms
  int drop1 = 0;
  int drop3 = 0;
  int drop7 = 0;
  int frame_count = static_cast<int>(frames_dur_.size());
  double fps = frames_dur_.size() / total_dur;
  for (double dur : frames_dur_) {
    double drop = dur / rate;
    if (drop <= 0) {
      continue;
    }
    if (drop >= 1) {
      drop1++;
    }
    if (drop >= 3) {
      drop3++;
    }
    if (drop >= 7) {
      drop7++;
    }
  }
  tasm::report::EventTracker::OnEvent([fps, total_dur, frame_count, drop1,
                                       drop3, drop7](
                                          tasm::report::MoveOnlyEvent& event) {
    event.SetName("lynxsdk_javascript_fluency_event");
    event.SetProps("lynxsdk_fluency_fps", fps);
    event.SetProps("lynxsdk_fluency_dur", total_dur * 1000);
    event.SetProps("lynxsdk_fluency_frames_number", frame_count);
    event.SetProps("lynxsdk_fluency_drop1_count", drop1);
    event.SetProps("lynxsdk_fluency_drop1_count_per_second", drop1 / total_dur);
    event.SetProps("lynxsdk_fluency_drop3_count", drop3);
    event.SetProps("lynxsdk_fluency_drop3_count_per_second", drop3 / total_dur);
    event.SetProps("lynxsdk_fluency_drop7_count", drop7);
    event.SetProps("lynxsdk_fluency_drop7_count_per_second", drop7 / total_dur);
  });
}
}  // namespace tasm
}  // namespace lynx
