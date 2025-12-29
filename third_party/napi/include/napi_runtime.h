/**
 * Copyright (c) 2017 Node.js API collaborators. All Rights Reserved.
 *
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file in the root of the source tree.
 */

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SRC_NAPI_ENV_NAPI_RUNTIME_H_
#define SRC_NAPI_ENV_NAPI_RUNTIME_H_

#include "js_native_api.h"
#ifdef USE_PRIMJS_NAPI
#include "primjs_napi_defines.h"
#endif
typedef struct napi_runtime_configuration__* napi_runtime_configuration;

typedef void (*napi_uncaught_exception_handler)(napi_env env,
                                                napi_value exception,
                                                void* ctx);

typedef void (*napi_foreground_cb)(void* task);

typedef void (*napi_foreground_handler)(napi_foreground_cb js_cb, void* task,
                                        void* ctx);

typedef void (*napi_worker_lifecycle_callback)(void* ctx);

typedef void (*napi_worker_task_runner)(void* task);

typedef void (*napi_worker_task_handler)(napi_worker_task_runner task_runner,
                                         void* task, void* ctx);

EXTERN_C_START

/**
 * Deprecated
 *  use `napi_attach_runtime_with_configuration` instead
 */
NAPI_EXTERN void NAPI_DEPRECATED napi_attach_runtime(
    napi_env env, napi_foreground_handler task_handler, void* task_ctx,
    napi_uncaught_exception_handler uncaught_handler, void* uncaught_ctx);

NAPI_EXTERN napi_runtime_configuration napi_create_runtime_configuration();

NAPI_EXTERN void napi_delete_runtime_configuration(napi_runtime_configuration);

NAPI_EXTERN void napi_runtime_config_foreground_handler(
    napi_runtime_configuration configuration,
    napi_foreground_handler task_handler, void* task_ctx);
NAPI_EXTERN void napi_runtime_config_uncaught_handler(
    napi_runtime_configuration configuration,
    napi_uncaught_exception_handler task_handler, void* uncaught_ctx);
NAPI_EXTERN void napi_runtime_config_worker_handler(
    napi_runtime_configuration configuration,
    napi_worker_lifecycle_callback on_worker_start,
    napi_worker_lifecycle_callback on_worker_stop,
    napi_worker_task_handler worker_task_handler, void* worker_ctx);
NAPI_EXTERN void napi_runtime_config_worker_stack_size(
    napi_runtime_configuration configuration, size_t stack_size);

NAPI_EXTERN void napi_attach_runtime_with_configuration(
    napi_env env, napi_runtime_configuration configuration);

NAPI_EXTERN void napi_detach_runtime(napi_env env);

NAPI_EXTERN napi_status napi_runtime_get_threadsafe_function_context(
    napi_threadsafe_function func, void** result);

NAPI_EXTERN napi_status napi_runtime_call_threadsafe_function(
    napi_threadsafe_function func, void* data,
    napi_threadsafe_function_call_mode is_blocking);

NAPI_EXTERN napi_status
napi_runtime_delete_threadsafe_function(napi_threadsafe_function func);

NAPI_EXTERN napi_status
napi_runtime_acquire_threadsafe_function(napi_threadsafe_function func);

NAPI_EXTERN napi_status napi_runtime_release_threadsafe_function(
    napi_threadsafe_function func, napi_threadsafe_function_release_mode mode);

EXTERN_C_END

#ifdef USE_PRIMJS_NAPI
#include "primjs_napi_undefs.h"
#endif
#endif  // SRC_NAPI_ENV_NAPI_RUNTIME_H_
