// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/log/logging.h"
#include "base/include/log/logging_base.h"

namespace lynx::base::logging {

void SetPlatformMinLogLevel(int level) { lynx::SetMinimumLoggingLevel(level); }

}  // namespace lynx::base::logging
