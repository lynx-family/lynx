// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstring>

#include "platform/embedder/fetcher/lynx_resource_response_priv.h"

lynx_resource_response_t* lynx_resource_response_create_internal(
    ResourceCallback callback) {
  lynx_resource_response_t* response = new lynx_resource_response_t();
  response->callback = callback;
  return response;
}

LYNX_EXTERN_C lynx_resource_response_t* lynx_resource_response_create(
    lynx_resource_response_callback_func c_callback, void* user_data) {
  ResourceCallback cpp_callback = nullptr;
  if (c_callback) {
    cpp_callback = [c_callback, user_data](lynx_resource_response_t* response) {
      c_callback(response, user_data);
    };
  }
  return lynx_resource_response_create_internal(cpp_callback);
}

void lynx_resource_response_release_data(lynx_resource_response_t* response) {
  if (response->data.content) {
    if (response->data.dtor) {
      response->data.dtor(response->data.content, response->data.length,
                          response->data.opaque);
    } else {
      delete[] response->data.content;
    }
  }
  response->data.content = nullptr;
  response->data.length = 0;
  response->data.dtor = nullptr;
  response->data.opaque = nullptr;
}

lynx_resource_response_t* lynx_resource_response_create_swap(
    lynx_resource_response_t* response) {
  lynx_resource_response_t* target =
      lynx_resource_response_create_internal(nullptr);
  // Move the response data to the target, it's safe since the data of origin
  // response will be cleared.
  target->code = response->code;
  target->error_message = std::move(response->error_message);
  target->data = std::move(response->data);
  target->template_bundle = std::move(response->template_bundle);
  response->data.content = nullptr;
  response->data.length = 0;
  response->data.dtor = nullptr;
  response->data.opaque = nullptr;
  return target;
}

LYNX_EXTERN_C void lynx_resource_response_set_code(
    lynx_resource_response_t* response, int code) {
  response->code = code;
}

LYNX_EXTERN_C void lynx_resource_response_set_error_message(
    lynx_resource_response_t* response, const char* msg) {
  response->error_message = msg ? msg : "";
}

LYNX_EXTERN_C void lynx_resource_response_set_data(
    lynx_resource_response_t* response, uint8_t* content, size_t length,
    void (*dtor)(uint8_t*, size_t, void*) = nullptr, void* opaque = nullptr) {
  lynx_resource_response_release_data(response);
  if (!content || length == 0) {
    return;
  }
  response->data.length = length;
  if (dtor) {
    response->data.content = content;
    response->data.dtor = dtor;
    response->data.opaque = opaque;
  } else {
    response->data.content = new uint8_t[length];
    memcpy(response->data.content, content, length);
  }
}

LYNX_EXTERN_C int lynx_resource_response_get_code(
    lynx_resource_response_t* response) {
  return response->code;
}

LYNX_EXTERN_C const char* lynx_resource_response_get_error_message(
    lynx_resource_response_t* response) {
  return response->error_message.c_str();
}

LYNX_EXTERN_C const uint8_t* lynx_resource_response_get_data(
    lynx_resource_response_t* response) {
  return response->data.content;
}

LYNX_EXTERN_C size_t
lynx_resource_response_get_data_length(lynx_resource_response_t* response) {
  return response->data.length;
}

LYNX_EXTERN_C void lynx_resource_response_set_template_bundle(
    lynx_resource_response_t* response, lynx_template_bundle_t* bundle) {
  response->template_bundle = bundle->template_bundle;
}

LYNX_EXTERN_C void lynx_resource_response_callback(
    lynx_resource_response_t* response) {
  if (response->completed) {
    return;
  }
  response->completed = true;
  if (response->callback) {
    response->callback(response);
  }
}

LYNX_EXTERN_C void lynx_resource_response_release(
    lynx_resource_response_t* response) {
  // complete the response first if necessary.
  lynx_resource_response_callback(response);
  lynx_resource_response_release_data(response);
  delete response;
}
