// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_LYNX_RESOURCE_RESPONSE_H_
#define PLATFORM_EMBEDDER_PUBLIC_LYNX_RESOURCE_RESPONSE_H_

#include "capi/lynx_resource_response_capi.h"

namespace lynx {
namespace pub {

/**
 * @apidoc
 * @brief `LynxResourceResponse` is a class that represents a resource response.
 */
class LynxResourceResponse {
 public:
  explicit LynxResourceResponse(lynx_resource_response_t* response)
      : response_(response) {}

  ~LynxResourceResponse() { lynx_resource_response_release(response_); }

  /**
   * @apidoc
   * @brief Set the code of the resource response.
   * @param code The code of the resource response.
   */
  void SetCode(int code) { lynx_resource_response_set_code(response_, code); }

  /**
   * @apidoc
   * @brief Set the error message of the resource response.
   * @param msg The error message of the resource response.
   */
  void SetErrorMessage(const char* msg) {
    lynx_resource_response_set_error_message(response_, msg);
  }

  /**
   * @apidoc
   * @brief Set the data of the resource response.
   * @param content The data of the resource response.
   * @param length The length of the data of the resource response.
   * @param dtor The destructor of the data of the resource response.
   * @param opaque The opaque data of the resource response.
   */
  void SetData(uint8_t* content, size_t length,
               void (*dtor)(uint8_t*, size_t, void*) = nullptr,
               void* opaque = nullptr) {
    lynx_resource_response_set_data(response_, content, length, dtor, opaque);
  }

  void Complete() { lynx_resource_response_callback(response_); }

 private:
  lynx_resource_response_t* response_;
};
}  // namespace pub
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PUBLIC_LYNX_RESOURCE_RESPONSE_H_
