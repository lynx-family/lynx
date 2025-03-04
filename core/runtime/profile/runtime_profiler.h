// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_PROFILE_RUNTIME_PROFILER_H_
#define CORE_RUNTIME_PROFILE_RUNTIME_PROFILER_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/include/base_export.h"
#include "base/include/fml/message_loop.h"
#include "base/trace/native/trace_controller.h"

namespace lynx {
namespace profile {

struct BASE_EXPORT RuntimeProfile {
  std::string runtime_profile_;
  uint64_t track_id_;

  RuntimeProfile(const std::string& runtime_profile, const uint64_t track_id)
      : runtime_profile_(runtime_profile), track_id_(track_id) {}
};

class BASE_EXPORT RuntimeProfiler
    : public std::enable_shared_from_this<RuntimeProfiler> {
 public:
  RuntimeProfiler();
  virtual ~RuntimeProfiler() = default;

  virtual void StartProfiling(bool is_create) = 0;
  virtual std::unique_ptr<RuntimeProfile> StopProfiling(bool is_destory) = 0;
  virtual void SetupProfiling(const int32_t sampling_interval) = 0;
  virtual trace::RuntimeProfilerType GetType() = 0;

  void SetTrackId(uint64_t track_id) { track_id_ = track_id; }

 protected:
  void StopProfiling(base::closure task, bool is_destory);
  void StartProfiling(base::closure task, bool is_create);
  void SetupProfiling(base::closure task);
  uint64_t track_id_;
  fml::RefPtr<fml::TaskRunner> task_runner_{};
};

}  // namespace profile
}  // namespace lynx

#endif  // CORE_RUNTIME_PROFILE_RUNTIME_PROFILER_H_
