// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "platform/embedder/core/performance/lynx_event_reporter.h"

#include <memory>
#include <utility>

#include "base/include/fml/thread.h"
#include "core/services/event_report/event_tracker.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "platform/embedder/core/performance/generic_info_storage.h"

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
}  // namespace

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

}  // namespace embedder
}  // namespace lynx
