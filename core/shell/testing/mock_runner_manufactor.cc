// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// use for hook
#define private protected

#include "core/shell/testing/mock_runner_manufactor.h"

#include <memory>

#include "base/include/fml/memory/task_runner_checker.h"
#include "base/include/fml/message_loop.h"

namespace lynx {
namespace shell {

MockRunnerManufactor::MockRunnerManufactor(
    base::ThreadStrategyForRendering strategy)
    // force base class not create thread
    : base::TaskRunnerManufactor(base::ThreadStrategyForRendering::ALL_ON_UI,
                                 true, true) {
  HookUIThread();
  HookTASMThread();
  HookLayoutThread();
  HookJSThread();
}

bool MockRunnerManufactor::IsOnUIThread() {
  return fml::TaskRunnerChecker::RunsOnTheSameThread(
      GetHookUITaskRunner()->GetTaskQueueId(),
      lynx::fml::MessageLoop::GetCurrentTaskQueueId());
}

bool MockRunnerManufactor::InOnTASMThread() {
  return fml::TaskRunnerChecker::RunsOnTheSameThread(
      GetHookTASMTaskRunner()->GetTaskQueueId(),
      lynx::fml::MessageLoop::GetCurrentTaskQueueId());
}

bool MockRunnerManufactor::InOnLayoutThread() {
  return fml::TaskRunnerChecker::RunsOnTheSameThread(
      GetHookLayoutTaskRunner()->GetTaskQueueId(),
      lynx::fml::MessageLoop::GetCurrentTaskQueueId());
}

bool MockRunnerManufactor::InOnJSThread() {
  return fml::TaskRunnerChecker::RunsOnTheSameThread(
      GetHookJsTaskRunner()->GetTaskQueueId(),
      lynx::fml::MessageLoop::GetCurrentTaskQueueId());
}

void MockRunnerManufactor::HookUIThread() {
  ui_task_runner_ = GetHookUITaskRunner();
}

void MockRunnerManufactor::HookTASMThread() {
  tasm_task_runner_ = GetHookTASMTaskRunner();
}

void MockRunnerManufactor::HookLayoutThread() {
  layout_task_runner_ = GetHookLayoutTaskRunner();
}

void MockRunnerManufactor::HookJSThread() {
  js_task_runner_ = GetHookJsTaskRunner();
}

fml::RefPtr<fml::TaskRunner> MockRunnerManufactor::GetHookUITaskRunner() {
  static base::NoDestructor<fml::Thread> ui_thread("Lynx_MOCK_UI");
  auto runner = ui_thread->GetTaskRunner();
  // hook global ui thread runner
  base::UIThread::GetRunner() = runner;
  return runner;
}

fml::RefPtr<fml::TaskRunner> MockRunnerManufactor::GetHookTASMTaskRunner() {
  static base::NoDestructor<fml::Thread> tasm_thread("Lynx_MOCK_TASM");
  return tasm_thread->GetTaskRunner();
}

fml::RefPtr<fml::TaskRunner> MockRunnerManufactor::GetHookLayoutTaskRunner() {
  static base::NoDestructor<fml::Thread> layout_thread("Lynx_MOCK_LAYOUT");
  return layout_thread->GetTaskRunner();
}

fml::RefPtr<fml::TaskRunner> MockRunnerManufactor::GetHookJsTaskRunner() {
  static base::NoDestructor<fml::Thread> js_thread("Lynx_JS");
  return js_thread->GetTaskRunner();
}

}  // namespace shell
}  // namespace lynx
