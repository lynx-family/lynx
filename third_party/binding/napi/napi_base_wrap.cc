// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "third_party/binding/napi/napi_base_wrap.h"

#include <string>

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace binding {

static void CheckStatus(napi_env env, napi_status status, const char* message) {
  if (status != napi_ok) {
    std::string msg_str =
        std::string(message) + ", napi status" + std::to_string(status);
    Napi::Error::New(env, msg_str.c_str()).ThrowAsJavaScriptException();
  }
}
napi_ref Wrap(napi_env env, napi_value obj, void* data,
              napi_finalize finalize_cb, void* hint) {
  napi_ref result;
  napi_status status =
      NAPI_ENV_CALL(wrap, env, obj, data, finalize_cb, hint, &result);
  CheckStatus(env, status, "failed to call napi_wrap");
  return result;
}

napi_class DefineClass(napi_env env, const char* utf8name, napi_callback ctor,
                       size_t props_count,
                       const napi_property_descriptor* descriptors, void* data,
                       napi_class super_class) {
  napi_class result;
  napi_status status =
      NAPI_ENV_CALL(define_class, env, utf8name, NAPI_AUTO_LENGTH, ctor, data,
                    props_count, descriptors, super_class, &result);
  CheckStatus(env, status, "failed to call napi_define_class");
  return result;
}

napi_value InstanceMethodCallbackWrapper(napi_env env,
                                         napi_callback_info info) {
  Napi::CallbackInfo callbackInfo(env, info);
  NapiBridgeMethodCallbackData* callbackData =
      reinterpret_cast<NapiBridgeMethodCallbackData*>(callbackInfo.Data());
  callbackInfo.SetData(callbackData->data);
  auto wrapper = callbackInfo.This().As<Napi::Object>();
  NapiBridge* instance = Napi::InstanceWrap<NapiBridge>::Unwrap(wrapper);
  auto cb = callbackData->callback;
  return instance ? (instance->*cb)(callbackInfo) : nullptr;
}

napi_value InstanceGetterCallbackWrapper(napi_env env,
                                         napi_callback_info info) {
  Napi::CallbackInfo callbackInfo(env, info);
  NapiBridgeAccessorCallbackData* callbackData =
      reinterpret_cast<NapiBridgeAccessorCallbackData*>(callbackInfo.Data());
  callbackInfo.SetData(callbackData->data);
  auto wrapper = callbackInfo.This().As<Napi::Object>();
  NapiBridge* instance = Napi::InstanceWrap<NapiBridge>::Unwrap(wrapper);
  auto cb = callbackData->getterCallback;
  return instance ? (instance->*cb)(callbackInfo) : nullptr;
}

napi_value InstanceSetterCallbackWrapper(napi_env env,
                                         napi_callback_info info) {
  Napi::CallbackInfo callbackInfo(env, info);
  NapiBridgeAccessorCallbackData* callbackData =
      reinterpret_cast<NapiBridgeAccessorCallbackData*>(callbackInfo.Data());
  callbackInfo.SetData(callbackData->data);
  auto wrapper = callbackInfo.This().As<Napi::Object>();
  NapiBridge* instance = Napi::InstanceWrap<NapiBridge>::Unwrap(wrapper);
  auto cb = callbackData->setterCallback;
  if (instance) {
    (instance->*cb)(callbackInfo, callbackInfo[0]);
  }
  return nullptr;
}

napi_value StaticMethodCallbackWrapper(napi_env env, napi_callback_info info) {
  Napi::CallbackInfo callbackInfo(env, info);
  NapiBridgeStaticMethodCallbackData* callbackData =
      reinterpret_cast<NapiBridgeStaticMethodCallbackData*>(
          callbackInfo.Data());
  callbackInfo.SetData(callbackData->data);
  return (*callbackData->callback)(callbackInfo);
}

napi_value StaticGetterCallbackWrapper(napi_env env, napi_callback_info info) {
  Napi::CallbackInfo callbackInfo(env, info);
  NapiBridgeStaticAccessorCallbackData* callbackData =
      reinterpret_cast<NapiBridgeStaticAccessorCallbackData*>(
          callbackInfo.Data());
  callbackInfo.SetData(callbackData->data);
  return (*callbackData->getterCallback)(callbackInfo);
}

napi_value StaticSetterCallbackWrapper(napi_env env, napi_callback_info info) {
  Napi::CallbackInfo callbackInfo(env, info);
  NapiBridgeStaticAccessorCallbackData* callbackData =
      reinterpret_cast<NapiBridgeStaticAccessorCallbackData*>(
          callbackInfo.Data());
  callbackInfo.SetData(callbackData->data);
  (*callbackData->setterCallback)(callbackInfo, callbackInfo[0]);
  return nullptr;
}

void AttachPropData(Napi::Object obj, size_t props_count,
                    const napi_property_descriptor* props) {
  for (size_t i = 0; i < props_count; i++) {
    auto& p = props[i];
    if ((p.attributes & napi_static) == 0) {
      if (p.method == InstanceMethodCallbackWrapper) {
        obj.AddFinalizer(p.data, [](napi_env, void* data, void*) {
          delete static_cast<NapiBridgeMethodCallbackData*>(data);
        });
      } else if (p.getter == InstanceGetterCallbackWrapper ||
                 p.setter == InstanceSetterCallbackWrapper) {
        obj.AddFinalizer(p.data, [](napi_env, void* data, void*) {
          delete static_cast<NapiBridgeAccessorCallbackData*>(data);
        });
      }
    } else {
      if (p.method == StaticMethodCallbackWrapper) {
        obj.AddFinalizer(p.data, [](napi_env, void* data, void*) {
          delete static_cast<NapiBridgeStaticMethodCallbackData*>(data);
        });
      } else if (p.getter == StaticGetterCallbackWrapper ||
                 p.setter == StaticSetterCallbackWrapper) {
        obj.AddFinalizer(p.data, [](napi_env, void* data, void*) {
          delete static_cast<NapiBridgeStaticAccessorCallbackData*>(data);
        });
      }
    }
  }
}

void FinalizeCallback(napi_env env, void* data, void* /*hint*/) {
  Napi::ScriptWrappable* instance = static_cast<Napi::ScriptWrappable*>(data);
  delete instance;
}

bool CheckIsConstructorCall(napi_env env, Napi::CallbackInfo& info) {
  if (!info.IsConstructCall()) {
    Napi::TypeError::New(env,
                         "Class constructors cannot be invoked without 'new'")
        .ThrowAsJavaScriptException();
    return false;
  }
  return true;
}

}  // namespace binding
}  // namespace lynx
