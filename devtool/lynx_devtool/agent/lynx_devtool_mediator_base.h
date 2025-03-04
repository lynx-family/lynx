// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_LYNX_DEVTOOL_MEDIATOR_BASE_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_LYNX_DEVTOOL_MEDIATOR_BASE_H_

#include <memory>

#include "base/include/fml/thread.h"
#include "base/include/log/logging.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"

namespace lynx {
namespace devtool {

// Why LynxDevToolMediator and LynxGlobalDevToolMediator?
// The life cycle of LynxGlobalDevToolMediator is associated with App,
// it is created once pre App. Each view contains a LynxDevToolMediator
// ,which is created after view opened. For the testbrench event that need to
// handle before view opened, LynxGlobalDevToolMediator is needed.

class LynxDevToolMediatorBase {
 public:
  LynxDevToolMediatorBase() {
    default_task_runner_ = GetDevToolsThread().GetTaskRunner();
  }
  virtual ~LynxDevToolMediatorBase() = default;

  static fml::Thread& GetDevToolsThread() {
    static base::NoDestructor<fml::Thread> devtools_thread(
        fml::Thread::ThreadConfig("devtool",
                                  fml::Thread::ThreadPriority::NORMAL));
    return *devtools_thread;
  }

  void RunOnTaskRunner(lynx::fml::RefPtr<lynx::fml::TaskRunner>& runner,
                       lynx::base::closure&& closure, bool run_now = true) {
    if (run_now) {
      lynx::fml::TaskRunner::RunNowOrPostTask(runner, std::move(closure));
    } else {
      CHECK_NULL_AND_LOG_RETURN(runner, "runner is null");
      runner->PostTask(std::move(closure));
    }
  }

 protected:
  fml::RefPtr<fml::TaskRunner> default_task_runner_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_LYNX_DEVTOOL_MEDIATOR_BASE_H_
