// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/headless/gl/clay_headless_renderer_swiftshader.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglext_angle.h>

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "angle_gl.h"
#include "clay/fml/logging.h"
#include "clay/shell/platform/embedder/embedder_struct_macros.h"

namespace clay {

namespace {

void LogEglError(const std::string& message) {
  FML_LOG(ERROR) << "SwiftShader EGL: " << message
                 << ", eglGetError: " << eglGetError();
}

}  // namespace

class ClayHeadlessSwiftShaderManager {
 public:
  ClayHeadlessSwiftShaderManager() = default;

  ~ClayHeadlessSwiftShaderManager() { CleanUp(); }

  bool MakeCurrent() {
    if (!EnsureInitialized()) {
      return false;
    }
    if (eglMakeCurrent(display_, surface_, surface_, context_) != EGL_TRUE) {
      LogEglError("Failed to make the context current");
      return false;
    }
    if (!renderer_logged_) {
      const auto* renderer = glGetString(GL_RENDERER);
      FML_LOG(INFO) << "Skity software rendering GL renderer: "
                    << (renderer ? reinterpret_cast<const char*>(renderer)
                                 : "unknown");
      renderer_logged_ = true;
    }
    return true;
  }

  bool ClearCurrent() {
    if (!initialized_) {
      return true;
    }
    return eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE,
                          EGL_NO_CONTEXT) == EGL_TRUE;
  }

 private:
  bool EnsureInitialized() {
    if (initialization_attempted_) {
      return initialized_;
    }
    initialization_attempted_ = true;
    initialized_ = Initialize();
    if (!initialized_) {
      CleanUp();
    }
    return initialized_;
  }

