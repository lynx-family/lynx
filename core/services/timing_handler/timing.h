// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_TIMING_HANDLER_TIMING_H_
#define CORE_SERVICES_TIMING_HANDLER_TIMING_H_

#include <chrono>
#include <string>
#include <utility>

#include "base/include/linked_hash_map.h"
#include "base/include/timer/time_utils.h"
#include "base/include/vector.h"
#include "core/public/pipeline_option.h"
#include "core/public/timing_key.h"

namespace lynx {
namespace tasm {

// Here we use an ordered map to ensure that the timestamps within
// the same scope are stored in the TimingMap in the order they were recorded.
// This is because in the Native TimingHandler, we determine whether the
// rendering pipeline's timestamping is complete based on the last timestamp of
// the sub-phase. For example, in TASM, we would check for load_template_end to
// determine if the timestamping is complete. In such cases, if the map's
// storage structure were: load_template_start, load_template_end, decode_start,
// which is not in the order of they were recorded. It could result in
// decode_start failing to be stored in the TimingHandler in time.
using TimingMap = base::LinkedHashMap<TimingKey, uint64_t>;

// Some timing scopes especially those on root functions such as
// LynxEngine::LoadTemplate() may contain many timing points.
// Initialize the timing map with larger allocation size.
constexpr size_t kTimingMapAllocationSize = 16;

class Timing {
 public:
  explicit Timing(const PipelineID& pipeline_id = "")
      : pipeline_id_(pipeline_id) {}
  Timing(const Timing& s) = delete;
  Timing& operator=(const Timing&) = delete;
  Timing(Timing&&) = default;
  Timing& operator=(Timing&&) = default;

  TimingMap timings_{kTimingMapAllocationSize};
  TimingMap framework_timings_{kTimingMapAllocationSize};
  PipelineID pipeline_id_;
};

class TimingCollector {
 public:
  template <typename D>
  class Scope {
   public:
    explicit Scope(D* delegate_ptr) : delegate_ptr_(delegate_ptr) {
      TimingCollector::Instance()->timing_stack_.emplace();
    }
    explicit Scope(D* delegate_ptr, const PipelineOptions& pipeline_options)
        : delegate_ptr_(delegate_ptr) {
      TimingCollector::Instance()->timing_stack_.emplace(
          pipeline_options.pipeline_id);
    }
    explicit Scope(D* delegate_ptr, const PipelineID& pipeline_id)
        : delegate_ptr_(delegate_ptr) {
      TimingCollector::Instance()->timing_stack_.emplace(pipeline_id);
    }

    ~Scope() {
      if (delegate_ptr_ != nullptr) {
        auto& timing = TimingCollector::Instance()->timing_stack_.top();
        delegate_ptr_->SetTiming(std::move(timing));
      }
      TimingCollector::Instance()->timing_stack_.pop();
    }

    Scope(const Scope& s) = delete;
    Scope& operator=(const Scope&) = delete;
    Scope(Scope&&) = delete;
    Scope& operator=(Scope&&) = delete;

   private:
    D* delegate_ptr_;
  };

  static TimingCollector* Instance();

  void Mark(const TimingKey& key, uint64_t timestamp = 0);
  void MarkFrameworkTiming(const TimingKey& key, uint64_t timestamp = 0);
  PipelineID GetTopPipelineID();

  TimingCollector(){};
  TimingCollector(const TimingCollector& timing) = delete;
  TimingCollector& operator=(const TimingCollector&) = delete;
  TimingCollector(TimingCollector&&) = delete;
  TimingCollector& operator=(TimingCollector&&) = delete;

 private:
  base::InlineStack<Timing, 16> timing_stack_;
  // Used to mark the loadTemplate/reloadTemplate start time. should be
  // consistent with SETUP_TIMING_FLAG_PREFIX of TimingHandler in platform.
  static const std::string SETUP_TIMING_FLAG_PREFIX;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_TIMING_HANDLER_TIMING_H_
