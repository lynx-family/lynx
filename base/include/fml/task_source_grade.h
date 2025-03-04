// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_TASK_SOURCE_GRADE_H_
#define BASE_INCLUDE_FML_TASK_SOURCE_GRADE_H_

namespace lynx {
namespace fml {

/**
 * Categories of work dispatched to `MessageLoopTaskQueues` dispatcher. By
 * specifying the `TaskSourceGrade`, you indicate the task's importance to the
 * dispatcher.
 */
enum class TaskSourceGrade {
  /// This `TaskSourceGrade` indicates that a task is critical to user
  /// interaction.
  kUserInteraction,
  /// The absence of a specialized `TaskSourceGrade`.
  kUnspecified,
  /// This `TaskSourceGrade` indicates that a task is urgent to execute.
  kEmergency,

  /// This `TaskSourceGrade` indicates that a task is a microtask.
  /// Note: this is only for the JS thread and is used to simulate microtasks.
  /// In the js thread, this level is the highest priority.
  kMicrotask,
  /// This `TaskSourceGrade` indicates that a task just executes when idle.
  kIdle,
};

}  // namespace fml
}  // namespace lynx

#endif  // BASE_INCLUDE_FML_TASK_SOURCE_GRADE_H_
