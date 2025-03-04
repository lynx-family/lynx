// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_FEATURE_COUNT_GLOBAL_FEATURE_COUNTER_H_
#define CORE_SERVICES_FEATURE_COUNT_GLOBAL_FEATURE_COUNTER_H_

#include <array>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include "base/include/thread/timed_task.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/services/feature_count/feature.h"

namespace lynx {
namespace tasm {
namespace report {

/// GlobalFeatureCounter is used to collect feature usage,
/// mainly responsible for  feature collection and temporary storage.
/// All methods of GlobalFeatureCounter can be called from any thread.
class GlobalFeatureCounter {
 public:
  ///  The interval of timer to report feature count.
  static constexpr int64_t LYNX_FEATURE_COUNT_MILLISECONDS_TIMER_INTERVAL =
      20000;
  /// Cache  feature to feature array and upload them later.
  /// Can be called from any thread.
  /// @param feature LynxFeature
  static void Count(LynxFeature feature, int32_t instance_id);

  /// Merge features into all_instance_features_ and report.
  /// @param features features of instance
  /// @param instance_id the unique id of template instance.
  static void MergeAndReport(std::array<bool, kAllFeaturesCount> features,
                             int32_t instance_id);

  /// Clear and report features of instance id when template instance be reset
  /// or destory. Can be called from any thread, the method will run on report
  /// thread.
  /// @param instance_id  the unique id of template instance.
  static void ClearAndReport(int32_t instance_id);

  /// If enable is false, GlobalFeatureCounter don't work.
  static bool Enable() { return Instance().enable_; }

 private:
  friend class base::NoDestructor<GlobalFeatureCounter>;
  static GlobalFeatureCounter& Instance();
  GlobalFeatureCounter()
      : enable_(LynxEnv::GetInstance().EnableFeatureCounter()){};
  GlobalFeatureCounter(const GlobalFeatureCounter& timing) = delete;
  GlobalFeatureCounter& operator=(const GlobalFeatureCounter&) = delete;
  GlobalFeatureCounter(GlobalFeatureCounter&&) = delete;
  GlobalFeatureCounter& operator=(GlobalFeatureCounter&&) = delete;
  /// Start the timer to report feature count.
  static void StartTimerIfNeed();
  /// The timer fires every 20s by default.
  static void TimerFired();
  static void Report(const std::array<bool, kAllFeaturesCount>& features,
                     int32_t instance_id);
  std::mutex lock_;
  /// Features of all template instance, will be converted to event reporting.
  std::unordered_map<int32_t, std::array<bool, kAllFeaturesCount>>
      all_instance_features_;
  /// Instances with new features to be reported.
  std::unordered_set<int32_t> all_instance_need_to_report_;
  std::unique_ptr<base::TimedTaskManager> timer_{nullptr};
  std::atomic<bool> is_timer_running_ = false;
  bool enable_ = false;
};

}  // namespace report
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_FEATURE_COUNT_GLOBAL_FEATURE_COUNTER_H_
