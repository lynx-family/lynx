// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GFX_GFX_EXPORT_H_
#define GFX_GFX_EXPORT_H_

#if defined(GFX_NO_EXPORT)
#define GFX_EXPORT
#else
#if defined(_WIN32)
#define GFX_EXPORT
#else  // _WIN32
#define GFX_EXPORT __attribute__((visibility("default")))
#endif  // _WIN32
#endif  // GFX_NO_EXPORT

#endif  // GFX_GFX_EXPORT_H_
