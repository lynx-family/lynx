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
