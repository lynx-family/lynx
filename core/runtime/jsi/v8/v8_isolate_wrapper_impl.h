// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JSI_V8_V8_ISOLATE_WRAPPER_IMPL_H_
#define CORE_RUNTIME_JSI_V8_V8_ISOLATE_WRAPPER_IMPL_H_

#include "core/runtime/jsi/v8/v8_isolate_wrapper.h"

namespace lynx {
namespace piper {

class V8IsolateInstanceImpl : public V8IsolateInstance {
 public:
  V8IsolateInstanceImpl();
  ~V8IsolateInstanceImpl() override;

  void InitIsolate(const char* arg, bool useSnapshot) override;

  // void AddObserver(base::Observer* obs) { observers_.AddObserver(obs); }
  // void RemoveObserver(base::Observer* obs) {
  // observers_.RemoveObserver(obs);
  // }
  v8::Isolate* Isolate() const override;

 private:
  v8::Isolate* isolate_;
  friend class V8Runtime;
  friend class V8ContextWrapper;
};

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JSI_V8_V8_ISOLATE_WRAPPER_IMPL_H_
