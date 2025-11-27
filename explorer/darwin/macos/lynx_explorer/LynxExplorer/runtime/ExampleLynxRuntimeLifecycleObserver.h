// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef EXPLORER_DARWIN_MACOS_EXAMPLE_RUNTIME_LIFECYCLE_OBSERVER_H_
#define EXPLORER_DARWIN_MACOS_EXAMPLE_RUNTIME_LIFECYCLE_OBSERVER_H_

#include "lynx_runtime_lifecycle_observer.h"
#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
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
#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_undefs.h"
#endif

#endif  // EXPLORER_DARWIN_MACOS_EXAMPLE_RUNTIME_LIFECYCLE_OBSERVER_H_
