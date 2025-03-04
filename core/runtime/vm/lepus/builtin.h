// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_BUILTIN_H_
#define CORE_RUNTIME_VM_LEPUS_BUILTIN_H_

#include "core/renderer/tasm/config.h"
#include "core/runtime/vm/lepus/context.h"
#include "core/runtime/vm/lepus/jsvalue_helper.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/runtime/vm/lepus/vm_context.h"

namespace lynx {
namespace lepus {
void RegisterBuiltin(Context* context);
void RegisterCFunction(Context* context, const char* name, CFunction function);
void RegisterBuiltinFunction(Context* context, const char* name,
                             CFunction function);
inline void RegisterBuiltinFunction(Context* context, const char* name,
                                    Value (*function)(VMContext*)) {
  RegisterBuiltinFunction(context, name, reinterpret_cast<CFunction>(function));
}
void RegisterBuiltinFunctionTable(Context* context, const char* name,
                                  fml::RefPtr<Dictionary> function);
void RegisterFunctionTable(Context* context, const char* name,
                           fml::RefPtr<Dictionary> function);
void RegisterTableFunction(Context* context,
                           const fml::RefPtr<Dictionary>& table,
                           const char* name, CFunction function);
inline void RegisterTableFunction(Context* context,
                                  const fml::RefPtr<Dictionary>& table,
                                  const char* name,
                                  Value (*function)(VMContext*)) {
  RegisterTableFunction(context, table, name,
                        reinterpret_cast<CFunction>(function));
}

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
                                      const char* name, LEPUSCFunction* func) {
  LEPUSValue cf = LEPUS_NewCFunction(ctx->context(), func, name, 0);
  Value value(ctx->context(), cf);  // for tracing gc
  LEPUSValueHelper::SetProperty(ctx->context(), obj.WrapJSValue(), name, cf);
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

#endif  // CORE_RUNTIME_VM_LEPUS_BUILTIN_H_
