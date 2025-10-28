// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "platform/embedder/core/performance/lynx_event_reporter.h"

#include <atomic>
#include <memory>
#include <utility>

#include "base/include/fml/thread.h"
#include "core/services/event_report/event_tracker.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "platform/embedder/core/performance/generic_info_storage.h"
#include "platform/embedder/public/capi/lynx_event_reporter_service_capi.h"
#include "platform/embedder/public/capi/lynx_service_center_capi.h"

namespace lynx {
namespace embedder {

namespace {
static void RunTaskInReportThread(base::closure task) {
  auto task_runner =
      lynx::tasm::report::EventTrackerPlatformImpl::GetReportTaskRunner();
  if (task_runner) {
    if (task_runner->RunsTasksOnCurrentThread()) {
      task();
    } else {
      task_runner->PostTask([task = std::move(task)]() { task(); });
    }
  }
}

// only be used in report thread
static std::unordered_map<uint64_t, std::weak_ptr<LynxEventReporterObserver>>
    observers;
static std::shared_ptr<LynxEventReporterObserver>
    event_report_service_observer = nullptr;
}  // namespace

// build-in LynxEventReporte observer,
// which is a wrapper of LynxEventReporterService
class LynxReporterServiceObserver : public LynxEventReporterObserver {
 public:
  LynxReporterServiceObserver() = default;
  ~LynxReporterServiceObserver() = default;

