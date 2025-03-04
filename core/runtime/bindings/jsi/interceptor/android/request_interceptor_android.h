// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_INTERCEPTOR_ANDROID_REQUEST_INTERCEPTOR_ANDROID_H_
#define CORE_RUNTIME_BINDINGS_JSI_INTERCEPTOR_ANDROID_REQUEST_INTERCEPTOR_ANDROID_H_

#include <memory>

#include "core/runtime/bindings/jsi/modules/android/callback_impl.h"
#include "core/runtime/bindings/jsi/modules/lynx_module_binding.h"
#include "core/runtime/bindings/jsi/modules/module_interceptor.h"

namespace lynx {
namespace piper {

class ModuleCallbackRequest : public ModuleCallbackAndroid {
 public:
  ModuleCallbackRequest(int64_t callback_id,
                        std::shared_ptr<MethodInvoker> invoker,
                        ModuleCallbackType type)
      : ModuleCallbackAndroid(callback_id, invoker), type_(type) {}
  void Invoke(Runtime* runtime, ModuleCallbackFunctionHolder* holder) override;

 private:
  [[maybe_unused]] const ModuleCallbackType type_;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_INTERCEPTOR_ANDROID_REQUEST_INTERCEPTOR_ANDROID_H_
