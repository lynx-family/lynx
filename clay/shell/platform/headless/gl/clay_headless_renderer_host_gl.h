// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_HEADLESS_GL_CLAY_HEADLESS_RENDERER_HOST_GL_H_
#define CLAY_SHELL_PLATFORM_HEADLESS_GL_CLAY_HEADLESS_RENDERER_HOST_GL_H_

#include <memory>

#include "base/include/fml/thread.h"
#ifdef ENABLE_SKITY
#include "clay/shell/gpu/gpu_surface_gl_skity.h"
#else
#include "clay/shell/gpu/gpu_surface_gl_skia.h"
#endif
#include "clay/shell/platform/headless/gl/clay_headless_renderer_gl.h"

namespace clay {
class SharedImageSink;
}

namespace clay {

class ClayHeadlessRendererHostGL final : public ClayHeadlessRendererGL {
 public:
  ClayHeadlessRendererHostGL(ClayHeadlessEngine* engine,
                             const ClayOpenGLRendererConfig& renderer_config);

  GPUSurfaceGLDelegate::GLProcResolver GetGLProcResolver() const override;

  bool MakeCurrent() override;
  bool ClearCurrent() override;
  bool Present() override;
  int64_t FBO(const ClayFrameInfo& frame_info) override;

  void* ResolveProc(const char* name);

  void CleanupGPUResources() override;

 private:
  ClayOpenGLRendererConfig config_;
};

// In this mode, we create a "fake" render thread
// which create skia environment from host gl.
// The render thread takes the front buffer of shared_image_sink_,
// calling `ReadbackToMemory` to get the pixel data,
// and draw the pixel data to SkSurface in host gl.
class ClayHeadlessRendererSharedImageHostGL final
    : public ClayHeadlessRenderer,
      public GPUSurfaceGLDelegate {
 public:
  ClayHeadlessRendererSharedImageHostGL(
      ClayHeadlessEngine* engine,
      const ClayOpenGLRendererConfig& renderer_config,
      ClaySharedImageSinkBufferMode buffer_mode);

  ~ClayHeadlessRendererSharedImageHostGL() override;

  void CleanupGPUResources() override;

  // |ClayHeadlessRenderer|
  EmbedderSurfaceSoftwareDelegate* GetSoftwareRendererDelegate() override;
#ifdef SHELL_ENABLE_GL
  // |ClayHeadlessRenderer|
  GPUSurfaceGLDelegate* GetGLRendererDelegate() override;
#endif
#ifdef SHELL_ENABLE_METAL
  // |ClayHeadlessRenderer|
  EmbedderSurfaceMetalDelegate* GetMetalRendererDelegate() override;
#endif

  // |ClayHeadlessRenderer|
  ClayHeadlessRenderer* GetEngineRenderer() override;

  // |GPUSurfaceGLDelegate|
  std::unique_ptr<GLContextResult> GLContextMakeCurrent() override;

  // |GPUSurfaceGLDelegate|
  bool GLContextClearCurrent() override;

  // |GPUSurfaceGLDelegate|
  bool GLContextPresent(const GLPresentInfo& present_info) override;

  // |GPUSurfaceGLDelegate|
  bool GLContextFBOResetAfterPresent() const override;

  // |GPUSurfaceGLDelegate|
  GLFBOInfo GLContextFBO(GLFrameInfo frame_info) const override;

  // |GPUSurfaceGLDelegate|
  GLProcResolver GetGLProcResolver() const override;

 private:
  void Draw();

  fml::Thread host_gl_thread_;
#ifdef ENABLE_SKITY
  std::unique_ptr<GPUSurfaceGLSkity> host_gl_surface_;
#else
  std::unique_ptr<GPUSurfaceGLSkia> host_gl_surface_;
#endif
  fml::RefPtr<clay::SharedImageSink> shared_image_sink_;
  std::mutex shared_image_sink_mutex_;
  std::unique_ptr<ClayHeadlessRenderer> renderer_;
  ClayOpenGLRendererConfig config_;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_HEADLESS_GL_CLAY_HEADLESS_RENDERER_HOST_GL_H_
