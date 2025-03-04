// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/jsi/v8/v8_host_object.h"

#include <mutex>
#include <utility>
#include <vector>

#include "base/include/compiler_specific.h"
#include "base/include/log/logging.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/jsi/v8/v8_helper.h"
#include "core/runtime/jsi/v8/v8_runtime.h"
#include "libplatform/libplatform.h"
#include "v8.h"

namespace lynx {
namespace piper {
namespace detail {

V8HostObjectProxy::V8HostObjectProxy(V8Runtime* rt,
                                     std::shared_ptr<piper::HostObject> sho)
    : HostObjectWrapperBase(rt, std::move(sho)){};

void V8HostObjectProxy::getProperty(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> obj = info.Holder();
  V8HostObjectProxy* proxy = static_cast<V8HostObjectProxy*>(
      obj->GetAlignedPointerFromInternalField(0));
  Runtime* rt = nullptr;
  std::shared_ptr<HostObject> lock_host_object;
  if (UNLIKELY(proxy == nullptr ||
               !proxy->GetRuntimeAndHost(rt, lock_host_object))) {
    LOGE("V8HostObjectProxy::getProperty Error!");
    return;
  }
  V8Runtime* v8_rt = static_cast<V8Runtime*>(rt);
#if OS_ANDROID
  piper::Value va = lock_host_object->get(
      v8_rt, V8Helper::createPropNameID(property, info.GetIsolate()));
#else
  piper::Value va = lock_host_object->get(
      v8_rt, V8Helper::createPropNameID(property, v8_rt->getContext()));
#endif
  info.GetReturnValue().Set(v8_rt->valueRef(va));
}

void V8HostObjectProxy::setProperty(
    v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> obj = info.Holder();
  V8HostObjectProxy* proxy = static_cast<V8HostObjectProxy*>(
      obj->GetAlignedPointerFromInternalField(0));
  Runtime* rt = nullptr;
  std::shared_ptr<HostObject> lock_host_object;
  if (UNLIKELY(proxy == nullptr ||
               !proxy->GetRuntimeAndHost(rt, lock_host_object))) {
    LOGE("V8HostObjectProxy::setProperty Error!");
    return;
  }
  V8Runtime* v8_rt = static_cast<V8Runtime*>(rt);
#if OS_ANDROID
  lock_host_object->set(rt,
                        V8Helper::createPropNameID(property, info.GetIsolate()),
                        V8Helper::createValue(value, v8_rt->getContext()));
#else
  lock_host_object->set(
      rt, V8Helper::createPropNameID(property, v8_rt->getContext()),
      V8Helper::createValue(value, v8_rt->getContext()));
#endif
}

void V8HostObjectProxy::getPropertyNames(
    const v8::PropertyCallbackInfo<v8::Array>& info) {
  v8::Local<v8::Object> obj = info.Holder();
  V8HostObjectProxy* proxy = static_cast<V8HostObjectProxy*>(
      obj->GetAlignedPointerFromInternalField(0));
  Runtime* rt = nullptr;
  std::shared_ptr<HostObject> lock_host_object;
  if (UNLIKELY(proxy == nullptr ||
               !proxy->GetRuntimeAndHost(rt, lock_host_object))) {
    LOGE("V8HostObjectProxy::getPropertyNames Error!");
    return;
  }

  std::vector<PropNameID> names = lock_host_object->getPropertyNames(*rt);

  auto ary = piper::Array::createWithLength(*rt, names.size());
  if (!ary) {
    return;
  }
  for (size_t i = 0; i < names.size(); i++) {
    if (!(*ary).setValueAtIndex(
            *rt, i, piper::String::createFromUtf8(*rt, names[i].utf8(*rt)))) {
      return;
    }
  }
  v8::Local<v8::Object> ary_obj = V8Helper::objectRef(*ary);
  v8::Local<v8::Array> result;
  result = result.Cast(ary_obj);
  info.GetReturnValue().Set(result);
}

void V8HostObjectProxy::onFinalize(
    const v8::WeakCallbackInfo<V8HostObjectProxy>& data) {
  V8HostObjectProxy* proxy = data.GetParameter();
  proxy->keeper_.Reset();
  delete proxy;
}

piper::Object V8HostObjectProxy::createObject(
    V8Runtime* rt, v8::Local<v8::Context> context,
    std::shared_ptr<piper::HostObject> ho) {
  ENTER_SCOPE(context)

  v8::Local<v8::ObjectTemplate> object_template = rt->GetHostObjectTemplate();
  if (object_template.IsEmpty()) {
    object_template = v8::ObjectTemplate::New(context->GetIsolate());
    object_template->SetInternalFieldCount(HOST_OBJ_COUNT);

    v8::NamedPropertyHandlerConfiguration config;
    config.getter = getProperty;
    config.setter = setProperty;
    config.enumerator = getPropertyNames;

    object_template->SetHandler(config);
    rt->SetHostObjectTemplate(object_template);
  }

  v8::Local<v8::Object> obj =
      object_template->NewInstance(context).ToLocalChecked();
  V8HostObjectProxy* proxy = new V8HostObjectProxy(rt, std::move(ho));
  obj->SetAlignedPointerInInternalField(0, proxy);
  proxy->keeper_.Reset(context->GetIsolate(), obj);
  proxy->keeper_.SetWeak(proxy, onFinalize, v8::WeakCallbackType::kParameter);
  return V8Helper::createObject(obj, context->GetIsolate());
}

}  // namespace detail
}  // namespace piper
}  // namespace lynx
