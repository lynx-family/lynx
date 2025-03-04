// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_INSPECTOR_OBSERVER_INSPECTOR_RUNTIME_OBSERVER_NG_H_
#define CORE_INSPECTOR_OBSERVER_INSPECTOR_RUNTIME_OBSERVER_NG_H_

#include <memory>
#include <string>

#include "core/inspector/console_message_postman.h"
#include "core/inspector/runtime_inspector_manager.h"

namespace lynx {
namespace runtime {
class RuntimeManagerDelegate;
}

namespace devtool {
class InspectorClientNG;
}
namespace piper {

// Only works for js runtime.
// Create some instances which implemented in LynxDevtool and observe the js
// runtime.
class InspectorRuntimeObserverNG {
 public:
  virtual ~InspectorRuntimeObserverNG() = default;

  virtual int GetViewId() { return -1; }

  // The following functions are used to create some instances which implemented
  // in LynxDevtool.
  virtual std::unique_ptr<runtime::RuntimeManagerDelegate>
  CreateRuntimeManagerDelegate() {
    return nullptr;
  }
  virtual std::unique_ptr<RuntimeInspectorManager>
  CreateRuntimeInspectorManager(const std::string& vm_type) {
    return nullptr;
  }
  virtual std::shared_ptr<ConsoleMessagePostMan> CreateConsoleMessagePostMan() {
    return nullptr;
  }

  // The following functions are used to observe the js runtime and notify
  // LynxDevtool.
  virtual void OnInspectorInited(
      const std::string& vm_type, int64_t runtime_id,
      const std::string& group_id, bool single_group,
      const std::shared_ptr<devtool::InspectorClientNG>& client) = 0;
  virtual void OnRuntimeDestroyed(int64_t runtime_id) = 0;

  virtual void PrepareForScriptEval() = 0;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_INSPECTOR_OBSERVER_INSPECTOR_RUNTIME_OBSERVER_NG_H_
