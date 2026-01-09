// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JS_JSI_JSC_JSC_CONTEXT_WRAPPER_H_
#define CORE_RUNTIME_JS_JSI_JSC_JSC_CONTEXT_WRAPPER_H_

#include <JavaScriptCore/JavaScript.h>

#include <atomic>
#include <memory>
#include <unordered_map>

#include "core/runtime/js/jsi/jsi.h"

namespace lynx {
namespace piper {

using RegisterWasmFuncType = void (*)(void*, void*);

class JSCContextWrapper : public JSIContext {
 public:
  JSCContextWrapper(std::shared_ptr<VMInstance> vm);
  ~JSCContextWrapper() override;
  void init();

  std::atomic<intptr_t>& objectCounter() const;

  JSGlobalContextRef getContext() const;

  static RegisterWasmFuncType& RegisterWasmFunc();

  static RegisterWasmFuncType register_wasm_func_;

 private:
  JSGlobalContextRef ctx_;
  // only use for wasm
  std::atomic<bool> ctx_invalid_;
  mutable std::atomic<intptr_t> objectCounter_;
};

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JS_JSI_JSC_JSC_CONTEXT_WRAPPER_H_
