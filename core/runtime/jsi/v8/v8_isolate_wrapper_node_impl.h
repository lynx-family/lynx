// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JSI_V8_V8_ISOLATE_WRAPPER_NODE_IMPL_H_
#define CORE_RUNTIME_JSI_V8_V8_ISOLATE_WRAPPER_NODE_IMPL_H_

#include <memory>

#include "core/runtime/jsi/v8/v8_isolate_wrapper.h"
#include "core/runtime/jsi/v8/v8_platform_data.h"

namespace lynx {
namespace piper {

class V8IsolateInstanceNodeImpl : public V8IsolateInstance {
 public:
  V8IsolateInstanceNodeImpl();
  ~V8IsolateInstanceNodeImpl() override;

  void InitIsolate(const char* arg, bool useSnapshot) override;
  void AddExtensionDataToGlobal(v8::Local<v8::Context> ctx,
                                v8::Local<v8::Object> data) override;

  v8::Isolate* Isolate() const override;

 private:
  v8::Global<v8::Value> node_exports_;
  v8::Global<v8::Context> node_ctx_;
  std::shared_ptr<V8PlatformData> platform_data_;

  v8::Isolate* isolate_ = nullptr;
  friend class V8Runtime;
  friend class V8ContextWrapper;
};

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JSI_V8_V8_ISOLATE_WRAPPER_NODE_IMPL_H_
