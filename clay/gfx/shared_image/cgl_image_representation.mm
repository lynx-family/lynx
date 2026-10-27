// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/cgl_image_representation.h"

#import <CoreVideo/CoreVideo.h>
#import <OpenGL/gl3.h>

#include "base/include/fml/memory/ref_ptr.h"
#include "clay/common/graphics/gl/scoped_framebuffer_binder.h"
#include "clay/common/graphics/gl/scoped_texture_binder.h"
#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/cv_pixelbuffer_image_backing.h"
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/iosurface_image_backing.h"

namespace clay {

CGLStorageManager::CGLStorageManager(CGLContextObj gl_context) {
  CVOpenGLTextureCacheRef out_texture_cache;
  if (CVOpenGLTextureCacheCreate(kCFAllocatorDefault, nil, gl_context,
                                 CGLGetPixelFormat(gl_context), nil,
                                 &out_texture_cache) != kCVReturnSuccess) {
    FML_LOG(ERROR) << "Failed to create CVOpenGLTextureCache.";
    return;
  }
  cv_gl_texture_cache_ = out_texture_cache;
}

CGLStorageManager::~CGLStorageManager() { FlushStorageRecycle(); }

CVOpenGLTextureRef CGLStorageManager::CreateTextureFromStorage(CVPixelBufferRef pixel_buffer) {
  if (!cv_gl_texture_cache_) {
    FML_LOG(ERROR) << "No available CVOpenGLTextureCacheRef to create texture";
    return nil;
  }
  CVOpenGLTextureRef out_texture = nil;
  if (CVOpenGLTextureCacheCreateTextureFromImage(kCFAllocatorDefault, cv_gl_texture_cache_,
                                                 pixel_buffer, nil,
                                                 &out_texture) != kCVReturnSuccess) {
    FML_LOG(ERROR) << "Failed to create CVOpenGLTexture from CVPixelBuffer.";
    return nil;
  }
  return out_texture;
}

void CGLStorageManager::FlushStorageRecycle() {
  if (cv_gl_texture_cache_) {
    CVOpenGLTextureCacheFlush(cv_gl_texture_cache_, 0);
  }
}

CGLImageRepresentation::CGLImageRepresentation(fml::RefPtr<SharedImageBacking> backing)
    : GLImageRepresentation(backing), gl_context_(CGLGetCurrentContext()) {
  if (gl_context_ == nil) {
    FML_LOG(ERROR) << "Failed to get current context";
  }
}

CGLImageRepresentation::~CGLImageRepresentation() {
  if (fbo_id_ != 0) {
    UnbindFrameBuffer();
  }
  if (texture_id_ != 0) {
    ReleaseTexImage();
  }
}

ImageRepresentationType CGLImageRepresentation::GetType() const {
  return ImageRepresentationType::kCGL;
}

std::optional<GLImageRepresentation::TextureInfo> CGLImageRepresentation::GetTexImage() {
  if (texture_id_ == 0) {
    switch (GLImageRepresentation::GetBacking()->GetType()) {
      case SharedImageBacking::BackingType::kIOSurface:
        if (!GetTexImageFromIOSurface()) {
          return {};
        }
        break;
      case SharedImageBacking::BackingType::kCVPixelBuffer:
        if (!GetTexImageFromCVPixelBuffer()) {
          return {};
        }
        break;
      default:
        FML_LOG(ERROR) << "Invalid shared image backing type for CGLImageRepr.";
        return {};
    }
  }

  return TextureInfo{.target = GL_TEXTURE_RECTANGLE,
                     .name = texture_id_,
                     .format = internal_format_,
                     .size = GetSize()};
}

bool CGLImageRepresentation::GetTexImageFromIOSurface() {
  GLuint texture;
  glGenTextures(1, &texture);
  clay::ScopedTextureBinder scoped_texture_binder(GL_TEXTURE_RECTANGLE, texture);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  internal_format_ = GL_RGBA8;
  GLenum type = GL_UNSIGNED_INT_8_8_8_8_REV;
  GLenum pixel_format;
  switch (GetBacking()->GetPixelFormat()) {
    case SharedImageBacking::PixelFormat::kNative8888:
      pixel_format = GL_BGRA;
      break;
    default:
      FML_LOG(ERROR) << "Unsupported pixel format.";
      return false;
  }
  CGLError error = CGLTexImageIOSurface2D(
      gl_context_, GL_TEXTURE_RECTANGLE, internal_format_, GetSize().x, GetSize().y, pixel_format,
      type, static_cast<IOSurfaceRef>(GetBacking()->GetGFXHandle()), 0);
  if (error != kCGLNoError) {
    FML_LOG(ERROR) << "Failed to create CGL texture";
    glDeleteTextures(1, &texture);
    return false;
  }

  texture_id_ = texture;
  return true;
}

bool CGLImageRepresentation::GetTexImageFromCVPixelBuffer() {
  if (!storage_manager_) {
    FML_LOG(ERROR) << "No available CGLStorageManager to create texture";
    return false;
  }
  storage_manager_->FlushStorageRecycle();
  CVOpenGLTextureRef out_texture = storage_manager_->CreateTextureFromStorage(
      static_cast<CVPixelBufferRef>(GetBacking()->GetGFXHandle()));
  if (!out_texture) {
    return false;
  }
  GLuint texture;
  texture = CVOpenGLTextureGetName(out_texture);
  CFRelease(out_texture);
  clay::ScopedTextureBinder scoped_texture_binder(GL_TEXTURE_RECTANGLE, texture);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  internal_format_ = GL_RGBA8;
  texture_id_ = texture;
  return true;
}

bool CGLImageRepresentation::ReleaseTexImage() {
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

std::optional<GLImageRepresentation::FramebufferInfo> CGLImageRepresentation::BindFrameBuffer() {
  if (fbo_id_ == 0) {
    auto texture_info = GetTexImage();
    if (!texture_info) {
      return {};
    }

    GLuint fbo_id;
    glGenFramebuffers(1, &fbo_id);
    clay::ScopedFramebufferBinder scoped_framebuffer_binder(GL_FRAMEBUFFER, fbo_id);
    clay::ScopedTextureBinder scoped_texture_binder(GL_TEXTURE_RECTANGLE, texture_info->name);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE,
                           texture_info->name, 0);
    GLenum framebuffer_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if (framebuffer_status != GL_FRAMEBUFFER_COMPLETE) {
      FML_LOG(ERROR) << "Failed to create frame buffer";
      glDeleteFramebuffers(1, &fbo_id);
      return {};
    }

    fbo_id_ = fbo_id;
  }

  return FramebufferInfo{.target = GL_TEXTURE_RECTANGLE, .name = fbo_id_};
}

bool CGLImageRepresentation::UnbindFrameBuffer() {
  if (fbo_id_ == 0) {
    return false;
  }
  glDeleteFramebuffers(1, &fbo_id_);
  fbo_id_ = 0;

  return true;
}

// Note that CGL fence sync requires producer and consumer
// in the same shared group
class CGLFenceSync final : public FenceSync {
 public:
  CGLFenceSync() {
    fence_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    GLenum error = glGetError();
    if (!fence_ || error != GL_NO_ERROR) {
      FML_LOG(ERROR) << "Failed to create fence sync in CGL env, glGetError: " << error;
    }
    FML_DCHECK(glIsSync(fence_));
    glFlush();
  }

