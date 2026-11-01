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

// Keep this public header usable with OpenHarmony SDKs without JSVM headers.
typedef struct JSVM_Env__* JSVM_Env;
EXTERN_C_START

// Always false in ENABLE_JSVM=OFF builds. Probe before creating a JSVM backend.
NAPI_EXTERN bool napi_is_harmony_available(void);
// On failure, the existing context and engine function table are unchanged.
NAPI_EXTERN napi_status napi_try_attach_harmony(napi_env, JSVM_Env);
NAPI_EXTERN void napi_attach_harmony(napi_env, JSVM_Env);
NAPI_EXTERN void napi_detach_harmony(napi_env);

EXTERN_C_END
#ifdef USE_PRIMJS_NAPI
#include "primjs_napi_undefs.h"
#endif
#endif
