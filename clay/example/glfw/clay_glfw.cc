// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <GLFW/glfw3.h>  // NOLINT

#include <chrono>
#include <deque>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>

// A hack to forbid GL includes from gl_api_common.h
// We don't want to include platform GL api.
// We use API from GLFW instead
#define CLAY_COMMON_GRAPHICS_GL_GL_API_COMMON_H_
#include "clay/common/graphics/gl/gl_api.h"

namespace clay {

class GlApiCommon : public GlApi {
 public:
  static GlApiCommon* GetInstance() { return nullptr; }

  void GlGetIntegerv(uint32_t pname, int32_t* data) override {}
  void GlBindFramebuffer(uint32_t target, uint32_t framebuffer) override {}
  void GlBindTexture(uint32_t target, uint32_t texture) override {}
};

}  // namespace clay

#include "build/build_config.h"
#include "clay/common/graphics/gl/scoped_framebuffer_binder.h"
#include "clay/common/graphics/gl/scoped_texture_binder.h"
#include "clay/example/glfw/event_loop.h"
#include "clay/example/glfw/shell_impl.h"
#include "clay/example/glfw/surface_gl_impl.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/public/clay.h"
#include "clay/shell/gpu/gpu_surface_gl_skia.h"
#include "third_party/skia/include/gpu/ganesh/SkImageGanesh.h"
#include "third_party/skia/include/gpu/gl/GrGLInterface.h"
#include "third_party/skia/src/gpu/gl/GrGLDefines.h"  // nogncheck

class GLDelegateGLFWWindow : public clay::GPUSurfaceGLDelegate {
 public:
  explicit GLDelegateGLFWWindow(GLFWwindow* window) : window_(window) {}

  std::unique_ptr<clay::GLContextResult> GLContextMakeCurrent() override {
    glfwMakeContextCurrent(window_);
    return std::make_unique<clay::GLContextDefaultResult>(true);
  }

  bool GLContextClearCurrent() override {
    glfwMakeContextCurrent(nullptr);
    return true;
  }

  bool GLContextPresent(const clay::GLPresentInfo& present_info) override {
    glfwSwapBuffers(window_);
    return true;
  }

  clay::GLFBOInfo GLContextFBO(clay::GLFrameInfo frame_info) const override {
    return clay::GLFBOInfo{.fbo_id = 0, .existing_damage = {}};
  }

  GLProcResolver GetGLProcResolver() const override {
    return [](const char* name) -> void* {
      return reinterpret_cast<void*>(glfwGetProcAddress(name));
    };
  }

 private:
  GLFWwindow* window_;
};

// This value is calculated after the window is created.
static double g_pixelRatio = 1.0;
static const size_t kInitialWindowWidth = 1000;
static const size_t kInitialWindowHeight = 800;

void GLFWcursorPositionCallbackAtPhase(GLFWwindow* window,
                                       ClayPointerPhase phase, double x,
                                       double y) {
  ClayPointerEvent event = {};
  event.struct_size = sizeof(event);
  event.phase = phase;
  event.x = x * g_pixelRatio;
  event.y = y * g_pixelRatio;
  event.timestamp =
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count();
  reinterpret_cast<clay::example::ShellImpl*>(glfwGetWindowUserPointer(window))
      ->SendPointerEvents(&event, 1);
}

void GLFWcursorPositionCallback(GLFWwindow* window, double x, double y) {
  GLFWcursorPositionCallbackAtPhase(window, kClayPointerPhaseMove, x, y);
}

void GLFWmouseButtonCallback(GLFWwindow* window, int key, int action,
                             int mods) {
  if (key == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS) {
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    GLFWcursorPositionCallbackAtPhase(window, kClayPointerPhaseDown, x, y);
    glfwSetCursorPosCallback(window, GLFWcursorPositionCallback);
  }

  if (key == GLFW_MOUSE_BUTTON_1 && action == GLFW_RELEASE) {
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    GLFWcursorPositionCallbackAtPhase(window, kClayPointerPhaseUp, x, y);
    glfwSetCursorPosCallback(window, nullptr);
  }
}

