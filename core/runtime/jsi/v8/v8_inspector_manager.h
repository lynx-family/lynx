// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSI_V8_V8_INSPECTOR_MANAGER_H_
#define CORE_RUNTIME_JSI_V8_V8_INSPECTOR_MANAGER_H_

#include "core/inspector/runtime_inspector_manager.h"

namespace lynx {
namespace piper {

class V8InspectorManager : public RuntimeInspectorManager {
 public:
  ~V8InspectorManager() override = default;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JSI_V8_V8_INSPECTOR_MANAGER_H_
