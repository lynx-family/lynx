// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/jsi/quickjs/quickjs_context_wrapper.h"

#include <climits>

#include "base/include/log/logging.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/jsi/quickjs/quickjs_helper.h"
#include "core/runtime/jsi/quickjs/quickjs_runtime_wrapper.h"

namespace lynx {
namespace piper {

QuickjsContextWrapper::QuickjsContextWrapper(std::shared_ptr<VMInstance> vm)
    : JSIContext(vm) {
  std::shared_ptr<QuickjsRuntimeInstance> iso =
      std::static_pointer_cast<QuickjsRuntimeInstance>(vm);

  LEPUSRuntime* rt = iso->Runtime();

  LEPUSContext* ctx;
  ctx = LEPUS_NewContext(rt);
  if (!ctx) {
    LOGR("init quickjs context failed!");
    return;
  }
  ctx_ = ctx;
  // register webassembly here, on ctx.global
  RegisterWasmFunc()(ctx, nullptr);

  LEPUS_SetMaxStackSize(ctx_, static_cast<size_t>(ULLONG_MAX));
}

QuickjsContextWrapper::~QuickjsContextWrapper() {
  if (ctx_) {
    LEPUS_FreeContext(ctx_);
  }
  LOGI("~QuickjsContextWrapper " << this << " LEPUSContext:" << ctx_);
}

LEPUSRuntime* QuickjsContextWrapper::getRuntime() const {
  std::shared_ptr<QuickjsRuntimeInstance> vm =
      std::static_pointer_cast<QuickjsRuntimeInstance>(vm_);
  return vm->Runtime();
}

LEPUSContext* QuickjsContextWrapper::getContext() const { return ctx_; }

// static
RegisterWasmFuncType QuickjsContextWrapper::register_wasm_func_ = [](void*,
                                                                     void*) {};
// static
RegisterWasmFuncType& QuickjsContextWrapper::RegisterWasmFunc() {
  static RegisterWasmFuncType RegisterWebAssembly = register_wasm_func_;
  return RegisterWebAssembly;
}

}  // namespace piper
}  // namespace lynx
