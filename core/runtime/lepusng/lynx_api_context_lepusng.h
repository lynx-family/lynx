// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUSNG_LYNX_API_CONTEXT_LEPUSNG_H_
#define CORE_RUNTIME_LEPUSNG_LYNX_API_CONTEXT_LEPUSNG_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LYNX_VALUE_COMPILE_UNIT
#define LYNX_VALUE_COMPILE_UNIT lepusng
#endif

#include "base/include/value/lynx_value_types.h"
#include "quickjs/include/quickjs.h"

#ifdef __cplusplus
}
#endif

struct lynx_api_context__lepusng {
  lynx_api_context__lepusng(lynx_api_env env, LEPUSContext* ctx)
      : env(env), rt{LEPUS_GetRuntime(ctx)}, ctx{ctx} {
    env->ctx = this;
  }

  lynx_api_env env;
  LEPUSRuntime* rt{};
  LEPUSContext* ctx{};
};

#endif  // CORE_RUNTIME_LEPUSNG_LYNX_API_CONTEXT_LEPUSNG_H_
