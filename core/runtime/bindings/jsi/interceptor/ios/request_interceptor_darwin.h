// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_INTERCEPTOR_IOS_REQUEST_INTERCEPTOR_DARWIN_H_
#define CORE_RUNTIME_BINDINGS_JSI_INTERCEPTOR_IOS_REQUEST_INTERCEPTOR_DARWIN_H_

#include <memory>
#include <string>

#include "core/runtime/bindings/jsi/modules/lynx_jsi_module_callback.h"
#include "core/runtime/bindings/jsi/modules/lynx_module.h"
#include "core/runtime/bindings/jsi/modules/lynx_module_binding.h"

namespace lynx {
namespace piper {

class ModuleCallbackRequest : public ModuleCallback {
 public:
  ModuleCallbackRequest(int64_t callback_id, ModuleCallbackType type)
      : ModuleCallback(callback_id), type_(type) {}
  void Invoke(Runtime* runtime, ModuleCallbackFunctionHolder* holder) override;

 private:
  [[maybe_unused]] const ModuleCallbackType type_;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_INTERCEPTOR_IOS_REQUEST_INTERCEPTOR_DARWIN_H_
