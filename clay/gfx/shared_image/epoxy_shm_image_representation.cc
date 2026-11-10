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
#include "clay/gfx/shared_image/linux_egl_fence_sync.h"
#include "clay/gfx/shared_image/shared_image_backing.h"

namespace clay {
namespace {
bool FinishLegacyWork() {
  for (int i = 0; i < 16; ++i) {
    const auto error = glGetError();
    if (error == GL_NO_ERROR) {
      glFinish();
      return glGetError() == GL_NO_ERROR;
    }
    if (error == GL_OUT_OF_MEMORY || error == 0x0507) return false;
  }
  return false;
}
LinuxEGLFenceSync::Dispatch FenceDispatch() {
  static int backend = 0;
  return {&backend,        eglGetCurrentDisplay, eglGetCurrentContext,
          eglQueryString,  eglGetProcAddress,    glFlush,
          FinishLegacyWork};
}
}  // namespace

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
  if (fence_sync && fence_sync->GetType() == FenceSync::Type::kLinuxEGL &&
      static_cast<LinuxEGLFenceSync*>(fence_sync.get())
          ->CanWaitOn(FenceDispatch()) &&
      fence_sync->EnqueueWait())
    return;
  WaitForFence(std::move(fence_sync));
}

std::unique_ptr<FenceSync> EpoxyShmImageRepresentation::ProduceFence() {
  if (HasFailedFence()) return TakeFailedFence();
  return std::make_unique<LinuxEGLFenceSync>(FenceDispatch());
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
    if (!texture_info) return std::nullopt;
    GLuint backend_texture = texture_info->name;
    GLuint fbo_id;
    glGenFramebuffers(1, &fbo_id);
    clay::ScopedTextureBinder scoped_texture_binder(GL_TEXTURE_2D,
                                                    backend_texture);
    clay::ScopedFramebufferBinder scoped_framebuffer_binder(GL_FRAMEBUFFER,
                                                            fbo_id);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           backend_texture, 0);
#ifndef NDEBUG
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
#else
    GLenum status = GL_FRAMEBUFFER_COMPLETE;
#endif
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
  if (HasFailedFence()) return false;
  static_cast<EpoxyShmImageBacking*>(GetBacking())->CopyPixelsToTexture();
  return GLImageRepresentation::BeginRead(out);
}

bool EpoxyShmImageRepresentation::EndWrite() {
  if (HasFailedFence()) return false;
  clay::ScopedFramebufferBinder scoped_framebuffer_binder(GL_FRAMEBUFFER,
                                                          fbo_id_);
  static_cast<EpoxyShmImageBacking*>(GetBacking())->CopyPixelsToShm();
  return GLImageRepresentation::EndWrite();
}

}  // namespace clay
