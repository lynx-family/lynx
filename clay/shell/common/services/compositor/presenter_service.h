// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SERVICES_COMPOSITOR_PRESENTER_SERVICE_H_
#define CLAY_SHELL_COMMON_SERVICES_COMPOSITOR_PRESENTER_SERVICE_H_

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "clay/common/service/service.h"
#include "clay/flow/embedded_views.h"
#include "clay/shell/common/services/compositor/platform_overlay_service.h"

namespace clay {

struct PresentFrame {
  std::unordered_map<int64_t, OverlayData> overlays;
  std::unordered_map<int64_t, std::unique_ptr<EmbeddedViewParams>>
      compositor_params;
  std::vector<int64_t> composite_order;
  std::vector<std::shared_ptr<PlatformOverlay>> unused_overlays;
  std::vector<std::pair<SurfaceFrame::SubmitCallback, SurfaceFrame::SubmitInfo>>
      submit_infos;
};

class PresenterService
    : public clay::Service<PresenterService, clay::Owner::kPlatform> {
 public:
  void Present(PresentFrame &frame);

  void MarkPlatformViewToDispose(int64_t id);

  static std::shared_ptr<PresenterService> Create();

 private:
  virtual void OnBeforePresent() {}
  virtual void OnAfterPresent() {}

  virtual void UpdateOverlay(const OverlayData &overlay_data) = 0;
  // Organize the layers by their z indexes.
  virtual void BringOverlayToFront(const PlatformOverlay &overlay) = 0;
  virtual void DisposeOverlay(PlatformOverlay &overlay) = 0;

  virtual void CompositePlatformView(int64_t id,
                                     const EmbeddedViewParams &params) = 0;
  virtual void BringPlatformViewToFront(int64_t id) = 0;
  virtual void DisposePlatformView(int64_t id) = 0;
  virtual void HidePlatformView(int64_t id) = 0;

  std::vector<int64_t> GetPlatformViewsToDispose(
      const std::vector<int64_t> &composition_order);
  void RemoveUnusedLayers(
      const std::vector<int64_t> &composite_order,
      const std::vector<std::shared_ptr<PlatformOverlay>> &unused_overlays);
  void BringLayersIntoView(
      const std::vector<int64_t> &composite_order,
      const std::unordered_map<int64_t, OverlayData> &overlays);

  std::unordered_map<int64_t, std::unique_ptr<EmbeddedViewParams>>
      current_composition_params_;

  std::unordered_set<int64_t> views_to_dispose_;
};

}  // namespace clay
   //
#endif  // CLAY_SHELL_COMMON_SERVICES_COMPOSITOR_PRESENTER_SERVICE_H_