  ~CGLFenceSync() override {
    FML_DCHECK(glIsSync(fence_));
    glDeleteSync(fence_);
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
      FML_LOG(ERROR) << "Failed to delete fence sync in CGL env, glGetError: " << error;
    }
  }

  bool ClientWait() override {
    FML_DCHECK(glIsSync(fence_));
    GLenum result = glClientWaitSync(fence_, 0, GL_TIMEOUT_IGNORED);
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
      FML_LOG(ERROR) << "Failed to glClientWaitSync in CGL env, glGetError: " << error;
    }
    return result == GL_CONDITION_SATISFIED || result == GL_ALREADY_SIGNALED;
  }

  Type GetType() const override { return Type::kCGL; }

  void ServerWait() {
    FML_DCHECK(glIsSync(fence_));
    glWaitSync(fence_, 0, GL_TIMEOUT_IGNORED);
    GLenum gl_error = glGetError();
    if (gl_error != GL_NO_ERROR) {
      FML_LOG(ERROR) << "Failed to glWaitSync in CGL env, glGetError: " << gl_error;
    }
  }

 private:
  GLsync fence_;
};

void CGLImageRepresentation::ConsumeFence(std::unique_ptr<FenceSync> fence_sync) {
  if (!fence_sync) {
    return;
  }
  if (fence_sync->GetType() == FenceSync::Type::kCGL) {
    CGLFenceSync* cgl_fence_sync = static_cast<CGLFenceSync*>(fence_sync.get());
    cgl_fence_sync->ServerWait();
    return;
  }
  fence_sync->ClientWait();
}

class DummyFenceSync final : public FenceSync {
  bool ClientWait() override { return true; }

  Type GetType() const override { return Type::kClientWaitOnly; }
};

std::unique_ptr<FenceSync> CGLImageRepresentation::ProduceFence() {
  // It seems that glFlush is enough for gl<->gl
  glFlush();
  // TODO(youfeng): use CGLFenceSync if consumer and producer are in the same
  // group
  return std::make_unique<DummyFenceSync>();
}

fml::RefPtr<RepresentationStorageManager> CGLImageRepresentation::GetTextureManager() const {
  return fml::MakeRefCounted<CGLStorageManager>(gl_context_);
}

void CGLImageRepresentation::SetTextureManager(
    fml::RefPtr<RepresentationStorageManager> storage_manager) {
  storage_manager_ = fml::Ref(static_cast<CGLStorageManager*>(storage_manager.get()));
}

}  // namespace clay
