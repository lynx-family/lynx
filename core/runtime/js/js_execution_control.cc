// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/js_execution_control.h"

#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>

#include "base/include/fml/message_loop.h"
#include "base/include/no_destructor.h"
#include "core/base/threading/task_runner_manufactor.h"

namespace lynx {
namespace runtime {
namespace js {
namespace {

class VMInstanceRegistry {
 public:
  static VMInstanceRegistry& GetInstance() {
    static base::NoDestructor<VMInstanceRegistry> instance;
    return *instance;
  }

  void Register(fml::MessageLoopImpl* js_loop,
                const std::shared_ptr<VMInstance>& vm) {
    std::lock_guard<std::mutex> lock(mutex_);
    vm_instances_by_loop_[js_loop] = vm;
  }

  void Unregister(fml::MessageLoopImpl* js_loop, VMInstance* vm) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto loop_it = vm_instances_by_loop_.find(js_loop);
    if (loop_it == vm_instances_by_loop_.end()) {
      return;
    }
    auto registered_vm = loop_it->second.lock();
    if (!registered_vm || registered_vm.get() == vm) {
      vm_instances_by_loop_.erase(loop_it);
    }
  }

  std::shared_ptr<VMInstance> Acquire(fml::MessageLoopImpl* js_loop) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = vm_instances_by_loop_.find(js_loop);
    return it == vm_instances_by_loop_.end() ? nullptr : it->second.lock();
  }

 private:
  friend class base::NoDestructor<VMInstanceRegistry>;

  VMInstanceRegistry() = default;

  std::mutex mutex_;
  std::unordered_map<fml::MessageLoopImpl*, std::weak_ptr<VMInstance>>
      vm_instances_by_loop_;
};

fml::MessageLoopImpl* GetCurrentJSLoop() {
  auto* loop = fml::MessageLoop::IsInitializedForCurrentThread();
  return loop == nullptr ? nullptr : loop->GetLoopImpl().get();
}

}  // namespace

void RegisterVMInstance(const std::shared_ptr<VMInstance>& vm) {
  auto* js_loop = GetCurrentJSLoop();
  if (!vm || js_loop == nullptr) {
    return;
  }
  VMInstanceRegistry::GetInstance().Register(js_loop, vm);
}

void UnregisterVMInstance(VMInstance* vm) {
  if (vm == nullptr) {
    return;
  }
  auto* js_loop = GetCurrentJSLoop();
  if (js_loop == nullptr) {
    return;
  }
  VMInstanceRegistry::GetInstance().Unregister(js_loop, vm);
}

void CaptureJavaScriptStack(
    const std::string& js_group_thread_name,
    base::MoveOnlyClosure<void, JSStackCaptureResult, std::string> callback) {
  auto completion = std::make_shared<
      base::MoveOnlyClosure<void, JSStackCaptureResult, std::string>>(
      std::move(callback));
  auto js_runner =
      base::TaskRunnerManufactor::GetJSRunner(js_group_thread_name);
  auto vm =
      VMInstanceRegistry::GetInstance().Acquire(js_runner->GetLoop().get());
  if (!vm) {
    // The VM was destroyed before the request reached the JS engine.
    (*completion)(JSStackCaptureResult::kVMDestroyed, {});
    return;
  }
  bool accepted =
      vm->CaptureJavaScriptStack([completion](std::string stack) mutable {
        (*completion)(JSStackCaptureResult::kSuccess, std::move(stack));
      });
  if (!accepted) {
    // The VM is alive but declined to return a stack (e.g. not running JS).
    (*completion)(JSStackCaptureResult::kStackUnavailable, {});
  }
}

bool TerminateJavaScriptExecution(const std::string& js_group_thread_name) {
  auto js_runner =
      base::TaskRunnerManufactor::GetJSRunner(js_group_thread_name);
  auto vm =
      VMInstanceRegistry::GetInstance().Acquire(js_runner->GetLoop().get());
  return vm && vm->TerminateJavaScriptExecution();
}

}  // namespace js
}  // namespace runtime
}  // namespace lynx
