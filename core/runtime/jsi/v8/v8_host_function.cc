// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/jsi/v8/v8_host_function.h"

#include <mutex>
#include <utility>

#include "base/include/compiler_specific.h"
#include "base/include/log/logging.h"
#include "core/runtime/common/args_converter.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/jsi/v8/v8_helper.h"
#include "core/runtime/jsi/v8/v8_host_object.h"
#include "core/runtime/jsi/v8/v8_runtime.h"
#include "libplatform/libplatform.h"
#include "v8.h"

namespace lynx {
namespace piper {
namespace detail {
const std::string V8HostFunctionProxy::HOST_FUN_KEY = "hostFunctionFlag";

V8HostFunctionProxy::V8HostFunctionProxy(piper::HostFunctionType host_func,
                                         V8Runtime* rt)
    : HostObjectWrapperBase(
          rt, std::make_unique<piper::HostFunctionType>(std::move(host_func))) {
}

v8::Local<v8::Object> V8HostFunctionProxy::createFunctionFromHostFunction(
    V8Runtime* rt, v8::Local<v8::Context> ctx, const piper::PropNameID& name,
    unsigned int paramCount, piper::HostFunctionType func) {
  v8::Local<v8::ObjectTemplate> function_template =
      rt->GetHostFunctionTemplate();
  if (function_template.IsEmpty()) {
    function_template = v8::ObjectTemplate::New(ctx->GetIsolate());
    function_template->SetInternalFieldCount(1);
    function_template->SetCallAsFunctionHandler(FunctionCallback);
    rt->SetHostFunctionTemplate(function_template);
  }

  v8::Local<v8::Object> obj =
      function_template->NewInstance(ctx).ToLocalChecked();
  V8HostFunctionProxy* proxy = new V8HostFunctionProxy(std::move(func), rt);
  obj->SetAlignedPointerInInternalField(0, proxy);
  v8::Local<v8::Private> key = v8::Private::New(
      ctx->GetIsolate(),
      V8Helper::ConvertToV8String(ctx->GetIsolate(), HOST_FUN_KEY));
  obj->SetPrivate(
      ctx, key, V8Helper::ConvertToV8String(ctx->GetIsolate(), "hostFunction"));

  // Add prototype to HostFunction
  v8::MaybeLocal<v8::Value> func_ctor = ctx->Global()->Get(
      ctx, V8Helper::ConvertToV8String(ctx->GetIsolate(), "Function"));
  if (LIKELY(!func_ctor.IsEmpty())) {
#if OS_ANDROID
    v8::Maybe<bool> result = obj->SetPrototype(ctx, func_ctor.ToLocalChecked()
                                                        ->ToObject(ctx)
                                                        .ToLocalChecked()
                                                        ->GetPrototype());
#else
    v8::Maybe<bool> result = obj->SetPrototype(ctx, func_ctor.ToLocalChecked()
                                                        ->ToObject(ctx)
                                                        .ToLocalChecked()
                                                        ->GetPrototype());
#endif
    ALLOW_UNUSED_LOCAL(result);
  }
  proxy->keeper_.Reset(ctx->GetIsolate(), obj);
  proxy->keeper_.SetWeak(proxy, onFinalize, v8::WeakCallbackType::kParameter);
  return obj;
}

std::weak_ptr<piper::HostFunctionType> getHostFunction(
    V8Runtime* rt, const piper::Function& obj) {
  v8::Local<v8::Object> v8_obj = V8Helper::objectRef(obj);
  void* data = v8_obj->GetAlignedPointerFromInternalField(0);
  auto proxy = static_cast<V8HostFunctionProxy*>(data);
  return proxy->GetHost();
}

void V8HostFunctionProxy::FunctionCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> obj = info.Holder();
  V8HostFunctionProxy* proxy = static_cast<V8HostFunctionProxy*>(
      obj->GetAlignedPointerFromInternalField(0));
  Runtime* rt = nullptr;
  std::shared_ptr<HostFunctionType> host_func;
  if (UNLIKELY(proxy == nullptr || !proxy->GetRuntimeAndHost(rt, host_func))) {
    LOGE("V8HostFunctionProxy::FunctionCallback Error!");
    // TODO(liyanbo): Throw exception without js binding api switch.
    return;
  }
  int count = info.Length();
  auto context = info.GetIsolate()->GetCurrentContext();
  auto converter = ArgsConverter<Value>(
      count, info, [&context](const v8::Local<v8::Value>& value) {
        return V8Helper::createValue(value, context);
      });
  JSINativeExceptionCollector::Scope scope;
  auto ret =
      (*host_func)(*rt,
                   V8Helper::createValue(
                       info.Holder(), info.GetIsolate()->GetCurrentContext()),
                   converter, count);
  const auto& exception =
      JSINativeExceptionCollector::Instance()->GetException();
  if (exception && rt->IsEnableJsBindingApiThrowException()) {
    return V8Helper::ThrowJsException(info.GetIsolate(), *exception);
  }
  if (ret.has_value()) {
    info.GetReturnValue().Set(
        static_cast<V8Runtime*>(rt)->valueRef(ret.value()));
    return;
  } else {
    // TODO(huzhanbo.luc): we can merge this usage into
    // JSINativeExceptionCollector
    if (rt->IsEnableJsBindingApiThrowException()) {
      V8Helper::ThrowJsException(info.GetIsolate(), ret.error());
    } else {
      rt->reportJSIException(ret.error());
      info.GetReturnValue().Set(v8::Local<v8::Value>());
    }
  }
}

void V8HostFunctionProxy::onFinalize(
    const v8::WeakCallbackInfo<V8HostFunctionProxy>& data) {
  V8HostFunctionProxy* proxy = data.GetParameter();
  proxy->keeper_.Reset();
  delete proxy;
}

}  // namespace detail

}  // namespace piper
}  // namespace lynx
