// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_DELAYED_TASK_H_
#define BASE_INCLUDE_FML_DELAYED_TASK_H_

#include <functional>
#include <queue>
#include <utility>

#include "base/include/base_defines.h"
#include "base/include/closure.h"
#include "base/include/fml/task_source_grade.h"
#include "base/include/fml/time/time_point.h"
#include "base/include/vector.h"

namespace lynx {
namespace fml {

class DelayedTask {
 public:
  DelayedTask(size_t order, base::closure task, fml::TimePoint target_time,
              fml::TaskSourceGrade task_source_grade)
      : order_(order),
        task_source_grade_(task_source_grade),
        target_time_(target_time),
        task_(std::move(task)) {}

  ~DelayedTask() = default;

  DelayedTask(const DelayedTask&) = delete;
  DelayedTask& operator=(const DelayedTask&) = delete;
  BASE_INLINE DelayedTask(DelayedTask&&) = default;
  BASE_INLINE DelayedTask& operator=(DelayedTask&&) = default;

  // after invoke this func, task_ will become nullptr!
  base::closure GetTask() const { return std::move(task_); }

  fml::TimePoint GetTargetTime() const { return target_time_; }

  fml::TaskSourceGrade GetTaskSourceGrade() const { return task_source_grade_; }

  // Let std::priority_queue algorithms to always inline compare function.
  BASE_INLINE bool operator>(const DelayedTask& other) const {
    if (target_time_ == other.target_time_) {
      return order_ > other.order_;
    }
    return target_time_ > other.target_time_;
  }

  // A flag telling base containers such as `base::Vector<DelayedTask>` to
  // optimize for reallocate, insert and erase.
  using TriviallyRelocatable = bool;

 private:
  size_t order_;
  fml::TaskSourceGrade task_source_grade_;
  fml::TimePoint target_time_;
  mutable base::closure task_;
};

class DelayedTaskQueue
    : public std::priority_queue<DelayedTask, base::Vector<DelayedTask>,
                                 std::greater<DelayedTask>> {
 public:
  using std::priority_queue<DelayedTask, base::Vector<DelayedTask>,
                            std::greater<DelayedTask>>::priority_queue;
  void reserve(size_t capacity) { this->c.reserve(capacity); }
  void clear() { this->c.clear(); }
};

}  // namespace fml
}  // namespace lynx

#endif  // BASE_INCLUDE_FML_DELAYED_TASK_H_
