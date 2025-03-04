// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_THREADING_THREAD_MERGER_H_
#define CORE_BASE_THREADING_THREAD_MERGER_H_

#include "base/include/fml/task_runner.h"

namespace lynx {
namespace base {

class ThreadMerger {
 public:
  ThreadMerger(fml::TaskRunner* owner, fml::TaskRunner* subsumed);
  ~ThreadMerger();

  static void Merge(fml::TaskRunner* owner, fml::TaskRunner* subsumed);
  ThreadMerger(const ThreadMerger&) = delete;
  ThreadMerger& operator=(const ThreadMerger&) = delete;
  ThreadMerger(ThreadMerger&&);
  ThreadMerger& operator=(ThreadMerger&&);

 private:
  fml::TaskRunner* owner_;

  fml::TaskRunner* subsumed_;
};

}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_THREADING_THREAD_MERGER_H_
