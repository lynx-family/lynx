// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_PIPER_JS_RUNTIME_LIFECYCLE_OBSERVER_IMPL_H_
#define CORE_RUNTIME_PIPER_JS_RUNTIME_LIFECYCLE_OBSERVER_IMPL_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "base/include/base_export.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/task_runner.h"
#include "core/public/vsync_observer_interface.h"
#include "lynx/core/public/runtime_lifecycle_observer.h"
#include "lynx/core/runtime/piper/js/runtime_lifecycle_listener_delegate.h"

namespace Napi {
class Env;
}

namespace lynx {
namespace runtime {

class RuntimeLifecycleObserverImpl : public RuntimeLifecycleObserver {
 public:
  RuntimeLifecycleObserverImpl() = default;
  ~RuntimeLifecycleObserverImpl() override = default;

  // run on js thread.
  void OnRuntimeCreate(std::shared_ptr<IVSyncObserver> observer) override;
  void OnRuntimeInit(int64_t runtime_id) override;
  void OnAppEnterForeground() override;
  void OnAppEnterBackground() override;
  void OnRuntimeAttach(Napi::Env env) override;
  void OnRuntimeDetach() override;

  void AddEventListener(
      std::unique_ptr<RuntimeLifecycleListenerDelegate> listener);

  enum LifecycleState {
    CREATE = 1 << 0,
    INIT = 1 << 1,
    ATTACH = 1 << 2,
    DETACH = 1 << 3,
    ENTER_FOREGROUND = 1 << 4,
    ENTER_BACKGROUND = 1 << 5,
  };

 private:
  void NotifyListenerChanged();

  std::unordered_map<std::unique_ptr<RuntimeLifecycleListenerDelegate>, int>
      delegates_;
  std::vector<LifecycleState> event_record_;

  std::shared_ptr<IVSyncObserver> args_vsync_observer_;
  int64_t args_runtime_id_;
  void* args_env_;
};

}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_PIPER_JS_RUNTIME_LIFECYCLE_OBSERVER_IMPL_H_
