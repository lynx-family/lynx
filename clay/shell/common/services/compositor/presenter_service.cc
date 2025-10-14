// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/services/compositor/presenter_service.h"

#include "base/trace/native/trace_event.h"

namespace clay {

void PresenterService::Present(PresentFrame& frame) {
  TRACE_EVENT("clay", "PresenterService::Present");
  OnBeforePresent();

  for (const auto& [cb, info] : frame.submit_infos) {
    cb(info);
  }

  for (const auto& [view_id, layer_data] : frame.overlays) {
    UpdateOverlay(layer_data);
    layer_data.overlay->OnSurfaceUpdated();
  }

  for (auto& view_id : GetPlatformViewsToDispose(frame.composite_order)) {
    DisposePlatformView(view_id);
  }

  for (auto& [view_id, embedded_params] : frame.compositor_params) {
    auto& current_composition_param = current_composition_params_[view_id];
    if (current_composition_param != nullptr &&
        *embedded_params == *current_composition_param) {
      continue;
    }
    CompositePlatformView(view_id, *embedded_params);
    current_composition_param = std::move(embedded_params);
  }

  RemoveUnusedLayers(frame.composite_order, frame.unused_overlays);

  BringLayersIntoView(frame.composite_order, frame.overlays);

  OnAfterPresent();
}

void PresenterService::MarkPlatformViewToDispose(int64_t view_id) {
  views_to_dispose_.insert(view_id);
}

std::vector<int64_t> PresenterService::GetPlatformViewsToDispose(
    const std::vector<int64_t>& composition_order) {
  std::vector<int64_t> views;
  if (views_to_dispose_.empty()) {
    return views;
  }

  std::unordered_set<int64_t> views_to_composite(composition_order.begin(),
                                                 composition_order.end());
  std::unordered_set<int64_t> views_to_delay_dispose;

  for (int64_t view_id : views_to_dispose_) {
    if (views_to_composite.count(view_id)) {
      views_to_delay_dispose.insert(view_id);
      continue;
    }
    views.push_back(view_id);
    current_composition_params_.erase(view_id);
  }
  views_to_dispose_ = std::move(views_to_delay_dispose);
  return views;
}

void PresenterService::RemoveUnusedLayers(
    const std::vector<int64_t>& composite_order,
    const std::vector<std::shared_ptr<PlatformOverlay>>& unused_overlays) {
  for (auto& overlay : unused_overlays) {
    DisposeOverlay(*overlay);
  }

  std::unordered_set<int64_t> composition_order_set(composite_order.begin(),
                                                    composite_order.end());

  for (const auto& [view_id, _] : current_composition_params_) {
    if (composition_order_set.count(view_id) == 0) {
      HidePlatformView(view_id);
    }
  }
}

void PresenterService::BringLayersIntoView(
    const std::vector<int64_t>& composite_order,
    const std::unordered_map<int64_t, OverlayData>& overlays) {
  // TODO(haoyoufeng.aji) optimize unnecessary BringToFront
  for (int64_t view_id : composite_order) {
    BringPlatformViewToFront(view_id);
    if (auto it = overlays.find(view_id); it != overlays.end()) {
      BringOverlayToFront(*it->second.overlay);
    }
  }
}

}  // namespace clay
