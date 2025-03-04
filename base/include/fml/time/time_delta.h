// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_TIME_TIME_DELTA_H_
#define BASE_INCLUDE_FML_TIME_TIME_DELTA_H_

#include <chrono>
#include <cstdint>
#include <ctime>
#include <iosfwd>
#include <limits>

namespace lynx {
namespace fml {

// A TimeDelta represents the difference between two time points.
class TimeDelta {
 public:
  constexpr TimeDelta() = default;

  static constexpr TimeDelta Zero() { return TimeDelta(); }
  static constexpr TimeDelta Min() {
    return TimeDelta(std::numeric_limits<int64_t>::min());
  }
  static constexpr TimeDelta Max() {
    return TimeDelta(std::numeric_limits<int64_t>::max());
  }
  static constexpr TimeDelta FromNanoseconds(int64_t nanos) {
    return TimeDelta(nanos);
  }
  static constexpr TimeDelta FromMicroseconds(int64_t micros) {
    return FromNanoseconds(micros * 1000);
  }
  static constexpr TimeDelta FromMilliseconds(int64_t millis) {
    return FromMicroseconds(millis * 1000);
  }
  static constexpr TimeDelta FromSeconds(int64_t seconds) {
    return FromMilliseconds(seconds * 1000);
  }

  static constexpr TimeDelta FromSecondsF(double seconds) {
    return FromNanoseconds(seconds * (1000.0 * 1000.0 * 1000.0));
  }

  static constexpr TimeDelta FromMillisecondsF(double millis) {
    return FromNanoseconds(millis * (1000.0 * 1000.0));
  }

  constexpr int64_t ToNanoseconds() const { return delta_; }
  constexpr int64_t ToMicroseconds() const { return ToNanoseconds() / 1000; }
  constexpr int64_t ToMilliseconds() const { return ToMicroseconds() / 1000; }
  constexpr int64_t ToSeconds() const { return ToMilliseconds() / 1000; }

  constexpr double ToNanosecondsF() const { return delta_; }
  constexpr double ToMicrosecondsF() const { return delta_ / 1000.0; }
  constexpr double ToMillisecondsF() const {
    return delta_ / (1000.0 * 1000.0);
  }
  constexpr double ToSecondsF() const {
    return delta_ / (1000.0 * 1000.0 * 1000.0);
  }

  constexpr TimeDelta operator-(TimeDelta other) const {
    return TimeDelta::FromNanoseconds(delta_ - other.delta_);
  }

  constexpr TimeDelta operator+(TimeDelta other) const {
    return TimeDelta::FromNanoseconds(delta_ + other.delta_);
  }

  constexpr TimeDelta operator/(int64_t divisor) const {
    return TimeDelta::FromNanoseconds(delta_ / divisor);
  }

  constexpr int64_t operator/(TimeDelta other) const {
    return delta_ / other.delta_;
  }

  constexpr TimeDelta operator*(int64_t multiplier) const {
    return TimeDelta::FromNanoseconds(delta_ * multiplier);
  }

  constexpr TimeDelta operator*(double multiplier) const {
    return TimeDelta::FromNanoseconds(delta_ * multiplier);
  }

  constexpr TimeDelta operator%(TimeDelta other) const {
    return TimeDelta::FromNanoseconds(delta_ % other.delta_);
  }

  bool operator==(TimeDelta other) const { return delta_ == other.delta_; }
  bool operator!=(TimeDelta other) const { return delta_ != other.delta_; }
  bool operator<(TimeDelta other) const { return delta_ < other.delta_; }
  bool operator<=(TimeDelta other) const { return delta_ <= other.delta_; }
  bool operator>(TimeDelta other) const { return delta_ > other.delta_; }
  bool operator>=(TimeDelta other) const { return delta_ >= other.delta_; }

  static constexpr TimeDelta FromTimespec(struct timespec ts) {
    return TimeDelta::FromSeconds(ts.tv_sec) +
           TimeDelta::FromNanoseconds(ts.tv_nsec);
  }
  struct timespec ToTimespec() {
    struct timespec ts;
    constexpr int64_t kNanosecondsPerSecond = 1000000000ll;
    ts.tv_sec = static_cast<time_t>(ToSeconds());
    ts.tv_nsec = delta_ % kNanosecondsPerSecond;
    return ts;
  }

 private:
  // Private, use one of the FromFoo() types
  explicit constexpr TimeDelta(int64_t delta) : delta_(delta) {}

  int64_t delta_ = 0;
};

}  // namespace fml
}  // namespace lynx

namespace fml {

// NOLINTNEXTLINE
using namespace std::chrono_literals;

using Milliseconds = std::chrono::duration<double, std::milli>;

// Default to 60fps.
constexpr Milliseconds kDefaultFrameBudget = Milliseconds(1s) / 60;

template <typename T>
Milliseconds RefreshRateToFrameBudget(T refresh_rate) {
  return Milliseconds(1s) / refresh_rate;
}

using lynx::fml::TimeDelta;

}  // namespace fml

#endif  // BASE_INCLUDE_FML_TIME_TIME_DELTA_H_
