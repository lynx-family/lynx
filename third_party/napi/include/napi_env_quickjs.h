/**
 * Copyright (c) 2017 Node.js API collaborators. All Rights Reserved.
 *
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file in the root of the source tree.
 */
// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef SRC_NAPI_QUICKJS_NAPI_ENV_QUICKJS_H_
#define SRC_NAPI_QUICKJS_NAPI_ENV_QUICKJS_H_

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus
#include "quickjs/include/quickjs.h"
#ifdef __cplusplus
}
#endif  // __cplusplus

#include "js_native_api.h"
#ifdef USE_PRIMJS_NAPI
#include "primjs_napi_defines.h"
#endif
EXTERN_C_START

NAPI_EXTERN void napi_attach_quickjs(napi_env env, LEPUSContext* ctx);

NAPI_EXTERN void napi_detach_quickjs(napi_env env);

NAPI_EXTERN LEPUSContext* napi_get_env_context_quickjs(napi_env env);

// return value should be freed by caller
NAPI_EXTERN LEPUSValue napi_js_value_to_quickjs_value(napi_env env,
                                                      napi_value value);

// input value is freed by napi
// return value is only valid in current handle scope
NAPI_EXTERN napi_value napi_quickjs_value_to_js_value(napi_env env,
                                                      LEPUSValue value);

EXTERN_C_END

#ifdef USE_PRIMJS_NAPI
#include "primjs_napi_undefs.h"
#endif
#endif  // SRC_NAPI_QUICKJS_NAPI_ENV_QUICKJS_H_
