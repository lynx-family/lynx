// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <utility>

#include "core/public/lynx_resource_loader.h"
#include "core/resource/lynx_resource_load_hook.h"

namespace lynx::pub {

void LoadResourceWithContext(LynxResourceLoader& loader,
                             const LynxResourceRequest& request,
                             base::LynxEntityId, shell::HostScriptDomain,
                             ResourceLoadCallback callback) {
  loader.LoadResource(request, std::move(callback));
}

}  // namespace lynx::pub
