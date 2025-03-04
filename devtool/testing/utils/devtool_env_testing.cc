// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/testing/utils/devtool_env_testing.h"

namespace lynx {
namespace testing {

std::shared_ptr<ElementManagerMock>
LynxDevToolEnvTesting::SetUpElementManager() {
  return std::make_shared<ElementManagerMock>();
}

std::shared_ptr<InspectorTasmExecutorMock>
LynxDevToolEnvTesting::SetUpInspectorTasmExecutor() {
  return std::make_shared<InspectorTasmExecutorMock>();
}

std::shared_ptr<LynxDevToolMediatorMock>
LynxDevToolEnvTesting::SetUpLynxDevToolMediator() {
  return std::make_shared<LynxDevToolMediatorMock>();
}

}  // namespace testing
}  // namespace lynx
