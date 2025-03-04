// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_INCLUDE_BASE_EXPORT_H_
#define BASE_INCLUDE_BASE_EXPORT_H_

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
