// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_HTTP_REQUEST_H_
#define PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_HTTP_REQUEST_H_

#include <map>
#include <string>
#include <vector>

#include "platform/embedder/public/capi/lynx_http_service_capi.h"

struct lynx_http_request_t {
  std::string url;
  std::string method = "GET";
  std::map<std::string, std::string> headers;
  std::vector<uint8_t> body;
};

lynx_http_request_t* lynx_http_request_create(const std::string& url);

#endif  // PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_HTTP_REQUEST_H_
