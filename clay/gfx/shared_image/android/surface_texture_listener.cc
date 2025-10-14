// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/android/surface_texture_listener.h"

#include <utility>

#include "platform/android/clay/src/main/jni/gen/SurfaceTextureListener_jni.h"
#include "platform/android/clay/src/main/jni/gen/SurfaceTextureListener_register_jni.h"

namespace clay {
namespace jni {
bool RegisterJNIForSurfaceTextureListener(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace clay

namespace clay {

SurfaceTextureListener::SurfaceTextureListener(fml::closure callback)
    : callback_(std::move(callback)) {}

SurfaceTextureListener::~SurfaceTextureListener() {}

void SurfaceTextureListener::Destroy(JNIEnv* env) { delete this; }

void SurfaceTextureListener::FrameAvailable(JNIEnv* env) { callback_(); }

}  // namespace clay

static void FrameAvailable(JNIEnv* env, jobject jcaller,
                           jlong nativeSurfaceTextureListener, jobject caller) {
  auto listener = reinterpret_cast<clay::SurfaceTextureListener*>(
      static_cast<intptr_t>(nativeSurfaceTextureListener));
  listener->FrameAvailable(env);
}

static void Destroy(JNIEnv* env, jobject jcaller,
                    jlong nativeSurfaceTextureListener, jobject caller) {
  auto listener = reinterpret_cast<clay::SurfaceTextureListener*>(
      static_cast<intptr_t>(nativeSurfaceTextureListener));
  listener->Destroy(env);
}
