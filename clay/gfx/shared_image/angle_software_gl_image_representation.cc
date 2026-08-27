// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/angle_software_gl_image_representation.h"

#include <utility>

#include "angle_gl.h"
#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/angle_software_shm_image_backing.h"

namespace clay {

AngleSoftwareGLImageRepresentation::AngleSoftwareGLImageRepresentation(
    fml::RefPtr<SharedImageBacking> backing)
    : GLImageRepresentation(std::move(backing)) {}

AngleSoftwareGLImageRepresentation::~AngleSoftwareGLImageRepresentation() {
  UnbindFrameBuffer();
  ReleaseTexImage();
}

ImageRepresentationType AngleSoftwareGLImageRepresentation::GetType() const {
  return ImageRepresentationType::kGL;
}

void AngleSoftwareGLImageRepresentation::ConsumeFence(
    std::unique_ptr<FenceSync> fence_sync) {
  if (fence_sync) {
    fence_sync->ClientWait();
  }
}

std::unique_ptr<FenceSync> AngleSoftwareGLImageRepresentation::ProduceFence() {
  glFinish();
  return nullptr;
}

bool AngleSoftwareGLImageRepresentation::BeginRead(
    ClaySharedImageReadResult* out) {
  const auto texture_info = GetTexImage();
  if (!texture_info) {
    return false;
  }

  GLint old_texture = 0;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &old_texture);
  glBindTexture(GL_TEXTURE_2D, texture_info->name);
  const auto size = GetSize();
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.x, size.y, GL_RGBA,
                  GL_UNSIGNED_BYTE, GetBacking()->GetGFXHandle());
  const GLenum error = glGetError();
  glBindTexture(GL_TEXTURE_2D, old_texture);

  return error == GL_NO_ERROR && GLImageRepresentation::BeginRead(out);
}

std::optional<GLImageRepresentation::TextureInfo>
AngleSoftwareGLImageRepresentation::GetTexImage() {
  if (texture_id_ == 0) {
    GLint old_texture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &old_texture);
    glGenTextures(1, &texture_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    const auto size = GetSize();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, old_texture);
  }
  return TextureInfo{.target = GL_TEXTURE_2D,
                     .name = texture_id_,
                     .format = GL_RGBA8,
                     .size = GetSize()};
}

bool AngleSoftwareGLImageRepresentation::ReleaseTexImage() {
  if (texture_id_ == 0) {
    return false;
  }
  glDeleteTextures(1, &texture_id_);
  texture_id_ = 0;
  return true;
}

std::optional<GLImageRepresentation::FramebufferInfo>
AngleSoftwareGLImageRepresentation::BindFrameBuffer() {
  if (fbo_id_ == 0) {
    const auto texture_info = GetTexImage();
    if (!texture_info) {
      return std::nullopt;
    }
    GLint old_framebuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_framebuffer);
    glGenFramebuffers(1, &fbo_id_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_id_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           texture_info->name, 0);
#ifndef NDEBUG
    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
#else
    const GLenum status = GL_FRAMEBUFFER_COMPLETE;
#endif
    glBindFramebuffer(GL_FRAMEBUFFER, old_framebuffer);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
      FML_LOG(ERROR) << "Failed to create the SwiftShader output FBO: "
                     << status;
      glDeleteFramebuffers(1, &fbo_id_);
      fbo_id_ = 0;
      return std::nullopt;
    }
  }
  return FramebufferInfo{.target = GL_FRAMEBUFFER, .name = fbo_id_};
}

bool AngleSoftwareGLImageRepresentation::UnbindFrameBuffer() {
  if (fbo_id_ == 0) {
    return false;
  }
  glDeleteFramebuffers(1, &fbo_id_);
  fbo_id_ = 0;
  return true;
}

bool AngleSoftwareGLImageRepresentation::EndWrite() {
  GLint old_framebuffer = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_id_);
  static_cast<AngleSoftwareShmImageBacking*>(GetBacking())->CopyPixelsToShm();
  glBindFramebuffer(GL_FRAMEBUFFER, old_framebuffer);
  return GLImageRepresentation::EndWrite();
}

}  // namespace clay
