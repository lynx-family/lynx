// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_LYNX_RESOURCE_REQUEST_H_
#define PLATFORM_EMBEDDER_PUBLIC_LYNX_RESOURCE_REQUEST_H_

#include "capi/lynx_resource_request_capi.h"

namespace lynx {
namespace pub {

/**
 * @apidoc
 * @brief `LynxResourceRequest` is a class that represents a resource request.
 */
class LynxResourceRequest {
 public:
  explicit LynxResourceRequest(lynx_resource_request_t* request)
      : request_(request) {}

  ~LynxResourceRequest() { lynx_resource_request_release(request_); }

  /**
   * @apidoc
   * @brief Get the request id of the resource request.
   * @return The request id of the resource request.
   */
  lynx_resource_request_id GetRequestId() {
    return lynx_resource_request_get_id(request_);
  }

  /**
   * @apidoc
   * @brief Get the URL of the resource request.
   * @return The URL of the resource request.
   */
  const char* GetUrl() { return lynx_resource_request_get_url(request_); }

  /**
   * @apidoc
   * @brief Get the type of the resource request.
   * @return The type of the resource request.
   */
  lynx_resource_type_e GetType() {
    return lynx_resource_request_get_type(request_);
  }

 private:
  lynx_resource_request_t* request_;
};
}  // namespace pub
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PUBLIC_LYNX_RESOURCE_REQUEST_H_
