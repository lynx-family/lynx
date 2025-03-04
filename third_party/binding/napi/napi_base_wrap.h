// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BINDING_NAPI_NAPI_BASE_WRAP_H_
#define BINDING_NAPI_NAPI_BASE_WRAP_H_

#include <vector>

#include "third_party/binding/napi/napi_bridge.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace binding {

typedef Napi::MethodCallbackData<NapiBridge, NapiBridge::InstanceCallbackPtr>
    NapiBridgeMethodCallbackData;
typedef Napi::AccessorCallbackData<NapiBridge, NapiBridge::GetterCallbackPtr,
                                   NapiBridge::SetterCallbackPtr>
    NapiBridgeAccessorCallbackData;
typedef Napi::MethodCallbackData<NapiBridge, NapiBridge::StaticMethodCallback>
    NapiBridgeStaticMethodCallbackData;
typedef Napi::AccessorCallbackData<NapiBridge, NapiBridge::StaticMethodCallback,
                                   NapiBridge::StaticSetterCallback>
    NapiBridgeStaticAccessorCallbackData;

napi_class DefineClass(napi_env env, const char* utf8name,
                                   napi_callback ctor, size_t props_count,
                                   const napi_property_descriptor* descriptors,
                                   void* data, napi_class super_class);

napi_ref Wrap(napi_env env, napi_value obj, void* data,
                          napi_finalize finalize_cb, void* hint);

napi_value InstanceMethodCallbackWrapper(napi_env env,
                                                     napi_callback_info info);
napi_value InstanceGetterCallbackWrapper(napi_env env,
                                                     napi_callback_info info);
napi_value InstanceSetterCallbackWrapper(napi_env env,
                                                     napi_callback_info info);
napi_value StaticMethodCallbackWrapper(napi_env env,
                                                   napi_callback_info info);
napi_value StaticGetterCallbackWrapper(napi_env env,
                                                   napi_callback_info info);
napi_value StaticSetterCallbackWrapper(napi_env env,
                                                   napi_callback_info info);

void AttachPropData(Napi::Object obj, size_t props_count,
                                const napi_property_descriptor* props);
void FinalizeCallback(napi_env env, void* data, void* /*hint*/);
bool CheckIsConstructorCall(napi_env env, Napi::CallbackInfo& info);

template <typename T>
class NapiBaseWrapped : public T, public Napi::Reference<Napi::Object> {
 public:
  typedef Napi::ClassPropertyDescriptor<T> PropertyDescriptor;
  typedef Napi::Value (T::*InstanceCallback)(const Napi::CallbackInfo& info);
  typedef void (T::*InstanceSetterCallback)(const Napi::CallbackInfo& info,
                                            const Napi::Value& value);
  typedef Napi::MethodCallbackData<T, InstanceCallback>
      InstanceMethodCallbackData;
  typedef Napi::AccessorCallbackData<T, InstanceCallback,
                                     InstanceSetterCallback>
      InstanceAccessorCallbackData;

  typedef Napi::Value (*StaticMethodCallback)(const Napi::CallbackInfo& info);
  typedef void (*StaticSetterCallback)(const Napi::CallbackInfo& info,
                                       const Napi::Value& value);
  using StaticMethodCallbackData =
      Napi::MethodCallbackData<T, StaticMethodCallback>;
  using StaticAccessorCallbackData =
      Napi::AccessorCallbackData<T, StaticMethodCallback, StaticSetterCallback>;

  static PropertyDescriptor InstanceValue(
      const char* utf8name, napi_value value,
      napi_property_attributes attributes) {
    napi_property_descriptor desc = napi_property_descriptor();
    desc.utf8name = utf8name;
    desc.value = value;
    desc.attributes = attributes;
    return desc;
  }

  static PropertyDescriptor InstanceValue(
      Napi::Name name, napi_value value, napi_property_attributes attributes) {
    napi_property_descriptor desc = napi_property_descriptor();
    desc.name = name;
    desc.value = value;
    desc.attributes = attributes;
    return desc;
  }

