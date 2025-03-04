// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/jsi/quickjs/quickjs_host_function.h"

#include <utility>

#include "base/include/compiler_specific.h"
#include "core/runtime/common/args_converter.h"
#include "core/runtime/jsi/quickjs/quickjs_helper.h"
#include "core/runtime/jsi/quickjs/quickjs_runtime.h"
extern "C" {
#include "quickjs/include/quickjs.h"
};
namespace lynx {
namespace piper {
namespace detail {

QuickjsHostFunctionProxy::QuickjsHostFunctionProxy(
    piper::HostFunctionType hostFunction, QuickjsRuntime* rt)
    : HostObjectWrapperBase(rt, std::make_shared<piper::HostFunctionType>(
                                    std::move(hostFunction))) {}

QuickjsHostFunctionProxy::~QuickjsHostFunctionProxy() {
  auto quickjs_runtime = static_cast<QuickjsRuntime*>(GetRuntime());
  if (quickjs_runtime) {
    if (LEPUS_IsGCMode(quickjs_runtime->getJSContext())) {
      p_val_.Reset(quickjs_runtime->getJSRuntime());
    }
  }
}

void QuickjsHostFunctionProxy::hostFinalizer(LEPUSRuntime* rt, LEPUSValue val) {
  LEPUSClassID function_id =
      lynx::piper::QuickjsRuntimeInstance::getFunctionId(rt);
  if (UNLIKELY(function_id == 0)) {
    LOGE("hostFinalizer Error! functionId is 0. LEPUSRuntime:" << rt);
    return;
  }
  QuickjsHostFunctionProxy* th =
      static_cast<QuickjsHostFunctionProxy*>(LEPUS_GetOpaque(val, function_id));
  if (th) {
    if (LEPUS_IsGCModeRT(rt)) th->p_val_.Reset(rt);
    delete th;
  }
}

LEPUSValue QuickjsHostFunctionProxy::FunctionCallback(
    LEPUSContext* ctx, LEPUSValueConst func_obj, LEPUSValueConst this_obj,
    int argc, LEPUSValueConst* argv, int flags) {
  LEPUSClassID functionId =
      lynx::piper::QuickjsRuntimeInstance::getFunctionId(ctx);
  if (UNLIKELY(functionId == 0)) {
    LOGE(
        "QuickjsHostFunctionProxy::FunctionCallback Error! functionId is 0. "
        "LEPUSContext:"
        << ctx);
    // TODO(liyanbo): Throw exception without js binding api switch.
    return LEPUS_UNDEFINED;
  }
  QuickjsHostFunctionProxy* proxy = static_cast<QuickjsHostFunctionProxy*>(
      LEPUS_GetOpaque(func_obj, functionId));
  Runtime* rt = nullptr;
  std::shared_ptr<HostFunctionType> host_func;
  if (UNLIKELY(proxy == nullptr || !proxy->GetRuntimeAndHost(rt, host_func))) {
    LOGE("QuickjsHostFunctionProxy::FunctionCallback Error! LEPUSContext:"
         << ctx);
    // TODO(liyanbo): Throw exception without js binding api switch.
    return LEPUS_UNDEFINED;
  }
  QuickjsRuntime* qjs_rt = static_cast<QuickjsRuntime*>(rt);
  auto converter = ArgsConverter<Value>(
      argc, argv, [&ctx, qjs_rt](const LEPUSValueConst& value) {
        return QuickjsHelper::createValue(LEPUS_DupValue(ctx, value), qjs_rt);
      });
  JSINativeExceptionCollector::Scope scope;
  auto ret = (*host_func)(
      *rt, QuickjsHelper::createValue(LEPUS_DupValue(ctx, this_obj), qjs_rt),
      converter, argc);
  const auto& exception =
      JSINativeExceptionCollector::Instance()->GetException();
  if (exception && rt->IsEnableJsBindingApiThrowException()) {
    return QuickjsHelper::ThrowJsException(ctx, *exception);
  }

  if (ret.has_value()) {
    return LEPUS_DupValue(ctx, qjs_rt->valueRef(ret.value()));
  } else if (rt->IsEnableJsBindingApiThrowException()) {
    // TODO(huzhanbo.luc): we can merge this usage into
    // JSINativeExceptionCollector
    return QuickjsHelper::ThrowJsException(ctx, ret.error());
  } else {
    rt->reportJSIException(ret.error());
    return LEPUS_UNDEFINED;
  }
}

// QuickjsHostFunctionProxy

std::weak_ptr<piper::HostFunctionType> getHostFunction(
    QuickjsRuntime* rt, const piper::Function& obj) {
  DCHECK(rt->getFunctionClassID() != 0);
  LEPUSValue quick_obj = QuickjsHelper::objectRef(obj);

  QuickjsHostFunctionProxy* proxy = static_cast<QuickjsHostFunctionProxy*>(
      LEPUS_GetOpaque(quick_obj, rt->getFunctionClassID()));
  return proxy->GetHost();
}

LEPUSValue QuickjsHostFunctionProxy::createFunctionFromHostFunction(
    QuickjsRuntime* rt, LEPUSContext* ctx, const piper::PropNameID& name,
    unsigned int paramCount, piper::HostFunctionType func) {
  LEPUSClassID function_id = rt->getFunctionClassID();
  if (UNLIKELY(function_id == 0)) {
    LOGE("createFunctionFromHostFunction Error! function_id is 0. LEPUSContext:"
         << ctx);
    return LEPUS_UNDEFINED;
  }
  auto proxy = new QuickjsHostFunctionProxy(std::move(func), rt);
  LEPUSValue obj = LEPUS_NewObjectClass(ctx, function_id);
  bool gc_enable = LEPUS_IsGCMode(rt->getJSContext());
  if (gc_enable) proxy->p_val_.Reset(rt->getJSRuntime(), obj);

  //  LOGE( "LYNX" << " createFunctionFromHostFunction name=" <<
  //  name.utf8(*rt)); LOGE( "LYNX" << " createFunctionFromHostFunction
  //  ptr=" << LEPUS_VALUE_GET_PTR(obj));
  LEPUS_SetOpaque(obj, proxy);

  // Add prototype to HostFunction
  LEPUSValue global_obj = LEPUS_GetGlobalObject(ctx);
  LEPUSValue func_ctor = LEPUS_GetPropertyStr(ctx, global_obj, "Function");
  if (LIKELY(!LEPUS_IsException(func_ctor))) {
    LEPUS_SetPrototype(ctx, obj, LEPUS_GetPrototype(ctx, func_ctor));

    // Setup `name` and `length` properties for HostFunction.
    // `name` property indicates the typical number of arguments expected by the
    // function. This property has the attributes
    // {
    //   [[Writable]]: false,
    //   [[Enumerable]]: false,
    //   [[Configurable]]: true
    // }.
    // See: // https://tc39.es/ecma262/#sec-function-instances-name
    LEPUS_DefinePropertyValueStr(
        ctx, obj, "name",
        // Here we have to call `LEPUS_DupValue` to avoid crashing
        LEPUS_DupValue(ctx, QuickjsHelper::valueRef(name)),
        LEPUS_PROP_CONFIGURABLE);
    // `length` is descriptive of the function
    // This property has the attributes
    // {
    //   [[Writable]]: false,
    //   [[Enumerable]]: false,
    //   [[Configurable]]: true
    // }.
    // See: https://tc39.es/ecma262/#sec-function-instances-length
    LEPUS_DefinePropertyValueStr(
        ctx, obj, "length",
        LEPUS_NewInt32(ctx, static_cast<int32_t>(paramCount)),
        LEPUS_PROP_CONFIGURABLE);
  }
  if (gc_enable) {
    proxy->p_val_.SetWeak(rt->getJSRuntime());
  } else {
    LEPUS_FreeValue(ctx, func_ctor);
    LEPUS_FreeValue(ctx, global_obj);
  }
  return obj;
}

}  // namespace detail

}  // namespace piper

}  // namespace lynx
