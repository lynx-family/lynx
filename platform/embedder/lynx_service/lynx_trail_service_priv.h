// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_TRAIL_SERVICE_PRIV_H_
#define PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_TRAIL_SERVICE_PRIV_H_

#include <atomic>
#include <optional>
#include <string>

#include "platform/embedder/lynx_service/lynx_service_base.h"
#include "platform/embedder/public/capi/lynx_trail_service_capi.h"

struct lynx_trail_service_t : public lynx::embedder::LynxServiceBase {
  lynx_trail_service_t() = default;
  ~lynx_trail_service_t();

  void* user_data = nullptr;
  void (*finalizer)(lynx_trail_service_t*, void*) = nullptr;

  std::atomic<lynx_trail_string_value_func> string_value_func = nullptr;
};

std::optional<std::string> lynx_trail_service_get_string_value(
    lynx_trail_service_t* trail_service, const std::string& key);

#endif  // PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_TRAIL_SERVICE_PRIV_H_
