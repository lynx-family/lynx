// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/eagl_image_representation.h"

#import <CoreVideo/CoreVideo.h>
#import <OpenGLES/ES2/glext.h>
#include <OpenGLES/ES3/gl.h>

#include "base/include/fml/memory/ref_ptr.h"
#include "clay/common/graphics/gl/scoped_framebuffer_binder.h"
#include "clay/common/graphics/gl/scoped_texture_binder.h"
#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/cv_pixelbuffer_image_backing.h"
#include "clay/gfx/shared_image/fence_sync.h"

namespace clay {

EAGLStorageManager::EAGLStorageManager(EAGLContext* gl_context) {
  CVOpenGLESTextureCacheRef out_texture_cache;

  if (CVOpenGLESTextureCacheCreate(kCFAllocatorDefault, nil, gl_context, nil, &out_texture_cache) !=
      kCVReturnSuccess) {
    FML_LOG(ERROR) << "Failed to create CVOpenGLESTextureCache.";
    return;
  }
  eagl_texture_cache_ = out_texture_cache;
}

EAGLStorageManager::~EAGLStorageManager() { FlushStorageRecycle(); }

CVOpenGLESTextureRef EAGLStorageManager::CreateTextureFromStorage(CVPixelBufferRef pixel_buffer) {
  if (!eagl_texture_cache_) {
    FML_LOG(ERROR) << "No available CVOpenGLESTextureCacheRef to create texture";
    return nil;
  }
  CVOpenGLESTextureRef out_texture = nil;
  size_t width = CVPixelBufferGetWidthOfPlane(pixel_buffer, 0);
  size_t height = CVPixelBufferGetHeightOfPlane(pixel_buffer, 0);
  if (CVOpenGLESTextureCacheCreateTextureFromImage(
          kCFAllocatorDefault, eagl_texture_cache_, pixel_buffer, nullptr, GL_TEXTURE_2D, GL_RGBA,
          (GLsizei)width, (GLsizei)height, GL_BGRA, GL_UNSIGNED_BYTE, 0,
          &out_texture) != kCVReturnSuccess) {
    FML_LOG(ERROR) << "Failed to create CVOpenGLTexture from CVPixelBuffer.";
    return nil;
  }
  return out_texture;
}

void EAGLStorageManager::FlushStorageRecycle() {
  if (eagl_texture_cache_) {
    CVOpenGLESTextureCacheFlush(eagl_texture_cache_, 0);
  }
}

EAGLImageRepresentation::EAGLImageRepresentation(fml::RefPtr<CVPixelBufferImageBacking> backing)
    : GLImageRepresentation(backing), gl_context_([EAGLContext currentContext]) {
  if (gl_context_ == nil) {
    FML_LOG(ERROR) << "Failed to get current context";
  }
}

EAGLImageRepresentation::~EAGLImageRepresentation() {
  if (fbo_id_ != 0) {
    UnbindFrameBuffer();
  }
  if (texture_id_ != 0) {
    ReleaseTexImage();
  }
}

ImageRepresentationType EAGLImageRepresentation::GetType() const {
  return ImageRepresentationType::kEAGL;
}

std::optional<GLImageRepresentation::TextureInfo> EAGLImageRepresentation::GetTexImage() {
  if (texture_id_ == 0) {
    switch (GLImageRepresentation::GetBacking()->GetType()) {
      case SharedImageBacking::BackingType::kCVPixelBuffer:
        if (!GetTexImageFromCVPixelBuffer()) {
          return {};
        }
        break;
      default:
        FML_LOG(ERROR) << "Invalid shared image backing type for EAGL09ImageRepr.";
        return {};
    }
  }

  return TextureInfo{
      .target = GL_TEXTURE_2D, .name = texture_id_, .format = internal_format_, .size = GetSize()};
}

bool EAGLImageRepresentation::GetTexImageFromCVPixelBuffer() {
  if (!storage_manager_) {
    FML_LOG(ERROR) << "No available EAGLStorageManager to create texture";
    return false;
  }
  storage_manager_->FlushStorageRecycle();
  fml::CFRef<CVOpenGLESTextureRef> out_texture = storage_manager_->CreateTextureFromStorage(
      static_cast<CVPixelBufferRef>(GetBacking()->GetGFXHandle()));
  if (!out_texture) {
    return false;
  }
  GLuint texture;
  texture = CVOpenGLESTextureGetName(out_texture);
  internal_format_ = GL_RGBA;
  texture_id_ = texture;
  return true;
}

bool EAGLImageRepresentation::ReleaseTexImage() {
  if (texture_id_ == 0) {
    return false;
  }
  texture_id_ = 0;

  // The texture is actually owned by the cache so there is no need to explicitly call
  // 'glDeleteTexture'. Just flush cache.
  if (storage_manager_) {
    storage_manager_->FlushStorageRecycle();
  }
  return true;
}

std::optional<GLImageRepresentation::FramebufferInfo> EAGLImageRepresentation::BindFrameBuffer() {
  if (fbo_id_ == 0) {
    auto texture_info = GetTexImage();
    if (!texture_info) {
      return {};
    }

    GLuint fbo_id;
    glGenFramebuffers(1, &fbo_id);
    clay::ScopedFramebufferBinder scoped_framebuffer_binder(GL_FRAMEBUFFER, fbo_id);
    clay::ScopedTextureBinder scoped_texture_binder(GL_TEXTURE_2D, texture_info->name);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_info->name,
                           0);
    GLenum framebuffer_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if (framebuffer_status != GL_FRAMEBUFFER_COMPLETE) {
      FML_LOG(ERROR) << "Failed to create frame buffer";
      glDeleteFramebuffers(1, &fbo_id);
      return {};
    }

    fbo_id_ = fbo_id;
  }

