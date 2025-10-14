// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_NET_LOADER_RESOURCE_LOADER_H_
#define CLAY_NET_LOADER_RESOURCE_LOADER_H_

#include <functional>
#include <string>

#include "clay/net/resource_type.h"

namespace clay {

class ResourceLoader {
 public:
  ResourceLoader() = default;
  virtual ~ResourceLoader() = default;

  virtual void Load(const std::string& src,
                    const std::function<void(const uint8_t*, size_t)>& callback,
                    const ResourceType resource_type = ResourceType::kOthers,
                    bool need_redirect = false) = 0;

  virtual RawResource LoadSync(const std::string& src,
                               const ResourceType = ResourceType::kOthers,
                               bool need_redirect = false) = 0;

  // Some loader may not support cancel yet.
  virtual void CancelAll() {}

  virtual bool NeedInterceptUrl() { return false; }
};
}  // namespace clay

#endif  // CLAY_NET_LOADER_RESOURCE_LOADER_H_