  void OnReportEvent(int32_t instance_id, const std::string& event_name,
                     const lynx::pub::LynxValue& props) {
    lynx_service_center_t* service_center = lynx_service_get_center_instance();
    if (service_center) {
      lynx_event_reporter_service_t* reporter_service =
          reinterpret_cast<lynx_event_reporter_service_t*>(
              lynx_service_get_service(service_center,
                                       kServiceTypeEventReporter));
      if (reporter_service) {
        lynx_event_reporter_service_on_event(reporter_service,
                                             event_name.c_str(), props.Value());
      }
    }
  }
};

void LynxEventReporter::OnEvent(int32_t instance_id,
                                tasm::report::MoveOnlyEvent&& event) {
  LynxEventReporter::InitLynxReporterServiceObserverIfNeeded();

  RunTaskInReportThread([instance_id, event = std::move(event)]() {
    auto map_value =
        lynx::pub::LynxValue(lynx::pub::LynxValue::kCreateAsMapTag);

    // 1. set properties
    {
      for (const auto& item : event.GetStringProps()) {
        map_value.SetProperty(item.first.c_str(),
                              lynx::pub::LynxValue(item.second));
      }
      for (const auto& item : event.GetIntProps()) {
        map_value.SetProperty(item.first.c_str(),
                              lynx::pub::LynxValue(item.second));
      }
      for (const auto& item : event.GetDoubleProps()) {
        map_value.SetProperty(item.first.c_str(),
                              lynx::pub::LynxValue(item.second));
      }
    }

    // 2. merge generic infos
    {
      const auto& generic_infos =
          GenericInfoStorage::Instance().GetGenericInfo(instance_id);
      for (const auto& str_info : generic_infos.GetStrGenericInfos()) {
        map_value.SetProperty(str_info.first.c_str(),
                              lynx::pub::LynxValue(str_info.second));
      }

      for (const auto& i64_info : generic_infos.GetInt64GenericInfos()) {
        map_value.SetProperty(i64_info.first.c_str(),
                              lynx::pub::LynxValue(i64_info.second));
      }

      for (const auto& float_info : generic_infos.GetFloatGenericInfos()) {
        map_value.SetProperty(float_info.first.c_str(),
                              lynx::pub::LynxValue(float_info.second));
      }
    }

    // 3. send event report to each observer
    for (const auto& ob : observers) {
      auto strong_ob = ob.second.lock();
      if (strong_ob) {
        strong_ob->OnReportEvent(instance_id, event.GetName(), map_value);
      }
    }
  });
}

void LynxEventReporter::UpdateGenericInfo(const std::string& key,
                                          const std::string& value,
                                          int32_t instance_id) {
  RunTaskInReportThread([key, value, instance_id]() {
    GenericInfoStorage::Instance().UpdateGenericInfo(key, value, instance_id);
  });
}

void LynxEventReporter::UpdateGenericInfo(
    std::unordered_map<std::string, std::string>&& infos, int32_t instance_id) {
  RunTaskInReportThread([instance_id, infos = std::move(infos)]() {
    GenericInfoStorage::Instance().UpdateGenericInfo(infos, instance_id);
  });
}

void LynxEventReporter::UpdateGenericInfo(const std::string& key, float value,
                                          int32_t instance_id) {
  RunTaskInReportThread([key, value, instance_id]() {
    GenericInfoStorage::Instance().UpdateGenericInfo(key, value, instance_id);
  });
}

void LynxEventReporter::UpdateGenericInfo(
    std::unordered_map<std::string, float>&& infos, int32_t instance_id) {
  RunTaskInReportThread([infos = std::move(infos), instance_id]() {
    GenericInfoStorage::Instance().UpdateGenericInfo(infos, instance_id);
  });
}

void LynxEventReporter::UpdateGenericInfo(const std::string& key, int64_t value,
                                          int32_t instance_id) {
  RunTaskInReportThread([key, value, instance_id]() {
    GenericInfoStorage::Instance().UpdateGenericInfo(key, value, instance_id);
  });
}

void LynxEventReporter::RemoveGenericInfo(const std::string& key,
                                          int32_t instance_id) {
  RunTaskInReportThread([key, instance_id]() {
    GenericInfoStorage::Instance().RemoveGenericInfo(key, instance_id);
  });
}

void LynxEventReporter::GetAllGenericInfosInReportThread(
    int32_t instance_id,
    base::MoveOnlyClosure<void, std::unique_ptr<const pub::Value>>
        on_get_generic_infos_cb) {
  if (!on_get_generic_infos_cb) {
    return;
  }

  RunTaskInReportThread(
      [on_get_generic_infos_cb = std::move(on_get_generic_infos_cb),
       instance_id]() mutable {
        auto infos =
            GenericInfoStorage::Instance().GetAllGenericInfosInReportThread(
                instance_id);
        on_get_generic_infos_cb(std::move(infos));
      });
}

std::unique_ptr<const pub::Value>
LynxEventReporter::GetAllGenericInfosInReportThread(int32_t instance_id) {
  return GenericInfoStorage::Instance().GetAllGenericInfosInReportThread(
      instance_id);
}

void LynxEventReporter::ClearCache(int32_t instance_id) {
  RunTaskInReportThread([instance_id]() {
    GenericInfoStorage::Instance().ClearCache(instance_id);
  });
}

void LynxEventReporter::ClearAll() {
  RunTaskInReportThread([]() { GenericInfoStorage::Instance().ClearAll(); });
}

uint32_t LynxEventReporter::AddEventReporterObserver(
    std::weak_ptr<LynxEventReporterObserver> observer) {
  static std::atomic<uint32_t> token = 0;
  auto token_copy = token++;
  // post to report thread for thread safety
  RunTaskInReportThread(
      [observer, token_copy]() { observers[token_copy] = observer; });
  return token_copy;
}

void LynxEventReporter::RemoveEventReporterObserver(uint32_t token) {
  RunTaskInReportThread([token]() { observers.erase(token); });
}

void LynxEventReporter::InitLynxReporterServiceObserverIfNeeded() {
  static std::once_flag flag;
  std::call_once(flag, []() {
    event_report_service_observer =
        std::make_shared<LynxReporterServiceObserver>();
    AddEventReporterObserver(event_report_service_observer);
  });
}

}  // namespace embedder
}  // namespace lynx
