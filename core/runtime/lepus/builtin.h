// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_LEPUS_BUILTIN_H_
#define CORE_RUNTIME_LEPUS_BUILTIN_H_

#include "base/include/value/table.h"
#include "core/renderer/tasm/config.h"
#include "core/runtime/lepus/builtin_function_table.h"
#include "core/runtime/lepus/vm_context.h"
#include "core/runtime/lepusng/jsvalue_helper.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/shell/runtime/mts/mts_runtime.h"

namespace lynx {
namespace lepus {
void RegisterBuiltin(VMContext* context);
void RegisterCFunction(VMContext* context, const char* name,
                       CFunction function);
void RegisterBuiltinFunction(VMContext* context, const char* name,
                             CFunction function);
void RegisterBuiltinFunction(VMContext* context, const char* name,
                             CFunctionBuiltin function);
void RegisterBuiltinFunctionTable(VMContext* context, const char* name,
                                  BuiltinFunctionTable* function_table);

void RegisterFunctionTable(VMContext* context, const char* name,
                           fml::RefPtr<Dictionary> function);

void RegisterFunctionTable(VMContext* context, const char* name,
                           BuiltinFunctionTable* function_table);

void RegisterTableFunction(VMContext* context,
                           const fml::RefPtr<Dictionary>& table,
                           const char* name, CFunction function);
void RegisterTableFunction(VMContext* context,
                           const fml::RefPtr<Dictionary>& table,
                           const char* name, CFunctionBuiltin function);
inline void RegisterNGCFunction(QuickContext* quick_ctx, const char* name,
                                LEPUSCFunction* function) {
  quick_ctx->RegisterGlobalFunction(name, function);
  return;
}

inline void RegisterNGCFunction(QuickContext* ctx,
                                const runtime::RenderBindingFunction* funcs,
                                size_t size) {
  ctx->RegisterGlobalFunction(funcs, size);
  return;
}

inline void RegisterObjectNGCFunction(
    runtime::MTSRuntime* ctx, lepus::Value& obj,
    const runtime::RenderBindingFunction* funcs, size_t size) {
  auto* qctx = runtime::MTSRuntime::ToQuickContext(ctx);
  qctx->RegisterObjectFunction(obj, funcs, size);
  return;
}
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_BUILTIN_H_
