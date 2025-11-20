// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JSI_V8_V8_ISOLATE_WRAPPER_H_
#define CORE_RUNTIME_JSI_V8_V8_ISOLATE_WRAPPER_H_

#include <memory>
#include <unordered_map>

#include "core/base/observer/observer_list.h"
#include "core/runtime/jsi/jsi.h"
#include "v8.h"

namespace lynx {
namespace piper {

class V8IsolateInstance : public VMInstance {
 public:
  static std::shared_ptr<V8IsolateInstance> CreateV8IsolateInstance();

  virtual ~V8IsolateInstance() = default;

  virtual void InitIsolate(const char* arg, bool useSnapshot) = 0;

  virtual v8::Isolate* Isolate() const = 0;

  virtual void AddExtensionDataToGlobal(v8::Local<v8::Context> ctx,
                                        v8::Local<v8::Object> data) {}

  JSRuntimeType GetRuntimeType() { return piper::JSRuntimeType::v8; }

 protected:
  V8IsolateInstance() = default;
};

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JSI_V8_V8_ISOLATE_WRAPPER_H_
