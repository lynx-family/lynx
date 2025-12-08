// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_STOPWATCH_H_
#define CLAY_FLOW_STOPWATCH_H_

#include <memory>
#include <vector>

#include "base/include/fml/macros.h"
#include "base/include/fml/time/time_delta.h"
#include "base/include/fml/time/time_point.h"
#include "clay/gfx/rendering_backend.h"
#include "skity/geometry/rect.hpp"

namespace clay {

class Stopwatch {
 public:
  /// The refresh rate interface for `Stopwatch`.
  class RefreshRateUpdater {
   public:
    /// Time limit for a smooth frame.
    /// See: `DisplayManager::GetMainDisplayRefreshRate`.
    virtual fml::Milliseconds GetFrameBudget() const = 0;
  };

  /// The constructor with a updater parameter, it will update frame_budget
  /// everytime when `GetFrameBudget()` is called.
  explicit Stopwatch(const RefreshRateUpdater& updater);

  ~Stopwatch();

  const fml::TimeDelta& LastLap() const;

  fml::TimeDelta MaxDelta() const;

  fml::TimeDelta AverageDelta() const;

  void InitVisualizeSurface(const skity::Rect& rect) const;

  void Visualize(clay::GrCanvas* canvas, const skity::Rect& rect) const;

  void Start(fml::TimePoint time = fml::TimePoint::Now());

  void Stop(fml::TimePoint time = fml::TimePoint::Now());

  void SetLapTime(const fml::TimeDelta& delta);

  /// All places which want to get frame_budget should call this function.
  fml::Milliseconds GetFrameBudget() const;
  bool CheckIfLastSampleInCycle() const;

  const std::vector<fml::TimeDelta>& GetLaps() const { return laps_; }

 private:
  inline double UnitFrameInterval(double time_ms) const;
  inline double UnitHeight(double time_ms, double max_height) const;

  const RefreshRateUpdater& refresh_rate_updater_;
  fml::TimePoint start_;
  std::vector<fml::TimeDelta> laps_;
  size_t current_sample_;

  // Mutable data cache for performance optimization of the graphs. Prevents
  // expensive redrawing of old data.
  mutable bool cache_dirty_;
  [[maybe_unused]] mutable clay::GrSurfacePtr visualize_cache_surface_;
  [[maybe_unused]] mutable std::unique_ptr<clay::GrBitmap>
      visualize_cache_bitmap_;
  mutable std::unique_ptr<clay::GrCanvas> visualize_cache_canvas_;
  mutable size_t prev_drawn_sample_index_;

  BASE_DISALLOW_COPY_AND_ASSIGN(Stopwatch);
};

/// Used for fixed refresh rate query cases.
class FixedRefreshRateUpdater : public Stopwatch::RefreshRateUpdater {
  fml::Milliseconds GetFrameBudget() const override;

 public:
  explicit FixedRefreshRateUpdater(
      fml::Milliseconds fixed_frame_budget = fml::kDefaultFrameBudget);

 private:
  fml::Milliseconds fixed_frame_budget_;
};

/// Used for fixed refresh rate cases.
class FixedRefreshRateStopwatch : public Stopwatch {
 public:
  explicit FixedRefreshRateStopwatch(
      fml::Milliseconds fixed_frame_budget = fml::kDefaultFrameBudget);

 private:
  FixedRefreshRateUpdater fixed_delegate_;
};

class Counter {
 public:
  Counter() : count_(0) {}

  size_t count() const { return count_; }

  void Reset(size_t count = 0) { count_ = count; }

  void Increment(size_t count = 1) { count_ += count; }

 private:
  size_t count_;

  BASE_DISALLOW_COPY_AND_ASSIGN(Counter);
};

class CounterValues {
 public:
  CounterValues();

  ~CounterValues();

  void Add(int64_t value);

  void Visualize(clay::GrCanvas* canvas, const skity::Rect& rect) const;

  int64_t GetMaxValue() const;

  int64_t GetMinValue() const;

 private:
  std::vector<int64_t> values_;
  size_t current_sample_;

  BASE_DISALLOW_COPY_AND_ASSIGN(CounterValues);
};

}  // namespace clay

#endif  // CLAY_FLOW_STOPWATCH_H_
