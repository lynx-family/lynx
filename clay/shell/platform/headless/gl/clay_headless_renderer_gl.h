// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_HEADLESS_GL_CLAY_HEADLESS_RENDERER_GL_H_
#define CLAY_SHELL_PLATFORM_HEADLESS_GL_CLAY_HEADLESS_RENDERER_GL_H_

#include <list>
#include <map>
#include <memory>
#include <optional>

#include "base/include/fml/macros.h"
#include "clay/shell/platform/headless/clay_headless_renderer.h"
#include "third_party/skia/include/core/SkRect.h"

namespace clay {

class ClayHeadlessRendererGL : public ClayHeadlessRenderer,
                               public GPUSurfaceGLDelegate {
 public:
  explicit ClayHeadlessRendererGL(ClayHeadlessEngine* engine);

  GPUSurfaceGLDelegate* GetGLRendererDelegate() override;

  // |GPUSurfaceGLDelegate|
  std::unique_ptr<GLContextResult> GLContextMakeCurrent() override;

  // |GPUSurfaceGLDelegate|
  bool GLContextClearCurrent() override;

  // |GPUSurfaceGLDelegate|
  void GLContextSetDamageRegion(
      const std::optional<skity::Rect>& region) override;

  // |GPUSurfaceGLDelegate|
  bool GLContextPresent(const GLPresentInfo& present_info) override;

  // |GPUSurfaceGLDelegate|
  GLFBOInfo GLContextFBO(GLFrameInfo frame_info) const override;

  // |GPUSurfaceGLDelegate|
  bool GLContextFBOResetAfterPresent() const override;

  // |GPUSurfaceGLDelegate|
  GLProcResolver GetGLProcResolver() const override;

  // |GPUSurfaceGLDelegate|
  SurfaceFrame::FramebufferInfo GLContextFramebufferInfo() const override;

 protected:
  virtual bool MakeCurrent() = 0;
  virtual bool ClearCurrent() = 0;
  virtual int64_t FBO(const ClayFrameInfo& frame_info) = 0;
  virtual bool Present() = 0;
};

class ClayHeadlessRendererSharedImageGL : public ClayHeadlessRendererGL {
 public:
  ClayHeadlessRendererSharedImageGL(ClayHeadlessEngine* engine,
                                    const ClayHardwareRendererConfig& config);

  ~ClayHeadlessRendererSharedImageGL() override;

 protected:
  void CleanupGPUResources() override;

  int64_t FBO(const ClayFrameInfo& frame_info) override;

  // |GPUSurfaceGLDelegate|
  bool GLContextPresent(const GLPresentInfo& present_info) override;

  // |GPUSurfaceGLDelegate|
  GLFBOInfo GLContextFBO(GLFrameInfo frame_info) const override;

  // |GPUSurfaceGLDelegate|
  SurfaceFrame::FramebufferInfo GLContextFramebufferInfo() const override;

  std::optional<skity::Rect> PopulateExistingDamage(int64_t fbo_id);

  bool Present() override;

  struct FBOSlot {
    ClayOpenGLFramebuffer framebuffer;
    uint32_t buffer_age;
    skity::Rect damage_rect;
  };
  bool disable_partial_repaint_ = false;
  std::optional<FBOSlot> fbo_storage_;
  ClaySize size_ = {0, 0};
  std::list<SkIRect> damage_history_;
  ClaySharedImageSinkAccessorRef surface_accessor_ = nullptr;

  static constexpr uint32_t kMaxHistorySize = 10;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_HEADLESS_GL_CLAY_HEADLESS_RENDERER_GL_H_
