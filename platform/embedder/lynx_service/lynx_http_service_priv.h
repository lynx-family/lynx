// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_HTTP_SERVICE_PRIV_H_
#define PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_HTTP_SERVICE_PRIV_H_

#include "platform/embedder/lynx_service/lynx_http_request.h"
#include "platform/embedder/lynx_service/lynx_http_response.h"
#include "platform/embedder/lynx_service/lynx_service_base.h"
#include "platform/embedder/public/capi/lynx_http_service_capi.h"

struct lynx_http_service_t : public lynx::embedder::LynxServiceBase {
  lynx_http_service_t() = default;
  ~lynx_http_service_t();

  void* user_data = nullptr;
  void (*finalizer)(lynx_http_service_t*, void*) = nullptr;

  lynx_http_request_func request_func = nullptr;
};

void lynx_http_service_request(lynx_http_service_t* http_service,
                               lynx_http_request_t* request,
                               lynx_http_response_t* response);

#endif  // PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_HTTP_SERVICE_PRIV_H_
