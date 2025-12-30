// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/engine.h"

#include <memory>
#include <utility>

#include "clay/common/graphics/shared_image_external_texture.h"
#include "clay/flow/frame_timings.h"
#include "clay/flow/layers/layer_tree.h"
#include "clay/shell/common/services/initialize_service.h"
#include "clay/shell/common/services/ui_frame_service.h"
#include "clay/shell/common/services/vsync_waiter_service.h"
#if (defined(OS_MAC) || defined(OS_WIN))
#include "clay/shell/common/vsync_waiter_fallback.h"
#endif
#include "clay/net/loader/resource_loader_intercept.h"
#include "clay/ui/resource/font_collection.h"

namespace clay {

Engine::Engine(std::shared_ptr<clay::ServiceManager> service_manager,
               TaskRunners task_runners, Settings settings,
               fml::RefPtr<GPUUnrefQueue> unref_queue, Delegate* delegate)
    : service_manager_(service_manager),
      pending_layout_(false),
      task_runners_(std::move(task_runners)),
      settings_(std::move(settings)),
      delegate_(delegate),
      weak_factory_(this) {
  Puppet<Owner::kUI, InitializeService> initialize_service =
      service_manager->GetService<InitializeService>();
  ui_frame_service_ = service_manager->GetService<UIFrameService>();
  raster_frame_service_ = service_manager->GetService<RasterFrameService>();
  page_view_ = initialize_service->CreatePageView(
      0, service_manager, std::move(unref_queue), task_runners_);
  shadow_node_owner_ =
      std::make_unique<clay::ShadowNodeOwner>(page_view_->GetTaskRunner());
  view_context_ = initialize_service->CreateViewContext(
      page_view_.get(), shadow_node_owner_.get());
  shadow_node_owner_->SetViewContext(view_context_.get());
  shadow_node_owner_->SetDelegate(this);
  page_view_->SetRenderDelegate(this);
  page_view_->SetDefaultFocusRingEnabled(settings_.enable_default_focus_ring);
  page_view_->SetPerformanceOverlayEnabled(
      settings_.enable_performance_overlay);
  page_view_->SetUseTextureBackend(!settings_.enable_software_rendering);
  clay::Puppet<clay::Owner::kUI, VsyncWaiterService> vsync_waiter_service =
      service_manager->GetService<VsyncWaiterService>();
  page_view_->SetRefreshRate(vsync_waiter_service->GetRefreshRate());
}

Engine::~Engine() {
  FML_DLOG(ERROR) << "Dealloc clay::Engine";
  page_view_->Destroy();
  // Clean up views following order: leaked views, PageView, ViewContext.
  view_context_->CleanLeakedViews();
  page_view_.reset();
  shadow_node_owner_.reset();
  view_context_.reset();
}

fml::WeakPtr<Engine> Engine::GetWeakPtr() const {
  return weak_factory_.GetWeakPtr();
}

std::unique_ptr<Engine> Engine::Spawn(
    std::shared_ptr<clay::ServiceManager> service_manager, Settings settings,
    fml::RefPtr<GPUUnrefQueue> unref_queue, Delegate* delegate) const {
  auto engine = std::make_unique<Engine>(service_manager, task_runners_,
                                         settings, unref_queue, delegate);
  engine->GetPageView()->SetImageResourceFetcher(
      page_view_->GetImageResourceFetcher());
  return engine;
}

bool Engine::BeginFrame(std::unique_ptr<FrameTimingsRecorder> recorder) {
  TRACE_EVENT("clay", "Engine::BeginFrame");
  if (view_context_ && view_context_->GetFrameObserver()) {
    view_context_->GetFrameObserver()->OnBeginFrame();
  }

  if (pending_layout_) {
    shadow_node_owner_->TriggerLayout();
    pending_layout_ = false;
  }
  if (page_view_) {
    return page_view_->BeginFrame(std::move(recorder));
  }
  return false;
}

void Engine::ScheduleLayout() {
  if (!pending_layout_) {
    pending_layout_ = true;
    ScheduleFrame();
  }
}

void Engine::OnPlatformViewCreated() { page_view_->OnPlatformViewCreated(); }

void Engine::OnOutputSurfaceCreated() { page_view_->OnOutputSurfaceCreated(); }

void Engine::OnOutputSurfaceCreateFailed() {
  page_view_->OnOutputSurfaceCreateFailed();
}

void Engine::OnOutputSurfaceDestroyed() {
  page_view_->OnOutputSurfaceDestroyed();
  // It's not enough to release graphic resources of view tree under PageView,
  // because some views may not attached when surface destroyed.
  // Also it's not enough to release resources of views under ViewContext,
  // because some views may created by clay inner.
  view_context_->OnOutputSurfaceDestroyed();
}

void Engine::CleanForRecycle() {
  view_context_->CleanLeakedViews();
  page_view_->CleanForRecycle();
  shadow_node_owner_->ClearNodes();
  pending_layout_ = false;
}

void Engine::PrepareForRecycle() {}

void Engine::SetViewportMetrics(const ViewportMetrics& metrics) {
  page_view_->SetViewportMetrics(metrics);
#if defined(OS_MAC) || defined(OS_WIN) || defined(OS_LINUX) || \
    defined(ENABLE_HEADLESS)
  // If it is Macos, we have to call begin frame without PostTask().
  // Otherwise, the thread will dead locked.
  // See: FlutterThreadSynchronizer.mm - beginResize
  ForceBeginFrame();
#else
  ScheduleFrame();
#endif
}

void Engine::SetupDefaultFontManager() {
  TRACE_EVENT("clay", "Engine::SetupDefaultFontManager");
  clay::FontCollection::Instance()->SetupDefaultFontManager(
      settings_.font_initialization_data);
}

bool Engine::DispatchPointerEvent(std::vector<clay::PointerEvent> events) {
  return page_view_->DispatchPointerEvent(std::move(events));
}

void Engine::DispatchKeyEvent(
    std::unique_ptr<clay::KeyEvent> event,
    std::function<void(bool /* handled */)> callback) {
  page_view_->DispatchKeyEvent(std::move(event), std::move(callback));
}

void Engine::SendKeyboardEvent(std::unique_ptr<clay::KeyEvent> key_event) {
  page_view_->OnKeyboardEvent(std::move(key_event));
}

void Engine::PerformEditorAction(clay::KeyboardAction action_code) {
  page_view_->OnPerformAction(action_code);
}

void Engine::OnAnimationEvent(const clay::ElementId& element_id,
                              const clay::AnimationParams& animation_params) {
  int owner_id = element_id.view_id();
  if (auto view = view_context_->GetViewById(owner_id);
      view != nullptr && view->HasAnimationEvent(animation_params.event_type)) {
    page_view_->DispatchAnimationEvent(animation_params, owner_id);
  }
}

void Engine::OnTransitionEvent(const clay::ElementId& element_id,
                               const clay::AnimationParams& animation_params,
                               ClayAnimationPropertyType property_type) {
  int owner_id = element_id.view_id();
  if (auto view = view_context_->GetViewById(owner_id);
      view != nullptr && view->HasAnimationEvent(animation_params.event_type)) {
    page_view_->DispatchTransitionEvent(animation_params, owner_id,
                                        property_type);
  }
}

void Engine::DeleteSurroundingText(int before_length, int after_length) {
  page_view_->OnDeleteSurroundingText(before_length, after_length);
}

void Engine::OnEnterForeground() { page_view_->DispatchEnterForeground(); }

void Engine::OnEnterBackground() { page_view_->DispatchEnterBackground(); }

void Engine::SetVisible(bool enable) { page_view_->SetVisible(enable); }

void Engine::RequestPaint() { page_view_->RequestPaintBase(); }

void Engine::SetDefaultFocusRingEnabled(bool enable) {
  page_view_->SetDefaultFocusRingEnabled(enable);
}

void Engine::SetPerformanceOverlayEnabled(bool enable) {
  page_view_->SetPerformanceOverlayEnabled(enable);
}

void Engine::SetDefaultOverflowVisible(bool visible) {
  view_context_->SetDefaultOverflowVisible(visible);
}

void Engine::SetExposureProps(int freq, bool enable_exposure_ui_margin) {
  view_context_->SetExposureProps(freq, enable_exposure_ui_margin);
}

void Engine::MakeRasterSnapshot(
    std::unique_ptr<clay::LayerTree> layer_tree,
    std::function<void(fml::RefPtr<clay::PaintImage>)> callback) {
  if (delegate_ == nullptr) {
    callback(nullptr);
    return;
  }
  delegate_->MakeRasterSnapshot(std::move(layer_tree), std::move(callback));
}

fml::RefPtr<PaintImage> Engine::MakeRasterSnapshot(GrPicturePtr picture,
                                                   skity::Vec2 picture_size) {
  if (delegate_ == nullptr) {
    return nullptr;
  }

  return delegate_->MakeRasterSnapshot(std::move(picture), picture_size);
}

clay::BaseView* Engine::FindViewById(int view_id) {
  return view_context_->FindViewByViewId(view_id);
}

clay::ShadowNode* Engine::FindShadowNodeById(int node_id) {
  return view_context_->FindShadowNodeByNodeId(node_id);
}

void Engine::SetRenderSettings(
    fml::RefPtr<clay::RenderSettings> render_settings) {
  page_view_->SetRenderSettings(render_settings);
}

void Engine::ScheduleFrame() {
  ui_frame_service_.Act([](auto& impl) { impl.RequestFrame(); });
}

void Engine::ForceBeginFrame() {
  clay::Puppet<clay::Owner::kUI, VsyncWaiterService> vsync_waiter_service =
      service_manager_->GetService<VsyncWaiterService>();
  const auto frame_begin_time = fml::TimePoint::Now();
  const auto frame_end_time =
      frame_begin_time + fml::TimeDelta::FromSecondsF(
                             1.f / vsync_waiter_service->GetRefreshRate());
  std::unique_ptr<FrameTimingsRecorder> recorder =
      std::make_unique<FrameTimingsRecorder>();
  recorder->RecordVsync(frame_begin_time, frame_end_time);
  recorder->RecordForced(true);
  ui_frame_service_.Act([recorder = std::move(recorder)](auto& impl) mutable {
    impl.ForceBeginFrame(std::move(recorder));
  });
}

void Engine::OnFirstMeaningfulLayout() {
  ui_frame_service_.Act([](auto& impl) { impl.OnFirstMeaningfulLayout(); });
}

bool Engine::Raster(std::unique_ptr<clay::LayerTree> layer_tree,
                    std::unique_ptr<clay::FrameTimingsRecorder> recorder,
                    bool force) {
  // Ensure frame dimensions are sane.
  if (layer_tree &&
      ((layer_tree->frame_size().x == 0 && layer_tree->frame_size().y == 0) ||
       layer_tree->device_pixel_ratio() <= 0.0f)) {
    layer_tree.reset();
  }
  if (!layer_tree) {
    raster_frame_service_.Act([](auto& impl) { impl.CommitWithNoUpdates(); });
  } else {
    if (!recorder) {
      recorder = std::make_unique<FrameTimingsRecorder>();
      const fml::TimePoint placeholder_time = fml::TimePoint::Now();
      recorder->RecordVsync(placeholder_time, placeholder_time);
      recorder->RecordBuildStart(placeholder_time);
    }
    recorder->RecordBuildEnd(fml::TimePoint::Now());
    layer_tree->SetServiceManagerForAnimation(service_manager_);
    raster_frame_service_.Act([layer_tree = std::move(layer_tree),
                               recorder = std::move(recorder),
                               force = force](auto& impl) mutable {
      if (force) {
        // It's used for unittests, the shell_test is not able to handle the
        // scheduler with state machine.
        impl.ForceCommit(std::move(layer_tree), std::move(recorder));
      } else {
        impl.Commit(std::move(layer_tree), std::move(recorder));
      }
    });
  }
  return true;
}

void Engine::ShowSoftInput(int type, int action) {
  delegate_->ShowSoftInput(type, action);
}

void Engine::HideSoftInput() { delegate_->HideSoftInput(); }

std::string Engine::ShouldInterceptUrl(const std::string& origin_url,
                                       bool should_decode) {
  return delegate_->ShouldInterceptUrl(origin_url, should_decode);
}

std::shared_ptr<clay::ResourceLoaderIntercept>
Engine::GetResourceLoaderIntercept() {
  return delegate_->GetResourceLoaderIntercept();
}

std::shared_ptr<clay::FontCollection> Engine::GetFontCollection() {
  return clay::FontCollection::Instance();
}

void Engine::SetPerfCollector(
    std::shared_ptr<clay::PerfCollector> perf_collector) {
  page_view_->SetPerfCollector(perf_collector);
}

void Engine::SetFontFaceCache(const char* font_family, const char* local_path) {
  std::vector<std::string> src_vec;
  src_vec.emplace_back(local_path);
  clay::FontCollection::Instance()->PreLoadFontOnMem(
      task_runners_.GetUITaskRunner(), GetResourceLoaderIntercept(),
      page_view_->GetServiceManager(), std::string(font_family),
      std::move(src_vec));
}

void Engine::UpdateMemoryCacheOptions() {
  clay::Isolate::Instance().UpdateResourceCacheMaxMemoryLimit(
      settings_.image_texture_cache_max_limit,
      settings_.low_end_image_texture_cache_max_limit);
}

bool Engine::MarkDrawableImageFrameAvailable(int64_t image_id) {
  return page_view_ && page_view_->MarkDrawableImageFrameAvailable(image_id);
}

const std::weak_ptr<VsyncWaiter> Engine::GetVsyncWaiter() const {
  return std::weak_ptr<VsyncWaiter>();
}

void Engine::ClearTextCache() {
  clay::FontCollection::Instance()->ClearFontFamilyCache();
}

void Engine::UpdateRootSize(int32_t width, int32_t height) {
  if (delegate_) {
    delegate_->UpdateRootSize(width, height);
  }
}

void Engine::RegisterDrawableImage(
    std::shared_ptr<DrawableImage> drawable_image) {
  if (delegate_) {
    delegate_->RegisterDrawableImage(drawable_image);
  }
}

void Engine::UnregisterDrawableImage(int64_t id) {
  if (delegate_) {
    delegate_->UnregisterDrawableImage(id);
  }
}

void Engine::ReportTiming(
    const std::unordered_map<std::string, int64_t>& timing,
    const std::string& flag) {
  delegate_->ReportTiming(timing, flag);
}

#ifdef ENABLE_ACCESSIBILITY
void Engine::UpdateSemantics(const clay::SemanticsUpdateNodes& update_nodes) {
  if (delegate_) {
    delegate_->UpdateSemantics(update_nodes);
  }
}

void Engine::SetSemanticsEnabled(bool enabled) {
  if (page_view_) {
    page_view_->SetSemanticsEnabled(enabled);
  }
}

void Engine::SetPageEnableAccessibilityElement(bool enabled) {
  if (page_view_) {
    page_view_->SetPageEnableAccessibilityElement(enabled);
  }
}

void Engine::DispatchSemanticsAction(int virtual_view_id, int action) {
  if (page_view_) {
    page_view_->DispatchSemanticsAction(virtual_view_id, action);
  }
}
#endif

}  // namespace clay
