// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_BENCHMARKING_BENCHMARKING_H_
#define CLAY_BENCHMARKING_BENCHMARKING_H_

#include "third_party/benchmark/include/benchmark/benchmark.h"

namespace benchmarking {

class ScopedPauseTiming {
 public:
  explicit ScopedPauseTiming(::benchmark::State& state,
                             bool enabled = true)  // NOLINT
      : state_(state), enabled_(enabled) {
    if (enabled_) {
      state_.PauseTiming();
    }
  }
  ~ScopedPauseTiming() {
    if (enabled_) {
      state_.ResumeTiming();
    }
  }

 private:
  ::benchmark::State& state_;
  const bool enabled_;
};

}  // namespace benchmarking

#endif  // CLAY_BENCHMARKING_BENCHMARKING_H_
