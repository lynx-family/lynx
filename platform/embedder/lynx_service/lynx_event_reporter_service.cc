// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "platform/embedder/lynx_service/lynx_event_reporter_service_priv.h"

lynx_event_reporter_service_t::~lynx_event_reporter_service_t() {
  if (finalizer) {
    finalizer(this, user_data);
  }
}

LYNX_EXTERN_C lynx_event_reporter_service_t* lynx_event_reporter_service_create(
    void* user_data) {
  return lynx_event_reporter_service_create_with_finalizer(user_data, nullptr);
}

LYNX_EXTERN_C lynx_event_reporter_service_t*
lynx_event_reporter_service_create_with_finalizer(
    void* user_data, void (*finalizer)(lynx_event_reporter_service_t*, void*)) {
  auto* event_report_service = new lynx_event_reporter_service_t;
  event_report_service->user_data = user_data;
  event_report_service->finalizer = finalizer;
  return event_report_service;
}

LYNX_EXTERN_C void lynx_event_reporter_service_bind(
    lynx_event_reporter_service_t* event_report_service,
    lynx_event_report_func f) {
  event_report_service->report_func = f;
}

LYNX_EXTERN_C void lynx_event_reporter_service_bind_performance_report_func(
    lynx_event_reporter_service_t* event_report_service,
    lynx_performance_event_report_func f) {
  event_report_service->performance_report_func = f;
}

LYNX_EXTERN_C void* lynx_event_reporter_service_get_user_data(
    lynx_event_reporter_service_t* event_report_service) {
  return event_report_service->user_data;
}

LYNX_EXTERN_C void lynx_event_reporter_service_release(
    lynx_event_reporter_service_t* event_report_service) {
  event_report_service->Release();
}

LYNX_EXTERN_C void lynx_event_reporter_service_on_event(
    lynx_event_reporter_service_t* event_reporter_service,
    const char* event_name, const lynx_value& params) {
  if (event_reporter_service && event_reporter_service->report_func) {
    event_reporter_service->report_func(event_reporter_service, event_name,
                                        params);
  }
}

LYNX_EXTERN_C void lynx_event_reporter_service_on_performance_event(
    lynx_event_reporter_service_t* event_reporter_service,
    const lynx_value& params) {
  if (event_reporter_service &&
      event_reporter_service->performance_report_func) {
    event_reporter_service->performance_report_func(event_reporter_service,
                                                    params);
  }
}
