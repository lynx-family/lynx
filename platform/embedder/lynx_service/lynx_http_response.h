// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_HTTP_RESPONSE_H_
#define PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_HTTP_RESPONSE_H_

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "platform/embedder/public/capi/lynx_http_service_capi.h"

using HttpResponseCallback = std::function<void(lynx_http_response_t*)>;

struct lynx_http_response_t {
  std::string url;
  int status_code = -1;
  std::string status_text;
  std::map<std::string, std::string> headers;

  struct body {
    uint8_t* content = nullptr;
    size_t length = 0;
    void (*dtor)(uint8_t*, size_t, void*) = nullptr;
    void* opaque = nullptr;
  } body;

  HttpResponseCallback callback = nullptr;
  bool completed = false;
};

lynx_http_response_t* lynx_http_response_create(HttpResponseCallback callback);

void lynx_http_response_wrap(lynx_http_response_t* response,
                             lynx_http_response_t* target);

#endif  // PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_HTTP_RESPONSE_H_
