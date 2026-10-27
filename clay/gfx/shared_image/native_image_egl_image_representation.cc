// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/native_image_egl_image_representation.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/native_image_image_backing.h"

namespace clay {

NativeImageEGLImageRepresentation::NativeImageEGLImageRepresentation(
    fml::RefPtr<NativeImageImageBacking> backing)
    : GLImageRepresentation(backing), backing_(backing) {}

NativeImageEGLImageRepresentation::~NativeImageEGLImageRepresentation() {
  UnbindFrameBuffer();
  ReleaseTexImage();
}

ImageRepresentationType NativeImageEGLImageRepresentation::GetType() const {
  return ImageRepresentationType::kEGL;
}

void NativeImageEGLImageRepresentation::ConsumeFence(
    std::unique_ptr<FenceSync> fence) {
  if (fence) {
    fence->ClientWait();
  }
}

std::unique_ptr<FenceSync> NativeImageEGLImageRepresentation::ProduceFence() {
  // The fence is created in NativeImage's internal implementations
  return nullptr;
}

std::optional<GLImageRepresentation::TextureInfo>
NativeImageEGLImageRepresentation::GetTexImage() {
  return TextureInfo{.target = GL_TEXTURE_EXTERNAL_OES,
                     .name = backing_->EnsureAttachedToGLContext(),
                     .format = GL_RGBA8_OES,
                     .size = GetSize()};
}

bool NativeImageEGLImageRepresentation::ReleaseTexImage() {
  backing_->DetachGLContext();

  return true;
}

std::optional<GLImageRepresentation::FramebufferInfo>
NativeImageEGLImageRepresentation::BindFrameBuffer() {
  // TODO(youfeng) support write to NativeImage
  return {};
}

bool NativeImageEGLImageRepresentation::UnbindFrameBuffer() {
  // TODO(youfeng) support write to NativeImage
  return false;
}

}  // namespace clay
