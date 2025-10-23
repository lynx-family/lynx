// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/native_image_image_backing.h"

#include <GLES2/gl2ext.h>

#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/native_image_egl_image_representation.h"
#include "clay/gfx/shared_image/skia_gl_image_representation.h"
#include "third_party/skia/include/gpu/GrDirectContext.h"

namespace clay {

NativeImageImageBacking::NativeImageImageBacking(
    PixelFormat pixel_format, skity::Vec2 size,
    std::optional<GraphicsMemoryHandle> gfx_handle)
    : SharedImageBackingUnmanaged(pixel_format, size) {
  if (gfx_handle.has_value()) {
    native_image_ = reinterpret_cast<OH_NativeImage*>(gfx_handle.value());
  } else {
    native_image_ = OH_NativeImage_Create(0, GL_TEXTURE_EXTERNAL_OES);
  }
  OH_NativeImage_SetOnFrameAvailableListener(
      native_image_,
      {.context = this, .onFrameAvailable = [](void* context) {
         static_cast<NativeImageImageBacking*>(context)->TriggerFrameCallback();
       }});
}

NativeImageImageBacking::~NativeImageImageBacking() {
  if (texture_ != 0) {
    FML_LOG(ERROR) << "Texture not detached, maybe leaked!";
  }
  if (native_image_) {
    OH_NativeImage_UnsetOnFrameAvailableListener(native_image_);
    OH_NativeImage_Destroy(&native_image_);
  }
}

const skity::Vec2 NativeImageImageBacking::GetSize() const {
  if (size_.x <= 0 || size_.y <= 0) {
    // If the size is not set, we assume the default size is 256x256.
    return skity::Vec2(256, 256);
  }
  return size_;
}

SharedImageBacking::BackingType NativeImageImageBacking::GetType() const {
  return BackingType::kNativeImage;
}

GraphicsMemoryHandle NativeImageImageBacking::GetGFXHandle() const {
  return native_image_;
}

fml::RefPtr<SharedImageRepresentation>
NativeImageImageBacking::CreateRepresentation(
    const ClaySharedImageRepresentationConfig* config) {
  auto type = config->type;
  switch (type) {
    case kClaySharedImageRepresentationTypeGL: {
      return fml::MakeRefCounted<NativeImageEGLImageRepresentation>(
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

fml::RefPtr<SkiaImageRepresentation>
NativeImageImageBacking::CreateSkiaRepresentation(GrDirectContext* gr_context) {
  switch (gr_context->backend()) {
    case GrBackendApi::kOpenGL: {
      return fml::MakeRefCounted<SkiaGLImageRepresentation>(
          gr_context, fml::MakeRefCounted<NativeImageEGLImageRepresentation>(
                          fml::Ref(this)));
    }
    default: {
      FML_LOG(ERROR)
          << "Unable to call "
             "AHardwareBufferImageBacking::CreateSkiaRepresentation with "
             "backend: "
          << static_cast<uint32_t>(gr_context->backend());
      return nullptr;
    }
  }
}

const skity::Matrix NativeImageImageBacking::GetTransformation() const {
  float mat[16];
  int ret = OH_NativeImage_GetTransformMatrix(native_image_, mat);
  if (ret) {
    FML_LOG(ERROR) << "NativeImageImageBacking getTransformMatrix failed "
                   << ret;
    return skity::Matrix();
  }
  // NativeImage buffer is always Y-down, so we need to flip y-axis.
  skity::Matrix flip_y_mat = skity::Matrix(1, 0, 0, 0, -1, 1, 0, 0, 1);
  skity::Matrix transformation = skity::Matrix();
  for (int col = 0; col < 4; ++col) {
    for (int row = 0; row < 4; ++row) {
      transformation.Set(row, col, mat[col * 4 + row]);
    }
  }
  return flip_y_mat * transformation;
}

void NativeImageImageBacking::SetTransformation(const skity::Matrix& mat) {
  FML_UNIMPLEMENTED();
}

/// `SharedImageBackingUnmanaged`
void NativeImageImageBacking::SetFrameAvailableCallback(
    const fml::closure& callback) {
  std::lock_guard<std::mutex> l(frame_callback_mutex_);
  frame_callback_ = callback;
}

bool NativeImageImageBacking::UpdateFront() {
  EnsureAttachedToGLContext();
  return OH_NativeImage_UpdateSurfaceImage(native_image_) == 0;
}

void NativeImageImageBacking::ReleaseFront() { FML_UNIMPLEMENTED(); }

uint32_t NativeImageImageBacking::AcquireBack() {
  // No need to call any functions, since eglMakeCurrent automatically requires
  // a new buffer.
  return 0;  // TODO(youfeng) support buffer age for native image.
}

bool NativeImageImageBacking::SwapBack() {
  // No need to call any functions, since eglSwap automatically swap back
  return true;
}

uint32_t NativeImageImageBacking::Capacity() const {
  // We assume it's multiple buffers backed
  return 2;
}

bool NativeImageImageBacking::SetSize(skity::Vec2 size) {
  size_ = size;
  return true;
}

GLuint NativeImageImageBacking::EnsureAttachedToGLContext() {
  if (texture_ == 0) {
    glGenTextures(1, &texture_);
    int ret = OH_NativeImage_AttachContext(native_image_, texture_);
    if (ret) {
      FML_LOG(ERROR) << "NativeImageImageBacking attachContext failed " << ret;
    }
  }
  return texture_;
}

void NativeImageImageBacking::DetachGLContext() {
  if (texture_ != 0) {
    int ret = OH_NativeImage_DetachContext(native_image_);
    if (ret) {
      FML_LOG(ERROR) << "NativeImageImageBacking detachContext failed " << ret;
    }
    glDeleteTextures(1, &texture_);
    texture_ = 0;
  }
}

void NativeImageImageBacking::TriggerFrameCallback() {
  fml::closure callback;
  {
    std::lock_guard<std::mutex> l(frame_callback_mutex_);
    callback = frame_callback_;
  }
  if (callback) {
    callback();
  }
}

}  // namespace clay
