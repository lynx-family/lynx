// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/common/utils/watch_dog.h"

#include <utility>

#include "clay/fml/logging.h"

namespace clay {

WatchDog::WatchDog(fml::RefPtr<fml::TaskRunner> task_runner)
    : timer_(task_runner, true) {}

WatchDog::~WatchDog() = default;

void WatchDog::Start(int64_t time_ms, int64_t interval_ms,
                     std::function<void()> callback) {
  FML_DCHECK(time_ms > interval_ms);
  FML_DCHECK(callback);
  if (time_ms % interval_ms) {
    FML_LOG(WARNING) << "time_ms is not a multiple of interval_ms.";
  }

  callback_ = std::move(callback);
  max_count_ = time_ms / interval_ms;
  elapsed_count_ = 0;
  timer_.Start(fml::TimeDelta::FromMilliseconds(interval_ms),
               [this]() { this->TimeFired(); });
}

void WatchDog::Stop() {
  callback_ = nullptr;
  max_count_ = 0;
  elapsed_count_ = 0;
  timer_.Stop();
}

bool WatchDog::IsAlive() { return max_count_ > 0; }

void WatchDog::FeedDog() { elapsed_count_ = 0; }

void WatchDog::TimeFired() {
  elapsed_count_++;
  if (elapsed_count_ >= max_count_) {
    FML_DCHECK(callback_);
    callback_();
    Stop();
  }
}

}  // namespace clay
