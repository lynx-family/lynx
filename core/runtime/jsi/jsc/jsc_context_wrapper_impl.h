// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JSI_JSC_JSC_CONTEXT_WRAPPER_IMPL_H_
#define CORE_RUNTIME_JSI_JSC_JSC_CONTEXT_WRAPPER_IMPL_H_

#include <memory>

#include "core/runtime/jsi/jsc/jsc_context_wrapper.h"

namespace lynx {
namespace piper {
using RegisterWasmFuncType = void (*)(void*, void*);

class JSCContextWrapperImpl : public JSCContextWrapper {
 public:
  JSCContextWrapperImpl(std::shared_ptr<VMInstance> vm);
  ~JSCContextWrapperImpl() override;
  void init() override;

  const std::atomic<bool>& contextInvalid() const override;
  std::atomic<intptr_t>& objectCounter() const override;

  JSGlobalContextRef getContext() const override;

  static RegisterWasmFuncType& RegisterWasmFunc();

  static RegisterWasmFuncType register_wasm_func_;

 private:
  JSGlobalContextRef ctx_;
  std::atomic<bool> ctx_invalid_;
  mutable std::atomic<intptr_t> objectCounter_;
};

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JSI_JSC_JSC_CONTEXT_WRAPPER_IMPL_H_
