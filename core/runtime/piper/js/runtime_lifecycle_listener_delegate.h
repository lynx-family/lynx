// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_PIPER_JS_RUNTIME_LIFECYCLE_LISTENER_DELEGATE_H_
#define CORE_RUNTIME_PIPER_JS_RUNTIME_LIFECYCLE_LISTENER_DELEGATE_H_

#include <memory>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/task_runner.h"
#include "core/public/vsync_observer_interface.h"
#include "lynx/core/public/runtime_lifecycle_observer.h"

namespace lynx {
namespace runtime {
class RuntimeLifecycleListenerDelegate : public RuntimeLifecycleObserver {
 public:
  enum DelegateType { PART, FULL };
  explicit RuntimeLifecycleListenerDelegate(DelegateType type) : type_(type) {}
  ~RuntimeLifecycleListenerDelegate() override = default;

  DelegateType Type() { return type_; }

 private:
  DelegateType type_;
};

}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_PIPER_JS_RUNTIME_LIFECYCLE_LISTENER_DELEGATE_H_
