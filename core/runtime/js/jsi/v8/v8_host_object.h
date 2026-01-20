// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_JSI_V8_V8_HOST_OBJECT_H_
#define CORE_RUNTIME_JS_JSI_V8_V8_HOST_OBJECT_H_

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

#if V8_MAJOR_VERSION >= 14
using v8_property_result = v8::Intercepted;
#else
using v8_property_result = void;
#endif

namespace detail {

// HostObject details
struct V8HostObjectProxy : public HostObjectWrapperBase<V8Runtime, HostObject> {
 public:
  V8HostObjectProxy(V8Runtime* rt, std::shared_ptr<HostObject> sho);
  ~V8HostObjectProxy() override = default;

  static v8_property_result getProperty(
      v8::Local<v8::Name> property,
      const v8::PropertyCallbackInfo<v8::Value>& info);

  static v8_property_result setProperty(
      v8::Local<v8::Name> property, v8::Local<v8::Value> value,
#if V8_MAJOR_VERSION >= 14
      const v8::PropertyCallbackInfo<void>& info);
#else
      const v8::PropertyCallbackInfo<v8::Value>& info);
#endif

  static void getPropertyNames(const v8::PropertyCallbackInfo<v8::Array>& info);

  static Object createObject(V8Runtime* rt, v8::Local<v8::Context> context,
                             std::shared_ptr<HostObject> ho);

  static void onFinalize(const v8::WeakCallbackInfo<V8HostObjectProxy>& data);

  constexpr static int HOST_OBJ_COUNT = 1;

  v8::Persistent<v8::Object> keeper_;

  friend class V8Runtime;
};

};  // namespace detail

}  // namespace js

}  // namespace runtime
}  // namespace lynx
#endif  // CORE_RUNTIME_JS_JSI_V8_V8_HOST_OBJECT_H_
