// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/vm/lepus/lepus_error_helper.h"

#include "core/build/gen/lynx_sub_error_code.h"

namespace lynx {
namespace lepus {
std::string LepusErrorHelper::GetErrorStack(LEPUSContext* ctx,
                                            LEPUSValue& value) {
  std::string err;
  if (LEPUS_IsError(ctx, value) || LEPUS_IsException(value)) {
    LEPUSValue val = LEPUS_GetPropertyStr(ctx, value, "stack");
    bool gc_flag = LEPUS_IsGCMode(ctx);
    if (!LEPUS_IsUndefined(val)) {
      const char* stack = LEPUS_ToCString(ctx, val);
      if (stack) {
        err.append(stack);
      }
      if (!gc_flag) LEPUS_FreeCString(ctx, stack);
    }
    if (!gc_flag) LEPUS_FreeValue(ctx, val);
  }
  return err;
}

std::string LepusErrorHelper::GetErrorMessage(LEPUSContext* ctx,
                                              LEPUSValue& exception_value) {
  auto str = LEPUS_ToCString(ctx, exception_value);
  std::string error_msg;
  if (str) {
    error_msg.append(str);
  }
  if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeCString(ctx, str);
  return error_msg;
}

int32_t LepusErrorHelper::GetErrorCode(LEPUSContext* ctx,
                                       LEPUSValue exception_value) {
  int32_t code = error::E_MTS_RUNTIME_ERROR;
  if (LEPUS_IsError(ctx, exception_value)) {
    LEPUSValue val = LEPUS_GetPropertyStr(ctx, exception_value, err_code_prop_);
    if (LEPUS_VALUE_IS_INT(val)) {
      code = LEPUS_VALUE_GET_INT(val);
    }
    if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeValue(ctx, val);
  }
  return code;
}
}  // namespace lepus
}  // namespace lynx
