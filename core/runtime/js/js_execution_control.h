// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_JS_EXECUTION_CONTROL_H_
#define CORE_RUNTIME_JS_JS_EXECUTION_CONTROL_H_

#include <string>

#include "core/runtime/js/jsi/jsi.h"

namespace lynx {
namespace runtime {
namespace js {

// Result of a JavaScript stack capture request. Currently only the V8 engine
// supports capturing, other JS engines always report kVMDestroyed/unsupported.
// The values must stay in sync with LynxJavaScriptExecutionCallback so the
// platform layer can forward them directly.
enum class JSStackCaptureResult : int32_t {
  // The stack was captured successfully.
  kSuccess = 0,
  // The JS VM is alive but cannot return a stack right now.
  kStackUnavailable = 2,
  // The JS VM has already been destroyed, so the request cannot be routed.
  kVMDestroyed = 3,
};

void RegisterVMInstance(VMInstance* vm);
void UnregisterVMInstance(VMInstance* vm);
void CaptureJavaScriptStack(
    VMInstance* vm,
    base::MoveOnlyClosure<void, JSStackCaptureResult, std::string> callback);
bool TerminateJavaScriptExecution(VMInstance* vm);

}  // namespace js
}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_JS_EXECUTION_CONTROL_H_
