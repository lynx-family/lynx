// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_FETCHER_LYNX_RESOURCE_RESPONSE_PRIV_H_
#define PLATFORM_EMBEDDER_FETCHER_LYNX_RESOURCE_RESPONSE_PRIV_H_

#include <memory>
#include <string>

#include "platform/embedder/lynx_template_bundle_priv.h"
#include "platform/embedder/public/capi/lynx_resource_response_capi.h"

using ResourceCallback = std::function<void(lynx_resource_response_t*)>;

struct lynx_resource_response_t {
  int code = -1;
  std::string error_message;

  struct data {
    uint8_t* content = nullptr;
    size_t length = 0;
    void (*dtor)(uint8_t*, size_t, void*) = nullptr;
    void* opaque = nullptr;
  } data;

  std::shared_ptr<lynx::tasm::LynxTemplateBundle> template_bundle;

  ResourceCallback callback = nullptr;
  bool completed = false;
};

lynx_resource_response_t* lynx_resource_response_create_internal(
    ResourceCallback);

lynx_resource_response_t* lynx_resource_response_create_swap(
    lynx_resource_response_t* response);

#endif  // PLATFORM_EMBEDDER_FETCHER_LYNX_RESOURCE_RESPONSE_PRIV_H_
