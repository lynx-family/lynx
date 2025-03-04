// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_FEATURE_COUNT_FEATURE_COUNTER_H_
#define CORE_SERVICES_FEATURE_COUNT_FEATURE_COUNTER_H_

#include <array>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "base/include/lynx_actor.h"
#include "base/include/thread/timed_task.h"
#include "base/include/vector.h"
#include "core/services/feature_count/feature.h"

namespace lynx {
namespace tasm {
namespace report {
/// FeatureCounter is used to collect feature usage associated with
/// LynxActor. mainly responsible
/// for  feature collection and temporary storage.
/// In the thread managed by LynxActor, you can use FeatureCounter, otherwise
/// use GlobalFeatureCounter. All methods of FeatureCounter can be called from
/// any thread.
class FeatureCounter {
 public:
  /// Cache  feature to feature array and upload them later.
  /// @param feature LynxFeature
  void Count(LynxFeature feature);
  /// Clear and report features of instance id when template instance be reset
  /// or destory. Called when the LynxActor of the current thread is destroyed.
  /// @param instance_id  the unique id of template instance.
  void ClearAndReport(int32_t instance_id);
  /// Push the instance_id onto the instance id stack and set it to
  /// current_instance_id_.
  ///  And reset features for the current instance.
  ///   Called when the LynxActor::BeforeInvoked of the current thread.
  /// @param instance_id  the unique id of template instance.
  void UpdateAndBackupCurrentInstanceId(int32_t instance_id);
  /// Restore features for the current instance.
  /// Called when the LynxActor::AfterInvoked of the current thread.
  void RestoreCurrentInstanceId();

  /// thread local instance of FeatureCounter;
  static FeatureCounter* Instance();

 private:
  /// Flush all features to GlobalFeatureCounter.
  void Flush();
  /// Start the timer to report feature count, the timer fires every 20s by
  /// default.
  /// The timer for event reporting will start after the first event is
  /// reported. Each thread will only create one timer, and different template
  /// instances can listen to timing events.
  void StartTimerIfNeed();
  /// Reset features for the current instance.
  void ResetCurrentFeatures();
  /// Cache  feature to feature array and upload them later.
  /// @param feature LynxFeature
  void CountIfNeed(LynxFeature feature);

  FeatureCounter() = default;
  ~FeatureCounter() = default;
  FeatureCounter(const FeatureCounter& timing) = delete;
  FeatureCounter& operator=(const FeatureCounter&) = delete;
  FeatureCounter(FeatureCounter&&) = delete;
  FeatureCounter& operator=(FeatureCounter&&) = delete;

  /// Features of all template instance, will be converted to event reporting.
  std::unordered_map<int32_t, std::array<bool, kAllFeaturesCount>>
      all_instance_features_;
  /// the features of current instance id.
  std::array<bool, kAllFeaturesCount>* current_features_{nullptr};
  /// The stack that stores instance id need to report features.
  std::unordered_set<int32_t> all_instance_need_to_report_;
  /// The instance id of the currently collected features.
  int32_t current_instance_id_ = shell::kUnknownInstanceId;
  /// The stack that stores instance id.
  base::InlineStack<int32_t, 32> instance_id_stack_;
  /// The timer for feature reporting.
  std::unique_ptr<base::TimedTaskManager> timer_manager_{nullptr};
};

}  // namespace report
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_FEATURE_COUNT_FEATURE_COUNTER_H_
