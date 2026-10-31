// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_RUNTIME_PLATFORM_ANDROID_HOST_SCRIPT_RUNTIME_CALLBACKS_H_
#define CORE_SHELL_HOST_SCRIPT_RUNTIME_PLATFORM_ANDROID_HOST_SCRIPT_RUNTIME_CALLBACKS_H_

#include <cstdint>

namespace lynx {
namespace shell {

// Registered through private JNI, never through an additional exported symbol.
// The provider and this immutable table stay loaded for the process lifetime.
struct HostScriptRuntimeCallbacks {
  uint32_t abi_version;
  uint32_t executor_features;
  void (*prepare)(bool view_created);
  void (*update_debug_state)(bool enabled);
};

constexpr uint32_t kHostScriptRuntimeAbiVersion = 6;
// Engine-owned executor allocation isolates replay/recorder field differences.
// Shared inline types still require matching build features.
constexpr uint32_t kHostScriptExecutorFeatures =
    (ENABLE_INSPECTOR ? 4u : 0u) | (ENABLE_NAPI_BINDING ? 32u : 0u) |
#if defined(ENABLE_CLAY) && ENABLE_CLAY
    8u |
#endif
#ifndef NDEBUG
    16u |
#endif
    0u;

// Engine-private entry points; no dynamic visibility is added.
// commit=false validates without publishing the provider or preparing runtimes.
bool SetHostScriptRuntimeProvider(const HostScriptRuntimeCallbacks* callbacks,
                                  bool commit);

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_HOST_SCRIPT_RUNTIME_PLATFORM_ANDROID_HOST_SCRIPT_RUNTIME_CALLBACKS_H_
