// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_TESTING_UTILS_DEVTOOL_ENV_TESTING_H_
#define DEVTOOL_TESTING_UTILS_DEVTOOL_ENV_TESTING_H_

#include <memory>
#include <set>

#include "devtool/testing/mock/element_manager_mock.h"
#include "devtool/testing/mock/inspector_tasm_executor_mock.h"
#include "devtool/testing/mock/lynx_devtool_mediator_mock.h"

namespace lynx {
namespace testing {

class LynxDevToolEnvTesting {
 public:
  static std::shared_ptr<ElementManagerMock> SetUpElementManager();

  static std::shared_ptr<InspectorTasmExecutorMock>
  SetUpInspectorTasmExecutor();

  static std::shared_ptr<LynxDevToolMediatorMock> SetUpLynxDevToolMediator();
};

}  // namespace testing
}  // namespace lynx

#endif  // DEVTOOL_TESTING_UTILS_DEVTOOL_ENV_TESTING_H_