  bool Initialize() {
    const EGLAttrib display_attributes[] = {
        EGL_PLATFORM_ANGLE_TYPE_ANGLE,
        EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_SWIFTSHADER_ANGLE,
        EGL_NONE,
    };
    // Isolate Clay from clients using EGL_DEFAULT_DISPLAY.
    display_ = eglGetPlatformDisplay(
        EGL_PLATFORM_ANGLE_ANGLE,
        reinterpret_cast<void*>(EGL_SOFTWARE_DISPLAY_ANGLE),
        display_attributes);
    if (display_ == EGL_NO_DISPLAY) {
      LogEglError("Failed to get the ANGLE SwiftShader display");
      return false;
    }
    EGLint major = 0;
    EGLint minor = 0;
    if (eglInitialize(display_, &major, &minor) != EGL_TRUE) {
      LogEglError("Failed to initialize the ANGLE SwiftShader display");
      return false;
    }

    const EGLint config_attributes[] = {
        EGL_RENDERABLE_TYPE,
        EGL_OPENGL_ES3_BIT_KHR,
        EGL_SURFACE_TYPE,
        EGL_PBUFFER_BIT,
        EGL_RED_SIZE,
        8,
        EGL_GREEN_SIZE,
        8,
        EGL_BLUE_SIZE,
        8,
        EGL_ALPHA_SIZE,
        8,
        EGL_DEPTH_SIZE,
        0,
        EGL_STENCIL_SIZE,
        8,
        EGL_NONE,
    };
    EGLint config_count = 0;
    if (eglChooseConfig(display_, config_attributes, &config_, 1,
                        &config_count) != EGL_TRUE ||
        config_count == 0) {
      LogEglError("Failed to choose an EGL config");
      return false;
    }

    const EGLint context_attributes[] = {EGL_CONTEXT_CLIENT_VERSION, 3,
                                         EGL_NONE};
    context_ =
        eglCreateContext(display_, config_, EGL_NO_CONTEXT, context_attributes);
    if (context_ == EGL_NO_CONTEXT) {
      LogEglError("Failed to create an EGL context");
      return false;
    }

    const EGLint surface_attributes[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
    surface_ = eglCreatePbufferSurface(display_, config_, surface_attributes);
    if (surface_ == EGL_NO_SURFACE) {
      LogEglError("Failed to create an EGL pbuffer surface");
      return false;
    }
    return true;
  }

  void CleanUp() {
    if (display_ == EGL_NO_DISPLAY) {
      return;
    }
    if (eglGetCurrentContext() == context_) {
      eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }
    if (surface_ != EGL_NO_SURFACE) {
      eglDestroySurface(display_, surface_);
      surface_ = EGL_NO_SURFACE;
    }
    if (context_ != EGL_NO_CONTEXT) {
      eglDestroyContext(display_, context_);
      context_ = EGL_NO_CONTEXT;
    }
    // ANGLE caches this display across renderers. Keep it alive for the process
    // lifetime instead of calling eglTerminate, which would invalidate other
    // renderers' contexts and surfaces.
    display_ = EGL_NO_DISPLAY;
  }

  bool initialization_attempted_ = false;
  bool initialized_ = false;
  bool renderer_logged_ = false;
  EGLDisplay display_ = EGL_NO_DISPLAY;
  EGLConfig config_ = nullptr;
  EGLContext context_ = EGL_NO_CONTEXT;
  EGLSurface surface_ = EGL_NO_SURFACE;
};

ClayHeadlessRendererSwiftShader::ClayHeadlessRendererSwiftShader(
    ClayHeadlessEngine* engine, const ClayHardwareRendererConfig& config)
    : ClayHeadlessRendererSharedImageGL(engine, config),
      surface_manager_(std::make_unique<ClayHeadlessSwiftShaderManager>()) {}

ClayHeadlessRendererSwiftShader::~ClayHeadlessRendererSwiftShader() = default;

GPUSurfaceGLDelegate::GLProcResolver
ClayHeadlessRendererSwiftShader::GetGLProcResolver() const {
  return [](const char* name) -> void* {
    return reinterpret_cast<void*>(eglGetProcAddress(name));
  };
}

bool ClayHeadlessRendererSwiftShader::MakeCurrent() {
  return surface_manager_->MakeCurrent();
}

bool ClayHeadlessRendererSwiftShader::ClearCurrent() {
  return surface_manager_->ClearCurrent();
}

#if defined(OS_LINUX)
std::unique_ptr<ClayHeadlessRenderer> ClayHeadlessRenderer::CreateGL(
    ClayHeadlessEngine* engine, const ClayHardwareRendererConfig& config) {
  return std::make_unique<ClayHeadlessRendererSwiftShader>(engine, config);
}

#endif  // defined(OS_LINUX)

namespace {

class SwiftShaderSoftwareRenderer final : public ClayHeadlessRendererGL {
 public:
  SwiftShaderSoftwareRenderer(ClayHeadlessEngine* engine,
                              const ClaySoftwareRendererConfig& config,
                              void* user_data)
      : ClayHeadlessRendererGL(engine), user_data_(user_data) {
    const auto* config_ptr = &config;
    present_callback_ = SAFE_ACCESS(config_ptr, present_callback, nullptr);
    present_callback_v2_ =
        SAFE_ACCESS(config_ptr, present_callback_v2, nullptr);
  }

  GLProcResolver GetGLProcResolver() const override {
    return [](const char* name) -> void* {
      return reinterpret_cast<void*>(eglGetProcAddress(name));
    };
  }

  bool GLContextFBOResetAfterPresent() const override { return false; }

  void CleanupGPUResources() override {
    if (framebuffer_ && MakeCurrent()) {
      DeleteBackingStore();
    }
  }

 protected:
  bool MakeCurrent() override { return manager_.MakeCurrent(); }
  bool ClearCurrent() override { return manager_.ClearCurrent(); }

