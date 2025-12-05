// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/gpu/gpu_surface_gl_skity.h"

#if defined(OS_OSX)
#else
#include <EGL/egl.h>
#include <GLES/gl.h>
#endif

#include <utility>

#include "base/trace/native/trace_event.h"
#include "clay/common/graphics/persistent_cache.h"
#include "clay/fml/base32.h"
#include "clay/fml/logging.h"
#include "clay/fml/size.h"
#include "clay/shell/common/context_options.h"
#include "skity/gpu/gpu_context_gl.hpp"
namespace clay {

std::shared_ptr<skity::GPUContext> GPUSurfaceGLSkity::MakeGLContext(
    GPUSurfaceGLDelegate* delegate) {
  auto context_switch = delegate->GLContextMakeCurrent();
  if (!context_switch->GetResult()) {
    FML_LOG(ERROR)
        << "Could not make the context current to set up the GPU context.";
    return nullptr;
  }
#if defined(OS_OSX)
  return nullptr;
#else
  return skity::GLContextCreate(reinterpret_cast<void*>(eglGetProcAddress));
#endif
}

GPUSurfaceGLSkity::GPUSurfaceGLSkity(
    GPUSurfaceGLDelegate* delegate,
    std::shared_ptr<skity::GPUContext> skity_context)
    : delegate_(delegate),
      gpu_context_(std::move(skity_context)),
      weak_factory_(this) {
  auto context_switch = delegate_->GLContextMakeCurrent();
  if (!context_switch->GetResult()) {
    FML_LOG(ERROR)
        << "Could not make the context current to set up the GPU context.";
    return;
  }

  valid_ = gpu_context_ != nullptr;
}

GPUSurfaceGLSkity::~GPUSurfaceGLSkity() {
  if (!valid_) {
    return;
  }
  auto context_switch = delegate_->GLContextMakeCurrent();
  if (!context_switch->GetResult()) {
    FML_LOG(ERROR) << "Could not make the context current to destroy the "
                      "GPUContext resource.";
    return;
  }
  gpu_surface_ = nullptr;
  fbo_id_ = 0;
  gpu_context_ = nullptr;
  delegate_->GLContextClearCurrent();
}

// |Surface|
bool GPUSurfaceGLSkity::IsValid() { return valid_; }

// |Surface|
skity::Matrix GPUSurfaceGLSkity::GetRootTransformation() const {
  return delegate_->GLContextSurfaceTransformation();
}

// |Surface|
std::unique_ptr<SurfaceFrame> GPUSurfaceGLSkity::AcquireFrame(
    const skity::Vec2& size) {
  if (!IsValid()) {
    FML_LOG(ERROR) << "OpenGL surface was invalid.";
    return nullptr;
  }

  auto swap_callback = [weak = weak_factory_.GetWeakPtr(),
                        delegate = delegate_]() -> bool {
    if (weak) {
      uint32_t fbo_id = 0;
      delegate->GLContextPresent(
          {fbo_id, std::nullopt, std::nullopt, std::nullopt});
    }
    return true;
  };

  auto context_switch = delegate_->GLContextMakeCurrent();
  if (!context_switch->GetResult()) {
    FML_LOG(ERROR)
        << "Could not make the context current to acquire the frame.";
    return nullptr;
  }

  SurfaceFrame::FramebufferInfo framebuffer_info;

  std::shared_ptr<skity::GPUSurface> gpu_surface = AcquireRenderSurface(size);

  SurfaceFrame::EncodeCallback encode_callback =
      [weak = weak_factory_.GetWeakPtr()](const SurfaceFrame& surface_frame,

                                          skity::Canvas* canvas) {
        if (!canvas) {
          FML_LOG(ERROR) << "Canvas is null during submit";
          return false;
        }
        {
          TRACE_EVENT("clay", "skity::Canvas::Flush");
          canvas->Flush();
        }
        return weak ? weak->PresentSurface(surface_frame) : false;
      };

  SurfaceFrame::SubmitCallback submit_callback =
      [](const SurfaceFrame::SubmitInfo& surface_frame) { return true; };

  framebuffer_info = delegate_->GLContextFramebufferInfo();
  if (!framebuffer_info.existing_damage.has_value()) {
    framebuffer_info.existing_damage = existing_damage_;
  }

  auto frame = std::make_unique<SurfaceFrame>(
      std::shared_ptr<skity::GPUSurface>(gpu_surface), framebuffer_info,
      encode_callback, submit_callback, size, std::move(context_switch));

  frame->SetPreparedCallback(
      [weak = weak_factory_.GetWeakPtr()](std::optional<skity::Rect> damage) {
        if (weak) {
          weak->delegate_->GLContextSetDamageRegion(damage);
        }
      });

  return frame;
}

// |Surface|
std::unique_ptr<GLContextResult> GPUSurfaceGLSkity::MakeRenderContextCurrent() {
  return delegate_->GLContextMakeCurrent();
}

// |Surface|
bool GPUSurfaceGLSkity::ClearRenderContext() {
  return delegate_->GLContextClearCurrent();
}

// |Surface|
bool GPUSurfaceGLSkity::EnableRasterCache() const { return true; }

// |Surface|
skity::GPUContext* GPUSurfaceGLSkity::GetContext() {
  return gpu_context_.get();
}

std::shared_ptr<skity::GPUSurface> GPUSurfaceGLSkity::AcquireRenderSurface(
    const skity::Vec2& size) {
  if (size != size_) {
    size_ = size;
    gpu_surface_map_.clear();
  }

  GLFrameInfo frame_info = {static_cast<uint32_t>(size.x),
                            static_cast<uint32_t>(size.y)};
  const GLFBOInfo fbo_info = delegate_->GLContextFBO(frame_info);

  fbo_id_ = fbo_info.fbo_id;
  existing_damage_ = fbo_info.existing_damage;

  auto iter = gpu_surface_map_.find(fbo_info.fbo_id);
  if (iter != gpu_surface_map_.end()) {
    gpu_surface_ = iter->second;
    return gpu_surface_;
  }

  skity::GPUSurfaceDescriptorGL desc{};
  desc.backend = skity::GPUBackendType::kOpenGL;
  desc.width = size.x;
  desc.height = size.y;
  desc.sample_count = delegate_->GetSampleCount();
  desc.content_scale = 1;
  desc.gl_id = fbo_info.fbo_id;
  desc.has_stencil_attachment = true;
  desc.surface_type = skity::GLSurfaceType::kFramebuffer;

  gpu_surface_map_[fbo_info.fbo_id] = gpu_context_->CreateSurface(&desc);
  gpu_surface_ = gpu_surface_map_[fbo_info.fbo_id];

  return gpu_surface_;
}

bool GPUSurfaceGLSkity::PresentSurface(const SurfaceFrame& frame) {
  if (delegate_ == nullptr || gpu_context_ == nullptr ||
      gpu_surface_ == nullptr) {
    return false;
  }

  {
    TRACE_EVENT("clay", "skity::Surface::Flush");
    gpu_surface_->Flush();
  }

  GLPresentInfo present_info = {
      .fbo_id = fbo_id_,
      .frame_damage = frame.submit_info().frame_damage,
      .presentation_time = frame.submit_info().presentation_time,
      .buffer_damage = frame.submit_info().buffer_damage,
  };
  if (!delegate_->GLContextPresent(present_info)) {
    return false;
  }

  if (delegate_->GLContextFBOResetAfterPresent()) {
    gpu_surface_ = nullptr;
    fbo_id_ = 0;
    existing_damage_ = {};
  }

  return true;
}

}  // namespace clay
