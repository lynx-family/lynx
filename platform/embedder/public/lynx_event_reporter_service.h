// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_LYNX_EVENT_REPORTER_SERVICE_H_
#define PLATFORM_EMBEDDER_PUBLIC_LYNX_EVENT_REPORTER_SERVICE_H_

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "capi/lynx_event_reporter_service_capi.h"
#include "capi/lynx_service_center_capi.h"
#include "lynx/base/include/value/lynx_api_types.h"
#include "lynx_service_center.h"

namespace lynx {
namespace pub {

class LynxEventReporterService
    : public LynxServiceBase,
      public std::enable_shared_from_this<LynxEventReporterService> {
 public:
  LynxEventReporterService() = default;
  ~LynxEventReporterService() = default;

  void InitIfNeeded() override {
    if (event_reporter_service_) {
      return;
    }
    event_reporter_service_ = lynx_event_reporter_service_create_with_finalizer(
        new std::weak_ptr<LynxEventReporterService>(weak_from_this()),
        [](lynx_event_reporter_service_t* event_reporter_service,
           void* user_data) {
          std::weak_ptr<LynxEventReporterService>* weak_ptr =
              reinterpret_cast<std::weak_ptr<LynxEventReporterService>*>(
                  user_data);
          delete weak_ptr;
        });
    lynx_event_reporter_service_bind(
        event_reporter_service_,
        [](lynx_event_reporter_service_t* event_reporter_service,
           const char* event_name, const lynx_value& params) {
          std::weak_ptr<LynxEventReporterService>* weak_ptr =
              reinterpret_cast<std::weak_ptr<LynxEventReporterService>*>(
                  lynx_event_reporter_service_get_user_data(
                      event_reporter_service));
          auto shared_event_reporter_service = weak_ptr->lock();
          if (shared_event_reporter_service) {
            shared_event_reporter_service->OnEvent(event_name, params);
          }
        });

    lynx_event_reporter_service_bind_performance_report_func(
        event_reporter_service_,
        [](lynx_event_reporter_service_t* event_reporter_service,
           const lynx_value& params) {
          std::weak_ptr<LynxEventReporterService>* weak_ptr =
              reinterpret_cast<std::weak_ptr<LynxEventReporterService>*>(
                  lynx_event_reporter_service_get_user_data(
                      event_reporter_service));
          auto shared_event_reporter_service = weak_ptr->lock();
          if (shared_event_reporter_service) {
            shared_event_reporter_service->OnPerformanceEvent(params);
          }
        });
  }

  lynx_service_type_e GetServiceType() override {
    return kServiceTypeEventReporter;
  }

  void* Impl() override { return event_reporter_service_; }

  /**
   * @brief Interface to receive reported events
   * @param event_name Name of reported event
   * @param props Content of reported event in key-value format
   */
  virtual void OnEvent(const std::string& event_name,
                       const lynx_value& params) = 0;

  /**
   * @brief Interface to receive reported performance events
   * @param params Content of reported event in key-value format
   */
  virtual void OnPerformanceEvent(const lynx_value& params) {}

 private:
  lynx_event_reporter_service_t* event_reporter_service_ = nullptr;
};
}  // namespace pub
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PUBLIC_LYNX_EVENT_REPORTER_SERVICE_H_
