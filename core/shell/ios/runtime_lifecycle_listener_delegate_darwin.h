// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_IOS_RUNTIME_LIFECYCLE_LISTENER_DELEGATE_DARWIN_H_
#define CORE_SHELL_IOS_RUNTIME_LIFECYCLE_LISTENER_DELEGATE_DARWIN_H_

#import <Lynx/LynxRuntimeLifecycleListener.h>
#import <Lynx/LynxView.h>

#include <memory>

#import "LynxErrorReceiverProtocol.h"
#include "core/public/vsync_observer_interface.h"
#include "core/runtime/piper/js/runtime_lifecycle_listener_delegate.h"
#include "third_party/binding/napi/shim/shim_napi.h"

namespace lynx {
namespace shell {
class RuntimeLifecycleListenerDelegateDarwin
    : public runtime::RuntimeLifecycleListenerDelegate {
 public:
  RuntimeLifecycleListenerDelegateDarwin(
      id<LynxRuntimeLifecycleListener> listener,
      id<LynxErrorReceiverProtocol> error_handler)
      : RuntimeLifecycleListenerDelegate(
            runtime::RuntimeLifecycleListenerDelegate::DelegateType::PART),
        _listener(listener),
        _error_handler(error_handler) {}
  ~RuntimeLifecycleListenerDelegateDarwin() override = default;

  void OnRuntimeCreate(
      std::shared_ptr<runtime::IVSyncObserver> observer) final{};
  void OnRuntimeInit(int64_t runtime_id) final{};
  void OnAppEnterForeground() final{};
  void OnAppEnterBackground() final{};
  void OnRuntimeAttach(Napi::Env env) override;
  void OnRuntimeDetach() override;

 private:
  void OnError(NSException* e);
  id<LynxRuntimeLifecycleListener> _listener;
  __weak id<LynxErrorReceiverProtocol> _error_handler;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_IOS_RUNTIME_LIFECYCLE_LISTENER_DELEGATE_DARWIN_H_
