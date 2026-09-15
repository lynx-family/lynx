// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/js_execution_control.h"

#include <memory>
#include <mutex>
#include <unordered_set>
#include <utility>

#include "base/include/no_destructor.h"

namespace lynx {
namespace runtime {
namespace js {
namespace {

base::NoDestructor<std::mutex> vm_mutex;
base::NoDestructor<std::unordered_set<VMInstance*>> vm_instances;

}  // namespace

void RegisterVMInstance(VMInstance* vm) {
  std::lock_guard<std::mutex> lock(*vm_mutex);
  vm_instances->insert(vm);
}

void UnregisterVMInstance(VMInstance* vm) {
  std::lock_guard<std::mutex> lock(*vm_mutex);
  vm_instances->erase(vm);
}

void CaptureJavaScriptStack(
    VMInstance* vm,
    base::MoveOnlyClosure<void, JSStackCaptureResult, std::string> callback) {
  auto completion = std::make_shared<
      base::MoveOnlyClosure<void, JSStackCaptureResult, std::string>>(
      std::move(callback));
  std::lock_guard<std::mutex> lock(*vm_mutex);
  if (vm_instances->find(vm) == vm_instances->end()) {
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

bool TerminateJavaScriptExecution(VMInstance* vm) {
  std::lock_guard<std::mutex> lock(*vm_mutex);
  // Confirm the VM is still registered before dereferencing it, otherwise the
  // pointer coming from the platform side may be dangling.
  return vm_instances->find(vm) != vm_instances->end() &&
         vm->TerminateJavaScriptExecution();
}

}  // namespace js
}  // namespace runtime
}  // namespace lynx
