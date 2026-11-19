// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RESOURCE_LYNX_RESOURCE_LOAD_HOOK_H_
#define CORE_RESOURCE_LYNX_RESOURCE_LOAD_HOOK_H_

#include "base/include/closure.h"
#include "base/include/log/log_context.h"
#include "core/shell/host_script/runtime/host_script_domain.h"

namespace lynx::pub {

class LynxResourceLoader;
struct LynxResourceRequest;
struct LynxResourceResponse;

using ResourceLoadCallback = base::MoveOnlyClosure<void, LynxResourceResponse&>;
// Returning true transfers completion of the request to the hook.
using ResourceLoadHook = bool (*)(LynxResourceLoader&,
                                  const LynxResourceRequest&,
                                  base::LynxEntityId, shell::HostScriptDomain,
                                  ResourceLoadCallback&);

// Implemented by the Android resource loader.
void SetResourceLoadHook(ResourceLoadHook hook);

void LoadResourceWithContext(LynxResourceLoader& loader,
                             const LynxResourceRequest& request,
                             base::LynxEntityId view,
                             shell::HostScriptDomain domain,
                             ResourceLoadCallback callback);

}  // namespace lynx::pub
#endif  // CORE_RESOURCE_LYNX_RESOURCE_LOAD_HOOK_H_
