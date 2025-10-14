// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/android/scoped_java_surface.h"

#include "base/include/platform/android/jni_convert_helper.h"
#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/android/surface_texture.h"
#include "platform/android/clay/src/main/jni/gen/SurfacePlatformWrapper_jni.h"
#include "platform/android/clay/src/main/jni/gen/SurfacePlatformWrapper_register_jni.h"

namespace clay {
namespace jni {
bool RegisterJNIForSurfacePlatformWrapper(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace clay

namespace clay {

ScopedJavaSurface::ScopedJavaSurface() = default;
ScopedJavaSurface::ScopedJavaSurface(std::nullptr_t) {}

ScopedJavaSurface::ScopedJavaSurface(const fml::jni::JavaRef<jobject>& surface,
                                     bool auto_release)
    : auto_release_(auto_release), j_surface_(surface) {}

ScopedJavaSurface::ScopedJavaSurface(const SurfaceTexture* surface_texture) {
  JNIEnv* env = fml::jni::AttachCurrentThread();
  fml::jni::ScopedLocalJavaRef<jobject> tmp(
      Java_SurfacePlatformWrapper_createSurface(
          env, surface_texture->j_surface_texture().Get()));
  FML_DCHECK(!tmp.IsNull());
  j_surface_.Reset(tmp);
}

ScopedJavaSurface::ScopedJavaSurface(ScopedJavaSurface&& rvalue) {
  MoveFrom(rvalue);
}

ScopedJavaSurface& ScopedJavaSurface::operator=(ScopedJavaSurface&& rhs) {
  MoveFrom(rhs);
  return *this;
}

ScopedJavaSurface::~ScopedJavaSurface() { ReleaseSurfaceIfNeeded(); }

ScopedJavaSurface ScopedJavaSurface::CopyRetainOwnership() const {
  return ScopedJavaSurface(j_surface_, /*auto_release=*/false);
}

void ScopedJavaSurface::ReleaseSurfaceIfNeeded() {
  if (auto_release_ && !j_surface_.IsNull()) {
    JNIEnv* env = fml::jni::AttachCurrentThread();
    Java_SurfacePlatformWrapper_releaseSurface(env, j_surface_.Get());
  }
}

void ScopedJavaSurface::MoveFrom(ScopedJavaSurface& other) {
  if (this == &other) {
    return;
  }
  ReleaseSurfaceIfNeeded();
  j_surface_.Reset(other.j_surface_);
  other.j_surface_.Reset();
  auto_release_ = other.auto_release_;
}

bool ScopedJavaSurface::IsEmpty() const { return j_surface_.IsNull(); }

bool ScopedJavaSurface::IsValid() const {
  JNIEnv* env = fml::jni::AttachCurrentThread();
  return !IsEmpty() &&
         Java_SurfacePlatformWrapper_surfaceIsValid(env, j_surface_.Get());
}

}  // namespace clay
