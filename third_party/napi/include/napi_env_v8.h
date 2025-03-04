/**
 * Copyright (c) 2017 Node.js API collaborators. All Rights Reserved.
 *
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file in the root of the source tree.
 */

#if !defined(SRC_NAPI_ENV_V8_H_) && !defined(_NAPI_V8_EXPORT_SOURCE_ONLY_)
#define SRC_NAPI_ENV_V8_H_

#include <v8.h>

#include "js_native_api.h"
#ifdef USE_PRIMJS_NAPI
#include "primjs_napi_defines.h"
#endif
NAPI_EXTERN void napi_attach_v8(napi_env env, v8::Local<v8::Context> ctx);

NAPI_EXTERN void napi_detach_v8(napi_env env);

NAPI_EXTERN v8::Local<v8::Context> napi_get_env_context_v8(napi_env env);

NAPI_EXTERN v8::Local<v8::Value> napi_js_value_to_v8_value(napi_env env,
                                                           napi_value value);

NAPI_EXTERN napi_value napi_v8_value_to_js_value(napi_env env,
                                                 v8::Local<v8::Value> value);
#ifdef USE_PRIMJS_NAPI
#include "primjs_napi_undefs.h"
#endif
#endif
