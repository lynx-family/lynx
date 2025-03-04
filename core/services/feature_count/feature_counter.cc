// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/feature_count/feature_counter.h"

#include <utility>

#include "base/include/thread/timed_task.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/services/event_report/event_tracker.h"
#include "core/services/feature_count/global_feature_counter.h"

namespace lynx {
namespace tasm {
namespace report {

FeatureCounter* FeatureCounter::Instance() {
  static thread_local FeatureCounter instance_;
  return &instance_;
}

void FeatureCounter::UpdateAndBackupCurrentInstanceId(int32_t instance_id) {
  if (!GlobalFeatureCounter::Enable() ||
      instance_id == shell::kUnknownInstanceId) {
    return;
  }
  instance_id_stack_.push(current_instance_id_);
  if (current_instance_id_ == instance_id) {
    return;
  }
  current_instance_id_ = instance_id;
  ResetCurrentFeatures();
}

void FeatureCounter::RestoreCurrentInstanceId() {
  if (!GlobalFeatureCounter::Enable() ||
      current_instance_id_ == shell::kUnknownInstanceId) {
    // do nothing when GlobalFeatureCounter is not enble or instance id is
    // unknown.
    return;
  }
  if (instance_id_stack_.empty()) {
    // If the instance id stack is empty, reset current_instance_id_ and
    // current_features_ as initial values.
    current_instance_id_ = shell::kUnknownInstanceId;
    current_features_ = nullptr;
    return;
  }
  current_instance_id_ = instance_id_stack_.top();
  instance_id_stack_.pop();
  ResetCurrentFeatures();
}

void FeatureCounter::Count(LynxFeature feature) {
  if (!GlobalFeatureCounter::Enable()) {
    return;
  }
  CountIfNeed(feature);
}

void FeatureCounter::ClearAndReport(int32_t instance_id) {
  if (!GlobalFeatureCounter::Enable() ||
      current_instance_id_ == shell::kUnknownInstanceId) {
    // do nothing when GlobalFeatureCounter is not enble or instance id is
    // unknown.
    return;
  }
  auto& all_instance_need_to_report = all_instance_need_to_report_;
  auto& all_instance_features = all_instance_features_;
  auto const& it = all_instance_need_to_report.find(instance_id);
  if (it == all_instance_need_to_report.end()) {
    // If the instance of 'instance_id' has no new feature to report, clear it
    // directly.
    all_instance_features.erase(instance_id);
    return;
  }
  all_instance_need_to_report.erase(instance_id);
  auto features_it = all_instance_features.find(instance_id);
  if (features_it == all_instance_features.end()) {
    // If the instance has a new feature to be reported, but the features cannot
    // be found, just ignore it.
    return;
  }

  std::array<bool, kAllFeaturesCount> features = std::move(features_it->second);
  all_instance_features.erase(instance_id);
  GlobalFeatureCounter::MergeAndReport(std::move(features), instance_id);
}

void FeatureCounter::Flush() {
  // no instances with new features need to be reported.
  if (!GlobalFeatureCounter::Enable() || all_instance_need_to_report_.empty()) {
    return;
  }
  // report features of instance
  for (auto const& it : all_instance_need_to_report_) {
    auto const& features_it = all_instance_features_.find(it);
    if (features_it != all_instance_features_.end()) {
      GlobalFeatureCounter::MergeAndReport(features_it->second,
                                           features_it->first);
    }
  }
  all_instance_need_to_report_.clear();
}

void FeatureCounter::CountIfNeed(LynxFeature feature) {
  if (current_instance_id_ == shell::kUnknownInstanceId) {
    LOGE(
        "The current thread did not find the lynx actor, please use "
        "GlobalFeatureCounter::Count");
    return;
  }
  if (current_features_ == nullptr) {
    std::array<bool, kAllFeaturesCount> features{false};
    all_instance_features_.insert({current_instance_id_, std::move(features)});
    auto it = all_instance_features_.find(current_instance_id_);
    current_features_ = &(it->second);
  }
  if (!(*current_features_)[feature]) {
    (*current_features_)[feature] = true;
    all_instance_need_to_report_.emplace(current_instance_id_);
    StartTimerIfNeed();
  }
}

void FeatureCounter::StartTimerIfNeed() {
  if (timer_manager_) {
    return;
  }
  timer_manager_ = std::make_unique<base::TimedTaskManager>(false);
  timer_manager_->SetInterval(
      []() { Instance()->Flush(); },
      GlobalFeatureCounter::LYNX_FEATURE_COUNT_MILLISECONDS_TIMER_INTERVAL);
}

void FeatureCounter::ResetCurrentFeatures() {
  if (all_instance_features_.empty()) {
    current_features_ = nullptr;
    return;
  }
  auto it = all_instance_features_.find(current_instance_id_);
  if (it == all_instance_features_.end()) {
    current_features_ = nullptr;
    return;
  }
  current_features_ = &(it->second);
}

}  // namespace report
}  // namespace tasm
}  // namespace lynx
