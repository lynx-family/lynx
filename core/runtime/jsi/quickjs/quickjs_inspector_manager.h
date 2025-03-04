// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSI_QUICKJS_QUICKJS_INSPECTOR_MANAGER_H_
#define CORE_RUNTIME_JSI_QUICKJS_QUICKJS_INSPECTOR_MANAGER_H_

#include <string>

#include "core/inspector/runtime_inspector_manager.h"

namespace lynx {
namespace piper {

class QuickjsInspectorManager : public RuntimeInspectorManager {
 public:
  ~QuickjsInspectorManager() override = default;

  virtual void InsertScript(const std::string& url) = 0;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JSI_QUICKJS_QUICKJS_INSPECTOR_MANAGER_H_
