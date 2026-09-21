// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/android/runtime/process_runtime_android.h"

#include <utility>

#include "base/include/log/logging.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/utils/devtool_lifecycle.h"

namespace lynx {
namespace shell {

bool InitializeHostScriptRuntime(ProcessRuntime::Completion ready,
                                 std::string bootstrap) {
  if (!tasm::DevToolLifecycle::GetInstance().IsEnabled()) return false;
  base::TaskRunnerManufactor runners(base::MOST_ON_TASM, false, false);
  DCHECK(runners.GetUITaskRunner()->RunsTasksOnCurrentThread());
  return ProcessRuntime::GetInstance().Initialize(
      {runners.GetJSTaskRunner(), runners.GetTASMTaskRunner(),
       runners.GetUITaskRunner()},
      std::move(ready), {}, std::move(bootstrap));
}

}  // namespace shell
}  // namespace lynx
