/*
 * QuickJS C library
 *
 * Copyright (c) 2017-2018 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef SRC_INTERPRETER_QUICKJS_INCLUDE_QUICKJS_LIBC_H_
#define SRC_INTERPRETER_QUICKJS_INCLUDE_QUICKJS_LIBC_H_

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <stdint.h>  // portable: uint64_t   MSVC: __int64
#include <winsock.h>
#endif

#include "quickjs.h"

QJS_HIDE LEPUSModuleDef *lepus_init_module_std(LEPUSContext *ctx,
                                               const char *module_name);
QJS_HIDE LEPUSModuleDef *lepus_init_module_os(LEPUSContext *ctx,
                                              const char *module_name);
QJS_HIDE void lepus_std_add_helpers(LEPUSContext *ctx, int argc, char **argv);
void lepus_std_loop(LEPUSContext *ctx);
QJS_HIDE void lepus_std_free_handlers(LEPUSRuntime *rt);
QJS_HIDE void lepus_std_dump_error(LEPUSContext *ctx);

#if LYNX_SIMPLIFY
uint8_t *lepus_load_file(LEPUSContext *ctx, size_t *pbuf_len,
                         const char *filename);
LEPUSModuleDef *lepus_module_loader(LEPUSContext *ctx, const char *module_name,
                                    void *opaque);
void lepus_std_eval_binary(LEPUSContext *ctx, const uint8_t *buf,
                           size_t buf_len, int flags);
#endif

#if defined(_WIN32)
int gettimeofday(struct timeval *tp, void *tzp);
#endif

#endif  // SRC_INTERPRETER_QUICKJS_INCLUDE_QUICKJS_LIBC_H_
