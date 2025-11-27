// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_service/lynx_http_response.h"

#include <cstring>
#include <utility>

lynx_http_response_t* lynx_http_response_create(HttpResponseCallback callback) {
  auto* response = new lynx_http_response_t();
  response->callback = callback;
  return response;
}

void lynx_http_response_release_body(lynx_http_response_t* response) {
  if (response->body.content) {
    if (response->body.dtor) {
      response->body.dtor(response->body.content, response->body.length,
                          response->body.opaque);
    } else {
      delete[] response->body.content;
    }
  }
  response->body.content = nullptr;
  response->body.length = 0;
  response->body.dtor = nullptr;
  response->body.opaque = nullptr;
}

void lynx_http_response_wrap(lynx_http_response_t* response,
                             lynx_http_response_t* target) {
  lynx_http_response_release_body(target);
  // Move the response data to the target, it's safe since the body of origin
  // response will be cleared.
  target->url = std::move(response->url);
  target->status_code = response->status_code;
  target->status_text = std::move(response->status_text);
  target->headers = std::move(response->headers);
  target->body = std::move(response->body);
  response->body.content = nullptr;
  response->body.length = 0;
  response->body.dtor = nullptr;
  response->body.opaque = nullptr;
}

LYNX_EXTERN_C void lynx_http_response_set_url(lynx_http_response_t* response,
                                              const char* url) {
  response->url = url ? url : "";
}

LYNX_EXTERN_C void lynx_http_response_set_status_code(
    lynx_http_response_t* response, int status_code) {
  response->status_code = status_code;
}

LYNX_EXTERN_C void lynx_http_response_set_status_text(
    lynx_http_response_t* response, const char* status_text) {
  response->status_text = status_text ? status_text : "";
}

LYNX_EXTERN_C void lynx_http_response_add_header(lynx_http_response_t* response,
                                                 const char* key,
                                                 const char* value) {
  if (key && value) {
    response->headers[key] = value;
  }
}

LYNX_EXTERN_C void lynx_http_response_set_body(
    lynx_http_response_t* response, uint8_t* content, size_t length,
    void (*dtor)(uint8_t*, size_t, void*), void* opaque) {
  lynx_http_response_release_body(response);
  if (!content || length == 0) {
    return;
  }
  response->body.length = length;
  if (dtor) {
    response->body.content = content;
    response->body.dtor = dtor;
    response->body.opaque = opaque;
  } else {
    response->body.content = new uint8_t[length];
    memcpy(response->body.content, content, length);
  }
}

LYNX_EXTERN_C void lynx_http_response_callback(lynx_http_response_t* response) {
  if (response->completed) {
    return;
  }
  response->completed = true;
  if (response->callback) {
    response->callback(response);
  }
}

LYNX_EXTERN_C void lynx_http_response_release(lynx_http_response_t* response) {
  // complete the response first if necessary.
  lynx_http_response_callback(response);
  lynx_http_response_release_body(response);
  delete response;
}
