// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_service/lynx_http_request.h"

LYNX_EXTERN_C lynx_http_request_t* lynx_http_request_create(const char* url) {
  lynx_http_request_t* request = new lynx_http_request_t();
  request->url = url ? url : "";
  return request;
}

LYNX_EXTERN_C const char* lynx_http_request_get_url(
    lynx_http_request_t* request) {
  return request->url.c_str();
}

LYNX_EXTERN_C void lynx_http_request_set_method(lynx_http_request_t* request,
                                                const char* method) {
  if (!request) {
    return;
  }
  request->method = method && method[0] != '\0' ? method : "GET";
}

LYNX_EXTERN_C const char* lynx_http_request_get_method(
    lynx_http_request_t* request) {
  return request->method.c_str();
}

LYNX_EXTERN_C void lynx_http_request_set_timeout_ms(
    lynx_http_request_t* request, int32_t timeout_ms) {
  if (!request || timeout_ms < 0) {
    return;
  }
  request->timeout_ms = timeout_ms;
}

LYNX_EXTERN_C int32_t
lynx_http_request_get_timeout_ms(lynx_http_request_t* request) {
  return request ? request->timeout_ms : 0;
}

LYNX_EXTERN_C void lynx_http_request_set_body(lynx_http_request_t* request,
                                              const uint8_t* data,
                                              size_t length) {
  if (!request) {
    return;
  }
  if (length == 0) {
    request->body.clear();
    return;
  }
  if (!data) {
    return;
  }
  // Copy before replacing the body so `data` may refer to the current body.
  std::vector<uint8_t> body(data, data + length);
  request->body.swap(body);
}

LYNX_EXTERN_C size_t lynx_http_request_get_body(lynx_http_request_t* request,
                                                const uint8_t** body) {
  if (body) {
    *body = request->body.data();
  }
  return request->body.size();
}

LYNX_EXTERN_C void lynx_http_request_add_header(lynx_http_request_t* request,
                                                const char* key,
                                                const char* value) {
  if (!request || !key || key[0] == '\0' || !value) {
    return;
  }
  request->headers[key] = value;
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
