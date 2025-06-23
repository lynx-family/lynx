// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_PUBLIC_PUB_LYNX_RESOURCE_LOADER_HARMONY_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_PUBLIC_PUB_LYNX_RESOURCE_LOADER_HARMONY_H_

#include <node_api.h>

#include <memory>

#include "base/include/base_export.h"

namespace lynx {
namespace pub {
class LynxResourceLoader;
}
namespace tasm {
namespace harmony {
class BASE_EXPORT PubLynxResourceLoaderHarmony {
 public:
  PubLynxResourceLoaderHarmony(napi_env env, napi_value resource_loader);

  ~PubLynxResourceLoaderHarmony();

  const std::shared_ptr<pub::LynxResourceLoader>& ResourceLoader() const;

 private:
  std::shared_ptr<pub::LynxResourceLoader> resource_loader_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_PUBLIC_PUB_LYNX_RESOURCE_LOADER_HARMONY_H_
