// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JSI_QUICKJS_QUICKJS_CONTEXT_WRAPPER_H_
#define CORE_RUNTIME_JSI_QUICKJS_QUICKJS_CONTEXT_WRAPPER_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "core/runtime/jsi/jsi.h"

struct LEPUSContext;
struct LEPUSRuntime;

namespace lynx {
namespace piper {
using RegisterWasmFuncType = void (*)(void*, void*);

class BASE_EXPORT_FOR_DEVTOOL QuickjsContextWrapper : public piper::JSIContext {
 public:
  QuickjsContextWrapper(std::shared_ptr<VMInstance> vm);
  ~QuickjsContextWrapper();

  LEPUSContext* getContext() const;
  LEPUSRuntime* getRuntime() const;

  static RegisterWasmFuncType& RegisterWasmFunc();

  static RegisterWasmFuncType register_wasm_func_;

 private:
  LEPUSContext* ctx_;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JSI_QUICKJS_QUICKJS_CONTEXT_WRAPPER_H_
