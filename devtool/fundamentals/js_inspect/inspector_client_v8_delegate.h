// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_FUNDAMENTALS_JS_INSPECT_INSPECTOR_CLIENT_V8_DELEGATE_H_
#define DEVTOOL_FUNDAMENTALS_JS_INSPECT_INSPECTOR_CLIENT_V8_DELEGATE_H_

#include <functional>

namespace lynx {
namespace devtool {

// Declare the interfaces only called by V8.
class InspectorClientV8Delegate {
 public:
  // Add timestamp to console messages.
  virtual double CurrentTimeMS() { return 0; }
  // Memory panel: Allocation instrumentation on timeline.
  virtual void StartRepeatingTimer(double interval,
                                   std::function<void(void*)> callback,
                                   void* data) {}
  virtual void CancelTimer(void* data) {}
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_FUNDAMENTALS_JS_INSPECT_INSPECTOR_CLIENT_V8_DELEGATE_H_
