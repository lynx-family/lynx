/**
 * Copyright (c) 2017 Node.js API collaborators. All Rights Reserved.
 *
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file in the root of the source tree.
 */

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SRC_NAPI_COMMON_NAPI_STATE_H_
#define SRC_NAPI_COMMON_NAPI_STATE_H_

#include "js_native_api.h"
#ifdef USE_PRIMJS_NAPI
#include "primjs_napi_defines.h"
#endif
typedef struct napi_env_data__* napi_env_data;

struct napi_state__ {
  napi_extended_error_info last_error;
  napi_env_data env_data;
};

inline napi_status napi_clear_last_error(napi_env env) {
  env->state->last_error.error_code = napi_ok;

  env->state->last_error.engine_error_code = 0;
  return napi_ok;
}

inline napi_status napi_set_last_error(napi_env env, napi_status error_code) {
  env->state->last_error.error_code = error_code;
  return error_code;
}
#ifdef USE_PRIMJS_NAPI
#include "primjs_napi_undefs.h"
#endif
#endif  // SRC_NAPI_COMMON_NAPI_STATE_H_
