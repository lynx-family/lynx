// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_LYNX_GLOBAL_DEVTOOL_MEDIATOR_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_LYNX_GLOBAL_DEVTOOL_MEDIATOR_H_

#include "core/base/threading/task_runner_manufactor.h"
#include "devtool/base_devtool/native/public/message_sender.h"
#include "devtool/lynx_devtool/agent/agent_defines.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator_base.h"

namespace lynx {
namespace devtool {

// Why LynxDevToolMediator and LynxGlobalDevToolMediator?
// The life cycle of LynxGlobalDevToolMediator is associated with App,
// it is created once pre App. Each view contains a LynxDevToolMediator
// ,which is created after view opened. For the testbrench event that need to
// handle before view opened, LynxGlobalDevToolMediator is needed.

class LynxGlobalDevToolMediator : public LynxDevToolMediatorBase {
 public:
  static LynxGlobalDevToolMediator& GetInstance();

  void EndReplayTest(
      const std::shared_ptr<lynx::devtool::MessageSender>& sender,
      const std::string& file_path);

  // Recording domain -> ui executor
  DECLARE_DEVTOOL_METHOD(RecordingStart)
  DECLARE_DEVTOOL_METHOD(RecordingEnd)

  // Replay domain -> ui executor
  DECLARE_DEVTOOL_METHOD(ReplayStart)
  DECLARE_DEVTOOL_METHOD(ReplayEnd)

  // IO domain -> devtool executor
  DECLARE_DEVTOOL_METHOD(IORead)
  DECLARE_DEVTOOL_METHOD(IOClose)

  // Memory domain -> devtools executor
  DECLARE_DEVTOOL_METHOD(MemoryStartTracing)
  DECLARE_DEVTOOL_METHOD(MemoryStopTracing)

  DECLARE_DEVTOOL_METHOD(TracingStart)
  DECLARE_DEVTOOL_METHOD(TracingEnd)
  DECLARE_DEVTOOL_METHOD(SetStartupTracingConfig)
  DECLARE_DEVTOOL_METHOD(GetStartupTracingConfig)
  DECLARE_DEVTOOL_METHOD(GetStartupTracingFile)

  // System Info domain
  DECLARE_DEVTOOL_METHOD(SystemInfoGetInfo)

 protected:
  fml::RefPtr<fml::TaskRunner> ui_task_runner_;
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
  int tracing_session_id_;
#endif

  friend class lynx::base::NoDestructor<LynxGlobalDevToolMediator>;
  LynxGlobalDevToolMediator();
  ~LynxGlobalDevToolMediator() = default;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_LYNX_GLOBAL_DEVTOOL_MEDIATOR_H_