  static PropertyDescriptor InstanceMethod(
      const char* utf8name, InstanceCallback method,
      napi_property_attributes attributes = napi_default,
      void* data = nullptr) {
    InstanceMethodCallbackData* callbackData =
        new InstanceMethodCallbackData({method, data});

    napi_property_descriptor desc = napi_property_descriptor();
    desc.utf8name = utf8name;
    desc.method = InstanceMethodCallbackWrapper;
    desc.data = callbackData;
    desc.attributes = attributes;
    return desc;
  }

  static PropertyDescriptor InstanceMethod(
      Napi::Name name, InstanceCallback method,
      napi_property_attributes attributes = napi_default,
      void* data = nullptr) {
    InstanceMethodCallbackData* callbackData =
        new InstanceMethodCallbackData({method, data});

    napi_property_descriptor desc = napi_property_descriptor();
    desc.name = name;
    desc.method = InstanceMethodCallbackWrapper;
    desc.data = callbackData;
    desc.attributes = attributes;
    return desc;
  }

  static PropertyDescriptor InstanceAccessor(
      const char* utf8name, InstanceCallback getter,
      InstanceSetterCallback setter = nullptr,
      napi_property_attributes attributes = napi_default,
      void* data = nullptr) {
    InstanceAccessorCallbackData* callbackData =
        new InstanceAccessorCallbackData({getter, setter, data});

    napi_property_descriptor desc = napi_property_descriptor();
    desc.utf8name = utf8name;
    desc.getter = getter != nullptr ? InstanceGetterCallbackWrapper : nullptr;
    desc.setter = setter != nullptr ? InstanceSetterCallbackWrapper : nullptr;
    desc.data = callbackData;
    desc.attributes = attributes;
    return desc;
  }

  static PropertyDescriptor InstanceAccessor(
      Napi::Name name, InstanceCallback getter,
      InstanceSetterCallback setter = nullptr,
      napi_property_attributes attributes = napi_default,
      void* data = nullptr) {
    InstanceAccessorCallbackData* callbackData =
        new InstanceAccessorCallbackData({getter, setter, data});

    napi_property_descriptor desc = napi_property_descriptor();
    desc.name = name;
    desc.getter = getter != nullptr ? InstanceGetterCallbackWrapper : nullptr;
    desc.setter = setter != nullptr ? InstanceSetterCallbackWrapper : nullptr;
    desc.data = callbackData;
    desc.attributes = attributes;
    return desc;
  }

  static Napi::Class DefineClass(
      Napi::Env env, const char* utf8name,
      const std::initializer_list<PropertyDescriptor>& properties,
      void* data = nullptr, napi_class super_class = nullptr) {
    return DefineClass(
        env, utf8name, properties.size(),
        reinterpret_cast<const napi_property_descriptor*>(properties.begin()),
        data, super_class);
  }

  static Napi::Class DefineClass(
      Napi::Env env, const char* utf8name,
      const std::vector<PropertyDescriptor>& properties, void* data = nullptr,
      napi_class super_class = nullptr) {
    return DefineClass(
        env, utf8name, properties.size(),
        reinterpret_cast<const napi_property_descriptor*>(properties.data()),
        data, super_class);
  }

  static Napi::Class DefineClass(
      Napi::Env env, const char* utf8name, const size_t props_count,
      const napi_property_descriptor* props, void* data = nullptr,
      napi_class super_class = nullptr) {
    Napi::Class clazz(env, lynx::binding::DefineClass(
                               env, utf8name, ConstructorCallbackWrapper,
                               props_count, props, data, super_class));
    auto fun = clazz.Get(env);
    AttachPropData(fun, props_count, props);
    return clazz;
  }

  static PropertyDescriptor StaticMethod(
      const char* utf8name, StaticMethodCallback method,
      napi_property_attributes attributes = napi_default,
      void* data = nullptr) {
    StaticMethodCallbackData* callbackData =
        new StaticMethodCallbackData({method, data});

    napi_property_descriptor desc = napi_property_descriptor();
    desc.utf8name = utf8name;
    desc.method = StaticMethodCallbackWrapper;
    desc.data = callbackData;
    desc.attributes =
        static_cast<napi_property_attributes>(attributes | napi_static);
    return desc;
  }

