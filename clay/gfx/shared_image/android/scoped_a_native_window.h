// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_ANDROID_SCOPED_A_NATIVE_WINDOW_H_
#define CLAY_GFX_SHARED_IMAGE_ANDROID_SCOPED_A_NATIVE_WINDOW_H_

#include <cstddef>

#include "base/include/fml/macros.h"

struct ANativeWindow;

namespace clay {

class ScopedJavaSurface;

class ScopedANativeWindow {
 public:
  static ScopedANativeWindow Wrap(ANativeWindow* a_native_window);
  constexpr ScopedANativeWindow() = default;
  constexpr explicit ScopedANativeWindow(std::nullptr_t) {}
  explicit ScopedANativeWindow(const ScopedJavaSurface& surface);
  ~ScopedANativeWindow();

  ScopedANativeWindow(ScopedANativeWindow&& other);
  ScopedANativeWindow& operator=(ScopedANativeWindow&& other);

  explicit operator bool() const { return !!a_native_window_; }

  ANativeWindow* a_native_window() const { return a_native_window_; }

  BASE_DISALLOW_COPY_AND_ASSIGN(ScopedANativeWindow);

 private:
  explicit ScopedANativeWindow(ANativeWindow* a_native_window);

  void DestroyIfNeeded();

  ANativeWindow* a_native_window_ = nullptr;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_ANDROID_SCOPED_A_NATIVE_WINDOW_H_
