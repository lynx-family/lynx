// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_TASK_QUEUE_ID_H_
#define BASE_INCLUDE_FML_TASK_QUEUE_ID_H_

#include <climits>

namespace lynx {
namespace fml {

/**
 * `MessageLoopTaskQueues` task dispatcher's internal task queue identifier.
 */
class TaskQueueId {
 public:
  /// This constant indicates whether a task queue has been subsumed by a task
  /// runner.
  static constexpr size_t kUnmerged = ULONG_MAX;

  /// Intializes a task queue with the given value as it's ID.
  explicit constexpr TaskQueueId(size_t value) : value_(value) {}

  constexpr operator size_t() const {  // NOLINT(google-explicit-constructor)
    return value_;
  }

 private:
  size_t value_ = kUnmerged;
};

constexpr TaskQueueId _kUnmerged = TaskQueueId(TaskQueueId::kUnmerged);

}  // namespace fml
}  // namespace lynx

namespace fml {
using lynx::fml::TaskQueueId;
}  // namespace fml

#endif  // BASE_INCLUDE_FML_TASK_QUEUE_ID_H_
