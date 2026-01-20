// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_LEPUS_BUILTIN_H_
#define CORE_RUNTIME_LEPUS_BUILTIN_H_

#include "base/include/value/table.h"
#include "core/renderer/tasm/config.h"
#include "core/runtime/lepus/builtin_function_table.h"
#include "core/runtime/lepus/context.h"
#include "core/runtime/lepus/vm_context.h"
#include "core/runtime/lepusng/jsvalue_helper.h"
#include "core/runtime/lepusng/quick_context.h"

namespace lynx {
namespace lepus {
void RegisterBuiltin(Context* context);
void RegisterCFunction(Context* context, const char* name, CFunction function);
void RegisterBuiltinFunction(Context* context, const char* name,
                             CFunction function);
void RegisterBuiltinFunction(Context* context, const char* name,
                             CFunctionBuiltin function);
void RegisterBuiltinFunctionTable(Context* context, const char* name,
                                  BuiltinFunctionTable* function_table);

void RegisterFunctionTable(Context* context, const char* name,
                           fml::RefPtr<Dictionary> function);

void RegisterFunctionTable(Context* context, const char* name,
                           BuiltinFunctionTable* function_table);

void RegisterTableFunction(Context* context,
                           const fml::RefPtr<Dictionary>& table,
                           const char* name, CFunction function);
void RegisterTableFunction(Context* context,
                           const fml::RefPtr<Dictionary>& table,
                           const char* name, CFunctionBuiltin function);
inline void RegisterNGCFunction(Context* ctx, const char* name,
                                LEPUSCFunction* function) {
  QuickContext* quick_ctx = QuickContext::Cast(ctx);
  quick_ctx->RegisterGlobalFunction(name, function);
  return;
}

inline void RegisterNGCFunction(Context* ctx,
                                const RenderBindingFunction* funcs,
                                size_t size) {
  lepus::QuickContext::Cast(ctx)->RegisterGlobalFunction(funcs, size);
  return;
}

inline void RegisterObjectNGCFunction(Context* ctx, lepus::Value& obj,
                                      const RenderBindingFunction* funcs,
                                      size_t size) {
  auto* qctx = lepus::QuickContext::Cast(ctx);
  qctx->RegisterObjectFunction(obj, funcs, size);
  return;
}
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_BUILTIN_H_
