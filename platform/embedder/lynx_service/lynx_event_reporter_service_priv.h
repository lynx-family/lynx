// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_EVENT_REPORTER_SERVICE_PRIV_H_
#define PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_EVENT_REPORTER_SERVICE_PRIV_H_

#include "platform/embedder/lynx_service/lynx_service_base.h"
#include "platform/embedder/public/capi/lynx_event_reporter_service_capi.h"

struct lynx_event_reporter_service_t : public lynx::embedder::LynxServiceBase {
  lynx_event_reporter_service_t() = default;
  ~lynx_event_reporter_service_t();

  void* user_data = nullptr;
  void (*finalizer)(lynx_event_reporter_service_t*, void*) = nullptr;
  lynx_event_report_func report_func = nullptr;
};

#endif  // PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_EVENT_REPORTER_SERVICE_PRIV_H_
