// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_FETCHER_LYNX_GENERIC_RESOURCE_FETCHER_PRIV_H_
#define PLATFORM_EMBEDDER_FETCHER_LYNX_GENERIC_RESOURCE_FETCHER_PRIV_H_

#include <atomic>
#include <string>

#include "platform/embedder/fetcher/lynx_resource_request_priv.h"
#include "platform/embedder/fetcher/lynx_resource_response_priv.h"
#include "platform/embedder/public/capi/lynx_generic_resource_fetcher_capi.h"

struct lynx_generic_resource_fetcher_t {
  void* user_data = nullptr;
  void (*finalizer)(lynx_generic_resource_fetcher_t*, void*) = nullptr;
  std::atomic_int ref_count = 0;

  fetch_resource_func fetch_resource = nullptr;
  fetch_resource_func fetch_resource_path = nullptr;
  cancel_fetch_func cancel_fetch = nullptr;

  lynx_resource_intercept_func intercept_func = nullptr;
};

void lynx_generic_resource_fetcher_ref(
    lynx_generic_resource_fetcher_t* fetcher);

void lynx_generic_resource_fetcher_unref(
    lynx_generic_resource_fetcher_t* fetcher);

#endif  // PLATFORM_EMBEDDER_FETCHER_LYNX_GENERIC_RESOURCE_FETCHER_PRIV_H_
