// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_HARMONY_LYNX_JSVM_INITIALIZER_SRC_MAIN_CPP_JSVM_INITIALIZER_H_
#define PLATFORM_HARMONY_LYNX_JSVM_INITIALIZER_SRC_MAIN_CPP_JSVM_INITIALIZER_H_

#include <ark_runtime/jsvm.h>
#include <ark_runtime/jsvm_types.h>

#ifdef __cplusplus
extern "C" {
#endif

// All participating callers must use the same shared library. Direct calls to
// OH_JSVM_Init outside this library are not covered by this guarantee.
// The first call supplies the initialization options. Its result, including a
// failure to resolve or initialize JSVM, is cached; later calls do not retry.
__attribute__((visibility("default"))) JSVM_Status Lynx_JSVM_Common_Init(
    const JSVM_InitOptions* options);

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_HARMONY_LYNX_JSVM_INITIALIZER_SRC_MAIN_CPP_JSVM_INITIALIZER_H_
