// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SRC_NAPI_JSC_NAPI_ENV_JSC_H_
#define SRC_NAPI_JSC_NAPI_ENV_JSC_H_

#include <JavaScriptCore/JavaScriptCore.h>

#include "js_native_api.h"

#ifdef USE_PRIMJS_NAPI
#include "primjs_napi_defines.h"
#endif

EXTERN_C_START

NAPI_EXTERN void napi_attach_jsc(napi_env env, JSGlobalContextRef global_ctx);

NAPI_EXTERN void napi_detach_jsc(napi_env env);

NAPI_EXTERN JSGlobalContextRef napi_get_env_context_jsc(napi_env env);

NAPI_EXTERN JSValueRef napi_js_value_to_jsc_value(napi_env env,
                                                  napi_value value);

NAPI_EXTERN napi_value napi_jsc_value_to_js_value(napi_env env,
                                                  JSValueRef value);

EXTERN_C_END

#ifdef USE_PRIMJS_NAPI
#include "primjs_napi_undefs.h"
#endif

#endif  // SRC_NAPI_JSC_NAPI_ENV_JSC_H_
