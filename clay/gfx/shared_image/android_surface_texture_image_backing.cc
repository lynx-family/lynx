// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/android_surface_texture_image_backing.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "base/include/platform/android/jni_convert_helper.h"
#include "clay/common/graphics/gl/scoped_texture_binder.h"
#include "clay/fml/logging.h"
#include "clay/gfx/gfx_rendering_backend.h"
#include "clay/gfx/shared_image/android/surface_texture.h"
#include "clay/gfx/shared_image/android_egl_image_representation.h"

namespace clay {

SurfaceTextureImageBacking::SurfaceTextureImageBacking(
    PixelFormat pixel_format, skity::Vec2 size,
    std::optional<GraphicsMemoryHandle> gfx_handle)
    : SharedImageBackingUnmanaged(pixel_format, size),
      surface_texture_(
          gfx_handle.has_value()
              ? SurfaceTexture::Retain(
                    reinterpret_cast<jobject>(gfx_handle.value()), false)
              : SurfaceTexture::Create()) {}

SurfaceTextureImageBacking::~SurfaceTextureImageBacking() {
  if (texture_ != 0) {
    FML_LOG(ERROR) << "Texture not detached, maybe leaked!";
  }
}

const skity::Vec2 SurfaceTextureImageBacking::GetSize() const {
  if (size_.x <= 0 || size_.y <= 0) {
    // If the size is not set, we assume the default size is 256x256.
    return skity::Vec2(256, 256);
  }
  return size_;
}

SharedImageBacking::BackingType SurfaceTextureImageBacking::GetType() const {
  return SharedImageBacking::BackingType::kSurfaceTexture;
}

GraphicsMemoryHandle SurfaceTextureImageBacking::GetGFXHandle() const {
  return surface_texture_->j_surface_texture().Get();
}

fml::RefPtr<SharedImageRepresentation>
SurfaceTextureImageBacking::CreateRepresentation(
    const ClaySharedImageRepresentationConfig* config) {
  auto type = config->type;
  switch (type) {
    case kClaySharedImageRepresentationTypeGL: {
      return fml::MakeRefCounted<SurfaceTextureEGLImageRepresentation>(
          fml::Ref(this));
    }
    default: {
      // NOT SUPPORTED.
      FML_LOG(ERROR)
          << "Unable to call AHardwareBufferImageBacking::CreateRepresentation "
             "with type: "
          << static_cast<uint32_t>(type);
      return nullptr;
    }
  }
}

#ifndef ENABLE_SKITY
fml::RefPtr<SkiaImageRepresentation>
SurfaceTextureImageBacking::CreateSkiaRepresentation(
    GrDirectContext* gr_context) {
  switch (gr_context->backend()) {
    case GrBackendApi::kOpenGL: {
      return fml::MakeRefCounted<SkiaGLImageRepresentation>(
          gr_context, fml::MakeRefCounted<SurfaceTextureEGLImageRepresentation>(
                          fml::Ref(this)));
    }
    default: {
      FML_LOG(ERROR)
          << "Unable to call "
             "SurfaceTextureImageBacking::CreateSkiaRepresentation with "
             "backend: "
          << static_cast<uint32_t>(gr_context->backend());
      return nullptr;
    }
  }
}
#else
fml::RefPtr<SkityImageRepresentation>
SurfaceTextureImageBacking::CreateSkityRepresentation(
    skity::GPUContext* skity_context) {
  switch (skity_context->GetBackendType()) {
    case skity::GPUBackendType::kOpenGL: {
      return fml::MakeRefCounted<SkityGLImageRepresentation>(
          skity_context,
          fml::MakeRefCounted<SurfaceTextureEGLImageRepresentation>(
              fml::Ref(this)));
    }
    default: {
      FML_LOG(ERROR)
          << "Unable to call "
             "SurfaceTextureImageBacking::CreateSkityRepresentation with "
             "backend: "
          << static_cast<uint32_t>(skity_context->GetBackendType());
      return nullptr;
    }
  }
}
#endif  // ENABLE_SKITY

// This is a workaround to address the limitation that the surface texture
// provides an API to `setDefaultBufferSize` without offering a method to
// `getBufferSize`.
// Utilizing `glGetTexLevelParameterfv` is also not reliable
// on certain devices, as discussed here: https://stackoverflow.com/q/53536401.
// So the SurfaceTextureImageBacking owner should set the size during layout (as
// with `VideoPlayerAndroid` or `PlatformViewPlugin`).
bool SurfaceTextureImageBacking::SetSize(skity::Vec2 size) {
  size_ = size;
  return true;
}

const skity::Matrix SurfaceTextureImageBacking::GetTransformation() const {
  float mat[16];
  surface_texture_->GetTransformMatrix(mat);
  return skity::Matrix{mat[0],  mat[1],  mat[2],  mat[3],   //
                       mat[4],  mat[5],  mat[6],  mat[7],   //
                       mat[8],  mat[9],  mat[10], mat[11],  //
                       mat[12], mat[13], mat[14], mat[15]};
}

void SurfaceTextureImageBacking::SetTransformation(const skity::Matrix& mat) {
  FML_UNIMPLEMENTED();
}

/// `SharedImageBackingUnmanaged`
void SurfaceTextureImageBacking::SetFrameAvailableCallback(
    const fml::closure& callback) {
  surface_texture_->SetFrameAvailableCallback(callback);
}

bool SurfaceTextureImageBacking::UpdateFront() {
  if (EnsureAttachedToGLContext() == 0) {
    FML_LOG(ERROR) << "Failed to attach to GL context";
    return false;
  }
  // https://developer.android.com/reference/android/graphics/SurfaceTexture#updateTexImage()
  // In Android docs, updateTexImage will implicitly bind
  // GL_TEXTURE_EXTERNAL_OES. To make Skia happy, we must reset the state.
  clay::ScopedTextureBinder scoped_texture_binder(GL_TEXTURE_EXTERNAL_OES,
                                                  texture_);
  surface_texture_->UpdateTexImage();
  return true;
}

void SurfaceTextureImageBacking::ReleaseFront() { FML_UNIMPLEMENTED(); }

uint32_t SurfaceTextureImageBacking::AcquireBack() {
  // No need to call any functions, since eglMakeCurrent automatically requires
  // a new buffer.
  return 0;  // TODO(youfeng) support buffer age for surface texture.
}

bool SurfaceTextureImageBacking::SwapBack() {
  // No need to call any functions, since eglSwap automatically swap back
  return true;
}

uint32_t SurfaceTextureImageBacking::Capacity() const {
  // Currently only support double buffering
  return 2;
}

const fml::jni::JavaRef<jobject>&
SurfaceTextureImageBacking::GetSurfaceTexture() const {
  return surface_texture_->j_surface_texture();
}

GLuint SurfaceTextureImageBacking::EnsureAttachedToGLContext() {
  if (texture_ == 0) {
    glGenTextures(1, &texture_);
    clay::ScopedTextureBinder scoped_texture_binder(GL_TEXTURE_EXTERNAL_OES,
                                                    texture_);
    surface_texture_->AttachToGLContext(texture_);
  }
  return texture_;
}

void SurfaceTextureImageBacking::DetachGLContext() {
  if (texture_ != 0) {
    surface_texture_->DetachFromGLContext();
    glDeleteTextures(1, &texture_);
    texture_ = 0;
  }
}

}  // namespace clay
