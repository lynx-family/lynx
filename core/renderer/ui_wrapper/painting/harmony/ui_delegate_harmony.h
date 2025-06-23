// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_UI_DELEGATE_HARMONY_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_UI_DELEGATE_HARMONY_H_

#include <memory>
#include <utility>
#include <vector>

#include "base/include/platform/harmony/napi_util.h"
#include "core/public/ui_delegate.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/module/module_factory_capi.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"

namespace lynx {

namespace shell {
class EmbedderPlatformHarmony;
}  // namespace shell

namespace tasm {
namespace harmony {

class UIDelegateHarmony : public UIDelegate {
 public:
  UIDelegateHarmony(
      UIOwner* ui_owner, ShadowNodeOwner* node_owner,
      const std::shared_ptr<LynxContext>& lynx_context,
      std::unique_ptr<lynx::harmony::ModuleFactoryCAPI> module_factory)
      : ui_owner_(ui_owner),
        node_owner_(node_owner),
        lynx_context_(lynx_context),
        module_factory_(std::move(module_factory)) {}
  ~UIDelegateHarmony() override {}

  std::unique_ptr<PaintingCtxPlatformImpl> CreatePaintingContext() override;
  std::unique_ptr<LayoutCtxPlatformImpl> CreateLayoutContext() override;
  std::unique_ptr<PropBundleCreator> CreatePropBundleCreator() override;
  std::unique_ptr<piper::NativeModuleFactory> GetCustomModuleFactory() override;

  bool UsesLogicalPixels() const override { return true; }

  void OnLynxCreate(
      const std::shared_ptr<shell::LynxEngineProxy>& engine_proxy,
      const std::shared_ptr<shell::LynxRuntimeProxy>& runtime_proxy,
      const std::shared_ptr<shell::PerfControllerProxy>& perf_controller_proxy,
      const std::shared_ptr<pub::LynxResourceLoader>& resource_loader,
      const fml::RefPtr<fml::TaskRunner>& ui_task_runner,
      const fml::RefPtr<fml::TaskRunner>& layout_task_runner) override;

  void OnUpdateScreenMetrics(float width, float height,
                             float device_pixel_ratio) override;

  void TakeSnapshot(
      size_t max_width, size_t max_height, int quality,
      float screen_scale_factor,
      const lynx::fml::RefPtr<lynx::fml::TaskRunner>& screenshot_runner,
      TakeSnapshotCompletedCallback callback) override;

  int GetNodeForLocation(int x, int y) override;

  std::vector<float> GetTransformValue(
      int id, const std::vector<float>& pad_border_margin_layout) override;

  float ScreenWidth() { return screen_width_; }
  float ScreenHeight() { return screen_height_; }
  float DevicePixelRatio() { return device_pixel_ratio_; }

  void OnPageConfigDecoded(const std::shared_ptr<PageConfig>& config) override;

  // TODO(chenyouhui): Remove this after EmbedderPlatformHarmony and
  // UIDelegateHarmony is merged.
  void SetPlatform(shell::EmbedderPlatformHarmony* platform) {
    platform_ = platform;
  }

  const std::shared_ptr<LynxContext> GetLynxContext() {
    return lynx_context_.lock();
  }

 private:
  UIOwner* ui_owner_;
  ShadowNodeOwner* node_owner_;
  std::weak_ptr<LynxContext> lynx_context_;
  std::unique_ptr<lynx::harmony::ModuleFactoryCAPI> module_factory_;
  shell::EmbedderPlatformHarmony* platform_ = nullptr;

  float screen_width_ = 0.f;
  float screen_height_ = 0.f;
  float device_pixel_ratio_ = 1.f;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_UI_DELEGATE_HARMONY_H_
