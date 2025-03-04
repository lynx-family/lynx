// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QJS_BASE_EXPORT_H_
#define QJS_BASE_EXPORT_H_

#if defined(WIN32)
#define QJS_EXPORT __declspec(dllimport)
#define QJS_EXPORT_FOR_DEVTOOL __declspec(dllimport)
#define QJS_HIDE
#else  // defined(WIN32)
#define QJS_EXPORT __attribute__((visibility("default")))
#define QJS_EXPORT_FOR_DEVTOOL __attribute__((visibility("default")))
#define QJS_HIDE __attribute__((visibility("hidden")))
#endif  // defined(WIN32)

#endif  // QJS_BASE_EXPORT_H_
