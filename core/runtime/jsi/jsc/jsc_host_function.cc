//  Copyright (c) Facebook, Inc. and its affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jsi/jsc/jsc_host_function.h"

#include <utility>

#include "base/include/compiler_specific.h"
#include "base/include/log/logging.h"
#include "core/runtime/common/args_converter.h"
#include "core/runtime/jsi/jsc/jsc_helper.h"
#include "core/runtime/jsi/jsc/jsc_host_object.h"
#include "core/runtime/jsi/jsc/jsc_runtime.h"

namespace lynx {
namespace piper {
std::once_flag hostFunctionClassOnceFlag;
JSClassRef hostFunctionClass{};
namespace detail {

Function HostFunctionMetadata::createFunctionFromHostFunction(
    JSCRuntime& rt, JSGlobalContextRef ctx, const PropNameID& name,
    unsigned int paramCount, HostFunctionType func) {
  std::call_once(hostFunctionClassOnceFlag, []() {
    JSClassDefinition functionClass = kJSClassDefinitionEmpty;
    functionClass.version = 0;
    functionClass.attributes = kJSClassAttributeNoAutomaticPrototype;
    functionClass.initialize = HostFunctionMetadata::initialize;
    functionClass.finalize = HostFunctionMetadata::finalize;
    functionClass.callAsFunction = HostFunctionMetadata::call;

    hostFunctionClass = JSClassCreate(&functionClass);
  });

  JSObjectRef funcRef =
      JSObjectMake(ctx, hostFunctionClass,
                   new HostFunctionMetadata(&rt, std::move(func), paramCount,
                                            JSCHelper::stringRef(name)));
  return JSCHelper::createObject(ctx, rt.getCtxInvalid(), rt.objectCounter(),
                                 funcRef)
      .getFunction(rt);
}

JSClassRef HostFunctionMetadata::getHostFunctionClass() {
  return hostFunctionClass;
}

void HostFunctionMetadata::initialize(JSContextRef ctx, JSObjectRef object) {
  HostFunctionMetadata* metadata =
      static_cast<HostFunctionMetadata*>(JSObjectGetPrivate(object));
  Runtime* rt = nullptr;
  if (UNLIKELY(metadata == nullptr ||
               (rt = metadata->GetRuntime()) == nullptr)) {
    LOGE("HostFunctionMetadata::initialize Error!");
    return;
  }
  JSValueRef exc = nullptr;
  JSObjectSetProperty(ctx, object, JSCHelper::getJSStringFromPool("length"),
                      JSValueMakeNumber(ctx, metadata->argCount_),
                      kJSPropertyAttributeReadOnly |
                          kJSPropertyAttributeDontEnum |
                          kJSPropertyAttributeDontDelete,
                      &exc);
  if (exc) {
    exc = nullptr;
  }

  JSStringRef name = nullptr;
  std::swap(metadata->name_, name);
  JSObjectSetProperty(ctx, object, JSCHelper::getJSStringFromPool("name"),
                      JSValueMakeString(ctx, name),
                      kJSPropertyAttributeReadOnly |
                          kJSPropertyAttributeDontEnum |
                          kJSPropertyAttributeDontDelete,
                      &exc);
  JSStringRelease(name);
  if (exc) {
    exc = nullptr;
  }

  JSObjectRef global = JSContextGetGlobalObject(ctx);
  JSValueRef value = JSObjectGetProperty(
      ctx, global, JSCHelper::getJSStringFromPool("Function"), &exc);
  // if (JSC_UNLIKELY(exc)) {
  //   abort();
  // }
  JSObjectRef funcCtor = JSValueToObject(ctx, value, &exc);
  if (!funcCtor) {
    return;
  }
  JSValueRef funcProto = JSObjectGetPrototype(ctx, funcCtor);
  JSObjectSetPrototype(ctx, object, funcProto);
}

JSValueRef HostFunctionMetadata::call(JSContextRef ctx, JSObjectRef function,
                                      JSObjectRef thisObject,
                                      size_t argumentCount,
                                      const JSValueRef arguments[],
                                      JSValueRef* exception) {
  HostFunctionMetadata* metadata =
      static_cast<HostFunctionMetadata*>(JSObjectGetPrivate(function));
  Runtime* rt = nullptr;
  std::shared_ptr<HostFunctionType> host_func;
  if (UNLIKELY(metadata == nullptr ||
               !metadata->GetRuntimeAndHost(rt, host_func))) {
    LOGE("HostFunctionMetadata::call Error!");
    // TODO(liyanbo): Throw exception without js binding api switch.
    return JSValueMakeUndefined(ctx);
  }
  JSCRuntime* jsc_rt = static_cast<JSCRuntime*>(rt);
  JSGlobalContextRef global_ctx = JSContextGetGlobalContext(ctx);
  auto converter = ArgsConverter<Value>(
      argumentCount, arguments, [jsc_rt](const auto& value) {
        return JSCHelper::createValue(*jsc_rt, value);
      });
  JSValueRef res;
  Value thisVal(JSCHelper::createObject(global_ctx, jsc_rt->getCtxInvalid(),
                                        jsc_rt->objectCounter(), thisObject));
  JSINativeExceptionCollector::Scope scope;
  auto ret = (*host_func)(*jsc_rt, thisVal, converter, argumentCount);
  const auto& native_exception =
      JSINativeExceptionCollector::Instance()->GetException();
  if (native_exception && rt->IsEnableJsBindingApiThrowException()) {
    JSCHelper::ThrowJsException(ctx, *native_exception, exception);
    return JSValueMakeUndefined(ctx);
  }

  if (ret.has_value()) {
    res = JSCHelper::valueRef(global_ctx, *jsc_rt, ret.value());
  } else {
    // TODO(huzhanbo.luc): we can merge this usage into
    // JSINativeExceptionCollector
    if (rt->IsEnableJsBindingApiThrowException()) {
      JSCHelper::ThrowJsException(ctx, ret.error(), exception);
    } else {
      rt->reportJSIException(ret.error());
    }
    res = JSValueMakeUndefined(ctx);
  }
  return res;
}

void HostFunctionMetadata::finalize(JSObjectRef object) {
  HostFunctionMetadata* metadata =
      static_cast<HostFunctionMetadata*>(JSObjectGetPrivate(object));
  JSObjectSetPrivate(object, nullptr);
  delete metadata;
}

}  // namespace detail
}  // namespace piper
}  // namespace lynx
