// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_NATIVE_MODULE_RECORD_OBSERVER_IMPL_H_
#define DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_NATIVE_MODULE_RECORD_OBSERVER_IMPL_H_

#include <memory>

#include "core/inspector/observer/native_module_record_observer.h"

namespace lynx {
namespace devtool {
class InspectorRuntimeObserverImpl;

class NativeModuleRecordObserverImpl
    : public runtime::js::NativeModuleRecordObserver,
      public std::enable_shared_from_this<NativeModuleRecordObserverImpl> {
 public:
  explicit NativeModuleRecordObserverImpl(
      const std::shared_ptr<InspectorRuntimeObserverImpl>& observer);
  ~NativeModuleRecordObserverImpl() override = default;

  void OnRecord(const lepus::Value& record) override;

  std::shared_ptr<runtime::js::NativeModuleInvocationContext> CreateInvocation(
      const std::string& module_name, const std::string& method_name) override;
  void OnGlobalEvent(const std::string& name,
                     const lepus::Value& arguments) override;

 private:
  std::weak_ptr<InspectorRuntimeObserverImpl> observer_wp_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_NATIVE_MODULE_RECORD_OBSERVER_IMPL_H_
