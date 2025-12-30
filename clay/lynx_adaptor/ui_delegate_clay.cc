// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/ui_delegate_clay.h"

#include <memory>
#include <string>
#include <utility>

#include "clay/lynx_adaptor/layout_context_clay.h"
#include "clay/lynx_adaptor/painting_context_clay.h"
#include "clay/lynx_adaptor/perf_controller_clay.h"
#include "clay/lynx_adaptor/prop_bundle_impl.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/view_context.h"
#include "core/services/timing_handler/timing.h"

namespace lynx {
namespace tasm {

UIDelegateClay::UIDelegateClay(
    clay::ViewContext* view_context,
    std::unique_ptr<lynx::piper::NativeModuleFactory> module_factory)
    : view_context_(view_context), module_factory_(std::move(module_factory)) {
  event_dispatcher_ = std::make_unique<clay::LynxEventDispatcher>();
  view_context->SetEventDelegate(event_dispatcher_.get());
}

UIDelegateClay::~UIDelegateClay() {
  if (view_context_->GetPageView() &&
      view_context_->GetPageView()->GetEventDelegate() ==
          this->event_dispatcher_.get()) {
    view_context_->SetEventDelegate(nullptr);
  }
}

std::unique_ptr<PaintingCtxPlatformImpl>
UIDelegateClay::CreatePaintingContext() {
  auto painting_context = std::make_unique<PaintingContextClay>(view_context_);
  painting_context_ = painting_context.get();
  return painting_context;
}

std::unique_ptr<LayoutCtxPlatformImpl> UIDelegateClay::CreateLayoutContext() {
  auto layout_context = std::make_unique<LayoutContextClay>(view_context_);
  layout_context_ = layout_context.get();
  return layout_context;
}

std::unique_ptr<PropBundleCreator> UIDelegateClay::CreatePropBundleCreator() {
  return std::make_unique<PropBundleCreatorClay>();
}

std::unique_ptr<piper::NativeModuleFactory>
UIDelegateClay::GetCustomModuleFactory() {
  return std::move(module_factory_);
}

bool UIDelegateClay::UsesLogicalPixels() const {
  return view_context_ && view_context_->UsesLogicalPixels();
}

double UIDelegateClay::GetScreenScaleFactor() const {
  auto page_view = view_context_->GetPageView();
  return page_view ? page_view->DevicePixelRatio() : 1.0;
}

void UIDelegateClay::OnLynxCreate(
    const std::shared_ptr<shell::ListEngineProxy>& list_engine_proxy,
    const std::shared_ptr<shell::LynxEngineProxy>& engine_proxy,
    const std::shared_ptr<shell::LynxRuntimeProxy>& runtime_proxy,
    const std::shared_ptr<shell::LynxLayoutProxy>& layout_proxy,
    const std::shared_ptr<shell::PerfControllerProxy>& perf_controller_proxy,
    const std::shared_ptr<pub::LynxResourceLoader>& resource_loader,
    const fml::RefPtr<fml::TaskRunner>& ui_task_runner,
    const fml::RefPtr<fml::TaskRunner>& layout_task_runner, int32_t instance_id,
    bool is_embedded_mode) {
  auto perf_controller =
      std::make_shared<PerfControllerClay>(perf_controller_proxy, instance_id);
  if (painting_context_) {
    painting_context_->SetListEngineProxy(list_engine_proxy);
    painting_context_->SetEngineProxy(engine_proxy);
    painting_context_->SetRuntimeProxy(runtime_proxy);
    auto ref = painting_context_->GetPlatformRef();
    if (ref) {
      auto* painting_context_ref =
          static_cast<PaintingContextClayRef*>(ref.get());
      painting_context_ref->SetPerfController(perf_controller);
    }
  }
  if (layout_context_) {
    layout_context_->SetEngineProxy(engine_proxy);
    layout_context_->SetLayoutProxy(layout_proxy);
  }
  if (event_dispatcher_) {
    event_dispatcher_->SetEngineProxy(engine_proxy);
    event_dispatcher_->SetRuntimeProxy(runtime_proxy);
    event_dispatcher_->SetPerfController(perf_controller);
  }
}

void UIDelegateClay::TakeSnapshot(
    size_t max_width, size_t max_height, int quality, float screen_scale_factor,
    const lynx::fml::RefPtr<lynx::fml::TaskRunner>& screenshot_runner,
    TakeSnapshotCompletedCallback callback) {
  auto page_view = view_context_->GetPageView();
  if (!page_view) {
    return;
  }
  clay::ScreenshotRequest request;
  request.max_width_ = max_width;
  request.max_height_ = max_height;
  request.quality_ = quality;
  request.type_ =
      quality >= 100 ? clay::ScreenshotType::PNG : clay::ScreenshotType::JPEG;
  request.task_runner_ = screenshot_runner;
  request.screen_scale_factor_ = screen_scale_factor;
  request.is_sync_ = false;
  request.callback_ = [callback = std::move(callback), screenshot_runner](
                          clay::GrDataPtr data,
                          const clay::ScreenMetadata& metadata) {
    screenshot_runner->PostTask([data = std::move(data), metadata,
                                 callback = std::move(callback)] {
      float timestamp =
          std::chrono::steady_clock::now().time_since_epoch().count();
      std::string base64_str = std::string(
          static_cast<const char*>(DATA_GET_DATA(data)), DATA_GET_SIZE(data));
      callback(std::move(base64_str), timestamp, metadata.device_width_,
               metadata.device_height_, metadata.page_scale_factor_);
    });
  };
  page_view->TakeScreenshotHardware(request);
}

int UIDelegateClay::GetNodeForLocation(int x, int y) {
  return view_context_->GetViewIdForLocation(x, y);
}

std::vector<float> UIDelegateClay::GetTransformValue(
    int id, const std::vector<float>& pad_border_margin_layout) {
  std::vector<float> res(32, 0);
  // res is std::vector<float>(32 ,0) passed by lynx
  clay::BaseView* current_view = view_context_->GetViewById(id);
  if (current_view == nullptr) {
    return std::vector<float>();
  }

  clay::TransOffset arr;

  // Returns the coordinates of the four types of boxes
  // - border-box:
  // That is, the width and height of the view set by the front-end, including
  // content, padding, border, corresponding to the real rendering view
  // - content-box: border-box with border and padding width and height removed
  // - padding-box: border-box without the padding width and height of the box
  // - margin-box: border-box plus margin box
  for (int i = 0; i < 4; i++) {
    if (i == 0) {
      current_view->GetTransformValue(
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
          arr);
    } else if (i == 1) {
      current_view->GetTransformValue(
          pad_border_margin_layout[BoxModelOffset::BORDER_LEFT] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_LEFT],
          -pad_border_margin_layout[BoxModelOffset::BORDER_RIGHT] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_RIGHT],
          pad_border_margin_layout[BoxModelOffset::BORDER_TOP] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_TOP],
          -pad_border_margin_layout[BoxModelOffset::BORDER_BOTTOM] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_BOTTOM],
          arr);
    } else if (i == 2) {
      current_view->GetTransformValue(
          pad_border_margin_layout[BoxModelOffset::LAYOUT_LEFT],
          -pad_border_margin_layout[BoxModelOffset::LAYOUT_RIGHT],
          pad_border_margin_layout[BoxModelOffset::LAYOUT_TOP],
          -pad_border_margin_layout[BoxModelOffset::LAYOUT_BOTTOM], arr);
    } else {
      current_view->GetTransformValue(
          -pad_border_margin_layout[BoxModelOffset::MARGIN_LEFT] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_LEFT],
          pad_border_margin_layout[BoxModelOffset::MARGIN_RIGHT] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_RIGHT],
          -pad_border_margin_layout[BoxModelOffset::MARGIN_TOP] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_TOP],
          pad_border_margin_layout[BoxModelOffset::MARGIN_BOTTOM] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_BOTTOM],
          arr);
    }
    res[i * 8] = arr.left_top[0];
    res[i * 8 + 1] = arr.left_top[1];
    res[i * 8 + 2] = arr.right_top[0];
    res[i * 8 + 3] = arr.right_top[1];
    res[i * 8 + 4] = arr.right_bottom[0];
    res[i * 8 + 5] = arr.right_bottom[1];
    res[i * 8 + 6] = arr.left_bottom[0];
    res[i * 8 + 7] = arr.left_bottom[1];
  }

  return res;
}

}  // namespace tasm
}  // namespace lynx
