// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/jsi/jsvm/jsvm_host_object.h"

#include <ark_runtime/jsvm.h>
#include <ark_runtime/jsvm_types.h>

#include <memory>
#include <utility>
#include <vector>

#include "base/include/log/logging.h"
#include "core/runtime/js/jsi/jsvm/jsvm_helper.h"
#include "core/runtime/js/jsi/jsvm/jsvm_runtime.h"
#include "core/runtime/js/jsi/jsvm/jsvm_util.h"

namespace lynx {
namespace piper {
namespace detail {
JSVMHostObjectProxy::JSVMHostObjectProxy(JSVMRuntime* rt,
                                         std::shared_ptr<piper::HostObject> ho)
    : HostObjectWrapperBase(rt, ho){};

JSVM_Value JSVMHostObjectProxy::getProperty(JSVM_Env env, JSVM_Value name,
                                            JSVM_Value this_arg,
                                            JSVM_Value data) {
  JSVMHostObjectProxy* proxy_ptr = nullptr;
  JSVM_CALL(nullptr, OH_JSVM_Unwrap, env, this_arg,
            reinterpret_cast<void**>(&proxy_ptr));
  JSVMRuntime* rt = nullptr;
  std::shared_ptr<HostObject> lock_host_object;
  if (proxy_ptr == nullptr ||
      !proxy_ptr->GetRuntimeAndHost(rt, lock_host_object)) {
    LOGE("JSVMHostObjectProxy::getProperty Error!");
    return nullptr;
  }

  piper::Value va =
      lock_host_object->get(rt, JSVMHelper::createPropNameID(name, rt));

  JSVM_Value ret;
  static_cast<JSVMRuntime*>(rt)->valueRef(va, &ret);
  return ret;
}

JSVM_Value JSVMHostObjectProxy::setProperty(JSVM_Env env, JSVM_Value name,
                                            JSVM_Value property,
                                            JSVM_Value this_arg,
                                            JSVM_Value data) {
  JSVMHostObjectProxy* proxy_ptr = nullptr;
  JSVM_CALL(nullptr, OH_JSVM_Unwrap, env, this_arg,
            reinterpret_cast<void**>(&proxy_ptr));
  JSVMRuntime* rt = nullptr;
  std::shared_ptr<HostObject> lock_host_object;
  if (proxy_ptr == nullptr ||
      !proxy_ptr->GetRuntimeAndHost(rt, lock_host_object)) {
    LOGE("JSVMHostObjectProxy::getProperty Error!");
    return nullptr;
  }

  lock_host_object->set(rt, JSVMHelper::createPropNameID(name, rt),
                        JSVMHelper::createValue(property, rt));
  return nullptr;
}

JSVM_Value JSVMHostObjectProxy::getPropertyNames(JSVM_Env env,
                                                 JSVM_Value this_arg,
                                                 JSVM_Value data) {
  JSVMHostObjectProxy* proxy_ptr = nullptr;
  JSVM_CALL(nullptr, OH_JSVM_Unwrap, env, this_arg,
            reinterpret_cast<void**>(&proxy_ptr));
  JSVMRuntime* rt = nullptr;
  std::shared_ptr<HostObject> lock_host_object;
  if (proxy_ptr == nullptr ||
      !proxy_ptr->GetRuntimeAndHost(rt, lock_host_object)) {
    LOGE("JSVMHostObjectProxy::getProperty Error!");
    return nullptr;
  }

  std::vector<PropNameID> names = lock_host_object->getPropertyNames(*rt);

  auto arr = piper::Array::createWithLength(*rt, names.size());
  if (!arr) {
    return nullptr;
  }
  for (size_t i = 0; i < names.size(); i++) {
    if (!(*arr).setValueAtIndex(
            *rt, i, piper::String::createFromUtf8(*rt, names[i].utf8(*rt)))) {
      return nullptr;
    }
  }

  HandleScopeWrapper scope(env);
  JSVM_Value result = nullptr;
  JSVMHelper::objectRef(*arr, &result);
  return result;
}

piper::Object JSVMHostObjectProxy::createObject(
    JSVMRuntime* rt, JSVM_Env env, std::shared_ptr<piper::HostObject> ho) {
  HandleScopeWrapper scope(env);

  JSVM_Value object_template = nullptr;
  JSVM_Ref object_template_ref = rt->GetHostObjectTemplate();
  if (object_template_ref == nullptr) {
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

          JSVMHostObjectProxy* proxy =
              reinterpret_cast<JSVMHostObjectProxy*>(ptr);
          JSVM_CALL(nullptr, OH_JSVM_Wrap, env, this_value,
                    reinterpret_cast<void*>(proxy),
                    JSVMHostObjectProxy::onFinalize, nullptr, nullptr);
          JSVM_CALL(nullptr, OH_JSVM_TypeTagObject, env, this_value,
                    GetHostObjectTag());
          return this_value;
        },
        .data = nullptr};

    property_cfg.genericNamedPropertyGetterCallback = getProperty;
    property_cfg.genericNamedPropertySetterCallback = setProperty;
    property_cfg.genericNamedPropertyEnumeratorCallback = getPropertyNames;
    JSVM_CALL(rt, OH_JSVM_DefineClassWithPropertyHandler, env,
              "JSVMHostObjectClazz", JSVM_AUTO_LENGTH, &ctor_callback, 0,
              nullptr, &property_cfg, nullptr, &object_template);

    JSVM_Ref object_template_ref = nullptr;
    JSVM_CALL(rt, OH_JSVM_CreateReference, env, object_template, 1,
              &object_template_ref);
    rt->SetHostObjectTemplate(object_template_ref);
  } else {
    JSVM_CALL(rt, OH_JSVM_GetReferenceValue, env, object_template_ref,
              &object_template);
  }

  JSVM_Value instance = nullptr;
  JSVMHostObjectProxy* proxy = new JSVMHostObjectProxy(rt, std::move(ho));
  JSVM_Value ptr_value = nullptr;
  JSVM_CALL(rt, OH_JSVM_CreateBigintUint64, env,
            reinterpret_cast<uint64_t>(proxy), &ptr_value);
  JSVM_CALL(rt, OH_JSVM_NewInstance, env, object_template, 1, &ptr_value,
            &instance);

  return JSVMHelper::createObject(instance, rt);
}

const JSVM_TypeTag* JSVMHostObjectProxy::GetHostObjectTag() {
  const static JSVM_TypeTag HOST_OBJ_TAG = {
      .lower = reinterpret_cast<uint64_t>(&HOST_OBJ_TAG), .upper = 0};
  return &HOST_OBJ_TAG;
}

void JSVMHostObjectProxy::onFinalize(JSVM_Env env, void* finalizeData,
                                     void* finalizeHint) {
  if (finalizeData != nullptr) {
    JSVMHostObjectProxy* proxy =
        reinterpret_cast<JSVMHostObjectProxy*>(finalizeData);
    delete proxy;
  }
}
}  // namespace detail
}  // namespace piper
}  // namespace lynx
