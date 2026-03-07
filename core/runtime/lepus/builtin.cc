// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/lepus/builtin.h"

#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/runtime/lepus/array_api.h"
#include "core/runtime/lepus/base_api.h"
#include "core/runtime/lepus/date_api.h"
#include "core/runtime/lepus/function_api.h"
#include "core/runtime/lepus/json_api.h"
#include "core/runtime/lepus/lepus_date_api.h"
#include "core/runtime/lepus/math_api.h"
#include "core/runtime/lepus/regexp_api.h"
#include "core/runtime/lepus/string_api.h"
#include "core/runtime/lepus/table_api.h"
#include "core/runtime/lepusng/jsvalue_helper.h"

namespace lynx {
namespace lepus {

void RegisterCFunction(lepus::VMContext* context, const char* name,
                       CFunction function) {
  context->SetGlobalData(name, lepus::Value(function));
}

void RegisterBuiltinFunction(lepus::VMContext* context, const char* name,
                             CFunction function) {
  context->SetBuiltinData(name, lepus::Value(function));
}

void RegisterBuiltinFunction(lepus::VMContext* context, const char* name,
                             CFunctionBuiltin function) {
  context->SetBuiltinData(name, lepus::Value(function));
}

void RegisterBuiltinFunctionTable(lepus::VMContext* context, const char* name,
                                  BuiltinFunctionTable* function_table) {
  context->builtin()->Set(name, lepus::Value(function_table));
}

void RegisterFunctionTable(lepus::VMContext* context, const char* name,
                           fml::RefPtr<Dictionary> table) {
  context->global()->Set(name, lepus::Value(std::move(table)));
}

void RegisterFunctionTable(lepus::VMContext* context, const char* name,
                           BuiltinFunctionTable* function_table) {
  context->global()->Set(name, lepus::Value(function_table));
}

void RegisterTableFunction(VMContext* context,
                           const fml::RefPtr<Dictionary>& table,
                           const char* name, CFunction function) {
  table->SetValue(name, function);
}

void RegisterTableFunction(VMContext* context,
                           const fml::RefPtr<Dictionary>& table,
                           const char* name, CFunctionBuiltin function) {
  table->SetValue(name, function);
}

void RegisterBuiltin(lepus::VMContext* context) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, REGISTER_BUILD_IN);
  RegisterBaseAPI(context);
  RegisterStringAPI(context);
  RegisterMathAPI(context);
  RegisterDateAPI(context);
  RegisterJSONAPI(context);
  if (lynx::tasm::Config::IsHigherOrEqual(
          reinterpret_cast<VMContext*>(context)->GetSdkVersion(),
          FEATURE_LEPUS_CLOSURE_AND_SWITCH_VERSION)) {
    RegisterLepusDateAPI(context);
    RegisterFunctionAPI(context);
    RegisterTableAPI(context);
  }
}

}  // namespace lepus
}  // namespace lynx
