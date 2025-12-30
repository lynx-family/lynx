// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMMON_FRAME_TIMING_COLLECTOR_H_
#define CLAY_UI_COMMON_FRAME_TIMING_COLLECTOR_H_

#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>

#include "base/include/fml/memory/ref_ptr.h"
#include "clay/flow/stopwatch.h"
#include "clay/ui/component/base_view.h"

namespace clay {

class PageView;

static const uint32_t kPerfErrorCodeOK = 0;
static const uint32_t kPerfErrorCodeSurfaceFailed = 1;

// Timing flag constants for setup and update operations
static constexpr const char* kSetupFlag = "clay_setup";
static constexpr const char* kUpdateFlag = "clay_update";
static constexpr const char* kForceUpdateFlag = "clay_force_update";

enum class Perf {
  kEnablePartialRepaint,

  kFirstLayoutCost,
  kFirstPaintCost,
  kFirstBuildFrameCost,
  kFirstRasterCost,
  kFirstRasterEnd,
  kFirstPresentEnd,

  kErrorCode,

  kFirstSep,  // DON'T USE THIS! THIS SHOULD ONLY BE USED BY PERFCOLLECTOR SELF

  kLayoutAndAnimationMax,
  kLayoutAndAnimationAvg,
  kLayoutAndAnimationBad,

  kRasterMax,
  kRasterAvg,
  kRasterBadLevel1,
  kRasterBadLevel2,
  kRasterBadLevel3,

  kFrameTotalMax,
  kFrameTotalAvg,
  kFrameTotalBadLevel1,
  kFrameTotalBadLevel2,
  kFrameTotalBadLevel3,

  kUpdateSep,  // DON'T USE THIS! THIS SHOULD ONLY BE USED BY PERFCOLLECTOR SELF

  kListLayoutNewItem,
  kMoveFocusUntilDraw,
  kMoveFocusUntilRaster,
  kMoveFocusDirection,
  kForceSep,  // DON'T USE THIS! THIS SHOULD ONLY BE USED BY PERFCOLLECTOR SELF
};

class FrameTimingCollector {
 public:
  using PerfMap = std::unordered_map<Perf, int64_t>;

  explicit FrameTimingCollector(
      fml::RefPtr<fml::TaskRunner> platform_task_runner);

  ~FrameTimingCollector();

  void SetPageView(PageView* page_view) { page_view_ = page_view; }

  void BeginRecord(Perf perf);
  void EndRecord(Perf perf);
  void InsertRecord(Perf perf, int64_t cost);
  void InsertForceRecord(const PerfMap& perf_map);
  bool IsForcePushRecord(Perf perf);

  void InsertLayoutAndAnimationRecord(const clay::Stopwatch& stopwatch);
  void InsertRasterRecord(const clay::Stopwatch& stopwatch);
  void InsertFrameTotalCostRecord(const clay::Stopwatch& stopwatch);

  bool IsRecordingFirstFramePerf() { return is_recording_first_frame_perf_; }

  int64_t GetReceivedFocusTime() const { return receive_focus_time_; }
  void SetReceivedFocusTime(int64_t timestamp,
                            FocusManager::Direction direction) {
    receive_focus_time_ = timestamp;
    focus_direction_ = direction;
    has_reported_after_focus_ = false;
  }
  void InsertFocusChangedUntilFirstRasterFinish();
  void InsertFocusChangedUntilFirstPaintFinish();
  void InsertForceTimeRecordUntilNow(int64_t from, Perf perf);

 private:
  const char* PerfToString(Perf perf) const;

  bool FirstPerfReady() const;
  bool UpdatePerfReady() const;

  void InsertRecordInternal(Perf perf, int64_t cost);
  void InsertForceRecordInternal(const PerfMap& perf_map);

  void MaybeReport(bool is_force = false);

  fml::WeakPtrFactory<FrameTimingCollector> weak_factory_;
  fml::WeakPtr<FrameTimingCollector> weak_;

  fml::RefPtr<fml::TaskRunner> platform_task_runner_;

  bool is_recording_first_frame_perf_ = true;
  bool is_first_send_ = true;
  PageView* page_view_ = nullptr;

  static const std::unordered_map<Perf, const char*> perf_to_string_map;

  std::unordered_map<Perf, int64_t> perf_record_;

  std::unordered_map<std::string, int64_t> first_perf_container_;
  std::unordered_map<std::string, int64_t> update_perf_container_;
  std::unordered_map<std::string, int64_t> force_perf_container_;

  int64_t receive_focus_time_ = 0;
  FocusManager::Direction focus_direction_ = FocusManager::Direction::kUnknown;
  bool has_reported_after_focus_ = false;
};

}  // namespace clay

#endif  // CLAY_UI_COMMON_FRAME_TIMING_COLLECTOR_H_
