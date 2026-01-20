// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_LEPUS_JSON_API_H_
#define CORE_RUNTIME_LEPUS_JSON_API_H_

#include <string>
#include <utility>

#include "core/runtime/lepus/json_parser.h"
#include "core/runtime/lepus/restricted_value.h"

namespace lynx {
namespace lepus {
RestrictedValue Stringify(VMContext* context) {
  long params_count = context->GetParamsSize();
  DCHECK(params_count == 1);
  auto* arg = context->GetParam(0);
  if (arg->IsString()) {
    return RestrictedValue(arg->String());
  } else if (arg->IsNil() || arg->IsUndefined()) {
    BASE_STATIC_STRING_DECL(kNull, "null");
    return RestrictedValue(kNull);
  }
  DCHECK(arg->IsTable() || arg->IsArray());
  return RestrictedValue(lepusValueToJSONString(Value(*arg)));
}

RestrictedValue Parse(VMContext* context) {
  long params_count = context->GetParamsSize();
  DCHECK(params_count == 1);
  auto* arg = context->GetParam(0);
  Value res;
  if (arg->IsString()) {
    res = jsonValueTolepusValue(arg->CString());
  } else {
    // other type
    res = jsonValueTolepusValue("");
  }
  return RestrictedValue(res);
}

void RegisterJSONAPI(Context* ctx) {
  static BuiltinFunctionTable apis(BuiltinFunctionTable::Json,
                                   {
                                       {"stringify", &Stringify},
                                       {"parse", &Parse},
                                   });
  RegisterBuiltinFunctionTable(ctx, "JSON", &apis);
}
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_JSON_API_H_