  int64_t FBO(const ClayFrameInfo& frame_info) override {
    if (!frame_info.width || !frame_info.height) {
      return -1;
    }
    if (framebuffer_ && width_ == frame_info.width &&
        height_ == frame_info.height) {
      return framebuffer_;
    }
    GLint max_size = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
    if (frame_info.width > static_cast<uint32_t>(max_size) ||
        frame_info.height > static_cast<uint32_t>(max_size) ||
        frame_info.width >
            std::numeric_limits<size_t>::max() / 4 / frame_info.height) {
      return -1;
    }

    DeleteBackingStore();
    width_ = frame_info.width;
    height_ = frame_info.height;
    pixels_.resize(static_cast<size_t>(width_) * height_ * 4);
    GLint old_texture = 0;
    GLint old_framebuffer = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &old_texture);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_framebuffer);
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
    glGenFramebuffers(1, &framebuffer_);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           texture_, 0);
    const auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindTexture(GL_TEXTURE_2D, old_texture);
    glBindFramebuffer(GL_FRAMEBUFFER, old_framebuffer);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
      FML_LOG(ERROR) << "Could not create SwiftShader software framebuffer: "
                     << status;
      DeleteBackingStore();
      return -1;
    }
    return framebuffer_;
  }

  bool Present() override {
    if (!framebuffer_) {
      return false;
    }
    GLint old_framebuffer = 0;
    const std::array<GLenum, 4> pack_parameters = {
        GL_PACK_ALIGNMENT, GL_PACK_ROW_LENGTH, GL_PACK_SKIP_ROWS,
        GL_PACK_SKIP_PIXELS};
    std::array<GLint, 4> old_pack_state{};
    GLint old_pack_buffer = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &old_framebuffer);
    for (size_t i = 0; i < pack_parameters.size(); ++i) {
      glGetIntegerv(pack_parameters[i], &old_pack_state[i]);
      glPixelStorei(pack_parameters[i], i == 0 ? 1 : 0);
    }
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &old_pack_buffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer_);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    // ReadPixels synchronizes the render commands before invoking the callback.
    glReadPixels(0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE,
                 pixels_.data());
    const auto error = glGetError();
    for (size_t i = 0; i < pack_parameters.size(); ++i) {
      glPixelStorei(pack_parameters[i], old_pack_state[i]);
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, old_pack_buffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, old_framebuffer);
    if (error != GL_NO_ERROR) {
      FML_LOG(ERROR) << "Could not read SwiftShader software pixels: " << error;
      return false;
    }

    // Return top-down pixels without changing RGBA channel order.
    const size_t row_bytes = static_cast<size_t>(width_) * 4;
    for (uint32_t y = 0; y < height_ / 2; ++y) {
      auto* top = pixels_.data() + y * row_bytes;
      auto* bottom = pixels_.data() + (height_ - 1 - y) * row_bytes;
      std::swap_ranges(top, top + row_bytes, bottom);
    }
    constexpr auto format = kClaySharedImageBackingPixelFormatRGBA8;
    if (present_callback_v2_) {
      return present_callback_v2_(user_data_, pixels_.data(), row_bytes,
                                  height_, format);
    }
    return present_callback_(user_data_, pixels_.data(), row_bytes, height_);
  }

 private:
  void DeleteBackingStore() {
    glDeleteFramebuffers(1, &framebuffer_);
    glDeleteTextures(1, &texture_);
    framebuffer_ = 0;
    texture_ = 0;
  }

  ClaySoftwarePresentCallback present_callback_;
  ClaySoftwarePresentCallbackV2 present_callback_v2_;
  void* user_data_;
  ClayHeadlessSwiftShaderManager manager_;
  GLuint texture_ = 0;
  GLuint framebuffer_ = 0;
  uint32_t width_ = 0;
  uint32_t height_ = 0;
  std::vector<uint8_t> pixels_;
};

}  // namespace

std::unique_ptr<ClayHeadlessRenderer> CreateSwiftShaderSoftwareRenderer(
    ClayHeadlessEngine* engine, const ClaySoftwareRendererConfig& config,
    void* user_data) {
  const auto* config_ptr = &config;
  if (!SAFE_EXISTS(config_ptr, present_callback) &&
      !SAFE_EXISTS(config_ptr, present_callback_v2)) {
    FML_LOG(ERROR) << "Software rendering requires a present callback.";
    return nullptr;
  }
  return std::make_unique<SwiftShaderSoftwareRenderer>(engine, config,
                                                       user_data);
}

}  // namespace clay
