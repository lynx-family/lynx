// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JSI_V8_V8_CONTEXT_WRAPPER_IMPL_H_
#define CORE_RUNTIME_JSI_V8_V8_CONTEXT_WRAPPER_IMPL_H_

#include <memory>

#include "core/runtime/jsi/v8/v8_context_wrapper.h"

namespace lynx {
namespace piper {

class V8ContextWrapperImpl : public V8ContextWrapper {
 public:
  V8ContextWrapperImpl(std::shared_ptr<VMInstance> vm);
  virtual ~V8ContextWrapperImpl() override;
  virtual void Init() override;
  virtual v8::Local<v8::Context> getContext() const override;
  virtual v8::Isolate* getIsolate() const override;

 private:
  v8::Persistent<v8::Context> ctx_;
};

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JSI_V8_V8_CONTEXT_WRAPPER_IMPL_H_
