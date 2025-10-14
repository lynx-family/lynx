// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SCHEDULER_SCHEDULER_CLIENT_H_
#define CLAY_SHELL_COMMON_SCHEDULER_SCHEDULER_CLIENT_H_

#include <memory>

namespace clay {

class SchedulerClient {
 public:
  virtual void ScheduledActionBeginFrame() = 0;
  virtual void ScheduledActionCommit() = 0;
  virtual void ScheduledActionActivePendingTree() = 0;
  virtual void ScheduledActionRasterInvalidate() = 0;
  virtual void ScheduledActionDraw() = 0;
  virtual void ScheduledActionUploadImage() = 0;
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SCHEDULER_SCHEDULER_CLIENT_H_