static void GLFWKeyCallback(GLFWwindow* window, int key, int scancode,
                            int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  }
}

void GLFWwindowSizeCallback(GLFWwindow* window, int width, int height) {
  reinterpret_cast<clay::example::ShellImpl*>(glfwGetWindowUserPointer(window))
      ->SendViewportMetrics(width * g_pixelRatio, height * g_pixelRatio,
                            g_pixelRatio);
}

class GLRenderer final : public clay::example::SurfaceDelegate,
                         public clay::GlApi {
 public:
  GLRenderer(GLFWwindow* window, clay::GPUSurfaceGLSkia& window_surface)
      : window_(window), window_surface_(window_surface) {
    // invisible window
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    offscreen_context_ = glfwCreateWindow(640, 480, "", nullptr, window);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
  }

  ~GLRenderer() {
    front_image_.reset();
    for (auto& texture : textures_) {
      window_surface_.GetContext()->deleteBackendTexture(texture);
    }
    glfwDestroyWindow(offscreen_context_);
  }

  // Main thread
  void DrawToWindow() {
    int width, height;
    glfwGetFramebufferSize(window_, &width, &height);

    std::unique_ptr<clay::SurfaceFrame> frame =
        window_surface_.AcquireFrame({width, height});
    SkCanvas* canvas = frame->GetCanvas();
    canvas->clear(SkColor4f{.9f, .3f, .3f, 1.0f});

    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!textures_.empty()) {
        front_image_ = SkImages::AdoptTextureFrom(
            window_surface_.GetContext(), textures_.front(),
            kBottomLeft_GrSurfaceOrigin, kRGBA_8888_SkColorType);
        textures_.pop_front();
      }
    }

    if (!front_image_) {
      FML_LOG(ERROR) << "Clay: No front_image_!";
      return;
    }

    canvas->drawImageRect(front_image_, SkRect::MakeIWH(width, height),
                          SkSamplingOptions());

    frame->Submit();

    static GrGLFunction<GrGLFinishFn> glFinishFn =
        GetFunction<GrGLFinishFn>("glFinish");
    glFinishFn();
  }

  void GlGetIntegerv(uint32_t pname, int32_t* data) override {
    static GrGLFunction<GrGLGetIntegervFn> glGetIntegerv =
        GetFunction<GrGLGetIntegervFn>("glGetIntegerv");
    glGetIntegerv(pname, data);
  }

  void GlBindFramebuffer(uint32_t target, uint32_t framebuffer) override {
    static GrGLFunction<GrGLBindFramebufferFn> glBindFramebuffer =
        GetFunction<GrGLBindFramebufferFn>("glBindFramebuffer");
    glBindFramebuffer(target, framebuffer);  // NOLINT(clay_custom/gl_bind)
  }

  void GlBindTexture(uint32_t target, uint32_t texture) override {
    static GrGLFunction<GrGLBindTextureFn> glBindTexture =
        GetFunction<GrGLBindTextureFn>("glBindTexture");
    glBindTexture(target, texture);  // NOLINT(clay_custom/gl_bind)
  }

 private:
  template <typename Fn>
  std::add_pointer_t<Fn> GetFunction(const char* name) {
    return reinterpret_cast<std::add_pointer_t<Fn>>(
        reinterpret_cast<void*>(glfwGetProcAddress(name)));
  }

  // raster thread
  // SurfaceDelegate
  bool MakeCurrent() override {
    glfwMakeContextCurrent(offscreen_context_);
    return true;
  }

  bool ClearCurrent() override {
    ClearFBO();
    glfwMakeContextCurrent(nullptr);
    return true;
  }

  bool Present() override {
    glfwSwapBuffers(offscreen_context_);
    static GrGLFunction<GrGLFinishFn> glFinishFn =
        GetFunction<GrGLFinishFn>("glFinish");
    glFinishFn();
    {
      std::lock_guard<std::mutex> lock(mutex_);
      textures_.push_back(current_write_texture_);
      current_write_texture_ = {};
    }
    return true;
  }

  void* GetGLProcResolver(const char* what) const override {
    return reinterpret_cast<void*>(glfwGetProcAddress(what));
  }

  uint32_t FBO(const ClayFrameInfo* frame_info) override {
    static GrGLFunction<GrGLGenFramebuffersFn> glGenFramebuffersFn =
        GetFunction<GrGLGenFramebuffersFn>("glGenFramebuffers");
    static GrGLFunction<GrGLGenTexturesFn> glGenTexturesFn =
        GetFunction<GrGLGenTexturesFn>("glGenTextures");
    static GrGLFunction<GrGLDeleteTexturesFn> glDeleteTexturesFn =
        GetFunction<GrGLDeleteTexturesFn>("glDeleteTextures");
    static GrGLFunction<GrGLTexParameteriFn> glTexParameteriFn =
        GetFunction<GrGLTexParameteriFn>("glTexParameteri");
    static GrGLFunction<GrGLTexImage2DFn> glTexImage2DFn =
        GetFunction<GrGLTexImage2DFn>("glTexImage2D");

    static GrGLFunction<GrGLFramebufferTexture2DFn> glFramebufferTexture2DFn =
        GetFunction<GrGLFramebufferTexture2DFn>("glFramebufferTexture2D");
    static GrGLFunction<GrGLCheckFramebufferStatusFn>
        glCheckFramebufferStatusFn = GetFunction<GrGLCheckFramebufferStatusFn>(
            "glCheckFramebufferStatus");

    if (fbo_ == 0) {
      glGenFramebuffersFn(1, &fbo_);
    }

    uint32_t texture;
    glGenTexturesFn(1, &texture);
    clay::ScopedTextureBinder scoped_texture_binder(GR_GL_TEXTURE_2D, texture,
                                                    this);
    glTexParameteriFn(GR_GL_TEXTURE_2D, GR_GL_TEXTURE_MIN_FILTER, GR_GL_LINEAR);
    glTexParameteriFn(GR_GL_TEXTURE_2D, GR_GL_TEXTURE_MAG_FILTER, GR_GL_LINEAR);
    glTexParameteriFn(GR_GL_TEXTURE_2D, GR_GL_TEXTURE_WRAP_S,
                      GR_GL_CLAMP_TO_EDGE);
    glTexParameteriFn(GR_GL_TEXTURE_2D, GR_GL_TEXTURE_WRAP_T,
                      GR_GL_CLAMP_TO_EDGE);
    glTexImage2DFn(GR_GL_TEXTURE_2D, 0, GR_GL_RGBA8, frame_info->width,
                   frame_info->height, 0, GR_GL_RGBA, GR_GL_UNSIGNED_BYTE,
                   nullptr);

    clay::ScopedFramebufferBinder scoped_fbo_binder(GR_GL_FRAMEBUFFER, fbo_,
                                                    this);
    glFramebufferTexture2DFn(GR_GL_FRAMEBUFFER, GR_GL_COLOR_ATTACHMENT0,
                             GR_GL_TEXTURE_2D, texture, 0);

    if (glCheckFramebufferStatusFn(GR_GL_FRAMEBUFFER) !=
        GR_GL_FRAMEBUFFER_COMPLETE) {
      FML_LOG(ERROR) << "Clay: GL_FRAMEBUFFER NOT COMPLETE";
      glDeleteTexturesFn(1, &texture);
      ClearFBO();
      return 0;
    }

    GrGLTextureInfo gl_texture_info{
        .fTarget = GR_GL_TEXTURE_2D,
        .fID = texture,
        .fFormat = GR_GL_RGBA8,
    };
    current_write_texture_ =
        GrBackendTexture(frame_info->width, frame_info->height,
                         GrMipmapped::kNo, gl_texture_info);

    return fbo_;
  }

  void ClearFBO() {
    if (fbo_ != 0) {
      static GrGLFunction<GrGLDeleteFramebuffersFn> glDeleteFrameBuffersFn =
          GetFunction<GrGLDeleteFramebuffersFn>("glDeleteFramebuffers");
      glDeleteFrameBuffersFn(1, &fbo_);
      fbo_ = 0;
    }
  }

  GLFWwindow* window_;
  GLFWwindow* offscreen_context_;
  clay::GPUSurfaceGLSkia& window_surface_;
  sk_sp<SkImage> front_image_;
  std::deque<GrBackendTexture> textures_;
  GrBackendTexture current_write_texture_;
  std::mutex mutex_;
  uint32_t fbo_ = 0;
};

