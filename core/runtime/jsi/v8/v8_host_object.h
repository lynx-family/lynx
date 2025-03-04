// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSI_V8_V8_HOST_OBJECT_H_
#define CORE_RUNTIME_JSI_V8_V8_HOST_OBJECT_H_

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

#include "core/base/observer/observer.h"
#include "core/runtime/jsi/jsi.h"
#include "v8.h"

namespace lynx {
namespace piper {
class V8Runtime;

namespace detail {

// HostObject details
struct V8HostObjectProxy : public HostObjectWrapperBase<HostObject> {
 public:
  V8HostObjectProxy(V8Runtime* rt, std::shared_ptr<piper::HostObject> sho);
  ~V8HostObjectProxy() override = default;

  static void getProperty(v8::Local<v8::Name> property,
                          const v8::PropertyCallbackInfo<v8::Value>& info);

  static void setProperty(v8::Local<v8::Name> property,
                          v8::Local<v8::Value> value,
                          const v8::PropertyCallbackInfo<v8::Value>& info);

  static void getPropertyNames(const v8::PropertyCallbackInfo<v8::Array>& info);

  static piper::Object createObject(V8Runtime* rt,
                                    v8::Local<v8::Context> context,
                                    std::shared_ptr<piper::HostObject> ho);

  static void onFinalize(const v8::WeakCallbackInfo<V8HostObjectProxy>& data);

  constexpr static int HOST_OBJ_COUNT = 1;

  v8::Persistent<v8::Object> keeper_;

  friend class V8Runtime;
};

};  // namespace detail

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JSI_V8_V8_HOST_OBJECT_H_
