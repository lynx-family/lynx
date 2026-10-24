// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_INTERFACES_RESOURCE_PROVIDER_H_
#define PLATFORM_EMBEDDER_INTERFACES_RESOURCE_PROVIDER_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "platform/embedder/public/capi/lynx_resource_request_capi.h"

namespace lynx::embedder {
// SDK-private capability. The shared buffer keeps its provider-owned storage
// alive across task dispatch. Releasing the last copy releases that storage.
struct ResourceData {
  std::shared_ptr<const uint8_t> bytes;
  size_t size = 0;
};
// The provider owns the fetcher and may complete on any thread.
class ResourceProvider {
 public:
  using Callback = std::function<void(ResourceData)>;
  virtual ~ResourceProvider() = default;
  virtual void Load(const std::string& url, lynx_resource_type_e type,
                    Callback callback) = 0;
  virtual std::string Intercept(const std::string& url, bool decode) = 0;
};
}  // namespace lynx::embedder

#endif  // PLATFORM_EMBEDDER_INTERFACES_RESOURCE_PROVIDER_H_
