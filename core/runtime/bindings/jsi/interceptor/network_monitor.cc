// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/interceptor/network_monitor.h"

#include <string>

namespace lynx {
namespace piper {
namespace network {

// Report external network response information
void ReportRequestSuccessIfNecessary(const NativeModuleInfoCollectorPtr& timing,
                                     ModuleCallback* callback) {
  // TODO (huzhanbo.luc): impl the network monitor
}

void SetNetworkCallbackInfo(
    const std::string& method_name, const std::unique_ptr<pub::Value>& args,
    size_t count, const NativeModuleInfoCollectorPtr& timing_collector) {
  // TODO (huzhanbo.luc): impl the network monitor
}

}  // namespace network
}  // namespace piper
}  // namespace lynx
