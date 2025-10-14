// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/shared_image_representation.h"

#include <cstring>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/shared_image_backing.h"

namespace clay {

RepresentationStorageManager::~RepresentationStorageManager() = default;

SharedImageRepresentation::SharedImageRepresentation(
    fml::RefPtr<SharedImageBacking> backing)
    : backing_(std::move(backing)) {}

SharedImageRepresentation::~SharedImageRepresentation() = default;

const skity::Vec2 SharedImageRepresentation::GetSize() const {
  return backing_->GetSize();
}

#ifndef ENABLE_SKITY
SkiaImageRepresentation::SkiaImageRepresentation(
    fml::RefPtr<SharedImageBacking> backing)
    : SharedImageRepresentation(std::move(backing)) {}

SkiaImageRepresentation::~SkiaImageRepresentation() = default;

ImageRepresentationType SkiaImageRepresentation::GetType() const {
  return ImageRepresentationType::kSkia;
}

bool SkiaImageRepresentation::BeginRead(ClaySharedImageReadResult* out) {
  FML_LOG(ERROR)
      << "SkiaImageRepresentation doesn't support directly read/write";
  return false;
}

bool SkiaImageRepresentation::BeginWrite(ClaySharedImageWriteResult* out) {
  FML_LOG(ERROR)
      << "SkiaImageRepresentation doesn't support directly read/write";
  return false;
}
bool SkiaImageRepresentation::EndWrite() {
  FML_LOG(ERROR)
      << "SkiaImageRepresentation doesn't support directly read/write";
  return false;
}
#else
SkityImageRepresentation::SkityImageRepresentation(
    fml::RefPtr<SharedImageBacking> backing)
    : SharedImageRepresentation(std::move(backing)) {}

SkityImageRepresentation::~SkityImageRepresentation() = default;

ImageRepresentationType SkityImageRepresentation::GetType() const {
  return ImageRepresentationType::kSkity;
}

bool SkityImageRepresentation::BeginRead(ClaySharedImageReadResult* out) {
  FML_LOG(ERROR)
      << "SkityImageRepresentation doesn't support directly read/write";
  return false;
}
bool SkityImageRepresentation::BeginWrite(ClaySharedImageWriteResult* out) {
  FML_LOG(ERROR)
      << "SkityImageRepresentation doesn't support directly read/write";
  return false;
}
bool SkityImageRepresentation::EndWrite() {
  FML_LOG(ERROR)
      << "SkityImageRepresentation doesn't support directly read/write";
  return false;
}
#endif  // ENABLE_SKITY

GLImageRepresentation::GLImageRepresentation(
    fml::RefPtr<SharedImageBacking> backing)
    : SharedImageRepresentation(std::move(backing)) {}

bool GLImageRepresentation::BeginRead(ClaySharedImageReadResult* out) {
  std::optional<TextureInfo> texture_info = GetTexImage();
  if (!texture_info) {
    return false;
  }
  memset(out, 0, sizeof(ClaySharedImageReadResult));
  out->struct_size = sizeof(ClaySharedImageReadResult);
  out->type = kClaySharedImageRepresentationTypeGL;
  out->opengl_texture.struct_size = sizeof(ClayOpenGLTexture);
  out->opengl_texture.format = texture_info->format;
  out->opengl_texture.name = texture_info->name;
  out->opengl_texture.target = texture_info->target;
  if (texture_info->size) {
    out->opengl_texture.size = {
        .width = static_cast<uint32_t>(texture_info->size->x),
        .height = static_cast<uint32_t>(texture_info->size->y)};
  }
  out->opengl_texture.user_data = this;
  this->AddRef();
  out->opengl_texture.destruction_callback = [](void* user_data) {
    static_cast<GLImageRepresentation*>(user_data)->Release();
  };
  return true;
}

bool GLImageRepresentation::EndRead() {
  // We don't delete internal texture here since it's time consuming
  return true;
}

bool GLImageRepresentation::BeginWrite(ClaySharedImageWriteResult* out) {
  std::optional<FramebufferInfo> fbo_info = BindFrameBuffer();
  if (!fbo_info) {
    return false;
  }

  memset(out, 0, sizeof(ClaySharedImageWriteResult));
  out->struct_size = sizeof(ClaySharedImageWriteResult);
  out->type = kClaySharedImageRepresentationTypeGL;
  out->opengl_framebuffer.struct_size = sizeof(ClayOpenGLFramebuffer);
  out->opengl_framebuffer.target = fbo_info->target;
  out->opengl_framebuffer.name = fbo_info->name;
  out->opengl_framebuffer.user_data = this;
  this->AddRef();
  out->opengl_framebuffer.destruction_callback = [](void* user_data) {
    static_cast<GLImageRepresentation*>(user_data)->Release();
  };
  return true;
}

bool GLImageRepresentation::EndWrite() {
  // We don't delete internal fbo here since it's time consuming
  return true;
}

}  // namespace clay
