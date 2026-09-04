// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/common/frame_timing_collector.h"

#include <cstdint>
#include <string>

#include "base/include/fml/task_runner.h"
#include "base/include/fml/time/time_point.h"
#include "base/include/timer/time_utils.h"
#include "clay/ui/common/isolate.h"
#include "clay/ui/component/page_view.h"

namespace clay {

namespace {
template <typename T>
static constexpr int PerfToInt(T perf) noexcept {
  return static_cast<int>(perf);
}

static constexpr size_t FIRST_THRESHOLD = PerfToInt(Perf::kFirstSep);
static constexpr size_t UPDATE_THRESHOLD =
    PerfToInt(Perf::kUpdateSep) - PerfToInt(Perf::kFirstSep) - 1;

static constexpr int64_t LAYOUT_COST_THRESHOLD = 17;

static constexpr int64_t RASTER_COST_THRESHOLD_LEVEL_1 = 17;
static constexpr int64_t RASTER_COST_THRESHOLD_LEVEL_2 = 34;
static constexpr int64_t RASTER_COST_THRESHOLD_LEVEL_3 = 68;

static constexpr int64_t FRAME_TOTAL_COST_THRESHOLD_LEVEL_1 = 17;
static constexpr int64_t FRAME_TOTAL_COST_THRESHOLD_LEVEL_2 = 34;
static constexpr int64_t FRAME_TOTAL_COST_THRESHOLD_LEVEL_3 = 68;

static constexpr const char* SETUP_FLAG = "clay_setup";
static constexpr const char* FORCE_UPDATE_FLAG = "clay_force_update";
static constexpr const char* UPDATE_FLAG = "clay_update";
}  // namespace

FrameTimingCollector::FrameTimingCollector(
    fml::RefPtr<fml::TaskRunner> platform_task_runner)
    : weak_factory_(this),
      weak_(weak_factory_.GetWeakPtr()),
      platform_task_runner_(platform_task_runner) {}

FrameTimingCollector::~FrameTimingCollector() = default;

bool FrameTimingCollector::FirstPerfReady() const {
  int64_t error_code = 0;
  auto error_code_iter =
      first_perf_container_.find(PerfToString(Perf::kErrorCode));
  if (error_code_iter != first_perf_container_.end()) {
    error_code = error_code_iter->second;
  }
  // All first perf data collected or has error code.
  return (first_perf_container_.size() == FIRST_THRESHOLD) || (error_code > 0);
}

bool FrameTimingCollector::UpdatePerfReady() const {
  return update_perf_container_.size() == UPDATE_THRESHOLD;
}

const char* FrameTimingCollector::PerfToString(Perf perf) const {
  switch (perf) {
    case Perf::kEnablePartialRepaint:
      return "rk_enable_partial_repaint";
    case Perf::kFirstLayoutCost:
      return "rk_first_layout";
    case Perf::kFirstPaintCost:
      return "rk_first_paint";
    case Perf::kFirstBuildFrameCost:
      return "rk_first_build_frame";
    case Perf::kFirstRasterCost:
      return "rk_first_raster";
    case Perf::kFirstRasterEnd:
      return "clay_first_raster_end";
    case Perf::kFirstPresentEnd:
      return "clay_first_present_end";
    case Perf::kErrorCode:
      return "rk_error_code";
    case Perf::kLayoutAndAnimationMax:
      return "rk_layout_and_animation_max";
    case Perf::kLayoutAndAnimationAvg:
      return "rk_layout_and_animation_avg";
    case Perf::kLayoutAndAnimationBad:
      return "rk_layout_and_animation_bad";
    case Perf::kRasterMax:
      return "rk_raster_max";
    case Perf::kRasterAvg:
      return "rk_raster_avg";
    case Perf::kRasterBadLevel1:
      return "rk_raster_bad_level_1";
    case Perf::kRasterBadLevel2:
      return "rk_raster_bad_level_2";
    case Perf::kRasterBadLevel3:
      return "rk_raster_bad_level_3";
    case Perf::kFrameTotalMax:
      return "rk_frame_total_max";
    case Perf::kFrameTotalAvg:
      return "rk_frame_total_avg";
    case Perf::kFrameTotalBadLevel1:
      return "rk_frame_total_bad_level_1";
    case Perf::kFrameTotalBadLevel2:
      return "rk_frame_total_bad_level_2";
    case Perf::kFrameTotalBadLevel3:
      return "rk_frame_total_bad_level_3";
    case Perf::kListLayoutNewItem:
      return "rk_list_layout_new_item";
    case Perf::kMoveFocusUntilDraw:
      return "rk_move_focus_until_draw";
    case Perf::kMoveFocusUntilRaster:
      return "rk_move_focus_until_raster";
    case Perf::kMoveFocusDirection:
      return "rk_move_focus_direction";
    case Perf::kFirstSep:
    case Perf::kUpdateSep:
    case Perf::kForceSep:
      break;
  }
  FML_DCHECK(false);
  return "";
}

void FrameTimingCollector::BeginRecord(Perf perf) {
  int64_t begin = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();
  fml::TaskRunner::RunNowOrPostTask(platform_task_runner_,
                                    [weak = weak_, perf, begin]() {
                                      if (weak) {
                                        weak->perf_record_[perf] = begin;
                                      }
                                    });
}

void FrameTimingCollector::EndRecord(Perf perf) {
  int64_t end = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
  fml::TaskRunner::RunNowOrPostTask(
      platform_task_runner_, [weak = weak_, perf, end]() {
        if (weak) {
          int64_t begin = weak->perf_record_[perf];
          int64_t cost = end - begin;
          weak->InsertRecordInternal(perf, cost);
        }
      });
}

void FrameTimingCollector::InsertRecord(Perf perf, int64_t cost) {
  if (is_recording_first_frame_perf_ && perf == Perf::kFirstPresentEnd) {
    // Finish recording first frame perf
    is_recording_first_frame_perf_ = false;
  }
  fml::TaskRunner::RunNowOrPostTask(platform_task_runner_,
                                    [weak = weak_, cost, perf]() {
                                      if (weak) {
                                        weak->InsertRecordInternal(perf, cost);
                                      }
                                    });
}

void FrameTimingCollector::InsertForceRecord(const PerfMap& perf_map) {
  fml::TaskRunner::RunNowOrPostTask(
      platform_task_runner_, [weak = weak_, perf_map]() {
        if (weak) {
          weak->InsertForceRecordInternal(perf_map);
        }
      });
}

void FrameTimingCollector::InsertLayoutAndAnimationRecord(
    const clay::Stopwatch& stopwatch) {
  // Record performance of 120 samples.
  if (!stopwatch.CheckIfLastSampleInCycle()) {
    return;
  }
  const auto& laps = stopwatch.GetLaps();
  size_t bad_count = 0;
  for (const auto& lap : laps) {
    int64_t cost = lap.ToMilliseconds();
    if (cost > LAYOUT_COST_THRESHOLD) {
      ++bad_count;
    }
  }
  int64_t max = stopwatch.MaxDelta().ToMilliseconds();
  int64_t avg = stopwatch.AverageDelta().ToMilliseconds();
  fml::TaskRunner::RunNowOrPostTask(
      platform_task_runner_, [weak = weak_, max, avg, bad_count]() {
        if (weak) {
          weak->InsertRecordInternal(Perf::kLayoutAndAnimationMax, max);
          weak->InsertRecordInternal(Perf::kLayoutAndAnimationAvg, avg);
          weak->InsertRecordInternal(Perf::kLayoutAndAnimationBad, bad_count);
        }
      });
}

void FrameTimingCollector::InsertRasterRecord(
    const clay::Stopwatch& stopwatch) {
  // Record performance of 120 samples.
  if (!stopwatch.CheckIfLastSampleInCycle()) {
    return;
  }
  const auto& laps = stopwatch.GetLaps();
  size_t level_1 = 0;
  size_t level_2 = 0;
  size_t level_3 = 0;
  for (const auto& lap : laps) {
    int64_t cost = lap.ToMilliseconds();
    if (cost < RASTER_COST_THRESHOLD_LEVEL_1) {
      continue;
    }
    if (cost >= RASTER_COST_THRESHOLD_LEVEL_3) {
      ++level_3;
    } else if (cost >= RASTER_COST_THRESHOLD_LEVEL_2) {
      ++level_2;
    } else if (cost >= RASTER_COST_THRESHOLD_LEVEL_1) {
      ++level_1;
    }
  }
  int64_t max = stopwatch.MaxDelta().ToMilliseconds();
  int64_t avg = stopwatch.AverageDelta().ToMilliseconds();
  fml::TaskRunner::RunNowOrPostTask(
      platform_task_runner_,
      [weak = weak_, max, avg, level_1, level_2, level_3]() {
        if (weak) {
          weak->InsertRecordInternal(Perf::kRasterMax, max);
          weak->InsertRecordInternal(Perf::kRasterAvg, avg);
          weak->InsertRecordInternal(Perf::kRasterBadLevel1, level_1);
          weak->InsertRecordInternal(Perf::kRasterBadLevel2, level_2);
          weak->InsertRecordInternal(Perf::kRasterBadLevel3, level_3);
        }
      });
}

void FrameTimingCollector::InsertFocusChangedUntilFirstRasterFinish() {
  InsertForceTimeRecordUntilNow(receive_focus_time_,
                                Perf::kMoveFocusUntilRaster);
  // this method is the last one which is the consumption of
  // receive_focus_time_.
  receive_focus_time_ = 0;
}

void FrameTimingCollector::InsertFocusChangedUntilFirstPaintFinish() {
  if (has_reported_after_focus_) {
    return;
  }
  InsertForceTimeRecordUntilNow(receive_focus_time_, Perf::kMoveFocusUntilDraw);
  has_reported_after_focus_ = true;
}

void FrameTimingCollector::InsertForceTimeRecordUntilNow(int64_t from,
                                                         Perf perf) {
  int64_t now = lynx::base::CurrentSystemTimeMilliseconds();
  if (now > from) {
    auto record_map = std::unordered_map<Perf, int64_t>();
    record_map.emplace(perf, now - from);
    record_map.emplace(Perf::kMoveFocusDirection,
                       static_cast<int64_t>(focus_direction_));
    InsertForceRecord(record_map);
  }
}

void FrameTimingCollector::InsertFrameTotalCostRecord(
    const clay::Stopwatch& stopwatch) {
  // Record performance of 120 samples.
  if (!stopwatch.CheckIfLastSampleInCycle()) {
    return;
  }
  const auto& laps = stopwatch.GetLaps();
  size_t level_1 = 0;
  size_t level_2 = 0;
  size_t level_3 = 0;
  for (const auto& lap : laps) {
    int64_t cost = lap.ToMilliseconds();
    if (cost < FRAME_TOTAL_COST_THRESHOLD_LEVEL_1) {
      continue;
    }
    if (cost >= FRAME_TOTAL_COST_THRESHOLD_LEVEL_3) {
      ++level_3;
    } else if (cost >= FRAME_TOTAL_COST_THRESHOLD_LEVEL_2) {
      ++level_2;
    } else if (cost >= FRAME_TOTAL_COST_THRESHOLD_LEVEL_1) {
      ++level_1;
    }
  }
  int64_t max = stopwatch.MaxDelta().ToMilliseconds();
  int64_t avg = stopwatch.AverageDelta().ToMilliseconds();
  fml::TaskRunner::RunNowOrPostTask(
      platform_task_runner_,
      [weak = weak_, max, avg, level_1, level_2, level_3]() {
        if (weak) {
          weak->InsertRecordInternal(Perf::kFrameTotalMax, max);
          weak->InsertRecordInternal(Perf::kFrameTotalAvg, avg);
          weak->InsertRecordInternal(Perf::kFrameTotalBadLevel1, level_1);
          weak->InsertRecordInternal(Perf::kFrameTotalBadLevel2, level_2);
          weak->InsertRecordInternal(Perf::kFrameTotalBadLevel3, level_3);
        }
      });
}

void FrameTimingCollector::InsertRecordInternal(Perf perf, int64_t cost) {
  if (is_first_send_ && perf < Perf::kFirstSep) {
    first_perf_container_.emplace(PerfToString(perf), cost);
  } else if (perf > Perf::kFirstSep && perf < Perf::kUpdateSep) {
    update_perf_container_.emplace(PerfToString(perf), cost);
  } else {
    return;
  }

  MaybeReport();
}

bool FrameTimingCollector::IsForcePushRecord(Perf perf) {
  if (perf > Perf::kUpdateSep && perf < Perf::kForceSep) {
    return true;
  }
  return false;
}

void FrameTimingCollector::InsertForceRecordInternal(const PerfMap& perf_map) {
  if (perf_map.empty()) {
    return;
  }
  for (const auto& perf : perf_map) {
    if (IsForcePushRecord(perf.first)) {
      force_perf_container_.emplace(PerfToString(perf.first), perf.second);
    }
  }

  if (force_perf_container_.size() == 0) {
    return;
  }
  MaybeReport(true);
}

void FrameTimingCollector::MaybeReport(bool is_force) {
  if (page_view_) {
    if (is_force) {
      page_view_->ReportTiming(force_perf_container_, FORCE_UPDATE_FLAG);
      force_perf_container_.clear();
    } else if (is_first_send_ && FirstPerfReady()) {
      is_first_send_ = false;
      page_view_->ReportTiming(first_perf_container_, SETUP_FLAG);
      first_perf_container_.clear();
    } else if (!is_first_send_ && UpdatePerfReady()) {
      page_view_->ReportTiming(update_perf_container_, UPDATE_FLAG);
      update_perf_container_.clear();
    }
  }
}

}  // namespace clay
