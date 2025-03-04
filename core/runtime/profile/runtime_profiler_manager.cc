// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/profile/runtime_profiler_manager.h"

#include "base/include/no_destructor.h"

#if ENABLE_TRACE_PERFETTO
#include <memory>
#include <string>
#include <utility>

#include "base/include/log/logging.h"
#include "base/trace/native/trace_event_utils_perfetto.h"
#include "base/trace/native/track_event_wrapper.h"
#endif

namespace lynx {
namespace profile {

BASE_EXPORT lynx::trace::TracePlugin* GetRuntimeProfilerManager() {
  return RuntimeProfilerManager::GetInstance();
}

RuntimeProfilerManager* RuntimeProfilerManager::GetInstance() {
  static base::NoDestructor<RuntimeProfilerManager> sRuntimeProfilerManager;
  return sRuntimeProfilerManager.get();
}

#if ENABLE_TRACE_PERFETTO
void RuntimeProfilerManager::AddRuntimeProfiler(
    std::shared_ptr<RuntimeProfiler> runtime_profiler) {
  if (runtime_profiler != nullptr) {
    auto track_id = lynx::perfetto::ThreadTrack::Current();
    runtime_profiler->SetTrackId(track_id);
    std::lock_guard<std::mutex> lock(lock_);
    runtime_profilers_.emplace_back(runtime_profiler);
    if (is_started_ && runtime_profiler->GetType() == js_profiler_type_) {
      runtime_profiler->SetupProfiling(js_profile_interval_);
      runtime_profiler->StartProfiling(true);
    }
  }
}

void RuntimeProfilerManager::RemoveRuntimeProfiler(
    std::shared_ptr<RuntimeProfiler> runtime_profiler) {
  std::lock_guard<std::mutex> lock(lock_);
  auto it = std::find(runtime_profilers_.begin(), runtime_profilers_.end(),
                      runtime_profiler);
  if (it != runtime_profilers_.end()) {
    if (is_started_ && runtime_profiler->GetType() == js_profiler_type_) {
      auto profile = runtime_profiler->StopProfiling(true);
      if (profile != nullptr) {
        profiles_.emplace_back(std::move(profile));
      }
    }
    runtime_profilers_.erase(it);
  }
}

void RuntimeProfilerManager::SaveRuntimeProfile(
    const std::shared_ptr<RuntimeProfile>& runtime_profile,
    const int32_t index) {
  trace::TraceRuntimeProfile(runtime_profile->runtime_profile_,
                             runtime_profile->track_id_, index);
}
#endif

void RuntimeProfilerManager::DispatchBegin() {
#if ENABLE_TRACE_PERFETTO
  std::lock_guard<std::mutex> lock(lock_);
  is_started_ = true;
  profiles_.clear();
  for (const auto& runtime_profiler : runtime_profilers_) {
    if (runtime_profiler->GetType() == js_profiler_type_) {
      runtime_profiler->StartProfiling(false);
    }
  }
  LOGI("RuntimeProfilerManager::DispatchBegin");
#endif
}

void RuntimeProfilerManager::DispatchEnd() {
#if ENABLE_TRACE_PERFETTO
  std::lock_guard<std::mutex> lock(lock_);
  is_started_ = false;
  for (const auto& runtime_profiler : runtime_profilers_) {
    if (runtime_profiler->GetType() == js_profiler_type_) {
      auto profile = runtime_profiler->StopProfiling(false);
      if (profile != nullptr) {
        profiles_.emplace_back(std::move(profile));
      }
    }
  }
  for (size_t i = 0; i < profiles_.size(); i++) {
    const auto& profile = profiles_[i];
    SaveRuntimeProfile(profile, static_cast<uint32_t>(i));
  }
  // clear profile data
  profiles_.clear();
  LOGI("RuntimeProfilerManager::DispatchEnd");
#endif
}

void RuntimeProfilerManager::DispatchSetup(
    const std::shared_ptr<lynx::trace::TraceConfig>& config) {
#if ENABLE_TRACE_PERFETTO
  std::lock_guard<std::mutex> lock(lock_);
  js_profile_interval_ = config->js_profile_interval;
  js_profiler_type_ = config->js_profile_type;
  for (const auto& runtime_profiler : runtime_profilers_) {
    if (runtime_profiler->GetType() == js_profiler_type_) {
      runtime_profiler->SetupProfiling(js_profile_interval_);
    }
  }
#endif
}

std::string RuntimeProfilerManager::Name() {
  static std::string name = "runtime_profiler";
  return name;
}
}  // namespace profile
}  // namespace lynx
