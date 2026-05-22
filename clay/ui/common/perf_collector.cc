// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/common/perf_collector.h"

#include <cstdint>
#include <string>

#include "base/include/fml/task_runner.h"
#include "base/include/fml/time/time_point.h"
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

const std::unordered_map<Perf, const char*> PerfCollector::perf_to_string_map =
    {
        {Perf::kEnablePartialRepaint, "rk_enable_partial_repaint"},

        {Perf::kFirstLayoutCost, "rk_first_layout"},
        {Perf::kFirstPaintCost, "rk_first_paint"},
        {Perf::kFirstBuildFrameCost, "rk_first_build_frame"},
        {Perf::kFirstRasterCost, "rk_first_raster"},
        {Perf::kFirstRasterEnd, "clay_first_raster_end"},
        {Perf::kFirstPresentEnd, "clay_first_present_end"},

        {Perf::kErrorCode, "rk_error_code"},

        {Perf::kLayoutAndAnimationMax, "rk_layout_and_animation_max"},
        {Perf::kLayoutAndAnimationAvg, "rk_layout_and_animation_avg"},
        {Perf::kLayoutAndAnimationBad, "rk_layout_and_animation_bad"},

        {Perf::kRasterMax, "rk_raster_max"},
        {Perf::kRasterAvg, "rk_raster_avg"},
        {Perf::kRasterBadLevel1, "rk_raster_bad_level_1"},
        {Perf::kRasterBadLevel2, "rk_raster_bad_level_2"},
        {Perf::kRasterBadLevel3, "rk_raster_bad_level_3"},

        {Perf::kFrameTotalMax, "rk_frame_total_max"},
        {Perf::kFrameTotalAvg, "rk_frame_total_avg"},
        {Perf::kFrameTotalBadLevel1, "rk_frame_total_bad_level_1"},
        {Perf::kFrameTotalBadLevel2, "rk_frame_total_bad_level_2"},
        {Perf::kFrameTotalBadLevel3, "rk_frame_total_bad_level_3"},

        {Perf::kMemoryAvailable, "memory_available"},
        {Perf::kMemoryFree, "memory_free"},
        {Perf::kMemoryTotal, "memory_total"},

        {Perf::kListLayoutNewItem, "rk_list_layout_new_item"},
        {Perf::kMoveFocusUntilDraw, "rk_move_focus_until_draw"},
        {Perf::kMoveFocusUntilRaster, "rk_move_focus_until_raster"},
        {Perf::kMoveFocusDirection, "rk_move_focus_direction"},
};

PerfCollector::PerfCollector(fml::RefPtr<fml::TaskRunner> platform_task_runner)
    : weak_factory_(this),
      weak_(weak_factory_.GetWeakPtr()),
      platform_task_runner_(platform_task_runner) {}

PerfCollector::~PerfCollector() = default;

bool PerfCollector::FirstPerfReady() const {
  int64_t error_code = 0;
  auto error_code_iter =
      first_perf_container_.find(PerfToString(Perf::kErrorCode));
  if (error_code_iter != first_perf_container_.end()) {
    error_code = error_code_iter->second;
  }
  // All first perf data collected or has error code.
  return (first_perf_container_.size() == FIRST_THRESHOLD) || (error_code > 0);
}

bool PerfCollector::UpdatePerfReady() const {
  return update_perf_container_.size() == UPDATE_THRESHOLD;
}

const char* PerfCollector::PerfToString(Perf perf) const {
  return perf_to_string_map.at(perf);
}

void PerfCollector::BeginRecord(Perf perf) {
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

void PerfCollector::EndRecord(Perf perf) {
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

void PerfCollector::InsertRecord(Perf perf, int64_t cost) {
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

void PerfCollector::InsertForceRecord(const PerfMap& perf_map) {
  fml::TaskRunner::RunNowOrPostTask(
      platform_task_runner_, [weak = weak_, perf_map]() {
        if (weak) {
          weak->InsertForceRecordInternal(perf_map);
        }
      });
}

void PerfCollector::InsertLayoutAndAnimationRecord(
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

void PerfCollector::InsertRasterRecord(const clay::Stopwatch& stopwatch) {
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

void PerfCollector::InsertFocusChangedUntilFirstRasterFinish() {
  InsertForceTimeRecordUntilNow(receive_focus_time_,
                                Perf::kMoveFocusUntilRaster);
  // this method is the last one which is the consumption of
  // receive_focus_time_.
  receive_focus_time_ = 0;
}

void PerfCollector::InsertFocusChangedUntilFirstPaintFinish() {
  if (has_reported_after_focus_) {
    return;
  }
  InsertForceTimeRecordUntilNow(receive_focus_time_, Perf::kMoveFocusUntilDraw);
  has_reported_after_focus_ = true;
}

void PerfCollector::InsertForceTimeRecordUntilNow(int64_t from, Perf perf) {
  int64_t now = NowInMilliseconds();
  if (now > from) {
    auto record_map = std::unordered_map<Perf, int64_t>();
    record_map.emplace(perf, now - from);
    record_map.emplace(Perf::kMoveFocusDirection,
                       static_cast<int64_t>(focus_direction_));
    InsertForceRecord(record_map);
  }
}

void PerfCollector::InsertFrameTotalCostRecord(
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

void PerfCollector::InsertRecordInternal(Perf perf, int64_t cost) {
  if (is_first_send_ && perf < Perf::kFirstSep) {
    first_perf_container_.emplace(PerfToString(perf), cost);
  } else if (perf > Perf::kFirstSep && perf < Perf::kUpdateSep) {
    update_perf_container_.emplace(PerfToString(perf), cost);
  } else {
    return;
  }

  MaybeReport();
}

bool PerfCollector::IsForcePushRecord(Perf perf) {
  if (perf > Perf::kUpdateSep && perf < Perf::kForceSep) {
    return true;
  }
  return false;
}

void PerfCollector::InsertForceRecordInternal(const PerfMap& perf_map) {
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

void PerfCollector::MaybeReport(bool is_force) {
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
