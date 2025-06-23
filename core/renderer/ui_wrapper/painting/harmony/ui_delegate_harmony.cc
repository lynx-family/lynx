// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/harmony/ui_delegate_harmony.h"

#include "core/renderer/ui_wrapper/layout/harmony/layout_context_harmony.h"
#include "core/renderer/ui_wrapper/painting/harmony/painting_context_harmony.h"
#include "core/shell/harmony/embedder_platform_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/event_target.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_root.h"

namespace lynx {
namespace tasm {
namespace harmony {

std::unique_ptr<PaintingCtxPlatformImpl>
UIDelegateHarmony::CreatePaintingContext() {
  return std::make_unique<PaintingContextHarmony>(ui_owner_);
}

std::unique_ptr<LayoutCtxPlatformImpl>
UIDelegateHarmony::CreateLayoutContext() {
  return std::make_unique<LayoutContextHarmony>(node_owner_);
}

std::unique_ptr<PropBundleCreator>
UIDelegateHarmony::CreatePropBundleCreator() {
  return std::make_unique<PropBundleCreatorHarmony>();
}

std::unique_ptr<piper::NativeModuleFactory>
UIDelegateHarmony::GetCustomModuleFactory() {
  return std::move(module_factory_);
}

void UIDelegateHarmony::OnLynxCreate(
    const std::shared_ptr<shell::LynxEngineProxy>& engine_proxy,
    const std::shared_ptr<shell::LynxRuntimeProxy>& runtime_proxy,
    const std::shared_ptr<shell::PerfControllerProxy>& perf_controller_proxy,
    const std::shared_ptr<pub::LynxResourceLoader>& resource_loader,
    const fml::RefPtr<fml::TaskRunner>& ui_task_runner,
    const fml::RefPtr<fml::TaskRunner>& layout_task_runner) {
  auto lynx_context = lynx_context_.lock();
  if (!lynx_context) {
    return;
  }
  lynx_context->OnLynxCreate(engine_proxy, runtime_proxy, perf_controller_proxy,
                             resource_loader, ui_task_runner,
                             layout_task_runner);
}

void UIDelegateHarmony::OnUpdateScreenMetrics(float width, float height,
                                              float device_pixel_ratio) {
  screen_width_ = width;
  screen_height_ = height;
  device_pixel_ratio_ = device_pixel_ratio;
  auto lynx_context = lynx_context_.lock();
  if (lynx_context && lynx_context->GetExtensionDelegate()) {
    lynx_context->GetExtensionDelegate()->OnDevicePixelRatioChanged(
        device_pixel_ratio);
  }
}

void UIDelegateHarmony::OnPageConfigDecoded(
    const std::shared_ptr<PageConfig>& config) {
  auto lynx_context = lynx_context_.lock();
  if (lynx_context) {
    lynx_context->GetFluencyTraceHelper().SetPageConfigProbability(
        config->GetEnableScrollFluencyMonitor());
    lynx_context->SetEnableTextOverflow(config->GetEnableTextOverflow());
    lynx_context->SetTapSlop(config->GetTapSlop());
    lynx_context->SetHasTouchPseudo(config->GetEnableFiberArch());
    lynx_context->SetLongPressDuration(config->GetLongPressDuration());
    if (config->GetEnableNewGesture() && ui_owner_) {
      ui_owner_->InitGestureArenaManager(lynx_context.get());
    }
  }
}

void UIDelegateHarmony::TakeSnapshot(
    size_t max_width, size_t max_height, int quality, float screen_scale_factor,
    const lynx::fml::RefPtr<lynx::fml::TaskRunner>& screenshot_runner,
    TakeSnapshotCompletedCallback callback) {
  if (!platform_) return;
  platform_->TakeSnapshot(max_width, max_height, quality, screenshot_runner,
                          std::move(callback));
}

int UIDelegateHarmony::GetNodeForLocation(int x, int y) {
  auto lynx_context = lynx_context_.lock();
  if (lynx_context) {
    auto root = lynx_context->GetUIOwner()->Root();
    float pos[2] = {
        static_cast<float>(x) / root->GetContext()->ScaledDensity(),
        static_cast<float>(y) / root->GetContext()->ScaledDensity()};
    tasm::harmony::EventTarget* ui = root->HitTest(pos);
    return ui ? ui->Sign() : 0;
  }
  return 0;
}

std::vector<float> UIDelegateHarmony::GetTransformValue(
    int id, const std::vector<float>& pad_border_margin_layout) {
  std::vector<float> res(32, 0);
  std::vector<float> point(8, 0);
  auto lynx_context = lynx_context_.lock();
  if (!lynx_context) {
    return {};
  }
  auto ui = lynx_context->GetUIOwner()->FindUIBySign(id);
  if (!ui) {
    return {};
  }
  for (int i = 0; i < 4; ++i) {
    if (i == 0) {
      ui->GetTransformValue(
          pad_border_margin_layout[BoxModelOffset::PAD_LEFT] +
              pad_border_margin_layout[BoxModelOffset::BORDER_LEFT] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_LEFT],
          -pad_border_margin_layout[BoxModelOffset::PAD_RIGHT] -
              pad_border_margin_layout[BoxModelOffset::BORDER_RIGHT] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_RIGHT],
          pad_border_margin_layout[BoxModelOffset::PAD_TOP] +
              pad_border_margin_layout[BoxModelOffset::BORDER_TOP] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_TOP],
          -pad_border_margin_layout[BoxModelOffset::PAD_BOTTOM] -
              pad_border_margin_layout[BoxModelOffset::BORDER_BOTTOM] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_BOTTOM],
          point);
    } else if (i == 1) {
      ui->GetTransformValue(
          pad_border_margin_layout[BoxModelOffset::BORDER_LEFT] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_LEFT],
          -pad_border_margin_layout[BoxModelOffset::BORDER_RIGHT] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_RIGHT],
          pad_border_margin_layout[BoxModelOffset::BORDER_TOP] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_TOP],
          -pad_border_margin_layout[BoxModelOffset::BORDER_BOTTOM] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_BOTTOM],
          point);
    } else if (i == 2) {
      ui->GetTransformValue(
          pad_border_margin_layout[BoxModelOffset::LAYOUT_LEFT],
          -pad_border_margin_layout[BoxModelOffset::LAYOUT_RIGHT],
          pad_border_margin_layout[BoxModelOffset::LAYOUT_TOP],
          -pad_border_margin_layout[BoxModelOffset::LAYOUT_BOTTOM], point);
    } else {
      ui->GetTransformValue(
          -pad_border_margin_layout[BoxModelOffset::MARGIN_LEFT] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_LEFT],
          pad_border_margin_layout[BoxModelOffset::MARGIN_RIGHT] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_RIGHT],
          -pad_border_margin_layout[BoxModelOffset::MARGIN_TOP] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_TOP],
          pad_border_margin_layout[BoxModelOffset::MARGIN_BOTTOM] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_BOTTOM],
          point);
    }
    res[i * 8] = point[0];
    res[i * 8 + 1] = point[1];
    res[i * 8 + 2] = point[2];
    res[i * 8 + 3] = point[3];
    res[i * 8 + 4] = point[4];
    res[i * 8 + 5] = point[5];
    res[i * 8 + 6] = point[6];
    res[i * 8 + 7] = point[7];
  }
  return res;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
