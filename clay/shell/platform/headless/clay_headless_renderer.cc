// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/headless/clay_headless_renderer.h"

#include "clay/fml/logging.h"
#include "clay/shell/platform/embedder/embedder_struct_macros.h"
#include "clay/shell/platform/headless/clay_headless_engine.h"
#ifndef ENABLE_SKITY
#include "third_party/skia/include/core/SkColorType.h"
#endif
#if defined(ENABLE_SKITY) && defined(ENABLE_SOFTWARE_RENDERING) && \
    (defined(OS_MACOSX) || defined(OS_WIN) || defined(OS_LINUX))
#include "clay/shell/platform/headless/gl/clay_headless_renderer_swiftshader.h"
#endif

namespace clay {

ClayHeadlessRenderer::ClayHeadlessRenderer(ClayHeadlessEngine* engine)
    : engine_(engine) {}

ClayHeadlessRenderer::~ClayHeadlessRenderer() = default;

class ClayHeadlessRendererSoftware final
    : public ClayHeadlessRenderer,
      public EmbedderSurfaceSoftwareDelegate {
 public:
  explicit ClayHeadlessRendererSoftware(
      ClayHeadlessEngine* engine, const ClaySoftwareRendererConfig& config)
      : ClayHeadlessRenderer(engine) {
    const auto* config_ptr = &config;
    present_callback_ = SAFE_ACCESS(config_ptr, present_callback, nullptr);
    present_callback_v2_ =
        SAFE_ACCESS(config_ptr, present_callback_v2, nullptr);
  }

  EmbedderSurfaceSoftwareDelegate* GetSoftwareRendererDelegate() override {
    return this;
  }

  bool OnPresentBackingStore(const void* allocation, size_t row_bytes,
                             size_t height) override {
#ifndef ENABLE_SKITY
    // EmbedderSurfaceSoftware allocates a premultiplied N32 raster surface.
    if (present_callback_v2_) {
      constexpr auto format =
          kN32_SkColorType == kRGBA_8888_SkColorType
              ? kClaySharedImageBackingPixelFormatRGBA8
              : kClaySharedImageBackingPixelFormatNative8888;
      return present_callback_v2_(engine_->UserData(), allocation, row_bytes,
                                  height, format);
    }
#endif
    return present_callback_ &&
           present_callback_(engine_->UserData(), allocation, row_bytes,
                             height);
  }

  void CleanupGPUResources() override {}

 private:
  ClaySoftwarePresentCallback present_callback_;
  ClaySoftwarePresentCallbackV2 present_callback_v2_;
};

std::unique_ptr<ClayHeadlessRenderer> ClayHeadlessRenderer::Create(
    ClayHeadlessEngine* engine, const ClayHeadlessRendererConfig& config) {
#ifdef SHELL_ENABLE_GL
  if (config.type == kClayRendererTypeHostGL) {
    return CreateHostGL(engine, config.opengl);
  }

  if (config.type == kClayRendererTypeOpenGL) {
    return CreateGL(engine, config.hardware);
  }
#endif
#ifdef SHELL_ENABLE_METAL
  if (config.type == kClayRendererTypeMetal) {
    return CreateMetal(engine, config.hardware);
  }
#endif
  if (config.type == kClayRendererTypeSoftware) {
#if defined(ENABLE_SKITY) && defined(ENABLE_SOFTWARE_RENDERING) && \
    (defined(OS_MACOSX) || defined(OS_WIN) || defined(OS_LINUX))
    return CreateSwiftShaderSoftwareRenderer(engine, config.software,
                                             engine->UserData());
#else
    return std::make_unique<ClayHeadlessRendererSoftware>(engine,
                                                          config.software);
#endif
  }

  FML_LOG(ERROR) << "Unknown renderer type: " << config.type;

  return nullptr;
}

}  // namespace clay
