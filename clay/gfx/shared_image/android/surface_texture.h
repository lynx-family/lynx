// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_ANDROID_SURFACE_TEXTURE_H_
#define CLAY_GFX_SHARED_IMAGE_ANDROID_SURFACE_TEXTURE_H_

#include <android/native_window.h>

#include "base/include/closure.h"
#include "base/include/fml/macros.h"
#include "base/include/fml/memory/ref_counted.h"
#include "base/include/platform/android/scoped_java_ref.h"
#include "clay/gfx/shared_image/android/scoped_a_native_window.h"

namespace clay {
class SurfaceTexture : public fml::RefCountedThreadSafe<SurfaceTexture> {
 public:
  static fml::RefPtr<SurfaceTexture> Create();
  static fml::RefPtr<SurfaceTexture> Retain(jobject j_surface_texture,
                                            bool auto_release);

  // Set the listener callback, it may be invoked on any thread.
  void SetFrameAvailableCallback(fml::closure callback);

  // Update the texture image to the most recent frame from the image stream.
  void UpdateTexImage();

  // Retrieve the 4x4 texture coordinate transform matrix associated with the
  // texture image set by the most recent call to updateTexImage.
  void GetTransformMatrix(float mtx[16]);

  // Attach the SurfaceTexture to the texture currently bound to
  // GL_TEXTURE_EXTERNAL_OES.
  void AttachToGLContext(int texture_id);

  // Detaches the SurfaceTexture from the context that owns its current GL
  // texture. Must be called with that context current on the calling thread.
  void DetachFromGLContext();

  // Creates a native render surface for this surface texture.
  ScopedANativeWindow CreateSurface();

  // Release the SurfaceTexture back buffers.  The SurfaceTexture is no longer
  // usable after calling this but the front buffer is still valid. Note that
  // this is not called 'Release', like the Android API, because scoped_refptr
  // calls that quite a bit.
  void ReleaseBackBuffers();

  // Set the default buffer size for the surface texture.
  void SetDefaultBufferSize(int width, int height);

  const fml::jni::JavaRef<jobject>& j_surface_texture() const {
    return j_surface_texture_;
  }

 protected:
  explicit SurfaceTexture(const fml::jni::JavaRef<jobject>& j_surface_texture,
                          bool auto_release);

 private:
  friend class fml::RefCountedThreadSafe<SurfaceTexture>;
  virtual ~SurfaceTexture();

  // Java SurfaceTexture instance.
  fml::jni::ScopedGlobalJavaRef<jobject> j_surface_texture_;
  bool auto_release_;
  BASE_DISALLOW_COPY_AND_ASSIGN(SurfaceTexture);
};
}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_ANDROID_SURFACE_TEXTURE_H_
