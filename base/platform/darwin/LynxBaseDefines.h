// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_PLATFORM_DARWIN_LYNXBASEDEFINES_H_
#define BASE_PLATFORM_DARWIN_LYNXBASEDEFINES_H_

#ifndef LYNX_BASE_CONCAT
#define LYNX_BASE_CONCAT2(A, B) A##B
#define LYNX_BASE_CONCAT(A, B) LYNX_BASE_CONCAT2(A, B)
#endif

#if defined(__cplusplus)
#define LYNX_BASE_EXTERN extern "C" __attribute__((visibility("default")))
#else
#define LYNX_BASE_EXTERN extern __attribute__((visibility("default")))
#endif

#endif  // BASE_PLATFORM_DARWIN_LYNXBASEDEFINES_H_
