// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_JSON_API_H_
#define CORE_RUNTIME_VM_LEPUS_JSON_API_H_

#include <string>
#include <utility>

#include "core/runtime/vm/lepus/json_parser.h"

namespace lynx {
namespace lepus {
Value Stringify(VMContext* context) {
  long params_count = context->GetParamsSize();
  DCHECK(params_count == 1);
  Value* arg = context->GetParam(0);
  if (arg->IsString()) {
    return Value(arg->String());
  } else if (arg->IsNil() || arg->IsUndefined()) {
    BASE_STATIC_STRING_DECL(kNull, "null");
    return Value(kNull);
  }
  DCHECK(arg->IsTable() || arg->IsArray());
  return Value(lepusValueToJSONString(*arg));
}

Value Parse(VMContext* context) {
  long params_count = context->GetParamsSize();
  DCHECK(params_count == 1);
  Value* arg = context->GetParam(0);
  Value res;
  if (arg->IsString()) {
    res = jsonValueTolepusValue(arg->CString());
  } else {
    // other type
    res = jsonValueTolepusValue("");
  }
  return res;
}

void RegisterJSONAPI(Context* ctx) {
  fml::RefPtr<Dictionary> table = Dictionary::Create();
  RegisterTableFunction(ctx, table, "stringify", &Stringify);
  RegisterTableFunction(ctx, table, "parse", &Parse);
  RegisterBuiltinFunctionTable(ctx, "JSON", std::move(table));
}
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_VM_LEPUS_JSON_API_H_
