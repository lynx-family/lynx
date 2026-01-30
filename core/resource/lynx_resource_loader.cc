// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/public/lynx_resource_loader.h"

#include "core/renderer/utils/lynx_env.h"

namespace lynx {
namespace pub {

void LynxResourceLoader::LoadResource(
    const LynxResourceRequest& request,
    base::MoveOnlyClosure<void, LynxResourceResponse&> callback) {
  // TODO: impl cache logic here;
  LoadResourceInternal(request, std::move(callback));
}

void LynxResourceLoader::LoadResourcePath(
    const LynxResourceRequest& request,
    base::MoveOnlyClosure<void, LynxPathResponse&> path_callback) {
  // TODO: impl cache logic here;
  LoadResourcePathInternal(request, std::move(path_callback));
}
}  // namespace pub
}  // namespace lynx
