// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/example/glfw/thread_host_holder.h"

#include <utility>

#include "base/include/fml/message_loop.h"
#include "base/include/fml/thread.h"
#include "clay/fml/logging.h"

namespace clay {
namespace example {

namespace {
constexpr const char* kThreadName = "clay.demo";

fml::Thread::ThreadConfig MakeThreadConfig(
    clay::ThreadHost::Type type, fml::Thread::ThreadPriority priority) {
  return fml::Thread::ThreadConfig(
      clay::ThreadHost::ThreadHostConfig::MakeThreadName(type, kThreadName),
      priority);
}

fml::RefPtr<fml::TaskRunner> GetCurrentThreadTaskRunner() {
  fml::MessageLoop::EnsureInitializedForCurrentThread();
  return fml::MessageLoop::GetCurrent().GetTaskRunner();
}

static std::pair<bool, fml::RefPtr<CustomTaskRunner>> CreateCustomTaskRunner(
    const ClayTaskRunnerDescription* description) {
  if (description == nullptr) {
    return {true, {}};
  }

  if (description->runs_task_on_current_thread_callback == nullptr) {
    FML_LOG(ERROR) << "ClayTaskRunnerDescription.runs_task_on_current_thread_"
                      "callback was nullptr.";
    return {false, {}};
  }

  if (description->post_task_callback == nullptr) {
    FML_LOG(ERROR)
        << "ClayTaskRunnerDescription.post_task_callback was nullptr.";
    return {false, {}};
  }

  auto user_data = description->user_data;

  // ABI safety checks have been completed.
  auto post_task_callback_c = description->post_task_callback;
  auto runs_task_on_current_thread_callback_c =
      description->runs_task_on_current_thread_callback;

  CustomTaskRunner::DispatchTable task_runner_dispatch_table = {
      // .post_task_callback
      [post_task_callback_c, user_data](CustomTaskRunner* task_runner,
                                        uint64_t task_baton,
                                        fml::TimePoint target_time) -> void {
        ClayTask task = {
            // runner
            reinterpret_cast<ClayTaskRunner>(task_runner),
            // task
            task_baton,
        };
        post_task_callback_c(task, target_time.ToEpochDelta().ToNanoseconds(),
                             user_data);
      },
      // runs_task_on_current_thread_callback
      [runs_task_on_current_thread_callback_c, user_data]() -> bool {
        return runs_task_on_current_thread_callback_c(user_data);
      }};

  return {true, fml::MakeRefCounted<CustomTaskRunner>(
                    task_runner_dispatch_table, description->identifier)};
}

}  // namespace

// static
std::unique_ptr<ThreadHostHolder> ThreadHostHolder::CreateThreadHostHolder(
    ClayTaskRunnerDescription* platform_task_runner_desc) {
  auto thread_host_config = ThreadHost::ThreadHostConfig();
  thread_host_config.SetRasterConfig(MakeThreadConfig(
      clay::ThreadHost::RASTER, fml::Thread::ThreadPriority::HIGH));
  thread_host_config.SetIOConfig(MakeThreadConfig(
      clay::ThreadHost::IO, fml::Thread::ThreadPriority::BACKGROUND));
  ThreadHost thread_host(thread_host_config);
  auto platform_task_runner_pair =
      CreateCustomTaskRunner(platform_task_runner_desc);
  if (!platform_task_runner_pair.first) {
    return nullptr;
  }
  auto platform_task_runner = platform_task_runner_pair.second
                                  ? static_cast<fml::RefPtr<fml::TaskRunner>>(
                                        platform_task_runner_pair.second)
                                  : GetCurrentThreadTaskRunner();
  auto ui_task_runner = platform_task_runner;

  clay::TaskRunners task_runners(
      kThreadName,
      platform_task_runner,                        // platform
      thread_host.raster_thread->GetTaskRunner(),  // raster
      ui_task_runner, thread_host.io_thread->GetTaskRunner());
  if (!task_runners.IsValid()) {
    return nullptr;
  }
  return std::make_unique<ThreadHostHolder>(std::move(thread_host),
                                            std::move(task_runners),
                                            platform_task_runner_pair.second);
}

ThreadHostHolder::ThreadHostHolder(
    clay::ThreadHost thread_host, const clay::TaskRunners& runners,
    fml::RefPtr<CustomTaskRunner> platform_task_runner)
    : thread_host_(std::move(thread_host)),
      runners_(runners),
      platform_task_runner_(platform_task_runner) {}

ThreadHostHolder::~ThreadHostHolder() {
  thread_host_.ui_thread.reset();
  thread_host_.raster_thread.reset();
  if (platform_task_runner_) {
    platform_task_runner_->PostSyncTask(
        [platform_task_runner = platform_task_runner_.get()]() {
          platform_task_runner->OnThreadHostDestroyed();
        });
  }
}

bool ThreadHostHolder::PostTask(uint64_t task) const {
  return platform_task_runner_ && platform_task_runner_->PostTask(task);
}

}  // namespace example
}  // namespace clay
