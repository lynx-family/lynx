//  Copyright (c) Facebook, Inc. and its affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSI_JSC_JSC_HOST_FUNCTION_H_
#define CORE_RUNTIME_JSI_JSC_JSC_HOST_FUNCTION_H_

#include <JavaScriptCore/JavaScript.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "core/runtime/jsi/jsc/jsc_runtime.h"
#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {
class JSCRuntime;
namespace detail {

class HostFunctionMetadata : public HostObjectWrapperBase<HostFunctionType> {
 public:
  HostFunctionMetadata(JSCRuntime* rt, HostFunctionType hf, unsigned ac,
                       JSStringRef n)
      : HostObjectWrapperBase(
            rt, std::make_shared<HostFunctionType>(std::move(hf))),
        argCount_(ac),
        name_(JSStringRetain(n)) {}
  ~HostFunctionMetadata() override = default;

  static Function createFunctionFromHostFunction(JSCRuntime& rt,
                                                 JSGlobalContextRef ctx,
                                                 const PropNameID& name,
                                                 unsigned int paramCount,
                                                 HostFunctionType func);

  static JSClassRef getHostFunctionClass();

  static void initialize(JSContextRef ctx, JSObjectRef object);

  static JSValueRef makeError(JSGlobalContextRef ctx, JSCRuntime& rt,
                              const std::string& desc);

  static JSValueRef call(JSContextRef ctx, JSObjectRef function,
                         JSObjectRef thisObject, size_t argumentCount,
                         const JSValueRef arguments[], JSValueRef* exception);

  static void finalize(JSObjectRef object);

  unsigned argCount_;
  JSStringRef name_;
};

}  // namespace detail
}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JSI_JSC_JSC_HOST_FUNCTION_H_
