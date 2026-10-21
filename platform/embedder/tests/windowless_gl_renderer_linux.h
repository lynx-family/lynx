// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0.
#ifndef PLATFORM_EMBEDDER_TESTS_WINDOWLESS_GL_RENDERER_LINUX_H_
#define PLATFORM_EMBEDDER_TESTS_WINDOWLESS_GL_RENDERER_LINUX_H_
#include <dlfcn.h>
#include <epoxy/egl.h>
#include <epoxy/gl.h>
// The SDK includes ANGLE. Resolve the system EGL explicitly so this fixture
// tests Mesa's surfaceless driver rather than accidentally calling ANGLE.
class NativeEGL {
 public:
  NativeEGL() {
    library = dlopen("libEGL.so.1", RTLD_NOW | RTLD_LOCAL);
    Check(library);
    get_proc = Symbol<PFNEGLGETPROCADDRESSPROC>("eglGetProcAddress");
    auto platform = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
        get_proc("eglGetPlatformDisplayEXT"));
    Check(platform);
    display = platform(EGL_PLATFORM_SURFACELESS_MESA, nullptr, nullptr);
    Check(display != EGL_NO_DISPLAY);
    Check(Symbol<PFNEGLINITIALIZEPROC>("eglInitialize")(display, nullptr,
                                                        nullptr));
    Check(Symbol<PFNEGLBINDAPIPROC>("eglBindAPI")(EGL_OPENGL_API));
    const EGLint attributes[] = {EGL_SURFACE_TYPE,
                                 EGL_PBUFFER_BIT,
                                 EGL_RENDERABLE_TYPE,
                                 EGL_OPENGL_BIT,
                                 EGL_RED_SIZE,
                                 8,
                                 EGL_GREEN_SIZE,
                                 8,
                                 EGL_BLUE_SIZE,
                                 8,
                                 EGL_ALPHA_SIZE,
                                 8,
                                 EGL_NONE};
    EGLint count = 0;
    Check(Symbol<PFNEGLCHOOSECONFIGPROC>("eglChooseConfig")(
              display, attributes, &config, 1, &count) &&
          count == 1);
  }
  ~NativeEGL() {
    Symbol<PFNEGLTERMINATEPROC>("eglTerminate")(display);
    dlclose(library);
  }
  template <class T>
  T Symbol(const char* name) const {
    auto result = reinterpret_cast<T>(dlsym(library, name));
    Check(result != nullptr);
    return result;
  }
  void* library = nullptr;
  EGLDisplay display = EGL_NO_DISPLAY;
  EGLConfig config{};
  PFNEGLGETPROCADDRESSPROC get_proc = nullptr;
};
class GLRenderer final : public Renderer {
 public:
  explicit GLRenderer(std::shared_ptr<NativeEGL> egl)
      : Renderer(kRendererTypeGLDirect), egl_(std::move(egl)) {
    const EGLint context_attributes[] = {
        EGL_CONTEXT_MAJOR_VERSION_KHR,
        3,
        EGL_CONTEXT_MINOR_VERSION_KHR,
        3,
        EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR,
        EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
        EGL_NONE};
    context_ = egl_->Symbol<PFNEGLCREATECONTEXTPROC>("eglCreateContext")(
        egl_->display, egl_->config, EGL_NO_CONTEXT, context_attributes);
    Check(context_ != EGL_NO_CONTEXT);
    const EGLint surface_attributes[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
    surface_ =
        egl_->Symbol<PFNEGLCREATEPBUFFERSURFACEPROC>("eglCreatePbufferSurface")(
            egl_->display, egl_->config, surface_attributes);
    Check(surface_ != EGL_NO_SURFACE);
  }
  ~GLRenderer() override {
    // The view/raster thread has already stopped before this owner is released.
    Check(OnGLMakeCurrent());
    glDeleteFramebuffers(1, &fbo_);
    glDeleteTextures(1, &texture_);
    glDeleteRenderbuffers(1, &stencil_);
    OnGLClearCurrent();
    egl_->Symbol<PFNEGLDESTROYSURFACEPROC>("eglDestroySurface")(egl_->display,
                                                                surface_);
    egl_->Symbol<PFNEGLDESTROYCONTEXTPROC>("eglDestroyContext")(egl_->display,
                                                                context_);
  }
  bool OnGLMakeCurrent() override {
    return egl_->Symbol<PFNEGLBINDAPIPROC>("eglBindAPI")(EGL_OPENGL_API) &&
           egl_->Symbol<PFNEGLMAKECURRENTPROC>("eglMakeCurrent")(
               egl_->display, surface_, surface_, context_);
  }
  bool OnGLClearCurrent() override {
    return egl_->Symbol<PFNEGLMAKECURRENTPROC>("eglMakeCurrent")(
        egl_->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  }
  void* OnGLProcResolver(const char* name) override {
    return reinterpret_cast<void*>(egl_->get_proc(name));
  }
  uint32_t OnGLCreateFBO(int width, int height) override {
    Check(width > 0 && height > 0);
    GLint tex, renderbuffer, draw, read;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &tex);
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &renderbuffer);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
    if (!fbo_) {
      glGenFramebuffers(1, &fbo_);
      glGenTextures(1, &texture_);
      glGenRenderbuffers(1, &stencil_);
    }
    glBindTexture(GL_TEXTURE_2D, texture_);
    if (width_ != width || height_ != height) {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
                   GL_UNSIGNED_BYTE, nullptr);
      glBindRenderbuffer(GL_RENDERBUFFER, stencil_);
      glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width,
                            height);
      width_ = width;
      height_ = height;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           texture_, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, stencil_);
    Check(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindTexture(GL_TEXTURE_2D, tex);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, read);
    return fbo_;
  }
  bool OnGLPresent() override {
    if (!loaded_pixels) {
      GLint read;
      glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
      glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_);
      std::vector<uint8_t> pixels(width_ * height_ * 4);
      // Explicit snapshot acceptance only; normal presentation never reads
      // back.
      glReadPixels(0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE,
                   pixels.data());
      glBindFramebuffer(GL_READ_FRAMEBUFFER, read);
      Check(glGetError() == GL_NO_ERROR);
      OnSoftwarePresent(pixels.data(), width_ * 4, height_);
    } else {
      ++frames;
      glFlush();
    }
    return true;
  }

 private:
  std::shared_ptr<NativeEGL> egl_;
  EGLContext context_ = EGL_NO_CONTEXT;
  EGLSurface surface_ = EGL_NO_SURFACE;
  GLuint fbo_ = 0, texture_ = 0, stencil_ = 0;
  int width_ = 0, height_ = 0;
};
#endif
