// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JSI_JSVM_JSVM_RUNTIME_WRAPPER_H_
#define CORE_RUNTIME_JSI_JSVM_JSVM_RUNTIME_WRAPPER_H_

#include <ark_runtime/jsvm.h>
#include <ark_runtime/jsvm_types.h>

#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {
class JSVMRuntimeInstance : public VMInstance {
 public:
  JSVMRuntimeInstance() = default;
  ~JSVMRuntimeInstance() override;

  JSRuntimeType GetRuntimeType() override { return JSRuntimeType::jsvm; };

  void InitInstance();

  JSVM_Env Env() const { return env_; };

 private:
  JSVM_VM vm_ = nullptr;
  JSVM_VMScope vm_scope_ = nullptr;
  JSVM_Env env_ = nullptr;
  JSVM_EnvScope env_scope_ = nullptr;
};
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JSI_JSVM_JSVM_RUNTIME_WRAPPER_H_
