// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef NAPI_HARMONY_NAPI_ENV_HARMONY_H_
#define NAPI_HARMONY_NAPI_ENV_HARMONY_H_
#include "js_native_api.h"
#include "js_native_api_types.h"

#ifdef USE_PRIMJS_NAPI
#include "primjs_napi_defines.h"
#endif

#include "ark_runtime/jsvm.h"
EXTERN_C_START

NAPI_EXTERN void napi_attach_harmony(napi_env, JSVM_Env);
NAPI_EXTERN void napi_detach_harmony(napi_env);

EXTERN_C_END
#ifdef USE_PRIMJS_NAPI
#include "primjs_napi_undefs.h"
#endif
#endif
