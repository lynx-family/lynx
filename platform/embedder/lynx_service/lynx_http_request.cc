// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_service/lynx_http_request.h"

lynx_http_request_t* lynx_http_request_create(const std::string& url) {
  lynx_http_request_t* request = new lynx_http_request_t();
  request->url = url;
  return request;
}

LYNX_EXTERN_C const char* lynx_http_request_get_url(
    lynx_http_request_t* request) {
  return request->url.c_str();
}

LYNX_EXTERN_C const char* lynx_http_request_get_method(
    lynx_http_request_t* request) {
  return request->method.c_str();
}

LYNX_EXTERN_C size_t lynx_http_request_get_body(lynx_http_request_t* request,
                                                const uint8_t** body) {
  if (body) {
    *body = request->body.data();
  }
  return request->body.size();
}

LYNX_EXTERN_C size_t
lynx_http_request_get_header_count(lynx_http_request_t* request) {
  return request->headers.size();
}

LYNX_EXTERN_C void lynx_http_request_get_header(lynx_http_request_t* request,
                                                uint32_t index,
                                                const char** key,
                                                const char** value) {
  if (index >= request->headers.size()) {
    return;
  }
  if (key && value) {
    auto iter = request->headers.begin();
    while (index > 0) {
      iter++;
      index--;
    }
    *key = iter->first.c_str();
    *value = iter->second.c_str();
  }
}

LYNX_EXTERN_C void lynx_http_request_release(lynx_http_request_t* request) {
  delete request;
}
