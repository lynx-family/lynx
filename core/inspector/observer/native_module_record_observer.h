// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_INSPECTOR_OBSERVER_NATIVE_MODULE_RECORD_OBSERVER_H_
#define CORE_INSPECTOR_OBSERVER_NATIVE_MODULE_RECORD_OBSERVER_H_

namespace lynx {
namespace lepus {
class Value;
}  // namespace lepus
namespace runtime {
namespace js {

// Receives NativeModule records collected by the runtime. Concrete delivery to
// DevTool/CDP is implemented outside the runtime layer.
class NativeModuleRecordObserver {
 public:
  virtual ~NativeModuleRecordObserver() = default;

  virtual void OnRecord(const lepus::Value& record) = 0;
};

}  // namespace js
}  // namespace runtime
}  // namespace lynx

#endif  // CORE_INSPECTOR_OBSERVER_NATIVE_MODULE_RECORD_OBSERVER_H_
