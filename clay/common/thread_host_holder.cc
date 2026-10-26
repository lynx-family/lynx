// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/common/thread_host_holder.h"

#include <memory>

namespace clay {

ThreadHostHolder& ThreadHostHolder::Instance() {
  static ThreadHostHolder holder;
  return holder;
}

ThreadHostHolder::ThreadHostHolder() = default;

void ThreadHostHolder::SetConfig(
    const clay::ThreadHost::ThreadHostConfig& config) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (thread_host_ == nullptr) {
    thread_host_ = std::make_shared<ThreadHost>(config);
    return;
  }
  thread_host_->UpdateConfig(config);
}

void ThreadHostHolder::SetMask(uint64_t mask, const std::string& thread_label,
                               const ThreadConfigSetter& setter) {
  std::string name_prefix = "clay" + thread_label;
  clay::ThreadHost::ThreadHostConfig host_config(name_prefix, mask, setter);

  if (mask & ThreadHost::Type::Platform) {
    host_config.platform_config = fml::Thread::ThreadConfig(
        clay::ThreadHost::ThreadHostConfig::MakeThreadName(
            clay::ThreadHost::Type::Platform, name_prefix),
        fml::Thread::ThreadPriority::LOW);
  }

  if (mask & ThreadHost::Type::UI) {
    host_config.ui_config = fml::Thread::ThreadConfig(
        clay::ThreadHost::ThreadHostConfig::MakeThreadName(
            clay::ThreadHost::Type::UI, name_prefix),
        fml::Thread::ThreadPriority::NORMAL);
  }

  if (mask & ThreadHost::Type::RASTER) {
    host_config.raster_config = fml::Thread::ThreadConfig(
        clay::ThreadHost::ThreadHostConfig::MakeThreadName(
            clay::ThreadHost::Type::RASTER, name_prefix),
        fml::Thread::ThreadPriority::HIGH);
  }

  if (mask & ThreadHost::Type::IO) {
    host_config.io_config = fml::Thread::ThreadConfig(
        clay::ThreadHost::ThreadHostConfig::MakeThreadName(
            clay::ThreadHost::Type::IO, name_prefix),
        fml::Thread::ThreadPriority::LOW);
  }

  if (mask & ThreadHost::Type::Profiler) {
    host_config.profiler_config = fml::Thread::ThreadConfig(
        clay::ThreadHost::ThreadHostConfig::MakeThreadName(
            clay::ThreadHost::Type::Profiler, name_prefix),
        fml::Thread::ThreadPriority::LOW);
  }
  SetConfig(host_config);
}

const std::shared_ptr<ThreadHost> ThreadHostHolder::GetThreadHost() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return thread_host_;
}

fml::RefPtr<fml::TaskRunner> ThreadHostHolder::GetTaskRunner(
    ThreadHost::Type type) const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!thread_host_) {
    return nullptr;
  }
  fml::Thread* thread = nullptr;
  switch (type) {
    case ThreadHost::Platform:
      thread = thread_host_->platform_thread.get();
      break;
    case ThreadHost::UI:
      thread = thread_host_->ui_thread.get();
      break;
    case ThreadHost::RASTER:
      thread = thread_host_->raster_thread.get();
      break;
    case ThreadHost::IO:
      thread = thread_host_->io_thread.get();
      break;
    case ThreadHost::Profiler:
      thread = thread_host_->profiler_thread.get();
      break;
  }
  return thread ? thread->GetTaskRunner() : nullptr;
}

}  // namespace clay
