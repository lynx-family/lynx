// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/common/thread_host.h"

#if OS_ANDROID
#include <android/looper.h>
#endif

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace clay {

#if OS_ANDROID
static std::atomic_bool g_has_android_looper = false;
#endif

static std::shared_ptr<fml::Thread> g_clay_io_thread = nullptr;

std::string ThreadHost::ThreadHostConfig::MakeThreadName(
    Type type, const std::string& prefix) {
  switch (type) {
    case Type::Platform:
      return prefix + ".platform";
    case Type::UI:
      return prefix + ".ui";
    case Type::IO:
      return prefix + ".io";
    case Type::RASTER:
      return prefix + ".raster";
    case Type::Profiler:
      return prefix + ".profiler";
  }
}

void ThreadHost::ThreadHostConfig::SetIOConfig(const ThreadConfig& config) {
  type_mask |= ThreadHost::Type::IO;
  io_config = config;
}

void ThreadHost::ThreadHostConfig::SetUIConfig(const ThreadConfig& config) {
  type_mask |= ThreadHost::Type::UI;
  ui_config = config;
}

void ThreadHost::ThreadHostConfig::SetPlatformConfig(
    const ThreadConfig& config) {
  type_mask |= ThreadHost::Type::Platform;
  platform_config = config;
}

void ThreadHost::ThreadHostConfig::SetRasterConfig(const ThreadConfig& config) {
  type_mask |= ThreadHost::Type::RASTER;
  raster_config = config;
}

void ThreadHost::ThreadHostConfig::SetProfilerConfig(
    const ThreadConfig& config) {
  type_mask |= ThreadHost::Type::Profiler;
  profiler_config = config;
}

std::unique_ptr<fml::Thread> ThreadHost::CreateThread(
    Type type, std::optional<ThreadConfig> thread_config,
    const ThreadHostConfig& host_config) const {
  /// if not specified ThreadConfig, create a ThreadConfig.
  if (!thread_config.has_value()) {
    thread_config = ThreadConfig(
        ThreadHostConfig::MakeThreadName(type, host_config.name_prefix));
  }
  return std::make_unique<fml::Thread>(host_config.config_setter,
                                       *thread_config);
}

ThreadHost::ThreadHost() = default;

ThreadHost::ThreadHost(ThreadHost&&) = default;

ThreadHost::ThreadHost(const std::string& name_prefix, uint64_t mask)
    : ThreadHost(ThreadHostConfig(name_prefix, mask)) {}

ThreadHost::ThreadHost(const ThreadHostConfig& host_config)
    : name_prefix(host_config.name_prefix) {
  if (host_config.isThreadNeeded(ThreadHost::Type::Platform)) {
    platform_thread =
        CreateThread(Type::Platform, host_config.platform_config, host_config);
  }

  if (host_config.isThreadNeeded(ThreadHost::Type::UI)) {
    ui_thread = CreateThread(Type::UI, host_config.ui_config, host_config);
  }

  if (host_config.isThreadNeeded(ThreadHost::Type::RASTER)) {
    raster_thread =
        CreateThread(Type::RASTER, host_config.raster_config, host_config);
  }

  if (host_config.isThreadNeeded(ThreadHost::Type::IO)) {
    if (!g_clay_io_thread) {
      g_clay_io_thread =
          CreateThread(Type::IO, host_config.io_config, host_config);
    }
    io_thread = g_clay_io_thread;
  }

  if (host_config.isThreadNeeded(ThreadHost::Type::Profiler)) {
    profiler_thread =
        CreateThread(Type::Profiler, host_config.profiler_config, host_config);
  }
}

ThreadHost::~ThreadHost() = default;

// static
bool ThreadHost::HasALooperInRenderThread() {
#if OS_ANDROID
  return g_has_android_looper.load();
#else
  return false;
#endif
}
// static
void ThreadHost::CheckALooperInRenderThread() {
#if OS_ANDROID
  g_has_android_looper = ALooper_forThread() != nullptr;
#endif
}

void ThreadHost::UpdateConfig(const ThreadHostConfig& config) {
  if (ui_thread == nullptr && config.isThreadNeeded(Type::UI)) {
    ui_thread = CreateThread(Type::UI, config.ui_config, config);
  }
  if (platform_thread == nullptr && config.isThreadNeeded(Type::Platform)) {
    platform_thread =
        CreateThread(Type::Platform, config.platform_config, config);
  }
  if (io_thread == nullptr && config.isThreadNeeded(Type::IO)) {
    if (!g_clay_io_thread) {
      g_clay_io_thread = CreateThread(Type::IO, config.io_config, config);
    }
    io_thread = g_clay_io_thread;
  }
  if (raster_thread == nullptr && config.isThreadNeeded(Type::RASTER)) {
    raster_thread = CreateThread(Type::RASTER, config.raster_config, config);
  }
  if (profiler_thread == nullptr && config.isThreadNeeded(Type::Profiler)) {
    profiler_thread =
        CreateThread(Type::Profiler, config.profiler_config, config);
  }
}

}  // namespace clay
