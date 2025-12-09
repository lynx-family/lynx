// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_SERVICE_CENTER_PRIV_H_
#define PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_SERVICE_CENTER_PRIV_H_

#include <unordered_map>

#include "platform/embedder/lynx_service/lynx_service_base.h"
#include "platform/embedder/public/capi/lynx_service_center_capi.h"

struct lynx_service_center_t {
  std::unordered_map<lynx_service_type_e, lynx::embedder::LynxServiceBase*>
      services;
};

#endif  // PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_SERVICE_CENTER_PRIV_H_
