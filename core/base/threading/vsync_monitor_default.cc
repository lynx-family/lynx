// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "core/base/threading/vsync_monitor.h"

namespace lynx {
namespace base {

std::shared_ptr<VSyncMonitor> VSyncMonitor::Create() { return nullptr; }

}  // namespace base
}  // namespace lynx
