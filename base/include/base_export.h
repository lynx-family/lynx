// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_INCLUDE_BASE_EXPORT_H_
#define BASE_INCLUDE_BASE_EXPORT_H_

// Symbols in the lynx engine are not exported by default due to
// -fvisibility=hidden. We provide two macros to help you export some symbols:
// 1. BASE_EXPORT. If certain symbols need to be accessed externally, they
// should be marked with the BASE_EXPORT.
// 2. BASE_EXPORT_FOR_DEVTOOL. If symbols need to be exported solely for use by
// the devtool, the BASE_EXPORT_FOR_DEVTOOL should be applied.
//
// If you need stricter symbol export rules from the Lynx engine for purposes
// such as better package management, you can provide NO_EXPORT to gn script or
// customize the export rules using --version-script. Importantly, if a symbol
// is not marked with BASE_EXPORT or BASE_EXPORT_FOR_DEVTOOL, it can not be
// exported, even if you use a
// --version-script to customize the export rules.
#ifdef NO_EXPORT
#define BASE_EXPORT
#define BASE_EXPORT_FOR_DEVTOOL
#else  // NO_EXPORT
#define BASE_EXPORT __attribute__((visibility("default")))

#if EXPORT_SYMBOLS_FOR_DEVTOOL
#define BASE_EXPORT_FOR_DEVTOOL __attribute__((visibility("default")))
#else
#define BASE_EXPORT_FOR_DEVTOOL
#endif
#endif  // NO_EXPORT

#define BASE_HIDE __attribute__((visibility("hidden")))

#endif  // BASE_INCLUDE_BASE_EXPORT_H_
