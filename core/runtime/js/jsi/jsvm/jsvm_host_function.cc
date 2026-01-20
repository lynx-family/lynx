// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/jsi/jsvm/jsvm_host_function.h"

#include <ark_runtime/jsvm.h>
#include <ark_runtime/jsvm_types.h>

#include <memory>
#include <utility>
#include <vector>

#include "core/runtime/common/args_converter.h"
#include "core/runtime/js/jsi/jsi.h"
#include "core/runtime/js/jsi/jsvm/jsvm_helper.h"
#include "core/runtime/js/jsi/jsvm/jsvm_runtime.h"
#include "core/runtime/js/jsi/jsvm/jsvm_util.h"

namespace lynx {
namespace runtime {
namespace js {
namespace detail {
JSVMHostFunctionProxy::JSVMHostFunctionProxy(HostFunctionType host_func,
                                             JSVMRuntime* rt)
    : HostObjectWrapperBase(
          rt, std::make_unique<HostFunctionType>(std::move(host_func))) {}

JSVM_Value JSVMHostFunctionProxy::createFunctionFromHostFunction(
    JSVMRuntime* rt, JSVM_Env env, const PropNameID& name,
    unsigned int paramCount, HostFunctionType func) {
  HandleScopeWrapper scope(env);
  JSVM_Ref function_template_ref = rt->GetHostFunctionTemplate();
  JSVM_Value function_template = nullptr;

  if (function_template_ref == nullptr) {
    static JSVM_PropertyHandlerConfigurationStruct property_cfg;
    static JSVM_CallbackStruct ctor_callback = {
        .callback = [](JSVM_Env env, JSVM_CallbackInfo info) -> JSVM_Value {
          JSVM_Value this_value = nullptr;
          size_t argc = 1;
          JSVM_Value argv = nullptr;
          JSVM_CALL(nullptr, OH_JSVM_GetCbInfo, env, info, &argc, &argv,
                    &this_value, nullptr);
          uint64_t ptr = 0;
          bool lossless = false;
          JSVM_CALL(nullptr, OH_JSVM_GetValueBigintUint64, env, argv, &ptr,
                    &lossless);

          JSVMHostFunctionProxy* proxy =
              reinterpret_cast<JSVMHostFunctionProxy*>(ptr);
          JSVM_CALL(nullptr, OH_JSVM_Wrap, env, this_value,
                    reinterpret_cast<void*>(proxy),
                    JSVMHostFunctionProxy::onFinalize, nullptr, nullptr);
          JSVM_CALL(nullptr, OH_JSVM_TypeTagObject, env, this_value,
                    GetHostFunctionTag());
          return this_value;
        },
        .data = nullptr};
    static JSVM_CallbackStruct call_as_func_callback = {
        .callback = JSVMHostFunctionProxy::FunctionCallback, .data = nullptr};
    JSVM_CALL(rt, OH_JSVM_DefineClassWithPropertyHandler, env,
              "HostFunctionClazz", JSVM_AUTO_LENGTH, &ctor_callback, 0, nullptr,
              &property_cfg, &call_as_func_callback, &function_template);
    JSVM_CALL(rt, OH_JSVM_CreateReference, env, function_template, 1,
              &function_template_ref);
    rt->SetHostFunctionTemplate(function_template_ref);
  } else {
    JSVM_CALL(rt, OH_JSVM_GetReferenceValue, env, function_template_ref,
              &function_template);
  }

  JSVM_Value instance = nullptr;
  JSVMHostFunctionProxy* proxy = new JSVMHostFunctionProxy(std::move(func), rt);
  JSVM_Value ptr_value = nullptr;
  JSVM_CALL(rt, OH_JSVM_CreateBigintUint64, env,
            reinterpret_cast<uint64_t>(proxy), &ptr_value);
  JSVM_CALL(rt, OH_JSVM_NewInstance, env, function_template, 1, &ptr_value,
            &instance);
  // Set instance's prototype to Function.prototype.
  JSVM_Value global = nullptr;
  JSVM_CALL(rt, OH_JSVM_GetGlobal, env, &global);
  JSVM_Value func_ctor = nullptr;
  JSVM_CALL(rt, OH_JSVM_GetNamedProperty, env, global, "Function", &func_ctor);
  JSVM_CALL(rt, OH_JSVM_ObjectSetPrototypeOf, env, instance, func_ctor);

  return instance;
}

const JSVM_TypeTag* JSVMHostFunctionProxy::GetHostFunctionTag() {
  const static JSVM_TypeTag HOST_FUN_TAG = {
      .lower = reinterpret_cast<uint64_t>(&HOST_FUN_TAG), .upper = 0};
  return &HOST_FUN_TAG;
}

JSVM_Value JSVMHostFunctionProxy::FunctionCallback(JSVM_Env env,
                                                   JSVM_CallbackInfo info) {
  JSVM_Value this_value = nullptr;
  size_t argc = 0;
  JSVM_CALL(nullptr, OH_JSVM_GetCbInfo, env, info, &argc, nullptr, &this_value,
            nullptr);
  JSVMHostFunctionProxy* proxy_ptr = nullptr;
  JSVM_CALL(nullptr, OH_JSVM_Unwrap, env, this_value,
            reinterpret_cast<void**>(&proxy_ptr));
  JSVMRuntime* rt = nullptr;
  std::shared_ptr<HostFunctionType> lock_host_func;
  if (proxy_ptr == nullptr ||
      !proxy_ptr->GetRuntimeAndHost(rt, lock_host_func)) {
    LOGE("JSVMHostFunctionProxy::FunctionCallback Error!");
    return nullptr;
  }

  std::vector<JSVM_Value> args(argc);
  JSVM_CALL(rt, OH_JSVM_GetCbInfo, env, info, &argc, args.data(), nullptr,
            nullptr);

  auto converter =
      ArgsConverter<Value>(argc, args.data(), [rt](const JSVM_Value& value) {
        return JSVMHelper::createValue(value, rt);
      });
  JSINativeExceptionCollector::Scope scope;
  auto ret = (*lock_host_func)(*rt, JSVMHelper::createValue(this_value, rt),
                               converter, argc);
  const auto& exception =
      JSINativeExceptionCollector::Instance()->GetException();
  if (exception && rt->IsEnableJsBindingApiThrowException()) {
    JSVMHelper::ThrowJsException(rt, exception->message(), exception->stack());
    return nullptr;
  }

  JSVM_Value result = nullptr;
  if (ret.has_value()) {
    static_cast<JSVMRuntime*>(rt)->valueRef(ret.value(), &result);
  } else {
    // TODO(huzhanbo.luc): we can merge this usage into
    // JSINativeExceptionCollector
    if (rt->IsEnableJsBindingApiThrowException()) {
      JSVMHelper::ThrowJsException(rt, ret.error().message(),
                                   ret.error().stack());
    } else {
      rt->reportJSIException(ret.error());
    }
  }
  return result;
}

void JSVMHostFunctionProxy::onFinalize(JSVM_Env env, void* finalizeData,
                                       void* finalizeHint) {
  if (finalizeData != nullptr) {
    JSVMHostFunctionProxy* proxy =
        reinterpret_cast<JSVMHostFunctionProxy*>(finalizeData);
    delete proxy;
  }
}

}  // namespace detail
}  // namespace js
}  // namespace runtime
}  // namespace lynx
