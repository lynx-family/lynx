// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/**
 * This empty implementation is used when the Trace functionality is turned off
 */

#include "base/trace/native/trace_controller.h"

namespace lynx {
namespace trace {

TraceController* TraceController::Instance() {
  static TraceController instance_;
  return &instance_;
}

}  // namespace trace
}  // namespace lynx
