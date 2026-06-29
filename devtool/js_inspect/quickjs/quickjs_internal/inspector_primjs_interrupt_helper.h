// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef DEVTOOL_JS_INSPECT_QUICKJS_QUICKJS_INTERNAL_INSPECTOR_PRIMJS_INTERRUPT_HELPER_H_
#define DEVTOOL_JS_INSPECT_QUICKJS_QUICKJS_INTERNAL_INSPECTOR_PRIMJS_INTERRUPT_HELPER_H_

#include <memory>
#include <mutex>
#include <vector>

#include "base/include/closure.h"
#include "quickjs/include/quickjs.h"

namespace lynx {
namespace devtool {

class InspectorPrimjsInterruptHelper {
 public:
  explicit InspectorPrimjsInterruptHelper(LEPUSRuntime* runtime);
  ~InspectorPrimjsInterruptHelper();

  InspectorPrimjsInterruptHelper(const InspectorPrimjsInterruptHelper&) =
      delete;
  InspectorPrimjsInterruptHelper& operator=(
      const InspectorPrimjsInterruptHelper&) = delete;
  InspectorPrimjsInterruptHelper(InspectorPrimjsInterruptHelper&&) = delete;
  InspectorPrimjsInterruptHelper& operator=(InspectorPrimjsInterruptHelper&&) =
      delete;

  void Request(base::closure closure);

 private:
  static int InterruptHandler(LEPUSRuntime* rt, void* opaque);

  LEPUSRuntime* runtime_{nullptr};
  std::mutex mutex_;
  std::vector<base::closure> tasks_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_JS_INSPECT_QUICKJS_QUICKJS_INTERNAL_INSPECTOR_PRIMJS_INTERRUPT_HELPER_H_