void GLFW_ErrorCallback(int error, const char* description) {
  std::cerr << "GLFW Error: (" << error << ") " << description << std::endl;
}

int main(int argc, const char* argv[]) {
  std::string icudtl_path = "./icudtl.dat";

  glfwSetErrorCallback(GLFW_ErrorCallback);

  /* Initialize the library */
  if (!glfwInit()) {
    return -1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GR_GL_TRUE);

  /* Create a windowed mode window and its OpenGL context */
  glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
  GLFWwindow* window =
      glfwCreateWindow(kInitialWindowWidth, kInitialWindowHeight, "Clay Engine",
                       nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return -1;
  }

  /* Make the window's context current */
  glfwMakeContextCurrent(window);

  glfwSwapInterval(1);

  int framebuffer_width, framebuffer_height;
  glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
  g_pixelRatio = framebuffer_width / kInitialWindowWidth;

  {
    GLDelegateGLFWWindow delegate(window);
    clay::GPUSurfaceGLSkia window_surface(&delegate, true);
    std::unique_ptr<GLRenderer> renderer =
        std::make_unique<GLRenderer>(window, window_surface);
    if (!renderer) {
      return -1;
    }

    clay::example::ShellImpl* engine = nullptr;

    clay::example::EventLoop event_loop(std::this_thread::get_id(),
                                        [&](const ClayTask* task) {
                                          if (engine) {
                                            engine->RunTask(task);
                                          }
                                        });

    ClayTaskRunnerDescription platform_task_runner_desc{};
    platform_task_runner_desc.struct_size = sizeof(ClayTaskRunnerDescription);
    platform_task_runner_desc.user_data = &event_loop;
    platform_task_runner_desc.runs_task_on_current_thread_callback =
        [](void* user_data) -> bool {
      return reinterpret_cast<clay::example::EventLoop*>(user_data)
          ->RunsTasksOnCurrentThread();
    };
    platform_task_runner_desc.post_task_callback =
        [](ClayTask task, uint64_t target_time, void* user_data) {
          reinterpret_cast<clay::example::EventLoop*>(user_data)->PostTask(
              task, target_time);
        };
    platform_task_runner_desc.identifier =
        reinterpret_cast<intptr_t>(&event_loop);

    engine = new clay::example::ShellImpl(icudtl_path.c_str(),  // icu data
                                          renderer.get(),  // surface delegate
                                          &platform_task_runner_desc);
    engine->DrawUI();

    glfwSetWindowUserPointer(window, engine);
    GLFWwindowSizeCallback(window, kInitialWindowWidth, kInitialWindowHeight);

    glfwSetKeyCallback(window, GLFWKeyCallback);
    glfwSetWindowSizeCallback(window, GLFWwindowSizeCallback);
    glfwSetMouseButtonCallback(window, GLFWmouseButtonCallback);

    while (!glfwWindowShouldClose(window)) {
      event_loop.WaitForEvents(std::chrono::milliseconds(32));
      renderer->DrawToWindow();
    }

    delete engine;
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return EXIT_SUCCESS;
}
