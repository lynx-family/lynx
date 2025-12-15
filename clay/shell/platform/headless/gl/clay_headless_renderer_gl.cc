// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/headless/gl/clay_headless_renderer_gl.h"

#include <memory>

#include "clay/fml/logging.h"
#include "clay/shell/platform/embedder/embedder_struct_macros.h"
#include "clay/shell/platform/headless/clay_headless_engine.h"

namespace clay {

ClayHeadlessRendererGL::ClayHeadlessRendererGL(ClayHeadlessEngine* engine)
    : ClayHeadlessRenderer(engine) {}

GPUSurfaceGLDelegate* ClayHeadlessRendererGL::GetGLRendererDelegate() {
  return this;
}

std::unique_ptr<GLContextResult>
ClayHeadlessRendererGL::GLContextMakeCurrent() {
  return std::make_unique<GLContextDefaultResult>(MakeCurrent());
}

bool ClayHeadlessRendererGL::GLContextClearCurrent() { return ClearCurrent(); }

void ClayHeadlessRendererGL::GLContextSetDamageRegion(
    const std::optional<skity::Rect>& region) {}

bool ClayHeadlessRendererGL::GLContextPresent(
    const GLPresentInfo& present_info) {
  return Present();
}

GLFBOInfo ClayHeadlessRendererGL::GLContextFBO(GLFrameInfo frame_info) const {
  ClayFrameInfo clay_frame_info{};
  clay_frame_info.struct_size = sizeof(clay_frame_info);
  clay_frame_info.width = frame_info.width;
  clay_frame_info.height = frame_info.height;
  return {
      .fbo_id = const_cast<ClayHeadlessRendererGL*>(this)->FBO(clay_frame_info),
      .existing_damage = std::nullopt};
}

bool ClayHeadlessRendererGL::GLContextFBOResetAfterPresent() const {
  return true;
}

GPUSurfaceGLDelegate::GLProcResolver ClayHeadlessRendererGL::GetGLProcResolver()
    const {
  return nullptr;
}

SurfaceFrame::FramebufferInfo ClayHeadlessRendererGL::GLContextFramebufferInfo()
    const {
  auto info = SurfaceFrame::FramebufferInfo{};
  info.supports_readback = true;
  info.supports_partial_repaint = false;
  return info;
}

ClayHeadlessRendererSharedImageGL::ClayHeadlessRendererSharedImageGL(
    ClayHeadlessEngine* engine, const ClayHardwareRendererConfig& config)
    : ClayHeadlessRendererGL(engine),
      disable_partial_repaint_(config.disable_partial_repaint) {
  ClaySharedImageRepresentationConfig image_repr{};
  image_repr.struct_size = sizeof(image_repr);
  image_repr.type = kClaySharedImageRepresentationTypeGL;
  surface_accessor_ =
      ClayCreateSharedImageSinkAccessor(config.sink_ref, &image_repr);
}

ClayHeadlessRendererSharedImageGL::~ClayHeadlessRendererSharedImageGL() =
    default;

void ClayHeadlessRendererSharedImageGL::CleanupGPUResources() {
  if (fbo_storage_) {
    if (fbo_storage_->framebuffer.destruction_callback) {
      fbo_storage_->framebuffer.destruction_callback(
          fbo_storage_->framebuffer.user_data);
    }

    fbo_storage_.reset();
  }

  if (surface_accessor_) {
    ClayDestroySharedImageSinkAccessor(surface_accessor_);
    surface_accessor_ = nullptr;
  }
}

int64_t ClayHeadlessRendererSharedImageGL::FBO(
    const ClayFrameInfo& frame_info) {
  if (fbo_storage_) {
    return fbo_storage_->framebuffer.name;
  }

  if (size_.width != frame_info.width || size_.height != frame_info.height) {
    damage_history_.clear();
  }

  size_.width = frame_info.width;
  size_.height = frame_info.height;

  ClaySharedImageWriteResult result;
  uint32_t buffer_age = 0;

  if (!surface_accessor_ ||
      !ClaySharedImageSinkBeginWrite(surface_accessor_, &size_, nullptr,
                                     &result, &buffer_age)) {
    FML_LOG(ERROR) << "failed to get fbo";
    return -1;
  }

  FML_DCHECK(result.type == kClaySharedImageRepresentationTypeGL);

  uint32_t fbo_name = result.opengl_framebuffer.name;

  fbo_storage_ = FBOSlot{
      .framebuffer = result.opengl_framebuffer,
      .buffer_age = buffer_age,
  };

  return fbo_name;
}

namespace {
static skity::Rect SkIRectToSkityRect(const SkIRect& sk_rect) {
  skity::Rect rect(static_cast<double>(sk_rect.fLeft),
                   static_cast<double>(sk_rect.fTop),
                   static_cast<double>(sk_rect.fRight),
                   static_cast<double>(sk_rect.fBottom));
  return rect;
}

static SkIRect SkityRectToSKIRect(const skity::Rect& rect) {
  return SkIRect::MakeLTRB(rect.Left(), rect.Top(), rect.Right(),
                           rect.Bottom());
}
}  // namespace

// |GPUSurfaceGLDelegate|
bool ClayHeadlessRendererSharedImageGL::GLContextPresent(
    const GLPresentInfo& present_info) {
  if (!fbo_storage_) {
    return false;
  }
  if (disable_partial_repaint_) {
    return Present();
  }

  FML_DCHECK(present_info.fbo_id == fbo_storage_->framebuffer.name);

  if (!Present()) {
    return false;
  }

  if (present_info.frame_damage) {
    damage_history_.push_back(SkityRectToSKIRect(*present_info.frame_damage));
  } else {
    damage_history_.push_back(SkIRect::MakeWH(size_.width, size_.height));
  }

  if (damage_history_.size() > kMaxHistorySize) {
    damage_history_.pop_front();
  }

  return true;
}

// |GPUSurfaceGLDelegate|
GLFBOInfo ClayHeadlessRendererSharedImageGL::GLContextFBO(
    GLFrameInfo frame_info) const {
  ClayFrameInfo clay_frame_info{};
  clay_frame_info.struct_size = sizeof(clay_frame_info);
  clay_frame_info.width = frame_info.width;
  clay_frame_info.height = frame_info.height;
  int64_t fbo_id = const_cast<ClayHeadlessRendererSharedImageGL*>(this)->FBO(
      clay_frame_info);
  return {
      .fbo_id = fbo_id,
      .existing_damage = const_cast<ClayHeadlessRendererSharedImageGL*>(this)
                             ->PopulateExistingDamage(fbo_id)};
}

// |GPUSurfaceGLDelegate|
SurfaceFrame::FramebufferInfo
ClayHeadlessRendererSharedImageGL::GLContextFramebufferInfo() const {
  auto info = SurfaceFrame::FramebufferInfo{};
  info.supports_readback = true;
  info.supports_partial_repaint = true;
  return info;
}

std::optional<skity::Rect>
ClayHeadlessRendererSharedImageGL::PopulateExistingDamage(int64_t fbo_id) {
  if (disable_partial_repaint_) {
    return std::nullopt;
  }
  if (!fbo_storage_) {
    return std::nullopt;
  }
  FML_DCHECK(fbo_id == fbo_storage_->framebuffer.name);
  uint32_t buffer_age = fbo_storage_->buffer_age;
  if (buffer_age == 0 || damage_history_.size() < buffer_age) {
    return std::nullopt;
  }
  SkIRect damage = SkIRect::MakeEmpty();
  // join up to (age - 1) last rects from damage history
  --buffer_age;
  for (auto i = damage_history_.rbegin(); buffer_age > 0; ++i, --buffer_age) {
    damage.join(*i);
  }
  fbo_storage_->damage_rect = SkIRectToSkityRect(damage);

  return fbo_storage_->damage_rect;
}

bool ClayHeadlessRendererSharedImageGL::Present() {
  if (!fbo_storage_ || !surface_accessor_) {
    return false;
  }

  if (!ClaySharedImageSinkEndWrite(surface_accessor_)) {
    return false;
  }

  if (fbo_storage_->framebuffer.destruction_callback) {
    fbo_storage_->framebuffer.destruction_callback(
        fbo_storage_->framebuffer.user_data);
  }

  fbo_storage_.reset();

  return true;
}

}  // namespace clay
