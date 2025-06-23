// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/public/pub_lynx_resource_loader_harmony.h"

#include <memory>

#include "core/public/lynx_resource_loader.h"
#include "core/resource/lynx_resource_loader_harmony.h"

namespace lynx {
namespace tasm {
namespace harmony {

PubLynxResourceLoaderHarmony::PubLynxResourceLoaderHarmony(
    napi_env env, napi_value resource_loader) {
  resource_loader_ = std::make_shared<lynx::harmony::LynxResourceLoaderHarmony>(
      env, resource_loader);
}

const std::shared_ptr<lynx::pub::LynxResourceLoader>&
PubLynxResourceLoaderHarmony::ResourceLoader() const {
  return resource_loader_;
}

PubLynxResourceLoaderHarmony::~PubLynxResourceLoaderHarmony() {
  if (resource_loader_) {
    static_cast<lynx::harmony::LynxResourceLoaderHarmony*>(
        resource_loader_.get())
        ->DeleteRef();
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
