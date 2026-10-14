// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglext_angle.h>
#include <GLES2/gl2.h>

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "clay/shell/gpu/gpu_surface_gl_skity.h"
#include "clay/shell/platform/headless/gl/clay_headless_renderer_swiftshader.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace {
enum class Callback { kLegacy, kOldSize, kV2, kBoth };

class DefaultDisplay {
 public:
  bool Initialize() {
    const EGLAttrib attributes[] = {
        EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_SWIFTSHADER_ANGLE, EGL_NONE};
    display_ =
        eglGetPlatformDisplay(EGL_PLATFORM_ANGLE_ANGLE, nullptr, attributes);
    if (display_ == EGL_NO_DISPLAY ||
        !eglInitialize(display_, nullptr, nullptr)) {
      return false;
    }
    const EGLint config_attributes[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR, EGL_SURFACE_TYPE,
        EGL_PBUFFER_BIT, EGL_NONE};
    EGLConfig config = nullptr;
    EGLint count = 0;
    if (!eglChooseConfig(display_, config_attributes, &config, 1, &count) ||
        !count) {
      return false;
    }
    const EGLint context_attributes[] = {EGL_CONTEXT_CLIENT_VERSION, 3,
                                         EGL_NONE};
    context_ =
        eglCreateContext(display_, config, EGL_NO_CONTEXT, context_attributes);
    return context_ != EGL_NO_CONTEXT;
  }
  bool MakeCurrent() {
    return eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, context_) ==
           EGL_TRUE;
  }
  ~DefaultDisplay() {
    if (display_ == EGL_NO_DISPLAY) return;
    if (eglGetCurrentContext() == context_) {
      eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }
    if (context_ != EGL_NO_CONTEXT) eglDestroyContext(display_, context_);
    eglTerminate(display_);
  }

 private:
  EGLDisplay display_ = EGL_NO_DISPLAY;
  EGLContext context_ = EGL_NO_CONTEXT;
};

class SwiftShaderSoftwareTest : public testing::TestWithParam<Callback> {
 protected:
  std::unique_ptr<ClayHeadlessRenderer> CreateRenderer() {
    ClaySoftwareRendererConfig config{};
    config.struct_size =
        GetParam() == Callback::kOldSize
            ? offsetof(ClaySoftwareRendererConfig, present_callback_v2)
            : sizeof(config);
    config.present_callback = [](void* data, const void* pixels,
                                 size_t row_bytes, size_t height) {
      return static_cast<SwiftShaderSoftwareTest*>(data)->Capture(
          pixels, row_bytes, height, false);
    };
    // Leave v2 populated even for the original struct size to check
    // SAFE_ACCESS.
    if (GetParam() != Callback::kLegacy) {
      config.present_callback_v2 =
          [](void* data, const void* pixels, size_t row_bytes, size_t height,
             ClaySharedImageBackingPixelFormat format) {
            EXPECT_EQ(format, kClaySharedImageBackingPixelFormatRGBA8);
            return static_cast<SwiftShaderSoftwareTest*>(data)->Capture(
                pixels, row_bytes, height, true);
          };
    }
    if (GetParam() == Callback::kV2) config.present_callback = nullptr;
    return CreateSwiftShaderSoftwareRenderer(nullptr, config, this);
  }
  bool Capture(const void* pixels, size_t row_bytes, size_t height, bool v2) {
    EXPECT_EQ(v2, GetParam() == Callback::kV2 || GetParam() == Callback::kBoth);
    const auto* bytes = static_cast<const uint8_t*>(pixels);
    pixels_.assign(bytes, bytes + row_bytes * height);
    row_bytes_ = row_bytes;
    height_ = height;
    ++presents_;
    return accept_;
  }
  std::unique_ptr<GPUSurfaceGLSkity> CreateSurface(
      ClayHeadlessRenderer* renderer) {
    auto* delegate = renderer->GetGLRendererDelegate();
    auto context = GPUSurfaceGLSkity::MakeGLContext(delegate);
    if (!context) return nullptr;
    EXPECT_NE(
        std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER)))
            .find("SwiftShader"),
        std::string::npos);
    return std::make_unique<GPUSurfaceGLSkity>(delegate, std::move(context));
  }
  void SetUp() override {
    renderer_ = CreateRenderer();
    ASSERT_TRUE(renderer_);
    surface_ = CreateSurface(renderer_.get());
    ASSERT_TRUE(surface_);
    ASSERT_TRUE(surface_->IsValid());
  }
  void DestroyRenderer() {
    surface_.reset();
    if (renderer_) renderer_->CleanupGPUResources();
    renderer_.reset();
  }
  void TearDown() override { DestroyRenderer(); }
  std::array<uint8_t, 4> Pixel(size_t x, size_t y) {
    const auto i = y * row_bytes_ + x * 4;
    return {pixels_[i], pixels_[i + 1], pixels_[i + 2], pixels_[i + 3]};
  }
  void Draw(GPUSurfaceGLSkity* surface, int width = 8) {
    auto frame = surface->AcquireFrame({width, 12});
    ASSERT_TRUE(frame);
    auto* canvas = frame->GetCanvas();
    canvas->Clear(skity::Color_TRANSPARENT);
    skity::Paint paint;
    paint.SetColor(0x80FF0000);
    canvas->DrawRect(skity::Rect::MakeXYWH(0, 0, width, 6), paint);
    frame->Prepare(std::nullopt);
    EXPECT_EQ(frame->Submit(), accept_);
    ASSERT_EQ(row_bytes_, static_cast<size_t>(width) * 4);
    ASSERT_EQ(height_, 12u);
    EXPECT_EQ(Pixel(3, 2), (std::array<uint8_t, 4>{128, 0, 0, 128}));
    EXPECT_EQ(Pixel(3, 10), (std::array<uint8_t, 4>{0, 0, 0, 0}));
  }
  std::unique_ptr<ClayHeadlessRenderer> renderer_;
  std::unique_ptr<GPUSurfaceGLSkity> surface_;
  std::vector<uint8_t> pixels_;
  size_t row_bytes_ = 0, height_ = 0;
  int presents_ = 0;
  bool accept_ = true;
};

TEST_P(SwiftShaderSoftwareTest, PixelsResizeAndRejection) {
  for (int width : {16, 31, 8}) {
    accept_ = width != 31;
    Draw(surface_.get(), width);
  }
  EXPECT_EQ(presents_, 3);
}

TEST_P(SwiftShaderSoftwareTest, RendererAndDisplayLifetimes) {
  auto other = CreateRenderer();
  ASSERT_TRUE(other);
  auto surface = CreateSurface(other.get());
  ASSERT_TRUE(surface);
  {
    DefaultDisplay external;
    ASSERT_TRUE(external.Initialize());
    ASSERT_TRUE(external.MakeCurrent());
    DestroyRenderer();
    ASSERT_TRUE(external.MakeCurrent()) << eglGetError();
    EXPECT_NE(glGetString(GL_RENDERER), nullptr);
  }
  Draw(surface.get());
  surface.reset();
  other->CleanupGPUResources();
}

INSTANTIATE_TEST_SUITE_P(Callbacks, SwiftShaderSoftwareTest,
                         testing::Values(Callback::kLegacy, Callback::kOldSize,
                                         Callback::kV2, Callback::kBoth));
}  // namespace
}  // namespace clay
