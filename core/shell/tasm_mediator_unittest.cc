// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/tasm_mediator.h"

#include "base/include/fml/task_runner.h"
#include "core/shell/native_facade_empty_implementation.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {
namespace {

class RecordingTaskRunner : public fml::TaskRunner {
 public:
  RecordingTaskRunner() : fml::TaskRunner(nullptr) {}

  bool RunsTasksOnCurrentThread() override { return true; }

  void PostTask(base::closure task) override {
    ++post_task_count_;
    task();
  }

  int post_task_count_{0};
};

TEST(TasmMediatorTest, TriggerLepusGlobalEventAsyncUsesPostTask) {
  auto runner = fml::MakeRefCounted<RecordingTaskRunner>();
  auto facade_runner = fml::MakeRefCounted<RecordingTaskRunner>();
  auto facade_actor = std::make_shared<LynxActor<NativeFacade>>(
      std::make_unique<NativeFacadeEmptyImpl>(), facade_runner);
  auto engine_actor = std::make_shared<LynxActor<LynxEngine>>(nullptr, runner);
  TasmMediator mediator(facade_actor, nullptr, nullptr, nullptr, nullptr);
  mediator.SetEngineActor(engine_actor);

  mediator.TriggerLepusGlobalEvent("frameLoaded", lepus::Value(), true);

  EXPECT_EQ(runner->post_task_count_, 1);
}

TEST(TasmMediatorTest, TriggerLepusGlobalEventSyncUsesActLite) {
  auto runner = fml::MakeRefCounted<RecordingTaskRunner>();
  auto facade_runner = fml::MakeRefCounted<RecordingTaskRunner>();
  auto facade_actor = std::make_shared<LynxActor<NativeFacade>>(
      std::make_unique<NativeFacadeEmptyImpl>(), facade_runner);
  auto engine_actor = std::make_shared<LynxActor<LynxEngine>>(nullptr, runner);
  TasmMediator mediator(facade_actor, nullptr, nullptr, nullptr, nullptr);
  mediator.SetEngineActor(engine_actor);

  mediator.TriggerLepusGlobalEvent("frameLoaded", lepus::Value(), false);

  EXPECT_EQ(runner->post_task_count_, 0);
}

}  // namespace
}  // namespace shell
}  // namespace lynx
