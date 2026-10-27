// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_TEST_SUPPORT_H_
#define CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_TEST_SUPPORT_H_
#include <deque>
#include <memory>
#include <utility>

#include "base/include/fml/message_loop_impl.h"
#include "base/include/fml/task_runner.h"
#include "clay/shell/common/display_compositor/frame_mailbox.h"
namespace clay::display::testing_support {
class Signal : public GPUCompletion {
 public:
  CompletionStatus Poll() const override { return status; }
  bool IsSafeToRelease() const override {
    return status != CompletionStatus::kPending;
  }
  CompletionStatus status = CompletionStatus::kComplete;
};
class ManualRunner : public fml::TaskRunner {
 public:
  ManualRunner() : TaskRunner(nullptr) {}
  void PostTask(lynx::base::closure task) override {
    tasks.push_back(std::move(task));
  }
  void PostDelayedTask(lynx::base::closure task, fml::TimeDelta) override {
    polls.push_back(std::move(task));
  }
  bool RunsTasksOnCurrentThread() override { return true; }
  void Run() {
    while (!tasks.empty()) {
      auto task = std::move(tasks.front());
      tasks.pop_front();
      task();
    }
  }
  void Poll() {
    if (!polls.empty()) {
      auto task = std::move(polls.front());
      polls.pop_front();
      task();
    }
  }
  std::deque<lynx::base::closure> tasks, polls;
};
inline RasterBindingKey Binding() { return {1, 1, 1, 1, 0}; }
inline OutputConfig Config() {
  OutputConfig c;
  c.width = 100;
  c.height = 200;
  c.device_id = 1;
  c.device_generation = 1;
  c.version = 1;
  return c;
}
inline std::shared_ptr<ResourceRecord> Record(
    std::shared_ptr<Signal> signal = std::make_shared<Signal>()) {
  ImageSpec image;
  image.width = 100;
  image.height = 200;
  image.valid_rect = {0, 0, 100, 200};
  auto record = std::make_shared<ResourceRecord>(
      ResourceKey{1, 1, 1, 1}, image, 80000, std::make_shared<int>(1));
  record->BeginWrite();
  record->EndWrite(signal);
  return record;
}
inline RasterPublication Publication(
    uint64_t render = 1, uint64_t layout = 1,
    std::shared_ptr<Signal> signal = std::make_shared<Signal>()) {
  auto scene = std::make_shared<PageSceneSnapshot>();
  scene->binding = Binding();
  scene->config = Config();
  scene->layout_version = layout;
  scene->segment_plan_version = layout;
  scene->ordered_segments.push_back({1, {}});
  auto result = std::make_shared<RasterResult>();
  result->binding = scene->binding;
  result->config = scene->config;
  result->layout_version = layout;
  result->segment_plan_version = layout;
  result->render_version = render;
  auto texture = std::make_shared<OutputTexture>(Record(signal), render);
  result->outputs.push_back({1, render, texture, {}, std::nullopt});
  return {scene, result};
}
}  // namespace clay::display::testing_support
#endif  // CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_TEST_SUPPORT_H_
