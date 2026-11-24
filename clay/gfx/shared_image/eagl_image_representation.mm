// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/eagl_image_representation.h"

#import <CoreVideo/CoreVideo.h>
#import <OpenGLES/ES2/glext.h>
#include <OpenGLES/ES3/gl.h>

#include <utility>

#include "clay/gfx/shared_image/apple_gl_fence_sync.h"

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
#ifndef NDEBUG
    GLenum framebuffer_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
#else
    GLenum framebuffer_status = GL_FRAMEBUFFER_COMPLETE;
#endif

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

void EAGLImageRepresentation::ConsumeFence(std::unique_ptr<FenceSync> fence_sync) {
  if (fence_sync && fence_sync->EnqueueWait()) return;
  WaitForFence(std::move(fence_sync));
}

class EAGLFenceContext {
 public:
  static constexpr FenceSync::Type kType = FenceSync::Type::kEAGL;
  explicit EAGLFenceContext(EAGLContext* producer)
      : context_([[EAGLContext alloc] initWithAPI:producer.API sharegroup:producer.sharegroup]),
        es3_(producer.API == kEAGLRenderingAPIOpenGLES3),
        supported_(es3_ || HasAppleGLFenceExtension("GL_APPLE_sync")) {}
  bool valid() const { return context_ != nil; }
  bool supported() const { return supported_; }
  bool IsCurrentCompatible() const {
    EAGLContext* current = [EAGLContext currentContext];
    return current && current.sharegroup == context_.sharegroup;
  }
  GLsync CreateSync() {
    return es3_ ? glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0)
                : glFenceSyncAPPLE(GL_SYNC_GPU_COMMANDS_COMPLETE_APPLE, 0);
  }
  void DeleteSync(GLsync sync) {
    if (es3_)
      glDeleteSync(sync);
    else
      glDeleteSyncAPPLE(sync);
  }
  GLenum ClientWaitSync(GLsync sync, uint64_t timeout_ns) {
    return es3_ ? glClientWaitSync(sync, 0, timeout_ns)
                : glClientWaitSyncAPPLE(sync, 0, timeout_ns);
  }
  void WaitSync(GLsync sync) {
    if (es3_)
      glWaitSync(sync, 0, GL_TIMEOUT_IGNORED);
    else
      glWaitSyncAPPLE(sync, 0, GL_TIMEOUT_IGNORED_APPLE);
  }
  class ScopedCurrent {
   public:
    explicit ScopedCurrent(EAGLFenceContext* context)
        : previous_([EAGLContext currentContext]),
          valid_([EAGLContext setCurrentContext:context->context_]) {}
    ~ScopedCurrent() {
      if (valid_) [EAGLContext setCurrentContext:previous_];
    }
    bool valid() const { return valid_; }

   private:
    EAGLContext* previous_;
    bool valid_;
  };
  std::mutex mutex;

 private:
  EAGLContext* context_;
  bool es3_;
  bool supported_;
};

std::unique_ptr<FenceSync> EAGLImageRepresentation::ProduceFence() {
  if (HasFailedFence()) return TakeFailedFence();
  if (!(gl_context_ && [EAGLContext currentContext] == gl_context_)) {
    MarkFenceFailed();
    return TakeFailedFence();
  }
  if (!fence_context_) fence_context_ = std::make_shared<EAGLFenceContext>(gl_context_);
  return std::make_unique<AppleGLFenceSync<EAGLFenceContext>>(fence_context_, true);
}

fml::RefPtr<RepresentationStorageManager> EAGLImageRepresentation::GetTextureManager() const {
  return fml::MakeRefCounted<EAGLStorageManager>(gl_context_);
}

void EAGLImageRepresentation::SetTextureManager(
    fml::RefPtr<RepresentationStorageManager> storage_manager) {
  storage_manager_ = fml::Ref(static_cast<EAGLStorageManager*>(storage_manager.get()));
}

}  // namespace clay
