// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_MARCO_H_
#define CORE_RUNTIME_VM_LEPUS_MARCO_H_
#define unlikely(x) __builtin_expect(!!(x), 0)
#define likely(x) __builtin_expect(!!(x), 1)

#if _MSC_VER
#define LEPUS_INLINE inline __forceinline
#define LEPUS_NOT_INLINE __declspec(noinline)
#else
#define LEPUS_INLINE inline __attribute__((always_inline))
#define LEPUS_NOT_INLINE __attribute__((noinline))
#endif

#endif  // CORE_RUNTIME_VM_LEPUS_MARCO_H_
