// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/js_debug/js/native_module_record_observer_impl.h"

#include "devtool/lynx_devtool/js_debug/js/inspector_runtime_observer_impl.h"
#include "devtool/lynx_devtool/native_module/native_module_invocation_context_impl.h"
#include "devtool/lynx_devtool/native_module/native_module_record_builder.h"

namespace lynx {
namespace devtool {

NativeModuleRecordObserverImpl::NativeModuleRecordObserverImpl(
    const std::shared_ptr<InspectorRuntimeObserverImpl>& observer)
    : observer_wp_(observer) {}

void NativeModuleRecordObserverImpl::OnRecord(const lepus::Value& record) {
  if (auto observer = observer_wp_.lock()) {
    observer->OnNativeModuleRecord(record);
  }
}

std::shared_ptr<runtime::js::NativeModuleInvocationContext>
NativeModuleRecordObserverImpl::CreateInvocation(
    const std::string& module_name, const std::string& method_name) {
  return std::make_shared<NativeModuleInvocationContextImpl>(
      shared_from_this(), module_name, method_name);
}

void NativeModuleRecordObserverImpl::OnGlobalEvent(
    const std::string& name, const lepus::Value& arguments) {
  OnRecord(BuildGlobalEventRecord(name, arguments));
}

}  // namespace devtool
}  // namespace lynx
