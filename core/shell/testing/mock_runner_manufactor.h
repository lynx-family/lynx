// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_TESTING_MOCK_RUNNER_MANUFACTOR_H_
#define CORE_SHELL_TESTING_MOCK_RUNNER_MANUFACTOR_H_

#include "core/base/threading/task_runner_manufactor.h"

namespace lynx {
namespace shell {

class MockRunnerManufactor : public base::TaskRunnerManufactor {
 public:
  explicit MockRunnerManufactor(base::ThreadStrategyForRendering strategy);
  ~MockRunnerManufactor() override = default;

  static bool IsOnUIThread();

  static bool InOnTASMThread();

  static bool InOnLayoutThread();

  static bool InOnJSThread();

  static fml::RefPtr<fml::TaskRunner> GetHookUITaskRunner();

  static fml::RefPtr<fml::TaskRunner> GetHookTASMTaskRunner();

  static fml::RefPtr<fml::TaskRunner> GetHookLayoutTaskRunner();

  static fml::RefPtr<fml::TaskRunner> GetHookJsTaskRunner();

 private:
  void HookUIThread();

  void HookTASMThread();

  void HookLayoutThread();

  void HookJSThread();
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_TESTING_MOCK_RUNNER_MANUFACTOR_H_
