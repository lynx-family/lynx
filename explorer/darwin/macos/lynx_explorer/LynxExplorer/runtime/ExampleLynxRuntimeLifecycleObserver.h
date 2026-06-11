// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef EXPLORER_DARWIN_MACOS_EXAMPLE_RUNTIME_LIFECYCLE_OBSERVER_H_
#define EXPLORER_DARWIN_MACOS_EXAMPLE_RUNTIME_LIFECYCLE_OBSERVER_H_

#include <cstdint>

#include "lynx_runtime_lifecycle_observer.h"
#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_defines.h"
#endif

namespace lynx {
namespace example {
class ExampleLynxRuntimeLifecycleObserver
    : public pub::LynxRuntimeLifecycleObserver {
 public:
  explicit ExampleLynxRuntimeLifecycleObserver(uint64_t token_id = 0)
      : token_id_(token_id) {}

  void OnRuntimeAttach(napi_env env) override;
  void OnRuntimeDetach() override;

 private:
  uint64_t token_id_ = 0;
};
}  // namespace example
}  // namespace lynx
#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_undefs.h"
#endif

#endif  // EXPLORER_DARWIN_MACOS_EXAMPLE_RUNTIME_LIFECYCLE_OBSERVER_H_
