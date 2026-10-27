// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef INCLUDE_SKITY_MACROS_HPP
#define INCLUDE_SKITY_MACROS_HPP

#if defined(_WIN32) || defined(_WIN64)
#define SKITY_WIN
#elif defined(__APPLE__) || defined(__MACH__)
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR == 1 || TARGET_OS_IPHONE == 1
#ifndef SKITY_IOS
#define SKITY_IOS 1
#endif
#elif TARGET_OS_MAC == 1
#ifndef SKITY_MACOS
#define SKITY_MACOS 1
#endif
#endif
#elif defined(__ANDROID__)
#define SKITY_ANDROID
#elif defined(__linux__)
#define SKITY_LINUX
#elif defined(__EMSCRIPTEN__) || defined(__wasm__) || defined(__wasm32__) || \
    defined(__wasm64__)
#define SKITY_WASM
#endif

// wasm do not support shared library
// iOS use cocoapods also not use shared library
// all other platform has been changed to use shared library
#if !defined(SKITY_WASM) && !defined(SKITY_IOS)
#ifndef SKITY_DLL
#define SKITY_DLL
#endif
#endif

#ifdef SKITY_DLL

#if defined(SKITY_WIN)
#define SKITY_API
#elif defined(_MSC_VER)
#define SKITY_API __declspec(dllexport)
#else
#define SKITY_API __attribute__((visibility("default")))
#endif

#else
#define SKITY_API
#endif

#if defined(__ARM_NEON__) || defined(__arm__) || defined(__aarch64__)
#define SKITY_ARM_NEON
#endif

#endif  // INCLUDE_SKITY_MACROS_HPP
