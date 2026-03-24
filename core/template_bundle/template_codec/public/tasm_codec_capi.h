// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

//! @file tasm_codec_capi.h
//! @brief TASM codec C API - External interface for C callers and language
//! bindings.
//!
//! This header exposes a pure C API for external users (C, Python, Go, etc.).
//! Internal C++ code should use tasm_codec.h instead.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_PUBLIC_TASM_CODEC_CAPI_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_PUBLIC_TASM_CODEC_CAPI_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Types
// ============================================================================

//! Result of encode operation.
//! Caller must free memory via Tasm_FreeEncodeResult.
typedef struct {
  int status;          // 0 = success, -1 = error
  char* error_msg;     // Error message, caller must free
  uint8_t* buffer;     // Encoded binary data, caller must free
  size_t buffer_size;  // Size of buffer
  char* lepus_code;    // Lepus code string, caller must free
  char* lepus_debug;   // Lepus debug info, caller must free
  char* section_size;  // Section size info, caller must free
} TasmEncodeResult;

//! Result of decode operation.
//! Caller must free memory via Tasm_FreeDecodeResult.
typedef struct {
  int status;       // 0 = success, -1 = error
  char* result;     // Decoded result (JSON string), caller must free
  char* error_msg;  // Error message, caller must free
} TasmDecodeResult;

// ============================================================================
// Functions
// ============================================================================

//! Encode template bundle from options JSON.
//!
//! @param options_json JSON string containing encode options.
//! @return TasmEncodeResult* that must be freed by Tasm_FreeEncodeResult.
//!         Returns nullptr on null input.
//!
//! @note The returned struct pointer contains pointers that must be freed by
//!       Tasm_FreeEncodeResult to avoid memory leaks.
TasmEncodeResult* Tasm_Encode(const char* options_json);

//! Decode template bundle from binary data.
//!
//! @param data Pointer to binary data.
//! @param len Length of binary data in bytes.
//! @return TasmDecodeResult* that must be freed by Tasm_FreeDecodeResult.
//!         Returns nullptr on invalid input.
//!
//! @note The returned struct pointer contains pointers that must be freed by
//!       Tasm_FreeDecodeResult to avoid memory leaks.
TasmDecodeResult* Tasm_Decode(const uint8_t* data, size_t len);

//! Free memory allocated by Tasm_Encode.
//!
//! @param result Pointer to TasmEncodeResult to be freed.
void Tasm_FreeEncodeResult(TasmEncodeResult* result);

//! Free memory allocated by Tasm_Decode.
//!
//! @param result Pointer to TasmDecodeResult to be freed.
void Tasm_FreeDecodeResult(TasmDecodeResult* result);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_PUBLIC_TASM_CODEC_CAPI_H_
