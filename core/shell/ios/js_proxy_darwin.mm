// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "core/shell/ios/js_proxy_darwin.h"
#import <Lynx/LynxRuntimeFullLifecycleListener.h>

#include <atomic>
#include <unordered_map>
#include "base/include/no_destructor.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/tasm/config.h"
#include "core/services/long_task_timing/long_task_monitor.h"

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/resource/lazy_bundle/lazy_bundle_utils.h"
#include "lynx/core/runtime/piper/js/runtime_lifecycle_listener_delegate.h"
#include "lynx/core/shell/ios/runtime_lifecycle_listener_delegate_darwin.h"

namespace lynx {
namespace shell {

std::shared_ptr<JSProxyDarwin> JSProxyDarwin::Create(
    const std::shared_ptr<LynxActor<runtime::LynxRuntime>>& actor,
    id<LynxErrorReceiverProtocol> error_handler, int64_t id,
    const std::string& js_group_thread_name, bool runtime_standalone_mode) {
  // constructor is private, cannot use std::make_shared
  auto js_proxy = std::shared_ptr<JSProxyDarwin>(
      new JSProxyDarwin(actor, error_handler, id, js_group_thread_name, runtime_standalone_mode));
  return js_proxy;
}

JSProxyDarwin::JSProxyDarwin(const std::shared_ptr<LynxActor<runtime::LynxRuntime>>& actor,
                             id<LynxErrorReceiverProtocol> error_handler, int64_t id,
                             const std::string& js_group_thread_name, bool runtime_standalone_mode)
    : LynxRuntimeProxyImpl(actor, runtime_standalone_mode),
      _error_handler(error_handler),
      id_(id),
      js_group_thread_name_(js_group_thread_name) {}

void JSProxyDarwin::RunOnJSThread(dispatch_block_t task) {
  actor_->Act([task](auto&) { task(); });
}

void JSProxyDarwin::AddLifecycleListener(id<LynxRuntimeLifecycleListener> listener) {
  if (!actor_) {
    return;
  }
  runtime::RuntimeLifecycleListenerDelegate::DelegateType type;
  if ([listener conformsToProtocol:@protocol(LynxRuntimeFullLifecycleListener)]) {
    type = runtime::RuntimeLifecycleListenerDelegate::DelegateType::FULL;
  } else {
    type = runtime::RuntimeLifecycleListenerDelegate::DelegateType::PART;
  }
  auto delegate = std::make_unique<lynx::shell::RuntimeLifecycleListenerDelegateDarwin>(
      listener, type, _error_handler);
  actor_->Act([delegate = std::move(delegate)](auto& runtime) mutable {
    runtime->AddLifecycleListener(std::move(delegate));
  });
}

}  // namespace shell
}  // namespace lynx
