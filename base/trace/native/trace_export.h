// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_TRACE_NATIVE_TRACE_EXPORT_H_
#define BASE_TRACE_NATIVE_TRACE_EXPORT_H_

#ifdef _WIN32
#define TRACE_EXPORT __declspec(dllexport)
#else
#define TRACE_EXPORT __attribute__((visibility("default")))
#endif

#endif  // BASE_TRACE_NATIVE_TRACE_EXPORT_H_
