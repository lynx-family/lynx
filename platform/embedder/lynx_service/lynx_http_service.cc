// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_service/lynx_http_service_priv.h"

lynx_http_service_t::~lynx_http_service_t() {
  if (finalizer) {
    finalizer(this, user_data);
  }
}

LYNX_EXTERN_C lynx_http_service_t* lynx_http_service_create(void* user_data) {
  return lynx_http_service_create_with_finalizer(user_data, nullptr);
}

LYNX_EXTERN_C lynx_http_service_t* lynx_http_service_create_with_finalizer(
    void* user_data, void (*finalizer)(lynx_http_service_t*, void*)) {
  auto* http_service = new lynx_http_service_t;
  http_service->user_data = user_data;
  http_service->finalizer = finalizer;
  // The ref count has been initialized to 1, there is no need to call AddRef.
  return http_service;
}

LYNX_EXTERN_C void* lynx_http_service_get_user_data(
    lynx_http_service_t* http_service) {
  return http_service->user_data;
}

LYNX_EXTERN_C void lynx_http_service_bind(lynx_http_service_t* http_service,
                                          lynx_http_request_func f) {
  http_service->request_func = f;
}

LYNX_EXTERN_C void lynx_http_service_release(
    lynx_http_service_t* http_service) {
  // Unref the http_service.
  http_service->Release();
}

void lynx_http_service_request(lynx_http_service_t* http_service,
                               lynx_http_request_t* request,
                               lynx_http_response_t* response) {
  lynx_http_response_set_url(response, lynx_http_request_get_url(request));
  if (http_service && http_service->request_func) {
    http_service->request_func(http_service, request, response);
  } else {
    lynx_http_request_release(request);
    lynx_http_response_set_status_code(response, -1);
    lynx_http_response_set_status_text(response,
                                       "request_func is unimplemented");
    lynx_http_response_callback(response);
    lynx_http_response_release(response);
  }
}
