// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "../../helper/js_debug_helper.h"
#include "devtool/lynx_devtool/js_debug/lepus/manager/rts_inspector_manager_factory.h"

extern "C" void LynxRegisterRTSInspectorManagerFactoryImpl(
    lynx::devtool::JSDebugHelper* helper);

extern "C" void LynxRegisterRTSInspectorManagerFactory(void) {
  auto* anchor = &LynxRegisterRTSInspectorManagerFactoryImpl;
  (void)anchor;
}
