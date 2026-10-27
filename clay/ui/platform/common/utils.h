// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PLATFORM_COMMON_UTILS_H_
#define CLAY_UI_PLATFORM_COMMON_UTILS_H_

#include <utility>

#include "base/include/fml/task_runner.h"
#include "clay/ui/common/isolate.h"

namespace clay {

inline void RunOnPlatformThread(lynx::base::closure task) {
  fml::TaskRunner::RunNowOrPostTask(Isolate::Instance().GetPlatformTaskRunner(),
                                    std::move(task));
}

}  // namespace clay

#endif  // CLAY_UI_PLATFORM_COMMON_UTILS_H_
