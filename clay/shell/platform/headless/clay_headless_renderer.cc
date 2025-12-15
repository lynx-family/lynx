// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/headless/clay_headless_renderer.h"

#include "clay/fml/logging.h"
#include "clay/shell/platform/headless/clay_headless_engine.h"

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
      : ClayHeadlessRenderer(engine), config_(config) {}

  EmbedderSurfaceSoftwareDelegate* GetSoftwareRendererDelegate() override {
    return this;
  }

  bool OnPresentBackingStore(const void* allocation, size_t row_bytes,
                             size_t height) override {
    return config_.present_callback(engine_->UserData(), allocation, row_bytes,
                                    height);
  }

  void CleanupGPUResources() override {}

 private:
  ClaySoftwareRendererConfig config_;
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
    return std::make_unique<ClayHeadlessRendererSoftware>(engine,
                                                          config.software);
  }

  FML_LOG(ERROR) << "Unknown renderer type: " << config.type;

  return nullptr;
}

}  // namespace clay
