// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture/velocity_tracker.h"

#include <list>

namespace clay {

namespace {

#if defined(OS_HARMONY)
constexpr float kVelocityScale = 1.5f;
#else
constexpr float kVelocityScale = 1.0f;
#endif

constexpr uint32_t kMicrosPerMills = 1000;
constexpr uint32_t kMillsPerSecond = 1000;
constexpr uint32_t kMicrosPerSecond = kMicrosPerMills * kMillsPerSecond;

// Threshold between Action::MOVE events for determining that a pointer has
// stopped moving. Some input devices do not send Action::MOVE events in the
// case where a pointer has stopped.  We need to detect this case so that we can
// accurately predict the velocity after the pointer starts moving again.
constexpr int32_t kAssumePointerMoveStoppedTimeMicros = 40 * kMicrosPerMills;

// Threshold between Action::MOVE and Action::{UP|POINTER_UP} events for
// determining that a pointer has stopped moving. This is a larger threshold
// than |kAssumePointerMoveStoppedTimeMs|, as some devices may delay synthesis
// of Action::{UP|POINTER_UP} to reduce risk of noisy release.
constexpr int32_t kAssumePointerUpStoppedTimeMicros = 80 * kMicrosPerMills;

float VectorDot(const float* left, const float* right, uint32_t length) {
  float result = 0;
  while (length--) {
    result += *(left++) * *(right++);
  }
  return result;
}

float VectorNorm(const float* vec, uint32_t length) {
  float result = 0;
  while (length--) {
    float value = *(vec++);
    result += value * value;
  }
  return sqrtf(result);
}

// Velocity tracker algorithm based on least-squares linear regression.
class LeastSquaresVelocityTrackerStrategy : public VelocityTrackerStrategy {
 public:
  // Number of samples to keep.
  static const uint8_t kHistorySize = 20;

  // Degree must be no greater than Estimator::kMaxDegree.
  explicit LeastSquaresVelocityTrackerStrategy(
      uint32_t degree = Estimator::kDefaultDegree);

  ~LeastSquaresVelocityTrackerStrategy() override = default;

  void Clear() override;

  void AddPosition(uint64_t event_time, const FloatPoint& position) override;

  bool GetEstimator(Estimator* out_estimator, bool is_vertical) const override;

 private:
  // Sample horizon.
  // We don't use too much history by default since we want to react to quick
  // changes in direction.
  static const int32_t kHorizonMicros = 100 * kMicrosPerMills;

  struct Movement {
    Movement(uint64_t event_time, const FloatPoint& pos)
        : event_time_in_micros(event_time), position(pos) {}
    uint64_t event_time_in_micros = 0;
    FloatPoint position;
  };

