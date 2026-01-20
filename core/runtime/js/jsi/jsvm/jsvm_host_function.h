// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JS_JSI_JSVM_JSVM_HOST_FUNCTION_H_
#define CORE_RUNTIME_JS_JSI_JSVM_JSVM_HOST_FUNCTION_H_

#include <ark_runtime/jsvm_types.h>

#include "core/runtime/js/jsi/jsi.h"

namespace lynx {
namespace runtime {
namespace js {
class JSVMRuntime;

namespace detail {
class JSVMHostFunctionProxy
    : public HostObjectWrapperBase<JSVMRuntime, HostFunctionType> {
 public:
  JSVMHostFunctionProxy(HostFunctionType hostFunction, JSVMRuntime* rt);
  ~JSVMHostFunctionProxy() override = default;
  static JSVM_Value createFunctionFromHostFunction(JSVMRuntime* rt,
                                                   JSVM_Env env,
                                                   const PropNameID& name,
                                                   unsigned int paramCount,
                                                   HostFunctionType func);
  static const JSVM_TypeTag* GetHostFunctionTag();

 protected:
  static JSVM_Value FunctionCallback(JSVM_Env env, JSVM_CallbackInfo info);

  static void onFinalize(JSVM_Env env, void* finalizeData, void* finalizeHint);
};
}  // namespace detail
}  // namespace js
}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_JSI_JSVM_JSVM_HOST_FUNCTION_H_
