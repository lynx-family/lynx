// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_RUNTIME_PLATFORM_ANDROID_HOST_SCRIPT_RESOURCE_INTERCEPTOR_H_
#define CORE_SHELL_HOST_SCRIPT_RUNTIME_PLATFORM_ANDROID_HOST_SCRIPT_RESOURCE_INTERCEPTOR_H_

#include "core/resource/lynx_resource_load_hook.h"

namespace lynx::shell {

bool InterceptResourceLoad(pub::LynxResourceLoader& loader,
                           const pub::LynxResourceRequest& request,
                           base::LynxEntityId view, HostScriptDomain domain,
                           pub::ResourceLoadCallback& callback);

}  // namespace lynx::shell
#endif  // CORE_SHELL_HOST_SCRIPT_RUNTIME_PLATFORM_ANDROID_HOST_SCRIPT_RESOURCE_INTERCEPTOR_H_
