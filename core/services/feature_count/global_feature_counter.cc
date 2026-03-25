// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/feature_count/global_feature_counter.h"

#include <utility>
#include <vector>

#include "base/trace/native/trace_event.h"
#include "core/services/event_report/event_tracker.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "core/services/trace/service_trace_event_def.h"

namespace lynx {
namespace tasm {
namespace report {

/// The event name of feature count.
static constexpr const char* const LYNX_FEATURE_COUNT_EVENT =
    "lynxsdk_feature_count_event";

GlobalFeatureCounter& GlobalFeatureCounter::Instance() {
  static base::NoDestructor<GlobalFeatureCounter> instance;
  return *instance.get();
}

void GlobalFeatureCounter::Count(LynxFeature feature, int32_t instance_id) {
  auto& inst = Instance();
  if (!inst.enable_) {
    return;
  }
  std::lock_guard<std::mutex> lock(inst.lock_);
  auto it = inst.all_instance_features_.find(instance_id);
  if (it == inst.all_instance_features_.end()) {
    // If the features of instance_id cannot be found, create an features array
    // and insert it into all_instance_features_.
    // At the same time, it is necessary to mark instance_id as to be reported,
    // and try to start the report timer.
    std::array<bool, kAllFeaturesCount> features{false};
    features[feature] = true;
    inst.all_instance_features_.insert({instance_id, std::move(features)});
    inst.all_instance_need_to_report_.emplace(instance_id);
    StartTimerIfNeed();
    return;
  }
  if (!it->second[feature]) {
    // If feature has not been collected, mark the instance_id need to be
    // reported.
    it->second[feature] = true;
    inst.all_instance_need_to_report_.emplace(instance_id);
  }
}

void GlobalFeatureCounter::MergeAndReport(
    std::array<bool, kAllFeaturesCount> features, int32_t instance_id,
    bool on_destroy) {
  if (!Instance().enable_) {
    return;
  }
  EventTrackerPlatformImpl::GetReportTaskRunner()->PostTask(
      [instance_id, features = std::move(features), on_destroy]() mutable {
        auto& inst = Instance();
        std::lock_guard<std::mutex> lock(inst.lock_);
        auto features_it = inst.all_instance_features_.find(instance_id);
        if (features_it == inst.all_instance_features_.end()) {
          if (on_destroy) {
            return;
          }
          // If the features of instance_id cannot be found, report it directly
          // and insert it into all_instance_features_.
          inst.Report(features, instance_id);
          inst.all_instance_features_.insert(
              {instance_id, std::move(features)});
          inst.all_instance_need_to_report_.erase(instance_id);
          return;
        }
        bool needToReport =
            inst.all_instance_need_to_report_.find(instance_id) !=
            inst.all_instance_need_to_report_.end();
        // Merge features into the already reported list, if there is a new
        // feature, mark needToReport as true.
        for (size_t i = 0; i < features.size(); i++) {
          if (features[i] && !features_it->second[i]) {
            needToReport = true;
            features_it->second[i] = true;
          } else {
            features[i] = features_it->second[i];
          }
        }
        if (needToReport) {
          inst.all_instance_need_to_report_.erase(instance_id);
          inst.Report(features, instance_id);
        }
      });
}

void GlobalFeatureCounter::ClearAndReport(int32_t instance_id) {
  if (!Instance().enable_) {
    return;
  }
  EventTrackerPlatformImpl::GetReportTaskRunner()->PostTask([instance_id]() {
    auto& inst = Instance();
    std::lock_guard<std::mutex> lock(inst.lock_);
    auto const& it = inst.all_instance_need_to_report_.find(instance_id);
    if (it == inst.all_instance_need_to_report_.end()) {
      // If the instance of 'instance_id' has no new feature to report, clear it
      // directly.
      inst.all_instance_features_.erase(instance_id);
      return;
    }
    inst.all_instance_need_to_report_.erase(instance_id);
    auto features_it = inst.all_instance_features_.find(instance_id);
    if (features_it == inst.all_instance_features_.end()) {
      // If the instance has a new feature to be reported, but the features
      // cannot be found, just ignore it.
      return;
    }
    std::array<bool, kAllFeaturesCount> features =
        std::move(features_it->second);
    inst.all_instance_features_.erase(instance_id);
    inst.Report(features, instance_id);
  });
}

void GlobalFeatureCounter::StartTimerIfNeed() {
  if (Instance().is_timer_running_) {
    return;
  }
  EventTrackerPlatformImpl::GetReportTaskRunner()->PostTask([]() {
    auto& inst = Instance();
    if (inst.is_timer_running_) {
      // Avoid multiple calls under multi-threading.
      return;
    }
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FEATURE_COUNT_REPORTER_START_TIMER);
    inst.timer_ = std::make_unique<base::TimedTaskManager>();
    inst.timer_->SetInterval([]() { TimerFired(); },
                             LYNX_FEATURE_COUNT_MILLISECONDS_TIMER_INTERVAL);
    inst.is_timer_running_ = true;
  });
}

// Run on report thread.
void GlobalFeatureCounter::TimerFired() {
  auto& inst = Instance();
  std::lock_guard<std::mutex> lock(inst.lock_);
  // no instances with new features need to be reported.
  if (inst.all_instance_need_to_report_.empty()) {
    return;
  }
  auto all_instance_need_to_report =
      std::move(inst.all_instance_need_to_report_);

  // report features of instance
  for (auto const& it : all_instance_need_to_report) {
    auto const& features_it = inst.all_instance_features_.find(it);
    if (features_it != inst.all_instance_features_.end()) {
      Report(features_it->second, features_it->first);
    }
  }
}

// Run on report thread.
void GlobalFeatureCounter::Report(
    const std::array<bool, kAllFeaturesCount>& features, int32_t instance_id) {
  MoveOnlyEvent event;
  event.SetName(LYNX_FEATURE_COUNT_EVENT);
  for (size_t i = 0; i < features.size(); i++) {
    const char* feature_name = LynxFeatureToString((LynxFeature)i);
    if (strlen(feature_name) == 0) {
      continue;
    }
    event.SetProps(feature_name, features[i]);
  }
  EventTrackerPlatformImpl::OnEvent(instance_id, std::move(event));
}
}  // namespace report
}  // namespace tasm
}  // namespace lynx
