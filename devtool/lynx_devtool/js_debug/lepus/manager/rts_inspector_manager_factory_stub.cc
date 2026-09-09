// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

namespace lynx {
namespace devtool {
class JSDebugHelper;
}  // namespace devtool
}  // namespace lynx

extern "C" __attribute__((visibility("default"))) void
LynxRegisterRTSInspectorManagerFactoryImpl(
    lynx::devtool::JSDebugHelper* helper) {}

// On iOS the registration entry point lives in
// rts_inspector_manager_factory_stub_ios.cc, which is compiled alongside this
// file; defining it here as well produces a duplicate symbol when the
// framework is linked dynamically.
#if !OS_IOS
extern "C" void LynxRegisterRTSInspectorManagerFactory(void) {
  auto* anchor = &LynxRegisterRTSInspectorManagerFactoryImpl;
  (void)anchor;
}
#endif
