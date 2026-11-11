// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/trace/native/platform/windows/trace_controller_win.h"

namespace lynx {
namespace trace {

TraceController* GetTraceControllerInstance() {
  return TraceController::Instance();
}

}  // namespace trace
}  // namespace lynx
