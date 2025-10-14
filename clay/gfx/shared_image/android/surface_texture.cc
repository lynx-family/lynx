// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/android/surface_texture.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <utility>

#include "base/include/platform/android/jni_convert_helper.h"
#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/android/scoped_a_native_window.h"
#include "clay/gfx/shared_image/android/scoped_java_surface.h"
#include "clay/gfx/shared_image/android/surface_texture_listener.h"
#include "platform/android/clay/src/main/jni/gen/SurfaceTexturePlatformWrapper_jni.h"
#include "platform/android/clay/src/main/jni/gen/SurfaceTexturePlatformWrapper_register_jni.h"

namespace clay {
namespace jni {
bool RegisterJNIForSurfaceTexturePlatformWrapper(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace clay

namespace clay {

fml::RefPtr<SurfaceTexture> SurfaceTexture::Create() {
  JNIEnv* env = fml::jni::AttachCurrentThread();
  auto j_surface_texture = Java_SurfaceTexturePlatformWrapper_create(env);
  return fml::AdoptRef(new SurfaceTexture(j_surface_texture, true));
}

fml::RefPtr<SurfaceTexture> SurfaceTexture::Retain(jobject j_surface_texture,
                                                   bool auto_release) {
  fml::jni::ScopedLocalJavaRef<jobject> j_surface_texture_ref;
  j_surface_texture_ref.Reset(fml::jni::AttachCurrentThread(),
                              j_surface_texture);
  return fml::AdoptRef(new SurfaceTexture(j_surface_texture_ref, auto_release));
}

SurfaceTexture::SurfaceTexture(
    const fml::jni::JavaRef<jobject>& j_surface_texture, bool auto_release)
    : j_surface_texture_(j_surface_texture), auto_release_(auto_release) {}

SurfaceTexture::~SurfaceTexture() {
  if (auto_release_) {
    JNIEnv* env = fml::jni::AttachCurrentThread();
    Java_SurfaceTexturePlatformWrapper_destroy(env, j_surface_texture_.Get());
  }
}

void SurfaceTexture::SetFrameAvailableCallback(fml::closure callback) {
  JNIEnv* env = fml::jni::AttachCurrentThread();
  SurfaceTextureListener* listener = nullptr;
  if (callback) {
    listener = new SurfaceTextureListener(std::move(callback));
  }
  Java_SurfaceTexturePlatformWrapper_setFrameAvailableCallback(
      env, j_surface_texture_.Get(), reinterpret_cast<intptr_t>(listener));
}

void SurfaceTexture::UpdateTexImage() {
  JNIEnv* env = fml::jni::AttachCurrentThread();
  Java_SurfaceTexturePlatformWrapper_updateTexImage(env,
                                                    j_surface_texture_.Get());
}

void SurfaceTexture::GetTransformMatrix(float mtx[16]) {
  JNIEnv* env = fml::jni::AttachCurrentThread();

  fml::jni::ScopedLocalJavaRef<jfloatArray> j_matrix(env,
                                                     env->NewFloatArray(16));
  Java_SurfaceTexturePlatformWrapper_getTransformMatrix(
      env, j_surface_texture_.Get(), j_matrix.Get());

  jfloat* elements = env->GetFloatArrayElements(j_matrix.Get(), nullptr);
  for (int i = 0; i < 16; ++i) {
    mtx[i] = static_cast<float>(elements[i]);
  }
  env->ReleaseFloatArrayElements(j_matrix.Get(), elements, JNI_ABORT);
}

void SurfaceTexture::AttachToGLContext(int texture_id) {
  FML_DCHECK(texture_id);

  JNIEnv* env = fml::jni::AttachCurrentThread();
  Java_SurfaceTexturePlatformWrapper_attachToGLContext(
      env, j_surface_texture_.Get(), texture_id);
}

void SurfaceTexture::DetachFromGLContext() {
  JNIEnv* env = fml::jni::AttachCurrentThread();
  Java_SurfaceTexturePlatformWrapper_detachFromGLContext(
      env, j_surface_texture_.Get());
}

ScopedANativeWindow SurfaceTexture::CreateSurface() {
  ScopedJavaSurface surface(this);
  return ScopedANativeWindow(surface);
}

void SurfaceTexture::ReleaseBackBuffers() {
  JNIEnv* env = fml::jni::AttachCurrentThread();
  Java_SurfaceTexturePlatformWrapper_release(env, j_surface_texture_.Get());
}

void SurfaceTexture::SetDefaultBufferSize(int width, int height) {
  JNIEnv* env = fml::jni::AttachCurrentThread();
  Java_SurfaceTexturePlatformWrapper_setDefaultBufferSize(
      env, j_surface_texture_.Get(), width, height);
}

}  // namespace clay