  return FramebufferInfo{.target = GL_TEXTURE_2D, .name = fbo_id_};
}

bool EAGLImageRepresentation::UnbindFrameBuffer() {
  if (fbo_id_ == 0) {
    return false;
  }
  glDeleteFramebuffers(1, &fbo_id_);
  fbo_id_ = 0;

  return true;
}

// Note that EAGL fence sync requires producer and consumer
// in the same shared group
class EAGLFenceSync final : public FenceSync {
 public:
  EAGLFenceSync() {
    fence_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    GLenum error = glGetError();
    if (!fence_ || error != GL_NO_ERROR) {
      FML_LOG(ERROR) << "Failed to create fence sync in EAGL env, glGetError: " << error;
    }
    FML_DCHECK(glIsSync(fence_));
    glFlush();
  }

  ~EAGLFenceSync() override {
    FML_DCHECK(glIsSync(fence_));
    glDeleteSync(fence_);
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
      FML_LOG(ERROR) << "Failed to delete fence sync in EAGL env, glGetError: " << error;
    }
  }

  bool ClientWait() override {
    FML_DCHECK(glIsSync(fence_));
    GLenum result = glClientWaitSync(fence_, 0, GL_TIMEOUT_IGNORED);
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
      FML_LOG(ERROR) << "Failed to glClientWaitSync in EAGL env, glGetError: " << error;
    }
    return result == GL_CONDITION_SATISFIED || result == GL_ALREADY_SIGNALED;
  }

  Type GetType() const override { return Type::kEAGL; }

  void ServerWait() {
    FML_DCHECK(glIsSync(fence_));
    glWaitSync(fence_, 0, GL_TIMEOUT_IGNORED);
    GLenum gl_error = glGetError();
    if (gl_error != GL_NO_ERROR) {
      FML_LOG(ERROR) << "Failed to glWaitSync in EAGL env, glGetError: " << gl_error;
    }
  }

 private:
  GLsync fence_;
};

void EAGLImageRepresentation::ConsumeFence(std::unique_ptr<FenceSync> fence_sync) {
  if (!fence_sync) {
    return;
  }
  if (fence_sync->GetType() == FenceSync::Type::kEAGL) {
    EAGLFenceSync* eagl_fence_sync = static_cast<EAGLFenceSync*>(fence_sync.get());
    eagl_fence_sync->ServerWait();
    return;
  }
  fence_sync->ClientWait();
}

class DummyFenceSync final : public FenceSync {
  bool ClientWait() override { return true; }

  Type GetType() const override { return Type::kClientWaitOnly; }
};

std::unique_ptr<FenceSync> EAGLImageRepresentation::ProduceFence() {
  // It seems that glFlush is enough for gl<->gl
  glFlush();
  // TODO(youfeng): use EAGLFenceSync if consumer and producer are in the same
  // group
  return std::make_unique<DummyFenceSync>();
}

fml::RefPtr<RepresentationStorageManager> EAGLImageRepresentation::GetTextureManager() const {
  return fml::MakeRefCounted<EAGLStorageManager>(gl_context_);
}

void EAGLImageRepresentation::SetTextureManager(
    fml::RefPtr<RepresentationStorageManager> storage_manager) {
  storage_manager_ = fml::Ref(static_cast<EAGLStorageManager*>(storage_manager.get()));
}

}  // namespace clay
