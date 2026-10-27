// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMMON_UTILS_WATCH_DOG_H_
#define CLAY_UI_COMMON_UTILS_WATCH_DOG_H_

#include "base/include/fml/time/timer.h"

namespace clay {

class WatchDog {
 public:
  explicit WatchDog(fml::RefPtr<fml::TaskRunner> task_runner);
  ~WatchDog();

  void Start(int64_t time_ms, int64_t interval_ms,
             std::function<void()> callback);
  void Stop();
  bool IsAlive();
  void FeedDog();

 private:
  void TimeFired();

  fml::Timer timer_;
  std::function<void()> callback_;
  int elapsed_count_ = 0;
  int max_count_ = 0;
};
}  // namespace clay
#endif  // CLAY_UI_COMMON_UTILS_WATCH_DOG_H_
