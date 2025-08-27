// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_LYNX_EXPORT_H_
#define CORE_BASE_LYNX_EXPORT_H_

// Symbols in the lynx engine are not exported by default due to
// -fvisibility=hidden. We provide two macros to help you export some symbols:
// 1. LYNX_EXPORT. If certain symbols need to be accessed externally, they
// should be marked with the LYNX_EXPORT.
// 2. LYNX_EXPORT_FOR_DEVTOOL. If symbols need to be exported solely for use by
// the devtool, the LYNX_EXPORT_FOR_DEVTOOL should be applied.
//
// If you need stricter symbol export rules from the Lynx engine for purposes
// such as better package management, you can provide LYNX_NO_EXPORT to gn
// script or customize the export rules using --version-script. Importantly, if
// a symbol is not marked with LYNX_EXPORT or LYNX_EXPORT_FOR_DEVTOOL, it can
// not be exported, even if you use a
// --version-script to customize the export rules.
#ifdef LYNX_NO_EXPORT
#define LYNX_EXPORT
#define LYNX_EXPORT_FOR_DEVTOOL
#else  // LYNX_NO_EXPORT
#define LYNX_EXPORT __attribute__((visibility("default")))

#if EXPORT_SYMBOLS_FOR_DEVTOOL
#define LYNX_EXPORT_FOR_DEVTOOL __attribute__((visibility("default")))
#else
#define LYNX_EXPORT_FOR_DEVTOOL
#endif
#endif  // LYNX_NO_EXPORT

#define LYNX_HIDE __attribute__((visibility("hidden")))

#endif  // CORE_BASE_LYNX_EXPORT_H_
