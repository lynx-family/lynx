// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_HARMONY_LYNX_JSVM_INITIALIZER_SRC_MAIN_CPP_JSVM_INITIALIZER_H_
#define PLATFORM_HARMONY_LYNX_JSVM_INITIALIZER_SRC_MAIN_CPP_JSVM_INITIALIZER_H_

#include <ark_runtime/jsvm.h>
#include <ark_runtime/jsvm_types.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// All participating callers must use the same shared library. Direct calls to
// OH_JSVM_Init outside this library are not covered by this guarantee.
// Saves a complete configuration without initializing JSVM. The trigger must be
// in [0, 100]; semi-space sizes are positive MiB values with min <= max.
// Returns false for invalid values or once initialization has started. Before
// that, the last successful call wins. These values override the corresponding
// flags supplied to Lynx_JSVM_Common_Init by any participating caller.
__attribute__((visibility("default"))) bool Lynx_JSVM_SetInitOptions(
    int32_t incremental_marking_hard_trigger, int32_t min_semi_space_size,
    int32_t max_semi_space_size);

// Without a saved configuration, forwards the first caller's options unchanged.
// The result of the first attempt, including a
// failure to resolve or initialize JSVM, is cached; later calls do not retry.
__attribute__((visibility("default"))) JSVM_Status Lynx_JSVM_Common_Init(
    const JSVM_InitOptions* options);

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_HARMONY_LYNX_JSVM_INITIALIZER_SRC_MAIN_CPP_JSVM_INITIALIZER_H_
