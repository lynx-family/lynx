// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/frame_child_runtime_embedder.h"

#include <memory>
#include <utility>

#include "clay/lynx_adaptor/frame_child_runtime.h"
#include "clay/lynx_adaptor/ui_delegate_clay.h"
#include "core/public/pipeline_option.h"
#include "core/renderer/data/template_data.h"
#include "core/template_bundle/lynx_template_bundle.h"

namespace lynx {
namespace tasm {
namespace {

class EmbedderFrameChildRuntime final : public FrameChildRuntime {
 public:
  EmbedderFrameChildRuntime(
      const embedder::LynxTemplateRenderer::Settings& settings,
      UIDelegateClay* ui_delegate)
      : renderer_(std::make_unique<embedder::LynxTemplateRenderer>(
            settings, ui_delegate, nullptr, nullptr)) {
    renderer_->SetFontScale(settings.font_scale);
  }

  bool LoadBundle(const std::string& url, const LynxTemplateBundle& bundle,
                  const std::shared_ptr<TemplateData>& data,
                  const std::shared_ptr<TemplateData>& global_props) override {
    if (global_props) {
      renderer_->UpdateGlobalProps(global_props->GetValue());
    }
    renderer_->LoadTemplateBundle(url, bundle,
                                  std::make_shared<PipelineOptions>(), data);
    return true;
  }

  bool UpdateMetaData(
      const std::shared_ptr<TemplateData>& data,
      const std::shared_ptr<TemplateData>& global_props) override {
    if (data) {
      renderer_->UpdateMetaData(
          data, global_props ? global_props->GetValue() : lepus::Value());
      return true;
    }
    if (global_props) {
      renderer_->UpdateGlobalProps(global_props->GetValue());
      return true;
    }
    return false;
  }

  void UpdateViewport(float width, int width_mode, float height,
                      int height_mode, bool need_layout) override {
    renderer_->UpdateViewport(width, width_mode, height, height_mode,
                              need_layout);
  }

 private:
  std::unique_ptr<embedder::LynxTemplateRenderer> renderer_;
};

class EmbedderFrameChildRuntimeFactory final : public FrameChildRuntimeFactory {
 public:
  explicit EmbedderFrameChildRuntimeFactory(
      embedder::LynxTemplateRenderer::Settings settings)
      : settings_(std::move(settings)) {}

  std::unique_ptr<FrameChildRuntime> CreateRuntime(
      UIDelegateClay* ui_delegate,
      const FrameChildRuntimeOptions& options) override {
    auto settings = settings_;
    settings.resource_loader = options.resource_loader;
    settings.global_props = nullptr;
    settings.device_pixel_ratio = options.device_pixel_ratio;
    settings.viewport_size.cx = options.viewport_width;
    settings.viewport_size.cy = options.viewport_height;
    settings.enable_pre_update_data = true;
    settings.embedded_mode = options.embedded_mode;
    if (options.enable_multi_async_thread.has_value()) {
      settings.enable_multi_async_thread =
          options.enable_multi_async_thread.value();
    }
    return std::make_unique<EmbedderFrameChildRuntime>(settings, ui_delegate);
  }

 private:
  embedder::LynxTemplateRenderer::Settings settings_;
};

}  // namespace

std::shared_ptr<FrameChildRuntimeFactory>
CreateFrameChildRuntimeFactoryEmbedder(
    embedder::LynxTemplateRenderer::Settings settings) {
  return std::make_shared<EmbedderFrameChildRuntimeFactory>(
      std::move(settings));
}

}  // namespace tasm
}  // namespace lynx
