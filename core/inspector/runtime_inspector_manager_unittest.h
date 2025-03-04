// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_INSPECTOR_RUNTIME_INSPECTOR_MANAGER_UNITTEST_H_
#define CORE_INSPECTOR_RUNTIME_INSPECTOR_MANAGER_UNITTEST_H_

#include <memory>

#include "core/inspector/runtime_inspector_manager.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace piper {
namespace testing {

class MockRuntimeInspectorManager : public RuntimeInspectorManager {
 public:
  MockRuntimeInspectorManager(int instance_id) { instance_id_ = instance_id; }

  void InitInspector(
      Runtime* runtime,
      const std::shared_ptr<InspectorRuntimeObserverNG>& observer) {}
  void DestroyInspector() {}
  void PrepareForScriptEval() {}
};

}  // namespace testing
}  // namespace piper
}  // namespace lynx

#endif  // CORE_INSPECTOR_RUNTIME_INSPECTOR_MANAGER_UNITTEST_H_
