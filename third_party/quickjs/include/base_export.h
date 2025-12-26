// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SRC_INTERPRETER_QUICKJS_INCLUDE_BASE_EXPORT_H_
#define SRC_INTERPRETER_QUICKJS_INCLUDE_BASE_EXPORT_H_

#if defined(WIN32)
#define QJS_EXPORT __declspec(dllimport)
#define QJS_EXPORT_FOR_DEVTOOL __declspec(dllimport)
#define QJS_HIDE
#else  // defined(WIN32)
#define QJS_EXPORT __attribute__((visibility("default")))
#define QJS_EXPORT_FOR_DEVTOOL __attribute__((visibility("default")))
#define QJS_HIDE __attribute__((visibility("hidden")))
#endif  // defined(WIN32)

#endif  // SRC_INTERPRETER_QUICKJS_INCLUDE_BASE_EXPORT_H_
