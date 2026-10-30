// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <atomic>

#include "platform/embedder/fetcher/lynx_resource_request_priv.h"

namespace {

std::atomic<lynx_resource_request_id> g_next_request_id{1};

lynx_resource_request_id NextResourceRequestId() {
  auto request_id = g_next_request_id.load(std::memory_order_relaxed);
  for (;;) {
    // Zero permanently marks exhaustion; never wrap back to an issued ID.
    if (request_id == kLynxResourceRequestIdInvalid) {
      return kLynxResourceRequestIdInvalid;
    }
    if (g_next_request_id.compare_exchange_weak(request_id, request_id + 1,
                                                std::memory_order_relaxed,
                                                std::memory_order_relaxed)) {
      return request_id;
    }
  }
}

}  // namespace

lynx_resource_request_t* lynx_resource_request_create_internal(
    const std::string& url, lynx_resource_type_e type) {
  const auto request_id = NextResourceRequestId();
  if (request_id == kLynxResourceRequestIdInvalid) {
    return nullptr;
  }
  lynx_resource_request_t* request = new lynx_resource_request_t;
  request->request_id = request_id;
  request->url = url;
  request->resource_type = type;
  return request;
}

LYNX_EXTERN_C lynx_resource_request_t* lynx_resource_request_create(
    const char* url, lynx_resource_type_e type) {
  return lynx_resource_request_create_internal(url ? url : "", type);
}

LYNX_EXTERN_C lynx_resource_request_id
lynx_resource_request_get_id(lynx_resource_request_t* request) {
  return request ? request->request_id : kLynxResourceRequestIdInvalid;
}

LYNX_EXTERN_C lynx_resource_type_e
lynx_resource_request_get_type(lynx_resource_request_t* request) {
  return request->resource_type;
}

LYNX_EXTERN_C const char* lynx_resource_request_get_url(
    lynx_resource_request_t* request) {
  return request->url.c_str();
}

LYNX_EXTERN_C void lynx_resource_request_release(
    lynx_resource_request_t* request) {
  delete request;
}