  static PropertyDescriptor StaticMethod(
      Napi::Name name, StaticMethodCallback method,
      napi_property_attributes attributes = napi_default,
      void* data = nullptr) {
    StaticMethodCallbackData* callbackData =
        new StaticMethodCallbackData({method, data});

    napi_property_descriptor desc = napi_property_descriptor();
    desc.name = name;
    desc.method = StaticMethodCallbackWrapper;
    desc.data = callbackData;
    desc.attributes =
        static_cast<napi_property_attributes>(attributes | napi_static);
    return desc;
  }

  static PropertyDescriptor StaticAccessor(
      const char* utf8name, StaticMethodCallback getter,
      StaticSetterCallback setter,
      napi_property_attributes attributes = napi_default,
      void* data = nullptr) {
    StaticAccessorCallbackData* callbackData =
        new StaticAccessorCallbackData({getter, setter, data});

    napi_property_descriptor desc = napi_property_descriptor();
    desc.utf8name = utf8name;
    desc.getter = getter != nullptr ? StaticGetterCallbackWrapper : nullptr;
    desc.setter = setter != nullptr ? StaticSetterCallbackWrapper : nullptr;
    desc.data = callbackData;
    desc.attributes =
        static_cast<napi_property_attributes>(attributes | napi_static);
    return desc;
  }

  static PropertyDescriptor StaticAccessor(
      Napi::Name name, StaticMethodCallback getter, StaticSetterCallback setter,
      napi_property_attributes attributes = napi_default,
      void* data = nullptr) {
    StaticAccessorCallbackData* callbackData =
        new StaticAccessorCallbackData({getter, setter, data});

    napi_property_descriptor desc = napi_property_descriptor();
    desc.name = name;
    desc.getter = getter != nullptr ? StaticGetterCallbackWrapper : nullptr;
    desc.setter = setter != nullptr ? StaticSetterCallbackWrapper : nullptr;
    desc.data = callbackData;
    desc.attributes =
        static_cast<napi_property_attributes>(attributes | napi_static);
    return desc;
  }

  static PropertyDescriptor StaticValue(
      const char* utf8name, napi_value value,
      napi_property_attributes attributes = napi_default) {
    napi_property_descriptor desc = napi_property_descriptor();
    desc.utf8name = utf8name;
    desc.value = value;
    desc.attributes =
        static_cast<napi_property_attributes>(attributes | napi_static);
    return desc;
  }

  static PropertyDescriptor StaticValue(
      Napi::Name name, napi_value value,
      napi_property_attributes attributes = napi_default) {
    napi_property_descriptor desc = napi_property_descriptor();
    desc.name = name;
    desc.value = value;
    desc.attributes =
        static_cast<napi_property_attributes>(attributes | napi_static);
    return desc;
  }

 private:
  NapiBaseWrapped(const Napi::CallbackInfo& callbackInfo) : T(callbackInfo) {
    napi_env env = callbackInfo.Env();
    napi_value wrapper = callbackInfo.This();

    static_assert(std::is_base_of<Napi::ScriptWrappable, T>::value,
                  "T must inherit ScriptWrappable");
    void* ptr = static_cast<Napi::ScriptWrappable*>(this);

    napi_ref ref = Wrap(env, wrapper, ptr, FinalizeCallback, nullptr);

    Napi::Reference<Napi::Object>* instanceRef = this;
    *instanceRef = Napi::Reference<Napi::Object>(env, ref);
  }

  static napi_value ConstructorCallbackWrapper(
      napi_env env, napi_callback_info info) {
    Napi::CallbackInfo callbackInfo(env, info);

    if (!CheckIsConstructorCall(env, callbackInfo)) {
      return nullptr;
    }

    new NapiBaseWrapped<T>(callbackInfo);

    return callbackInfo.This();
  }
};

}  // namespace binding
}  // namespace lynx

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_undefs.h"
#endif

#endif  // BINDING_NAPI_NAPI_BASE_WRAP_H_