  const uint32_t degree_;
  std::list<Movement> movements_;
};

}  // namespace

Velocity& Velocity::Clamp(float min_velocity, float max_velocity) {
  float velocity = pixels_per_second_.distance();
  if (velocity < min_velocity) {
    pixels_per_second_.ExpandByRatio(min_velocity / velocity);
  } else if (velocity > max_velocity) {
    pixels_per_second_.ExpandByRatio(max_velocity / velocity);
  }
  return *this;
}

VelocityTracker::VelocityTracker()
    : is_first_(true),
      last_event_time_in_micros_(0),
      strategy_(new LeastSquaresVelocityTrackerStrategy()) {}

void VelocityTracker::Clear() { strategy_->Clear(); }

void VelocityTracker::AddPosition(const FloatPoint& position,
                                  uint64_t event_time_in_micros, bool end) {
  if (end) {
    // Note that Action::UP and Action::POINTER_UP always report the last
    // known position of the pointers that went up.  Action::POINTER_UP does
    // include the new position of pointers that remained down but we will
    // also receive an Action::MOVE with this information if any of them
    // actually moved.  Since we don't know how many pointers will be going up
    // at once it makes sense to just wait for the following Action::MOVE
    // before adding the movement. However, if the up event itself is delayed
    // because of (difficult albeit possible) prolonged stationary screen
    // contact, assume that motion has stopped.
    if (event_time_in_micros - last_event_time_in_micros_ >=
        kAssumePointerUpStoppedTimeMicros) {
      // We have not received any movements for too long. Assume that all
      // pointers have stopped.
      strategy_->Clear();
      return;
    }
  } else if (!is_first_ && (event_time_in_micros - last_event_time_in_micros_ >=
                            kAssumePointerMoveStoppedTimeMicros)) {
    // We have not received any movements for too long. Assume that all pointers
    // have stopped.
    strategy_->Clear();
  }

  strategy_->AddPosition(event_time_in_micros, position);

  if (!is_first_) {
    total_movement_.Expand(position.x() - last_position_.x(),
                           position.y() - last_position_.y());
  }
  is_first_ = false;
  last_event_time_in_micros_ = event_time_in_micros;
  last_position_ = position;
}

VelocityEstimate VelocityTracker::GetVelocityEstimate(bool is_vertical) {
  VelocityEstimate result;
  result.movement = total_movement_;
  Estimator out_estimator;
  if (strategy_->GetEstimator(&out_estimator, is_vertical) &&
      out_estimator.degree >= 1) {
    result.pixels_per_second = {out_estimator.x_coeff[1] * kVelocityScale,
                                out_estimator.y_coeff[1] * kVelocityScale};
  } else {
    result.pixels_per_second = {0, 0};
  }

  return result;
}

/**
 * Solves a linear least squares problem to obtain a N degree polynomial that
 * fits the specified input data as nearly as possible.
 *
 * Returns true if a solution is found, false otherwise.
 *
 * The input consists of two vectors of data points X and Y with indices 0..m-1
 * along with a weight vector W of the same size.
 *
 * The output is a vector B with indices 0..n that describes a polynomial
 * that fits the data, such the sum of W[i] * W[i] * abs(Y[i] - (B[0] + B[1]
 * X[i] * + B[2] X[i]^2 ... B[n] X[i]^n)) for all i between 0 and m-1 is
 * minimized.
 *
 * Accordingly, the weight vector W should be initialized by the caller with the
 * reciprocal square root of the variance of the error in each input data point.
 * In other words, an ideal choice for W would be W[i] = 1 / var(Y[i]) = 1 /
 * stddev(Y[i]).
 * The weights express the relative importance of each data point.  If the
 * weights are* all 1, then the data points are considered to be of equal
 * importance when fitting the polynomial.  It is a good idea to choose weights
 * that diminish the importance of data points that may have higher than usual
 * error margins.
 *
 * Errors among data points are assumed to be independent.  W is represented
 * here as a vector although in the literature it is typically taken to be a
 * diagonal matrix.
 *
 * That is to say, the function that generated the input data can be
 * approximated by y(x) ~= B[0] + B[1] x + B[2] x^2 + ... + B[n] x^n.
 *
 * The coefficient of determination (R^2) is also returned to describe the
 * goodness of fit of the model for the given data.  It is a value between 0
 * and 1, where 1 indicates perfect correspondence.
 *
 * This function first expands the X vector to a m by n matrix A such that
 * A[i][0] = 1, A[i][1] = X[i], A[i][2] = X[i]^2, ..., A[i][n] = X[i]^n, then
 * multiplies it by w[i]./
 *
 * Then it calculates the QR decomposition of A yielding an m by m orthonormal
 * matrix Q and an m by n upper triangular matrix R.  Because R is upper
 * triangular (lower part is all zeroes), we can simplify the decomposition into
 * an m by n matrix Q1 and a n by n matrix R1 such that A = Q1 R1.
 *
 * Finally we solve the system of linear equations given by
 * R1 B = (Q's transpose W Y) to find B.
 *
 * For efficiency, we lay out A and Q column-wise in memory because we
 * frequently operate on the column vectors.  Conversely, we lay out R row-wise.
 *
 * http://en.wikipedia.org/wiki/Numerical_methods_for_linear_least_squares
 * http://en.wikipedia.org/wiki/Gram-Schmidt
 */
static bool SolveLeastSquares(const float* x, const float* y, const float* w,
                              uint32_t m, uint32_t n, float* out_b,
                              float* out_det) {
  const uint32_t M_ARRAY_LENGTH = m;
  const uint32_t N_ARRAY_LENGTH = n;

  // Expand the X vector to a matrix A, pre-multiplied by the weights.
  float a[N_ARRAY_LENGTH][M_ARRAY_LENGTH];  // column-major order
  for (uint32_t h = 0; h < m; h++) {
    a[0][h] = w[h];
    for (uint32_t i = 1; i < n; i++) {
      a[i][h] = a[i - 1][h] * x[h];
    }
  }

  // Apply the Gram-Schmidt process to A to obtain its QR decomposition.

  // Orthonormal basis, column-major order.
  float q[N_ARRAY_LENGTH][M_ARRAY_LENGTH];
  // Upper triangular matrix, row-major order.
  float r[N_ARRAY_LENGTH][N_ARRAY_LENGTH];
  for (uint32_t j = 0; j < n; j++) {
    for (uint32_t h = 0; h < m; h++) {
      q[j][h] = a[j][h];
    }
    for (uint32_t i = 0; i < j; i++) {
      float dot = VectorDot(&q[j][0], &q[i][0], m);
      for (uint32_t h = 0; h < m; h++) {
        q[j][h] -= dot * q[i][h];
      }
    }

    float norm = VectorNorm(&q[j][0], m);
    if (norm < 0.000001f) {
      // vectors are linearly dependent or zero so no solution
      return false;
    }

    float invNorm = 1.0f / norm;
    for (uint32_t h = 0; h < m; h++) {
      q[j][h] *= invNorm;
    }
    for (uint32_t i = 0; i < n; i++) {
      r[j][i] = i < j ? 0 : VectorDot(&q[j][0], &a[i][0], m);
    }
  }

  // Solve R B = Qt W Y to find B.  This is easy because R is upper triangular.
  // We just work from bottom-right to top-left calculating B's coefficients.
  float wy[M_ARRAY_LENGTH];
  for (uint32_t h = 0; h < m; h++) {
    wy[h] = y[h] * w[h];
  }
  for (uint32_t i = n; i-- != 0;) {
    out_b[i] = VectorDot(&q[i][0], wy, m);
    for (uint32_t j = n - 1; j > i; j--) {
      out_b[i] -= r[i][j] * out_b[j];
    }
    out_b[i] /= r[i][i];
  }

  // Calculate the coefficient of determination as 1 - (SSerr / SStot) where
  // SSerr is the residual sum of squares (variance of the error),
  // and SStot is the total sum of squares (variance of the data) where each
  // has been weighted.
  float y_mean = 0;
  for (uint32_t h = 0; h < m; h++) {
    y_mean += y[h];
  }
  y_mean /= m;

  float ss_err = 0;
  float ss_tot = 0;
  for (uint32_t h = 0; h < m; h++) {
    float err = y[h] - out_b[0];
    float term = 1;
    for (uint32_t i = 1; i < n; i++) {
      term *= x[h];
      err -= term * out_b[i];
    }
    ss_err += w[h] * w[h] * err * err;
    float var = y[h] - y_mean;
    ss_tot += w[h] * w[h] * var * var;
  }
  *out_det = ss_tot > 0.000001f ? 1.0f - (ss_err / ss_tot) : 1;
  return true;
}

LeastSquaresVelocityTrackerStrategy::LeastSquaresVelocityTrackerStrategy(
    uint32_t degree)
    : degree_(degree) {}

void LeastSquaresVelocityTrackerStrategy::Clear() { movements_.clear(); }

void LeastSquaresVelocityTrackerStrategy::AddPosition(
    uint64_t event_time_in_micros, const FloatPoint& position) {
  movements_.emplace_back(event_time_in_micros, position);
  if (movements_.size() > kHistorySize) {
    movements_.pop_front();
  }
}

bool LeastSquaresVelocityTrackerStrategy::GetEstimator(Estimator* out_estimator,
                                                       bool is_vertical) const {
  out_estimator->Clear();

  if (movements_.empty()) {
    return false;
  }

  // Iterate over movement samples in reverse time order and collect samples.
  float position[kHistorySize];
  float w[kHistorySize];
  float time[kHistorySize];

  uint64_t newest_event_time_in_micros =
      movements_.rbegin()->event_time_in_micros;
  uint32_t sample_num = 0;
  for (auto iter = movements_.rbegin(); iter != movements_.rend(); ++iter) {
    uint64_t age = newest_event_time_in_micros - iter->event_time_in_micros;
    if (age > kHorizonMicros) break;

    position[sample_num] =
        is_vertical ? iter->position.y() : iter->position.x();
    w[sample_num] = 1.0f;
    time[sample_num] = -static_cast<float>(age) / kMicrosPerSecond;
    sample_num++;
  }

  if (sample_num == 0) return false;  // no data

  // Calculate a least squares polynomial fit.
  uint32_t degree = degree_;
  if (degree > sample_num - 1) degree = sample_num - 1;

  if (degree >= 1) {
    float det = 0;
    uint32_t n = degree + 1;
    if (SolveLeastSquares(
            time, position, w, sample_num, n,
            is_vertical ? out_estimator->y_coeff : out_estimator->x_coeff,
            &det)) {
      float move_distance = position[0] - position[sample_num - 1];
      float move_velocity =
          is_vertical ? out_estimator->y_coeff[1] : out_estimator->x_coeff[1];
      // If the velocity is in a sufficiently different direction from the
      // primary movement, ignore it.
      if (move_distance * move_velocity < 0) {
        return false;
      }

      out_estimator->time = newest_event_time_in_micros;
      out_estimator->degree = degree;
      out_estimator->confidence = det;
      return true;
    }
  }

  // No velocity data available for this pointer, but we do have its current
  // position.
  out_estimator->x_coeff[0] = is_vertical ? 0.f : position[0];
  out_estimator->y_coeff[0] = is_vertical ? position[0] : 0.f;
  out_estimator->time = newest_event_time_in_micros;
  out_estimator->degree = 0;
  out_estimator->confidence = 1.f;
  return true;
}

}  // namespace clay
