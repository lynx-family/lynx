// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/js_debug/js/native_module_record_observer_impl.h"

#include "devtool/lynx_devtool/js_debug/js/inspector_runtime_observer_impl.h"

namespace lynx {
namespace devtool {

NativeModuleRecordObserverImpl::NativeModuleRecordObserverImpl(
    const std::shared_ptr<InspectorRuntimeObserverImpl>& observer)
    : observer_wp_(observer) {}

void NativeModuleRecordObserverImpl::OnRecord(const lepus::Value& record) {
  auto observer = observer_wp_.lock();
  if (observer) {
    observer->OnNativeModuleRecord(record);
  }
}

}  // namespace devtool
}  // namespace lynx
