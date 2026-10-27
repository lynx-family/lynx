// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/epoxy_shm_image_representation.h"

#include <epoxy/egl.h>
#include <epoxy/gl.h>

#include <utility>

#include "clay/common/graphics/gl/scoped_framebuffer_binder.h"
#include "clay/common/graphics/gl/scoped_texture_binder.h"
#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/epoxy_shm_image_backing.h"
#include "clay/gfx/shared_image/shared_image_backing.h"

namespace clay {

EpoxyShmImageRepresentation::EpoxyShmImageRepresentation(
    fml::RefPtr<SharedImageBacking> backing)
    : GLImageRepresentation(std::move(backing)) {}

EpoxyShmImageRepresentation::~EpoxyShmImageRepresentation() {
  UnbindFrameBuffer();
  ReleaseTexImage();
}

ImageRepresentationType EpoxyShmImageRepresentation::GetType() const {
  return ImageRepresentationType::kGL;
}

void EpoxyShmImageRepresentation::ConsumeFence(
    std::unique_ptr<FenceSync> fence_sync) {
  if (!fence_sync) {
    return;
  }
  fence_sync->ClientWait();
}

std::unique_ptr<FenceSync> EpoxyShmImageRepresentation::ProduceFence() {
  glFinish();
  return nullptr;
}

std::optional<GLImageRepresentation::TextureInfo>
EpoxyShmImageRepresentation::GetTexImage() {
  if (texture_id_ == 0) {
    SharedImageBacking* backing = GetBacking();
    if (!backing) {
      return std::nullopt;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    clay::ScopedTextureBinder scoped_texture_binder(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    switch (backing->GetType()) {
      case SharedImageBacking::BackingType::kShmImage: {
        static_cast<EpoxyShmImageBacking*>(backing)->BindToTexture(
            GL_TEXTURE_2D);
        break;
      }
      default: {
        // NOT SUPPORTED.
        FML_UNREACHABLE();
      }
    }
    texture_id_ = texture;
  }
  return TextureInfo{.target = GL_TEXTURE_2D,
                     .name = texture_id_,
                     .format = GL_RGBA8,
                     .size = GetSize()};
}

bool EpoxyShmImageRepresentation::ReleaseTexImage() {
  if (texture_id_ == 0) {
    return false;
  }
  glDeleteTextures(1, &texture_id_);
  texture_id_ = 0;
  return true;
}

std::optional<GLImageRepresentation::FramebufferInfo>
EpoxyShmImageRepresentation::BindFrameBuffer() {
  if (fbo_id_ == 0) {
    auto texture_info = GetTexImage();
    GLuint backend_texture = texture_info->name;
    GLuint fbo_id;
    glGenFramebuffers(1, &fbo_id);
    clay::ScopedTextureBinder scoped_texture_binder(GL_TEXTURE_2D,
                                                    backend_texture);
    clay::ScopedFramebufferBinder scoped_framebuffer_binder(GL_FRAMEBUFFER,
                                                            fbo_id);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           backend_texture, 0);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
      FML_LOG(ERROR) << "Failed to create FBO, error: " << status;
      glDeleteFramebuffers(1, &fbo_id);
      return std::nullopt;
    }
    fbo_id_ = fbo_id;
  }
  return FramebufferInfo{.target = GL_FRAMEBUFFER, .name = fbo_id_};
}

bool EpoxyShmImageRepresentation::UnbindFrameBuffer() {
  if (fbo_id_ == 0) {
    return false;
  }
  glDeleteFramebuffers(1, &fbo_id_);
  fbo_id_ = 0;
  return true;
}

bool EpoxyShmImageRepresentation::BeginRead(ClaySharedImageReadResult* out) {
  static_cast<EpoxyShmImageBacking*>(GetBacking())->CopyPixelsToTexture();
  return GLImageRepresentation::BeginRead(out);
}

bool EpoxyShmImageRepresentation::EndWrite() {
  clay::ScopedFramebufferBinder scoped_framebuffer_binder(GL_FRAMEBUFFER,
                                                          fbo_id_);
  static_cast<EpoxyShmImageBacking*>(GetBacking())->CopyPixelsToShm();
  return GLImageRepresentation::EndWrite();
}

}  // namespace clay
