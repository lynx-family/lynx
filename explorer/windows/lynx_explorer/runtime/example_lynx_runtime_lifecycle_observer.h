// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef EXPLORER_WINDOWS_LYNX_EXPLORER_RUNTIME_EXAMPLE_LYNX_RUNTIME_LIFECYCLE_OBSERVER_H_
#define EXPLORER_WINDOWS_LYNX_EXPLORER_RUNTIME_EXAMPLE_LYNX_RUNTIME_LIFECYCLE_OBSERVER_H_

#include "lynx_runtime_lifecycle_observer.h"

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_defines.h"
#endif

namespace lynx {
namespace example {
class ExampleLynxRuntimeLifecycleObserver
    : public pub::LynxRuntimeLifecycleObserver {
 public:
  void OnRuntimeAttach(napi_env env) override;
  void OnRuntimeDetach() override;
};
}  // namespace example
}  // namespace lynx
#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_undefs.h"
#endif

#endif  // EXPLORER_WINDOWS_LYNX_EXPLORER_RUNTIME_EXAMPLE_LYNX_RUNTIME_LIFECYCLE_OBSERVER_H_
