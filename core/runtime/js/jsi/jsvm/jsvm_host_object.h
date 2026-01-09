// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JS_JSI_JSVM_JSVM_HOST_OBJECT_H_
#define CORE_RUNTIME_JS_JSI_JSVM_JSVM_HOST_OBJECT_H_

#include <memory>

#include "core/runtime/js/jsi/jsi.h"
#include "core/runtime/js/jsi/jsvm/jsvm_runtime.h"

namespace lynx {
namespace piper {
namespace detail {
class JSVMHostObjectProxy
    : public HostObjectWrapperBase<JSVMRuntime, HostObject> {
 public:
  JSVMHostObjectProxy(JSVMRuntime* rt, std::shared_ptr<piper::HostObject> ho);
  ~JSVMHostObjectProxy() override = default;
  static JSVM_Value getProperty(JSVM_Env env, JSVM_Value name,
                                JSVM_Value this_arg, JSVM_Value data);
  static JSVM_Value setProperty(JSVM_Env env, JSVM_Value name,
                                JSVM_Value property, JSVM_Value this_arg,
                                JSVM_Value data);

  static JSVM_Value getPropertyNames(JSVM_Env env, JSVM_Value this_arg,
                                     JSVM_Value data);
  static piper::Object createObject(JSVMRuntime* rt, JSVM_Env env,
                                    std::shared_ptr<piper::HostObject> ho);
  static const JSVM_TypeTag* GetHostObjectTag();
  static void onFinalize(JSVM_Env env, void* finalizeData, void* finalizeHint);
};
}  // namespace detail
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_JSI_JSVM_JSVM_HOST_OBJECT_H_
