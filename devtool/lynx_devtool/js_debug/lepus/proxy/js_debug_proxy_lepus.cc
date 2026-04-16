// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/js_debug/lepus/proxy/js_debug_proxy_lepus.h"

#include "devtool/lynx_devtool/js_debug/lepus/manager/lepus_inspector_manager_impl.h"

namespace lynx {
namespace devtool {

std::unique_ptr<lepus::LepusInspectorManager>
JSDebugProxyLepus::CreateLepusInspectorManager(
    runtime::ContextType context_type) {
  (void)context_type;
  return std::make_unique<lepus::LepusInspectorManagerImpl>();
}

}  // namespace devtool
}  // namespace lynx
