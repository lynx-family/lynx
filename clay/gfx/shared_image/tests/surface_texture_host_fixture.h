// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_TESTS_SURFACE_TEXTURE_HOST_FIXTURE_H_
#define CLAY_GFX_SHARED_IMAGE_TESTS_SURFACE_TEXTURE_HOST_FIXTURE_H_

// Force-included only for the macOS SurfaceTexture state-machine executable.
// Replace Android/JNI and renderer boundaries, not the backing, representation,
// dependency ordering or EGL fence implementation under test.
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#include <cstdlib>
#include <utility>

#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/gfx/shared_image/skity_gl_image_representation.h"

#define BASE_INCLUDE_PLATFORM_ANDROID_SCOPED_JAVA_REF_H_
#define BASE_INCLUDE_PLATFORM_ANDROID_JNI_CONVERT_HELPER_H_
#define CLAY_GFX_SHARED_IMAGE_ANDROID_SURFACE_TEXTURE_H_
#define CLAY_GFX_GFX_RENDERING_BACKEND_H_
#define CLAY_GFX_SHARED_IMAGE_ANDROID_HARDWAREBUFFER_IMAGE_BACKING_H_
#define CLAY_GFX_SHARED_IMAGE_EGL_IMAGE_BACKING_H_
#define CLAY_COMMON_GRAPHICS_GL_SCOPED_TEXTURE_BINDER_H_
#define CLAY_COMMON_GRAPHICS_GL_SCOPED_FRAMEBUFFER_BINDER_H_

using jobject = void*;
namespace fml::jni {
template <typename T>
class JavaRef {
 public:
  T Get() const { return nullptr; }
};
}  // namespace fml::jni

namespace clay {
namespace surface_test {
inline fml::closure frame_callback;
inline bool attach_succeeds = true;
inline bool update_succeeds = true;
inline int attaches = 0;
inline int updates = 0;
}  // namespace surface_test

class SurfaceTexture : public fml::RefCountedThreadSafe<SurfaceTexture> {
 public:
  static fml::RefPtr<SurfaceTexture> Create() {
    return fml::MakeRefCounted<SurfaceTexture>();
  }
  static fml::RefPtr<SurfaceTexture> Retain(jobject, bool) { return Create(); }
  void SetFrameAvailableCallback(fml::closure callback) {
    surface_test::frame_callback = std::move(callback);
  }
  bool AttachToGLContext(int) {
    ++surface_test::attaches;
    return surface_test::attach_succeeds;
  }
  bool DetachFromGLContext() { return true; }
  bool UpdateTexImage() {
    ++surface_test::updates;
    return surface_test::update_succeeds;
  }
  void GetTransformMatrix(float matrix[16]) {
    for (int i = 0; i < 16; ++i) matrix[i] = i % 5 == 0 ? 1 : 0;
  }
  const fml::jni::JavaRef<jobject>& j_surface_texture() const { return ref_; }

 private:
  fml::jni::JavaRef<jobject> ref_;
};

// Texture binding is a driver boundary. No renderer/image allocation is needed
// to exercise frame epochs and dependency ownership.
class ScopedTextureBinder {
 public:
  ScopedTextureBinder(unsigned int, unsigned int) {}
};
class ScopedFramebufferBinder {
 public:
  ScopedFramebufferBinder(unsigned int, unsigned int) {}
};
class AHardwareBufferImageBacking : public SharedImageBacking {
 public:
  EGLImageKHR CreateEGLImage(EGLDisplay) { std::abort(); }
};
class EGLImageBacking : public SharedImageBacking {
 public:
  void BindToTexture(GLenum) { std::abort(); }
};
}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_TESTS_SURFACE_TEXTURE_HOST_FIXTURE_H_
