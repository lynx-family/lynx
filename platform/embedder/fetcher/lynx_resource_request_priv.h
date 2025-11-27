// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_FETCHER_LYNX_RESOURCE_REQUEST_PRIV_H_
#define PLATFORM_EMBEDDER_FETCHER_LYNX_RESOURCE_REQUEST_PRIV_H_

#include <string>

#include "platform/embedder/public/capi/lynx_resource_request_capi.h"

struct lynx_resource_request_t {
  std::string url;
  lynx_resource_type_e resource_type = kLynxResourceTypeGeneric;
};

lynx_resource_request_t* lynx_resource_request_create_internal(
    const std::string& url, lynx_resource_type_e type);

#endif  // PLATFORM_EMBEDDER_FETCHER_LYNX_RESOURCE_REQUEST_PRIV_H_
