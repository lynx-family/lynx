// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_HEADLESS_GL_CLAY_HEADLESS_RENDERER_HOST_GL_H_
#define CLAY_SHELL_PLATFORM_HEADLESS_GL_CLAY_HEADLESS_RENDERER_HOST_GL_H_

#include <atomic>
#include <memory>

#include "base/include/fml/thread.h"
#include "clay/shell/platform/headless/gl/clay_headless_renderer_gl.h"

namespace clay {
class SharedImageSink;
class HostGLRenderer;
}  // namespace clay

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
// which blits shared image CPU readback with host GL.
// The render thread takes the front buffer of shared_image_sink_,
// calling `ReadbackToMemory` to get the pixel data,
// and draws the pixel data to the host framebuffer with raw GL.
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
  // Synchronously destroys |host_gl_renderer_| on |host_gl_thread_|.
  //
  // Thread/Blocking semantics:
  // - May block the calling thread until destruction completes.
  // - Must be called before |host_gl_thread_| starts shutting down.
  // - If already on |host_gl_thread_|, destruction runs inline to avoid
  //   self-wait deadlock.
  // - Otherwise this posts to |host_gl_thread_| and waits for completion.
  void DestroyHostGLRendererSync();
  void DestroyHostGLRendererOnHostThread();

  fml::Thread host_gl_thread_;
  std::shared_ptr<std::atomic_bool> draw_tasks_enabled_;
  std::unique_ptr<HostGLRenderer> host_gl_renderer_;
  fml::RefPtr<clay::SharedImageSink> shared_image_sink_;
  std::mutex shared_image_sink_mutex_;
  std::unique_ptr<ClayHeadlessRenderer> renderer_;
  ClayOpenGLRendererConfig config_;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_HEADLESS_GL_CLAY_HEADLESS_RENDERER_HOST_GL_H_
