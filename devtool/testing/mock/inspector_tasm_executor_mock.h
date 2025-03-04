// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_TESTING_MOCK_INSPECTOR_TASM_EXECUTOR_MOCK_H_
#define DEVTOOL_TESTING_MOCK_INSPECTOR_TASM_EXECUTOR_MOCK_H_

#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace testing {

class InspectorTasmExecutorMock : public lynx::devtool::InspectorTasmExecutor {
 public:
  InspectorTasmExecutorMock() : lynx::devtool::InspectorTasmExecutor(nullptr) {}
};

}  // namespace testing
}  // namespace lynx

#endif  // DEVTOOL_TESTING_MOCK_INSPECTOR_TASM_EXECUTOR_MOCK_H_
