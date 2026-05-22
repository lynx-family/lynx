// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/android/scoped_a_native_window.h"

#include <android/native_window_jni.h>

#include "base/include/platform/android/jni_convert_helper.h"
#include "base/include/platform/android/jni_utils.h"
#include "clay/gfx/shared_image/android/scoped_java_surface.h"

namespace clay {

// static
ScopedANativeWindow ScopedANativeWindow::Wrap(ANativeWindow* a_native_window) {
  return ScopedANativeWindow(a_native_window);
}

ScopedANativeWindow::ScopedANativeWindow(const ScopedJavaSurface& surface) {
  if (surface.j_surface().IsNull()) {
    return;
  }

  JNIEnv* env = fml::jni::AttachCurrentThread();
  // Note: This ensures that any local references used by
  // ANativeWindow_fromSurface are released immediately. This is needed as a
  // workaround for https://code.google.com/p/android/issues/detail?id=68174
  fml::jni::ScopedJavaLocalFrame scoped_local_reference_frame(env);
  a_native_window_ = ANativeWindow_fromSurface(env, surface.j_surface().Get());
}

ScopedANativeWindow::ScopedANativeWindow(ANativeWindow* a_native_window)
    : a_native_window_(a_native_window) {
  if (a_native_window_) {
    ANativeWindow_acquire(a_native_window_);
  }
}

ScopedANativeWindow::~ScopedANativeWindow() { DestroyIfNeeded(); }

ScopedANativeWindow::ScopedANativeWindow(ScopedANativeWindow&& other)
    : a_native_window_(other.a_native_window_) {
  other.a_native_window_ = nullptr;
}

ScopedANativeWindow& ScopedANativeWindow::operator=(
    ScopedANativeWindow&& other) {
  if (this != &other) {
    DestroyIfNeeded();
    a_native_window_ = other.a_native_window_;
    other.a_native_window_ = nullptr;
  }
  return *this;
}

void ScopedANativeWindow::DestroyIfNeeded() {
  if (a_native_window_) {
    ANativeWindow_release(a_native_window_);
  }
  a_native_window_ = nullptr;
}

}  // namespace clay
