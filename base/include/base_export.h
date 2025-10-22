// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_INCLUDE_BASE_EXPORT_H_
#define BASE_INCLUDE_BASE_EXPORT_H_

// Symbols in the lynx base are not exported by default due to
// -fvisibility=hidden. We provide a macro to help you export some symbols:
// 1.BASE_EXPORT. If certain symbols need to be accessed externally, they
// should be marked with the BASE_EXPORT.
//
// If you need stricter symbol export rules from the Lynx base for purposes
// such as better package management, you can provide BASE_NO_EXPORT to gn
// script or customize the export rules using --version-script. Importantly, if
// a symbol is not marked with BASE_EXPORT, it can not be exported, even if you
// use a
// --version-script to customize the export rules.
#ifdef BASE_NO_EXPORT
#define BASE_EXPORT
#else  // BASE_NO_EXPORT

#ifdef _WIN32
#ifndef lynx_EXPORTS
#define BASE_EXPORT __declspec(dllimport)
#else  // lynx_EXPORTS
#define BASE_EXPORT __declspec(dllexport)
#endif  // lynx_EXPORTS
#else   // _WIN32
#define BASE_EXPORT __attribute__((visibility("default")))
#endif  // _WIN32
#endif  // BASE_NO_EXPORT

#define BASE_HIDE __attribute__((visibility("hidden")))

#endif  // BASE_INCLUDE_BASE_EXPORT_H_
