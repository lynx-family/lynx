// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSI_V8_V8_HOST_FUNCTION_H_
#define CORE_RUNTIME_JSI_V8_V8_HOST_FUNCTION_H_

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

#include "core/base/observer/observer.h"
#include "core/runtime/jsi/jsi.h"
#include "v8.h"

namespace lynx {
namespace piper {
class V8Runtime;

namespace detail {

std::weak_ptr<piper::HostFunctionType> getHostFunction(
    V8Runtime* rt, const piper::Function& obj);

class V8HostFunctionProxy
    : public HostObjectWrapperBase<piper::HostFunctionType> {
 public:
  V8HostFunctionProxy(piper::HostFunctionType hostFunction, V8Runtime* rt);

  ~V8HostFunctionProxy() override = default;

  static v8::Local<v8::Object> createFunctionFromHostFunction(
      V8Runtime* rt, v8::Local<v8::Context> ctx, const piper::PropNameID& name,
      unsigned int paramCount, piper::HostFunctionType func);
  const static std::string HOST_FUN_KEY;

 protected:
  static void FunctionCallback(const v8::FunctionCallbackInfo<v8::Value>& info);

  static void onFinalize(const v8::WeakCallbackInfo<V8HostFunctionProxy>& data);

  v8::Persistent<v8::Object> keeper_;
};

}  // namespace detail
}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JSI_V8_V8_HOST_FUNCTION_H_
