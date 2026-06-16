// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <mutex>

#include "primjs_weak_node_api_installer.h"
#include "primjs_weak_node_api_provider.h"

namespace {

const void* PrimJSProvideWeakNodeApiRawPtrHost() {
  return PrimJSGetWeakNodeApiRawPtrHost();
}

void InstallPrimJSWeakNodeApiBridge() {
  static std::once_flag once_flag;
  std::call_once(once_flag, []() {
    // Register PrimJS as the raw host provider before Lynx creates any runtime.
    PrimJSInstallWeakNodeApiRawPtrHostProvider(
        PrimJSProvideWeakNodeApiRawPtrHost);

    // Initialize the weak Node-API side once so later runtimes can reuse it.
    SetupWeakNodeApiEnv();
  });
}

}  // namespace

extern "C" __attribute__((constructor)) void
InstallPrimJSWeakNodeApiBridgeForApple() {
  InstallPrimJSWeakNodeApiBridge();
}
