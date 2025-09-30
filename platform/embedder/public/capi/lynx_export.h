// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_EXPORT_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_EXPORT_H_

#ifdef _WIN32
#ifndef lynx_EXPORTS
#define LYNX_CAPI_EXPORT __declspec(dllimport)
#else
#define LYNX_CAPI_EXPORT __declspec(dllexport)
#endif
#else
#define LYNX_CAPI_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
#define LYNX_EXTERN_C extern "C"
#define LYNX_EXTERN_C_BEGIN LYNX_EXTERN_C {
#define LYNX_EXTERN_C_END }
#else
#define LYNX_EXTERN_C
#define LYNX_EXTERN_C_BEGIN
#define LYNX_EXTERN_C_END
#endif

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_EXPORT_H_
