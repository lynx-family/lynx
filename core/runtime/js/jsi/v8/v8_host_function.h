// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_JSI_V8_V8_HOST_FUNCTION_H_
#define CORE_RUNTIME_JS_JSI_V8_V8_HOST_FUNCTION_H_

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

#include "core/base/observer/observer.h"
#include "core/runtime/js/jsi/jsi.h"
#include "v8.h"

namespace lynx {
namespace runtime {
namespace js {
class V8Runtime;

namespace detail {

std::weak_ptr<HostFunctionType> getHostFunction(V8Runtime* rt,
                                                const Function& obj);

class V8HostFunctionProxy
    : public HostObjectWrapperBase<V8Runtime, HostFunctionType> {
 public:
  V8HostFunctionProxy(HostFunctionType hostFunction, V8Runtime* rt);

  ~V8HostFunctionProxy() override = default;

  static v8::Local<v8::Object> createFunctionFromHostFunction(
      V8Runtime* rt, v8::Local<v8::Context> ctx, const PropNameID& name,
      unsigned int paramCount, HostFunctionType func);
  const static std::string HOST_FUN_KEY;

 protected:
  static void FunctionCallback(const v8::FunctionCallbackInfo<v8::Value>& info);

  static void onFinalize(const v8::WeakCallbackInfo<V8HostFunctionProxy>& data);

  v8::Persistent<v8::Object> keeper_;
};

}  // namespace detail
}  // namespace js
}  // namespace runtime
}  // namespace lynx
#endif  // CORE_RUNTIME_JS_JSI_V8_V8_HOST_FUNCTION_H_
