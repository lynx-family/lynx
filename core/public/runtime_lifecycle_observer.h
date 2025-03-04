// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_RUNTIME_LIFECYCLE_OBSERVER_H_
#define CORE_PUBLIC_RUNTIME_LIFECYCLE_OBSERVER_H_

#include <memory>

#include "base/include/base_export.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/task_runner.h"
#include "core/public/vsync_observer_interface.h"

namespace Napi {
class Env;
}

namespace lynx {
namespace runtime {

// Runtime lifecycle observer used to listen the events of lynx runtime.
// Triggered on runtime thread
class BASE_EXPORT IRuntimeLifecycleObserver {
 public:
  IRuntimeLifecycleObserver() = default;
  virtual ~IRuntimeLifecycleObserver() = default;

  virtual void OnRuntimeCreate(std::shared_ptr<IVSyncObserver> observer) = 0;
  virtual void OnRuntimeInit(int64_t runtime_id) = 0;
  virtual void OnRuntimeDestroy() = 0;
  virtual void OnAppEnterForeground() = 0;
  virtual void OnAppEnterBackground() = 0;
  virtual void OnRuntimeAttach(Napi::Env env) = 0;
  virtual void OnRuntimeDetach() = 0;
};

}  // namespace runtime
}  // namespace lynx

#endif  // CORE_PUBLIC_RUNTIME_LIFECYCLE_OBSERVER_H_
