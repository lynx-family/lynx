// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_VELOCITY_TRACKER_H_
#define CLAY_UI_GESTURE_VELOCITY_TRACKER_H_

#include <memory>

#include "clay/gfx/geometry/float_point.h"

namespace clay {

struct Estimator {
  static const uint8_t kMaxDegree = 4;
  static const uint8_t kDefaultDegree = 2;

  // Estimator time base.
  int64_t time;
  // Polynomial coefficients describing motion in X and Y.
  float x_coeff[kMaxDegree + 1], y_coeff[kMaxDegree + 1];
  // Polynomial degree (number of coefficients), or zero if no information is
  // available.
  uint32_t degree;
  // Confidence (coefficient of determination), between 0 (no fit)
  // and 1 (perfect fit).
  float confidence;

  inline void Clear() {
    time = 0;
    degree = 0;
    confidence = 0;
    for (size_t i = 0; i <= kMaxDegree; i++) {
      x_coeff[i] = 0;
      y_coeff[i] = 0;
    }
  }
};

class VelocityTrackerStrategy {
 public:
  virtual ~VelocityTrackerStrategy() = default;

  virtual void Clear() = 0;
  virtual void AddPosition(uint64_t event_time_in_micros,
                           const FloatPoint& position) = 0;
  virtual bool GetEstimator(Estimator* out_estimator,
                            bool is_vertical) const = 0;

 protected:
  VelocityTrackerStrategy() = default;
};

class Velocity {
 public:
  Velocity() = default;
  explicit Velocity(const FloatSize& velocity) : pixels_per_second_(velocity) {}

  const FloatSize& pixels_per_second() const { return pixels_per_second_; }

  Velocity& Clamp(float min_velocity, float max_velocity);

 private:
  FloatSize pixels_per_second_;
};

// A intermediate util class stands for an estimated velocity including
// informations about how the velocity calculated.
struct VelocityEstimate {
  FloatSize pixels_per_second;
  // The distance corresponding to the estimated |pixels_per_second|.
  FloatSize movement;
};

class VelocityTracker {
 public:
  VelocityTracker();

  void AddPosition(const FloatPoint& position, uint64_t event_time_in_micros,
                   bool end = false);
  void Clear();

  VelocityEstimate GetVelocityEstimate(bool is_vertical);

 private:
  bool is_first_;
  uint64_t last_event_time_in_micros_;
  FloatPoint last_position_;
  FloatSize total_movement_;
  std::unique_ptr<VelocityTrackerStrategy> strategy_;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_VELOCITY_TRACKER_H_
