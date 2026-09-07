// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/host_script_js_dispatcher.h"

#include <utility>

namespace lynx {
namespace shell {

std::shared_ptr<HostScriptJsDispatcher> HostScriptJsDispatcher::Create(
    napi_env env) {
  if (!env) {
    return nullptr;
  }
  auto dispatcher = std::unique_ptr<RuntimeDispatcher>(
      RuntimeDispatcher::New(env, nullptr, FinalizeDispatcher));
  return std::shared_ptr<HostScriptJsDispatcher>(
      new HostScriptJsDispatcher(std::move(dispatcher)));
}

HostScriptJsDispatcher::HostScriptJsDispatcher(
    std::unique_ptr<RuntimeDispatcher> dispatcher)
    : dispatcher_(std::move(dispatcher)) {}

HostScriptJsDispatcher::~HostScriptJsDispatcher() { Detach(); }

void HostScriptJsDispatcher::InvokeTask(Napi::Env env, std::nullptr_t*,
                                        Task task) {
  if (env && task) {
    task(env);
  }
}

void HostScriptJsDispatcher::FinalizeDispatcher(Napi::Env, std::nullptr_t*,
                                                std::nullptr_t*) {}

bool HostScriptJsDispatcher::Post(Task task) {
  if (!task) {
    return false;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  return dispatcher_ &&
         dispatcher_->NonBlockingCall(std::move(task)) == napi_ok;
}

void HostScriptJsDispatcher::Detach() {
  std::unique_ptr<RuntimeDispatcher> dispatcher;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    dispatcher = std::move(dispatcher_);
  }
  dispatcher.reset();
}

bool HostScriptJsDispatcher::IsAttached() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return dispatcher_ != nullptr;
}

}  // namespace shell
}  // namespace lynx
