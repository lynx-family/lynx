// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_RESOURCE_RESOURCE_PROVIDER_EMBEDDER_H_
#define PLATFORM_EMBEDDER_RESOURCE_RESOURCE_PROVIDER_EMBEDDER_H_

#include <memory>

#include "platform/embedder/interfaces/resource_provider.h"
#include "platform/embedder/public/capi/lynx_generic_resource_fetcher_capi.h"

namespace lynx::embedder {
std::shared_ptr<ResourceProvider> CreateResourceProvider(
    lynx_generic_resource_fetcher_t* fetcher);
}  // namespace lynx::embedder

#endif  // PLATFORM_EMBEDDER_RESOURCE_RESOURCE_PROVIDER_EMBEDDER_H_
