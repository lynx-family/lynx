/**
 * Copyright (c) 2017 Node.js API collaborators. All Rights Reserved.
 *
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file in the root of the source tree.
 */

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SRC_NAPI_NAPI_MODULE_H_
#define SRC_NAPI_NAPI_MODULE_H_

#include "js_native_api.h"
#ifdef USE_PRIMJS_NAPI
#include "primjs_napi_defines.h"
#endif
typedef napi_value (*napi_addon_register_func)(napi_env env,
                                               napi_value exports);

#define NAPI_MODULE_VERSION 1

typedef struct napi_module {
  int nm_version;
  const char* nm_filename;
  napi_addon_register_func nm_register_func;
  const char* nm_modname;
  struct napi_module* nm_link;
} napi_module;

#define NAPI_C_CTOR_PRIMJS(fn)                \
  void fn(void) __attribute__((constructor)); \
  void fn(void)

EXTERN_C_START
// do not use `napi_module_register` to prevent symbol conflict with node.js
NAPI_EXTERN void napi_module_register_xx(napi_module* mod);

NAPI_EXTERN const napi_module* napi_find_module(const char* name);
EXTERN_C_END

#define NAPI_MODULE_PRIMJS(modname, regfunc)                            \
  EXTERN_C_START                                                        \
  static napi_module _module = {NAPI_MODULE_VERSION, __FILE__, regfunc, \
                                #modname, 0};                           \
  NAPI_C_CTOR_PRIMJS(_napi_register_xx_##modname) {                     \
    napi_module_register_xx(&_module);                                  \
  }                                                                     \
  EXTERN_C_END

#define NAPI_USE(modname)                                                 \
  EXTERN_C_START                                                          \
  extern void _napi_register_xx_##modname(void);                          \
  void* _napi_module_##modname##_p = (void*)&_napi_register_xx_##modname; \
  EXTERN_C_END
#ifdef USE_PRIMJS_NAPI
#include "primjs_napi_undefs.h"
#endif
#endif  // SRC_NAPI_NAPI_MODULE_H_
