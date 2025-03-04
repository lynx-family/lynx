// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "core/shell/ios/js_proxy_darwin.h"

#include <atomic>
#include <unordered_map>
#include "base/include/no_destructor.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/tasm/config.h"
#include "core/services/long_task_timing/long_task_monitor.h"

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/resource/lazy_bundle/lazy_bundle_utils.h"

namespace lynx {
namespace shell {

std::shared_ptr<JSProxyDarwin> JSProxyDarwin::Create(
    const std::shared_ptr<LynxActor<runtime::LynxRuntime>>& actor, LynxView* lynx_view, int64_t id,
    const std::string& js_group_thread_name, bool runtime_standalone_mode) {
  // constructor is private, cannot use std::make_shared
  auto js_proxy = std::shared_ptr<JSProxyDarwin>(
      new JSProxyDarwin(actor, lynx_view, id, js_group_thread_name, runtime_standalone_mode));
  return js_proxy;
}

JSProxyDarwin::JSProxyDarwin(const std::shared_ptr<LynxActor<runtime::LynxRuntime>>& actor,
                             LynxView* lynx_view, int64_t id,
                             const std::string& js_group_thread_name, bool runtime_standalone_mode)
    : LynxRuntimeProxyImpl(actor, runtime_standalone_mode),
      lynx_view_(lynx_view),
      id_(id),
      js_group_thread_name_(js_group_thread_name) {}

void JSProxyDarwin::RunOnJSThread(dispatch_block_t task) {
  actor_->Act([task](auto&) { task(); });
}

}  // namespace shell
}  // namespace lynx
