//  Copyright (c) Facebook, Inc. and its affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSI_JSC_JSC_HOST_OBJECT_H_
#define CORE_RUNTIME_JSI_JSC_JSC_HOST_OBJECT_H_

#include <JavaScriptCore/JavaScript.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "core/runtime/jsi/jsc/jsc_helper.h"
#include "core/runtime/jsi/jsc/jsc_runtime.h"
#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {
class JSCRuntime;
namespace detail {

struct JSCHostObjectProxy : public HostObjectWrapperBase<HostObject> {
 public:
  JSCHostObjectProxy(JSCRuntime* rt, std::shared_ptr<HostObject> sho)
      : HostObjectWrapperBase(rt, std::move(sho)) {}

  ~JSCHostObjectProxy() override = default;

  static JSValueRef getProperty(JSContextRef ctx, JSObjectRef object,
                                JSStringRef propertyName,
                                JSValueRef* exception);

  static bool setProperty(JSContextRef ctx, JSObjectRef object,
                          JSStringRef propertyName, JSValueRef value,
                          JSValueRef* exception);

  static void getPropertyNames(
      JSContextRef ctx, JSObjectRef object,
      JSPropertyNameAccumulatorRef propertyNames) noexcept;

  static void finalize(JSObjectRef obj);

  static Object createObject(JSCRuntime& rt, JSGlobalContextRef ctx,
                             std::shared_ptr<HostObject> ho);

  static JSClassRef getHostObjectClass();

  friend class JSCRuntime;
};

}  // namespace detail
}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JSI_JSC_JSC_HOST_OBJECT_H_
