// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/common/thread_host.h"
#define FML_USED_ON_EMBEDDER

#include <algorithm>
#include <utility>

#include "base/include/fml/message_loop.h"
#include "clay/fml/logging.h"
#include "clay/shell/platform/embedder/embedder_struct_macros.h"
#include "clay/shell/platform/embedder/embedder_thread_host.h"

namespace clay {

//------------------------------------------------------------------------------
/// @brief      Attempts to create a task runner from an embedder task runner
///             description. The first boolean in the pair indicate whether the
///             embedder specified an invalid task runner description. In this
///             case, engine launch must be aborted. If the embedder did not
///             specify any task runner, an engine managed task runner and
///             thread must be selected instead.
///
/// @param[in]  description  The description
///
/// @return     A pair that returns if the embedder has specified a task runner
///             (null otherwise) and whether to terminate further engine launch.
///
static std::pair<bool, fml::RefPtr<EmbedderTaskRunner>>
CreateEmbedderTaskRunner(const ClayTaskRunnerDescription* description) {
  if (description == nullptr) {
    // This is not embedder error. The embedder API will just have to create a
    // plain old task runner (and create a thread for it) instead of using a
    // task runner provided to us by the embedder.
    return {true, {}};
  }

  if (SAFE_ACCESS(description, runs_task_on_current_thread_callback, nullptr) ==
      nullptr) {
    FML_LOG(ERROR) << "ClayTaskRunnerDescription.runs_task_on_current_"
                      "thread_callback was nullptr.";
    return {false, {}};
  }

  if (SAFE_ACCESS(description, post_task_callback, nullptr) == nullptr) {
    FML_LOG(ERROR)
        << "ClayTaskRunnerDescription.post_task_callback was nullptr.";
    return {false, {}};
  }

  auto user_data = SAFE_ACCESS(description, user_data, nullptr);

  // ABI safety checks have been completed.
  auto post_task_callback_c = description->post_task_callback;
  auto runs_task_on_current_thread_callback_c =
      description->runs_task_on_current_thread_callback;

  EmbedderTaskRunner::DispatchTable task_runner_dispatch_table = {
      // .post_task_callback
      [post_task_callback_c, user_data](EmbedderTaskRunner* task_runner,
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

  return {true, fml::MakeRefCounted<EmbedderTaskRunner>(
                    task_runner_dispatch_table,
                    SAFE_ACCESS(description, identifier, 0u))};
}

std::unique_ptr<EmbedderThreadHost>
EmbedderThreadHost::CreateEmbedderOrEngineManagedThreadHost(
    const ClayCustomTaskRunners* custom_task_runners,
    const clay::ThreadConfigSetter& config_setter) {
  {
    auto host =
        CreateEmbedderManagedThreadHost(custom_task_runners, config_setter);
    if (host && host->IsValid()) {
      return host;
    }
  }

  // Only attempt to create the engine managed host if the embedder did not
  // specify a custom configuration. Don't fallback to the engine managed
  // configuration if the embedder attempted to specify a configuration but
  // messed up with an incorrect configuration.
  if (custom_task_runners == nullptr) {
    auto host = CreateEngineManagedThreadHost(config_setter);
    if (host && host->IsValid()) {
      return host;
    }
  }

  return nullptr;
}

static fml::RefPtr<fml::TaskRunner> GetCurrentThreadTaskRunner() {
  fml::MessageLoop::EnsureInitializedForCurrentThread();
  return fml::MessageLoop::GetCurrent().GetTaskRunner();
}

constexpr const char* kThreadNamePrefix = "lynx.clay";

fml::Thread::ThreadConfig MakeThreadConfig(
    clay::ThreadHost::Type type, fml::Thread::ThreadPriority priority) {
  return fml::Thread::ThreadConfig(
      clay::ThreadHost::ThreadHostConfig::MakeThreadName(type,
                                                         kThreadNamePrefix),
      priority);
}

// static
std::unique_ptr<EmbedderThreadHost>
EmbedderThreadHost::CreateEmbedderManagedThreadHost(
    const ClayCustomTaskRunners* custom_task_runners,
    const clay::ThreadConfigSetter& config_setter) {
  if (custom_task_runners == nullptr) {
    return nullptr;
  }

  auto thread_host_config = ThreadHost::ThreadHostConfig(config_setter);

  thread_host_config.SetIOConfig(MakeThreadConfig(
      ThreadHost::Type::IO, fml::Thread::ThreadPriority::BACKGROUND));

  auto platform_task_runner_pair = CreateEmbedderTaskRunner(
      SAFE_ACCESS(custom_task_runners, platform_task_runner, nullptr));
  auto render_task_runner_pair = CreateEmbedderTaskRunner(
      SAFE_ACCESS(custom_task_runners, render_task_runner, nullptr));

  if (!platform_task_runner_pair.first || !render_task_runner_pair.first) {
    // User error while supplying a custom task runner. Return an invalid thread
    // host. This will abort engine initialization. Don't fallback to defaults
    // if the user wanted to specify a task runner but just messed up instead.
    return nullptr;
  }

  // If the embedder has not supplied a raster task runner, one needs to be
  // created.
  if (!render_task_runner_pair.second) {
    thread_host_config.SetRasterConfig(MakeThreadConfig(
        ThreadHost::Type::RASTER, fml::Thread::ThreadPriority::HIGH));
  }

  // If both the platform task runner and the raster task runner are specified
  // and have the same identifier, store only one.
  if (platform_task_runner_pair.second && render_task_runner_pair.second) {
    if (platform_task_runner_pair.second->GetEmbedderIdentifier() ==
        render_task_runner_pair.second->GetEmbedderIdentifier()) {
      render_task_runner_pair.second = platform_task_runner_pair.second;
    }
  }

  // Create a thread host with just the threads that need to be managed by the
  // engine. The embedder has provided the rest.
  ThreadHost thread_host(thread_host_config);

  // If the embedder has supplied a platform task runner, use that. If not, use
  // the current thread task runner.
  auto platform_task_runner = platform_task_runner_pair.second
                                  ? static_cast<fml::RefPtr<fml::TaskRunner>>(
                                        platform_task_runner_pair.second)
                                  : GetCurrentThreadTaskRunner();

  auto ui_task_runner = platform_task_runner;

  // If the embedder has supplied a raster task runner, use that. If not, use
  // the one from our thread host.
  auto render_task_runner = render_task_runner_pair.second
                                ? static_cast<fml::RefPtr<fml::TaskRunner>>(
                                      render_task_runner_pair.second)
                                : thread_host.raster_thread->GetTaskRunner();

  clay::TaskRunners task_runners(
      kThreadNamePrefix,
      platform_task_runner,  // platform
      render_task_runner,    // raster
      ui_task_runner,
      thread_host.io_thread->GetTaskRunner());  // io (always engine managed)

  if (!task_runners.IsValid()) {
    return nullptr;
  }

  auto embedder_host = std::make_unique<EmbedderThreadHost>(
      std::move(thread_host), std::move(task_runners),
      platform_task_runner_pair.second, render_task_runner_pair.second);

  if (embedder_host->IsValid()) {
    return embedder_host;
  }

  return nullptr;
}

// static
std::unique_ptr<EmbedderThreadHost>
EmbedderThreadHost::CreateEngineManagedThreadHost(
    const clay::ThreadConfigSetter& config_setter) {
  // Crate a thraed host config, and specified the thread name and priority.
  auto thread_host_config = ThreadHost::ThreadHostConfig(config_setter);
  thread_host_config.SetRasterConfig(MakeThreadConfig(
      clay::ThreadHost::RASTER, fml::Thread::ThreadPriority::HIGH));
  thread_host_config.SetIOConfig(MakeThreadConfig(
      clay::ThreadHost::IO, fml::Thread::ThreadPriority::BACKGROUND));

  // Create a thread host with the current thread as the platform thread and all
  // other threads managed.
  ThreadHost thread_host(thread_host_config);

  // For embedder platforms that don't have native message loop interop, this
  // will reference a task runner that points to a null message loop
  // implementation.
  auto platform_task_runner = GetCurrentThreadTaskRunner();
  auto ui_task_runner = platform_task_runner;

  clay::TaskRunners task_runners(
      kThreadNamePrefix,
      platform_task_runner,                        // platform
      thread_host.raster_thread->GetTaskRunner(),  // raster
      ui_task_runner,                              // ui
      thread_host.io_thread->GetTaskRunner());     // io

  if (!task_runners.IsValid()) {
    return nullptr;
  }

  auto embedder_host = std::make_unique<EmbedderThreadHost>(
      std::move(thread_host), std::move(task_runners), nullptr, nullptr);

  if (embedder_host->IsValid()) {
    return embedder_host;
  }

  return nullptr;
}

EmbedderThreadHost::EmbedderThreadHost(
    ThreadHost thread_host, const clay::TaskRunners& runners,
    fml::RefPtr<EmbedderTaskRunner> platform_task_runner,
    fml::RefPtr<EmbedderTaskRunner> render_task_runner)
    : thread_host_(std::move(thread_host)),
      runners_(runners),
      platform_task_runner_(platform_task_runner),
      render_task_runner_(render_task_runner) {
  if (platform_task_runner) {
    runners_map_[reinterpret_cast<int64_t>(platform_task_runner.get())] =
        platform_task_runner;
  }
  if (render_task_runner) {
    runners_map_[reinterpret_cast<int64_t>(render_task_runner.get())] =
        render_task_runner;
  }
}

EmbedderThreadHost::~EmbedderThreadHost() {
  // Make sure these threads's pending tasks are executed before the thread host
  // is destroyed.
  thread_host_.ui_thread.reset();
  if (render_task_runner_) {
    render_task_runner_->PostTask(
        [render_task_runner = render_task_runner_.get()]() {
          render_task_runner->OnThreadHostDestroyed();
        });
  }
  thread_host_.raster_thread.reset();
  if (platform_task_runner_) {
    platform_task_runner_->PostSyncTask(
        [platform_task_runner = platform_task_runner_.get()]() {
          platform_task_runner->OnThreadHostDestroyed();
        });
  }
}

bool EmbedderThreadHost::IsValid() const { return runners_.IsValid(); }

const clay::TaskRunners& EmbedderThreadHost::GetTaskRunners() const {
  return runners_;
}

bool EmbedderThreadHost::PostTask(int64_t runner, uint64_t task) const {
  auto found = runners_map_.find(runner);
  if (found == runners_map_.end()) {
    return false;
  }
  return found->second->PostTask(task);
}

}  // namespace clay
