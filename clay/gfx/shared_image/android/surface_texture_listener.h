// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_ANDROID_SURFACE_TEXTURE_LISTENER_H_
#define CLAY_GFX_SHARED_IMAGE_ANDROID_SURFACE_TEXTURE_LISTENER_H_

#include <jni.h>

#include "base/include/closure.h"
#include "base/include/fml/macros.h"

namespace clay {

// Listener class for all the callbacks from android SurfaceTexture.
class SurfaceTextureListener {
 public:
  SurfaceTextureListener() = delete;

  // Destroy this listener.
  void Destroy(JNIEnv* env);

  // A new frame is available to consume.
  void FrameAvailable(JNIEnv* env);

  BASE_DISALLOW_COPY_AND_ASSIGN(SurfaceTextureListener);

 private:
  // Native code should not hold any reference to this object, and instead pass
  // it up to Java for being referenced by a SurfaceTexture instance.
  // If use_any_thread is true, then the FrameAvailable callback will happen
  // on whatever thread calls us.  Otherwise, we will call it back on the same
  // thread that was used to construct us.
  explicit SurfaceTextureListener(fml::closure callback);
  ~SurfaceTextureListener();

  friend class SurfaceTexture;

  fml::closure callback_;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_ANDROID_SURFACE_TEXTURE_LISTENER_H_
