// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/fetcher/lynx_resource_request_priv.h"

lynx_resource_request_t* lynx_resource_request_create_internal(
    const std::string& url, lynx_resource_type_e type) {
  lynx_resource_request_t* request = new lynx_resource_request_t;
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
  return reinterpret_cast<uint64_t>(request);
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
