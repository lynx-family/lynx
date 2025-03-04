// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JSI_V8_V8_CONTEXT_WRAPPER_H_
#define CORE_RUNTIME_JSI_V8_V8_CONTEXT_WRAPPER_H_

#include <memory>
#include <unordered_map>

#include "core/base/observer/observer_list.h"
#include "core/runtime/jsi/jsi.h"
#include "v8.h"

namespace lynx {
namespace piper {

class V8ContextWrapper : public JSIContext {
 public:
  V8ContextWrapper(std::shared_ptr<VMInstance> vm) : JSIContext(vm) {}
  virtual ~V8ContextWrapper() = default;
  virtual void Init() = 0;
  virtual v8::Local<v8::Context> getContext() const = 0;
  virtual v8::Isolate* getIsolate() const = 0;
};

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JSI_V8_V8_CONTEXT_WRAPPER_H_
