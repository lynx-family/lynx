// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_INTERCEPTOR_NETWORK_MONITOR_H_
#define CORE_RUNTIME_BINDINGS_JSI_INTERCEPTOR_NETWORK_MONITOR_H_

#include <memory>
#include <string>

#include "core/public/prop_bundle.h"
#include "core/runtime/bindings/jsi/modules/lynx_module.h"
#include "lynx/base/include/debug/lynx_error.h"

namespace lynx {
namespace piper {
namespace network {

void SetNetworkCallbackInfo(
    const std::string& method_name, const std::unique_ptr<pub::Value>& args,
    size_t count, const NativeModuleInfoCollectorPtr& timing_collector);

// Report external network response information
void ReportRequestSuccessIfNecessary(const NativeModuleInfoCollectorPtr& timing,
                                     ModuleCallback* callback);

}  // namespace network
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_INTERCEPTOR_NETWORK_MONITOR_H_
