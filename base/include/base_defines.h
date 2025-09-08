// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_BASE_DEFINES_H_
#define BASE_INCLUDE_BASE_DEFINES_H_

#define unlikely(x) __builtin_expect(!!(x), 0)
#define likely(x) __builtin_expect(!!(x), 1)

#if _MSC_VER
#define BASE_INLINE inline __forceinline
#define BASE_NEVER_INLINE __declspec(noinline)
#else
#define BASE_INLINE inline __attribute__((always_inline))
#define BASE_NEVER_INLINE __attribute__((noinline))
#endif

#ifdef __cplusplus
#define BASE_EXTERN_C extern "C"
#define BASE_EXTERN_C_BEGIN BASE_EXTERN_C {
#define BASE_EXTERN_C_END }
#else
#define BASE_EXTERN_C
#define BASE_EXTERN_C_BEGIN
#define BASE_EXTERN_C_END
#endif

#endif  // BASE_INCLUDE_BASE_DEFINES_H_
