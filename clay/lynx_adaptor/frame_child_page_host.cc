// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/frame_child_page_host.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <utility>

#include "base/include/fml/time/time_point.h"
#include "clay/flow/layers/layer_tree.h"
#include "clay/fml/logging.h"
#include "clay/lynx_adaptor/ui_delegate_clay.h"
#include "clay/ui/common/input_client_manager.h"
#include "clay/ui/component/frame_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/view_context.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/event/key_event.h"
#include "clay/ui/shadow/shadow_node_owner.h"
#include "core/public/pipeline_option.h"
#include "core/renderer/starlight/layout/layout_global.h"
#include "core/template_bundle/lynx_template_bundle.h"
#include "platform/embedder/core/lynx_template_renderer.h"

namespace lynx {
namespace tasm {

FrameChildPageHost::FrameChildPageHost(
    clay::ViewContext* parent_view_context,
    std::shared_ptr<pub::LynxResourceLoader> resource_loader,
    embedder::LynxTemplateRenderer::Settings parent_renderer_settings)
    : parent_view_context_(parent_view_context),
      parent_page_view_(parent_view_context ? parent_view_context->GetPageView()
                                            : nullptr),
      resource_loader_(std::move(resource_loader)),
      parent_renderer_settings_(std::move(parent_renderer_settings)),
      weak_factory_(this) {}

FrameChildPageHost::~FrameChildPageHost() { Destroy(); }

void FrameChildPageHost::Destroy() {
  if (destroyed_) {
    return;
  }
  destroyed_ = true;
  if (parent_page_view_) {
    parent_page_view_->CancelChildFrame(this);
  }
  UnregisterDrawableFirstFrameBridges();
#if defined(OS_WIN) || defined(OS_MAC) || defined(ENABLE_HEADLESS)
  ClearTextInputIfNeeded();
#endif
  frame_view_id_ = 0;
  loaded_url_.clear();
  child_viewport_size_ = clay::FloatSize();
  child_viewport_device_pixel_ratio_ = 0.f;
  child_viewport_width_mode_ = -1;
  child_viewport_height_mode_ = -1;
  child_viewport_initialized_ = false;
  pending_layout_ = false;
  inside_begin_frame_ = false;
  child_first_meaningful_layout_ = false;
  has_submitted_child_surface_ = false;
  producing_initial_child_frame_ = false;
  previous_child_layer_tree_.reset();
  renderer_.reset();
  ui_delegate_.reset();
  if (child_page_view_) {
    child_page_view_->Destroy();
  }
  if (child_view_context_) {
    child_view_context_->CleanLeakedViews();
  }
  child_page_view_.reset();
  child_shadow_node_owner_.reset();
  child_view_context_.reset();
}

embedder::LynxTemplateRenderer::Settings
FrameChildPageHost::BuildChildRendererSettings(
    const clay::FrameView* frame_view) const {
  auto settings = parent_renderer_settings_;
  const auto& parent_metrics = parent_page_view_->GetViewportMetrics();
  const float dpr = parent_metrics.device_pixel_ratio > 0
                        ? parent_metrics.device_pixel_ratio
                        : 1.f;
  const auto viewport_size = frame_view->child_viewport_size();

  settings.resource_loader = resource_loader_;
  settings.global_props = nullptr;
  settings.device_pixel_ratio = dpr;
  settings.viewport_size.cx = std::max(viewport_size.width(), 0.f);
  settings.viewport_size.cy = std::max(viewport_size.height(), 0.f);
  settings.enable_pre_update_data = true;
  settings.embedded_mode = frame_view->embedded_mode();
  if (frame_view->enable_multi_async_thread()) {
    settings.enable_multi_async_thread =
        *frame_view->enable_multi_async_thread();
  }
  return settings;
}

void FrameChildPageHost::EnsureChildPage(clay::FrameView* frame_view) {
  if (child_page_view_ || !parent_page_view_ || destroyed_) {
    return;
  }

  child_page_view_ = std::make_unique<clay::PageView>(
      0, parent_page_view_->GetServiceManager(),
      parent_page_view_->GetUnrefQueue(), parent_page_view_->GetTaskRunners());
  child_shadow_node_owner_ = std::make_unique<clay::ShadowNodeOwner>(
      child_page_view_->GetTaskRunner());
  child_view_context_ = std::make_shared<clay::ViewContext>(
      child_page_view_.get(), child_shadow_node_owner_.get());
  child_shadow_node_owner_->SetViewContext(child_view_context_.get());
  child_shadow_node_owner_->SetDelegate(this);
  child_page_view_->SetRenderDelegate(this);
  child_page_view_->SetImageResourceFetcher(
      parent_page_view_->GetImageResourceFetcher());
  child_page_view_->SetDefaultOverflow(parent_page_view_->DefaultOverflow());
  child_page_view_->SetUseTextureBackend(
      parent_page_view_->UseTextureBackend());

  auto settings = BuildChildRendererSettings(frame_view);
  ui_delegate_ = std::make_unique<UIDelegateClay>(child_view_context_.get());
  ui_delegate_->SetParentFrameRendererSettings(settings);

  renderer_ = std::make_unique<embedder::LynxTemplateRenderer>(
      settings, ui_delegate_.get(), nullptr, nullptr);
  renderer_->SetFontScale(settings.font_scale);
}

bool FrameChildPageHost::LoadBundle(
    clay::FrameView* frame_view,
    const std::shared_ptr<LynxTemplateBundle>& bundle,
    const std::shared_ptr<TemplateData>& data,
    const std::shared_ptr<TemplateData>& global_props) {
  if (!frame_view || !bundle) {
    return false;
  }
  frame_view_id_ = frame_view->id();
  loaded_url_ = frame_view->url();
  child_viewport_size_ = clay::FloatSize();
  child_viewport_device_pixel_ratio_ = 0.f;
  child_viewport_width_mode_ = -1;
  child_viewport_height_mode_ = -1;
  child_viewport_initialized_ = false;
  child_first_meaningful_layout_ = false;
  has_submitted_child_surface_ = false;
  producing_initial_child_frame_ = false;
  previous_child_layer_tree_.reset();
  UnregisterDrawableFirstFrameBridges();
  EnsureChildPage(frame_view);
  if (!renderer_) {
    return false;
  }

  UpdateViewport(frame_view);
  if (global_props) {
    renderer_->UpdateGlobalProps(global_props->GetValue());
  }
  auto options = std::make_shared<PipelineOptions>();
  renderer_->LoadTemplateBundle(frame_view->url(), *bundle, options, data);
  TryProduceInitialChildFrameNow();
  return true;
}

bool FrameChildPageHost::UpdateMetaData(
    const std::shared_ptr<TemplateData>& data,
    const std::shared_ptr<TemplateData>& global_props) {
  if (!renderer_) {
    return false;
  }
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

void FrameChildPageHost::UpdateViewport(clay::FrameView* frame_view) {
  if (!frame_view || !child_page_view_) {
    return;
  }
  if (!MatchesLoadedFrame(frame_view)) {
    return;
  }
  const auto& parent_metrics = parent_page_view_->GetViewportMetrics();
  const auto viewport_size = frame_view->child_viewport_size();
  const float logical_width = std::max(viewport_size.width(), 0.f);
  const float logical_height = std::max(viewport_size.height(), 0.f);
  const float dpr = parent_metrics.device_pixel_ratio;
  const int width_mode = frame_view->auto_width() ? SLMeasureModeIndefinite
                                                  : SLMeasureModeDefinite;
  const int height_mode = frame_view->auto_height() ? SLMeasureModeIndefinite
                                                    : SLMeasureModeDefinite;
  const bool viewport_changed = !child_viewport_initialized_ ||
                                child_viewport_size_ != viewport_size ||
                                child_viewport_device_pixel_ratio_ != dpr ||
                                child_viewport_width_mode_ != width_mode ||
                                child_viewport_height_mode_ != height_mode;

  clay::ViewportMetrics metrics = parent_metrics;
  metrics.physical_width = std::lround(logical_width * dpr);
  metrics.physical_height = std::lround(logical_height * dpr);
  child_page_view_->SetViewportMetrics(metrics);
  if (!viewport_changed) {
    return;
  }
  if (renderer_) {
    renderer_->UpdateViewport(logical_width, width_mode, logical_height,
                              height_mode, true);
  }
  child_viewport_size_ = viewport_size;
  child_viewport_device_pixel_ratio_ = dpr;
  child_viewport_width_mode_ = width_mode;
  child_viewport_height_mode_ = height_mode;
  child_viewport_initialized_ = true;

  if (HasRunnableViewport(frame_view)) {
    pending_layout_ = true;
    if (!inside_begin_frame_) {
      if (!TryProduceInitialChildFrameNow()) {
        ScheduleParentFrame(true);
      }
    }
  }
}

bool FrameChildPageHost::ForwardPointerEvent(const clay::PointerEvent& event) {
  if (!child_page_view_ || !parent_page_view_) {
    return false;
  }
  auto* frame_view = FindFrameView();
  if (!frame_view) {
    return false;
  }
  if (!MatchesLoadedFrame(frame_view)) {
    return false;
  }

  clay::PointerEvent child_event = event;
  child_event.position = frame_view->GetPointBySelf(event.position);
  child_event.position.Move(-frame_view->ContentInsetLeft(),
                            -frame_view->ContentInsetTop());

  const float dpr = parent_page_view_->DevicePixelRatio();
  child_event.position.Scale(dpr, dpr);
  child_event.delta.ScaleSize(dpr, dpr);
  child_event.pan.Scale(dpr, dpr);
  child_event.pan_delta.ScaleSize(dpr, dpr);
  child_event.scroll_delta_x *= dpr;
  child_event.scroll_delta_y *= dpr;
  return child_page_view_->DispatchPointerEvent({child_event});
}

bool FrameChildPageHost::ForwardKeyEvent(const clay::KeyEvent* event) {
  if (!event || !child_page_view_) {
    return false;
  }
  if (!MatchesLoadedFrame(FindFrameView())) {
    return false;
  }

  bool handled = false;
  child_page_view_->DispatchKeyEvent(
      std::make_unique<clay::KeyEvent>(
          event->GetTimestamp(), event->GetType(), event->GetPhysical(),
          event->GetLogical(), event->GetSynthesized(), event->GetCharacter()),
      [&handled](bool result) { handled = result; });
  return handled;
}

void FrameChildPageHost::ScheduleFrame() {
  if (!TryProduceInitialChildFrameNow()) {
    ScheduleParentFrame(false);
  }
}

void FrameChildPageHost::ForceBeginFrame() {
  if (!TryProduceInitialChildFrameNow()) {
    ScheduleParentFrame(true);
  }
}

void FrameChildPageHost::OnFirstMeaningfulLayout() {
  child_first_meaningful_layout_ = true;
  if (!TryProduceInitialChildFrameNow()) {
    ScheduleParentFrame(true);
  }
}

void FrameChildPageHost::ScheduleLayout() {
  pending_layout_ = true;
  if (!TryProduceInitialChildFrameNow()) {
    ScheduleParentFrame(false);
  }
}

bool FrameChildPageHost::BeginScheduledFrame(const clay::FrameTickInfo& tick,
                                             bool forced) {
  return BeginChildFrame(tick, forced || tick.parent_forced);
}

void FrameChildPageHost::ScheduleParentFrame(bool forced) {
  if (destroyed_ || !parent_page_view_) {
    return;
  }
  if (!HasRunnableViewport(FindFrameView())) {
    return;
  }
  parent_page_view_->ScheduleChildFrame(this, forced);
}

void FrameChildPageHost::RegisterDrawableFirstFrameBridge(int64_t image_id) {
  if (!parent_page_view_ || image_id < 0) {
    return;
  }
  if (!bridged_drawable_image_ids_.insert(image_id).second) {
    return;
  }

  parent_page_view_->RegisterFirstFrameAvailable(
      image_id, [weak_self = weak_factory_.GetWeakPtr(), image_id] {
        if (!weak_self || weak_self->destroyed_ ||
            !weak_self->child_page_view_) {
          return;
        }

        weak_self->dispatching_drawable_first_frame_bridge_ids_.insert(
            image_id);
        const bool handled =
            weak_self->child_page_view_->MarkDrawableImageFrameAvailable(
                image_id);
        weak_self->dispatching_drawable_first_frame_bridge_ids_.erase(
            image_id);
        const bool pending_unregister =
            weak_self->pending_unbridged_drawable_image_ids_.erase(image_id) >
            0;

        if (handled) {
          weak_self->ScheduleParentFrame(true);
        }
        if (handled || pending_unregister) {
          auto task_runner = weak_self->parent_page_view_->GetTaskRunner();
          task_runner->PostTask(
              [weak_self, image_id] {
                if (weak_self && !weak_self->destroyed_) {
                  weak_self->UnregisterDrawableFirstFrameBridge(image_id);
                }
              });
        }
      });
}

void FrameChildPageHost::UnregisterDrawableFirstFrameBridge(int64_t image_id) {
  if (image_id < 0) {
    return;
  }
  if (dispatching_drawable_first_frame_bridge_ids_.find(image_id) !=
      dispatching_drawable_first_frame_bridge_ids_.end()) {
    pending_unbridged_drawable_image_ids_.insert(image_id);
    return;
  }
  if (parent_page_view_) {
    parent_page_view_->UnRegisterFirstFrameAvailable(image_id);
  }
  bridged_drawable_image_ids_.erase(image_id);
  pending_unbridged_drawable_image_ids_.erase(image_id);
}

void FrameChildPageHost::UnregisterDrawableFirstFrameBridges() {
  if (parent_page_view_) {
    for (int64_t image_id : bridged_drawable_image_ids_) {
      parent_page_view_->UnRegisterFirstFrameAvailable(image_id);
    }
  }
  bridged_drawable_image_ids_.clear();
  dispatching_drawable_first_frame_bridge_ids_.clear();
  pending_unbridged_drawable_image_ids_.clear();
}

bool FrameChildPageHost::BeginChildFrame(const clay::FrameTickInfo& tick,
                                         bool forced) {
  if (inside_begin_frame_) {
    ScheduleParentFrame(forced);
    return false;
  }
  if (!child_page_view_) {
    return false;
  }
  auto* frame_view = FindFrameView();
  if (!frame_view) {
    return false;
  }
  if (!MatchesLoadedFrame(frame_view)) {
    return false;
  }
  UpdateViewport(frame_view);
  if (!HasRunnableViewport(frame_view)) {
    return false;
  }

  if (pending_layout_ && child_shadow_node_owner_) {
    child_shadow_node_owner_->TriggerLayout();
    pending_layout_ = false;
  }

  auto recorder = std::make_unique<clay::FrameTimingsRecorder>();
  recorder->RecordVsync(tick.vsync_start, tick.vsync_target);
  recorder->RecordForced(forced);
  if (tick.vsync_sequence_id != -1) {
    recorder->RecordVsyncSequenceId(tick.vsync_sequence_id);
  }
  recorder->RecordBuildStart(fml::TimePoint::Now());

  inside_begin_frame_ = true;
  const bool did_build = child_page_view_->BeginFrame(std::move(recorder));
  inside_begin_frame_ = false;
  return did_build;
}

bool FrameChildPageHost::TryProduceInitialChildFrameNow() {
  if (destroyed_ || !parent_page_view_ || !child_page_view_ ||
      !child_first_meaningful_layout_ || has_submitted_child_surface_ ||
      inside_begin_frame_ || producing_initial_child_frame_) {
    return false;
  }
  if (!HasRunnableViewport(FindFrameView())) {
    return false;
  }

  const auto now = fml::TimePoint::Now();
  clay::FrameTickInfo tick;
  tick.vsync_start = now;
  tick.vsync_target = now;
  tick.parent_forced = true;

  producing_initial_child_frame_ = true;
  BeginChildFrame(tick, true);
  producing_initial_child_frame_ = false;

  if (!has_submitted_child_surface_) {
    return false;
  }

  parent_page_view_->CancelChildFrame(this);
  parent_page_view_->RequestPaint();
  return true;
}

bool FrameChildPageHost::Raster(
    std::unique_ptr<clay::LayerTree> layer_tree,
    std::unique_ptr<clay::FrameTimingsRecorder> recorder, bool force) {
  auto* frame_view = FindFrameView();
  if (!frame_view || !layer_tree) {
    return false;
  }
  if (!MatchesLoadedFrame(frame_view)) {
    return false;
  }
  if (!HasRunnableViewport(frame_view)) {
    return false;
  }
  auto shared_layer_tree =
      std::shared_ptr<clay::LayerTree>(std::move(layer_tree));
  clay::FrameDamage frame_damage;
  frame_damage.SetPreviousLayerTree(previous_child_layer_tree_.get());
  frame_damage.ComputeClipRect(*shared_layer_tree, false);
  // Submit child-local frame damage to the parent. The parent maps this rect
  // into its own damage space, while the root framebuffer's buffer damage still
  // handles double/triple-buffer accumulation.
  std::optional<skity::Rect> damage_rect = frame_damage.GetFrameDamage();
  if (!frame_view->SubmitChildLayerTree(shared_layer_tree, damage_rect)) {
    return false;
  }
  has_submitted_child_surface_ = true;
  previous_child_layer_tree_ = std::move(shared_layer_tree);
  return true;
}

void FrameChildPageHost::ShowSoftInput(int type, int action) {}

void FrameChildPageHost::HideSoftInput() {}

std::string FrameChildPageHost::ShouldInterceptUrl(
    const std::string& origin_url, bool should_decode) {
  return parent_page_view_
             ? parent_page_view_->ShouldInterceptUrl(origin_url, should_decode)
             : std::string();
}

std::shared_ptr<clay::ResourceLoaderIntercept>
FrameChildPageHost::GetResourceLoaderIntercept() {
  return parent_page_view_ ? parent_page_view_->GetResourceLoaderIntercept()
                           : nullptr;
}

void FrameChildPageHost::MakeRasterSnapshot(
    std::unique_ptr<clay::LayerTree> layer_tree,
    std::function<void(fml::RefPtr<clay::PaintImage>)> callback) {
  if (callback) {
    callback(nullptr);
  }
}

fml::RefPtr<clay::PaintImage> FrameChildPageHost::MakeRasterSnapshot(
    clay::GrPicturePtr picture, skity::Vec2 picture_size) {
  return parent_page_view_ ? parent_page_view_->MakeRasterSnapshot(
                                 std::move(picture), picture_size)
                           : nullptr;
}

void FrameChildPageHost::SetClipboardData(const std::u16string& data) {
  if (parent_page_view_) {
    parent_page_view_->SetClipboardData(data);
  }
}

std::u16string FrameChildPageHost::GetClipboardData() {
  return parent_page_view_ ? parent_page_view_->GetClipboardData()
                           : std::u16string();
}

#if defined(OS_WIN) || defined(OS_MAC) || defined(ENABLE_HEADLESS)
void FrameChildPageHost::SetTextInputClient(int client_id,
                                            const char* input_action,
                                            const char* input_type) {
  text_input_active_ = true;
  RegisterTextInputBridge(client_id);
  if (auto* frame_view = FindFrameView()) {
    if (!frame_view->GetFocusable()) {
      frame_view->SetFocusable(true);
      frame_focus_enabled_by_child_ = true;
    }
    frame_view->RequestFocus();
  }
  if (parent_page_view_) {
    parent_page_view_->SetTextInputClient(client_id, input_action, input_type);
  }
}
void FrameChildPageHost::ClearTextInputClient() { ClearTextInputIfNeeded(); }
void FrameChildPageHost::SetEditableTransform(
    const float transform_matrix[16]) {
  if (!parent_page_view_) {
    return;
  }
  float mapped_transform[16];
  std::copy(transform_matrix, transform_matrix + 16, mapped_transform);
  const auto origin = FrameContentOriginInParent();
  mapped_transform[12] += origin.x();
  mapped_transform[13] += origin.y();
  parent_page_view_->SetEditableTransform(mapped_transform);
}
void FrameChildPageHost::SetEditingState(uint64_t selection_base,
                                         uint64_t composing_extent,
                                         const std::string& selection_affinity,
                                         const std::string& text,
                                         bool selection_directional,
                                         uint64_t selection_extent,
                                         uint64_t composing_base) {
  if (parent_page_view_) {
    parent_page_view_->SetEditingState(
        selection_base, composing_extent, selection_affinity, text,
        selection_directional, selection_extent, composing_base);
  }
}
void FrameChildPageHost::SetCaretRect(float x, float y, float width,
                                      float height) {
  if (!parent_page_view_) {
    return;
  }
  float mapped_x = 0.f;
  float mapped_y = 0.f;
  float mapped_width = 0.f;
  float mapped_height = 0.f;
  MapChildRectToParent(x, y, width, height, &mapped_x, &mapped_y, &mapped_width,
                       &mapped_height);
  parent_page_view_->SetCaretRect(mapped_x, mapped_y, mapped_width,
                                  mapped_height);
}
void FrameChildPageHost::UpdateCaretPosition(float x, float y, float width,
                                             float height) {
  if (!parent_page_view_) {
    return;
  }
  float mapped_x = 0.f;
  float mapped_y = 0.f;
  float mapped_width = 0.f;
  float mapped_height = 0.f;
  MapChildRectToParent(x, y, width, height, &mapped_x, &mapped_y, &mapped_width,
                       &mapped_height);
  parent_page_view_->UpdateCaretPosition(mapped_x, mapped_y, mapped_width,
                                         mapped_height);
}
void FrameChildPageHost::setMarkedTextRect(float x, float y, float width,
                                           float height) {
  if (!parent_page_view_) {
    return;
  }
  float mapped_x = 0.f;
  float mapped_y = 0.f;
  float mapped_width = 0.f;
  float mapped_height = 0.f;
  MapChildRectToParent(x, y, width, height, &mapped_x, &mapped_y, &mapped_width,
                       &mapped_height);
  parent_page_view_->setMarkedTextRect(mapped_x, mapped_y, mapped_width,
                                       mapped_height);
}
void FrameChildPageHost::ShowTextInput() {
  if (parent_page_view_) {
    parent_page_view_->ShowTextInput();
  }
}
void FrameChildPageHost::HideTextInput() {
  if (parent_page_view_) {
    parent_page_view_->HideTextInput();
  }
}
void FrameChildPageHost::SetCursorPosition(int position) {
  if (parent_page_view_) {
    parent_page_view_->SetCursorPosition(position);
  }
}
void FrameChildPageHost::WindowMove() {
  if (parent_page_view_) {
    parent_page_view_->MoveWindow();
  }
}
void FrameChildPageHost::ActivateSystemCursor(int type,
                                              const std::string& path) {
  if (parent_page_view_) {
    parent_page_view_->ActivateSystemCursor(type, path);
  }
}
#endif

void FrameChildPageHost::FilterInputAsync(
    const std::string& input, const std::string& pattern,
    std::function<void(const std::string&)> callback) {
  if (parent_page_view_) {
    parent_page_view_->FilterInputAsync(input, pattern, std::move(callback));
  } else if (callback) {
    callback(input);
  }
}

void FrameChildPageHost::ReportTiming(
    const std::unordered_map<std::string, int64_t>& timing,
    const std::string& flag) {
  if (parent_page_view_) {
    parent_page_view_->ReportTiming(timing, flag);
  }
}

clay::BaseView* FrameChildPageHost::FindViewById(int view_id) {
  return child_view_context_ ? child_view_context_->FindViewByViewId(view_id)
                             : nullptr;
}

clay::ShadowNode* FrameChildPageHost::FindShadowNodeById(int node_id) {
  return child_view_context_
             ? child_view_context_->FindShadowNodeByNodeId(node_id)
             : nullptr;
}

void FrameChildPageHost::UpdateRootSize(int32_t width, int32_t height) {
  // Root size is child layout output. Feed auto axes to parent measurement
  // instead of turning the result into a new definite child viewport.
  auto* frame_view = FindFrameView();
  if (frame_view && MatchesLoadedFrame(frame_view)) {
    frame_view->SetIntrinsicContentSize(width, height);
  }
}

void FrameChildPageHost::RegisterDrawableImage(
    std::shared_ptr<clay::DrawableImage> drawable_image) {
  if (parent_page_view_ && drawable_image) {
    parent_page_view_->RegisterDrawableImage(std::move(drawable_image));
  }
}

void FrameChildPageHost::UnregisterDrawableImage(int64_t id) {
  UnregisterDrawableFirstFrameBridge(id);
  if (parent_page_view_) {
    parent_page_view_->UnregisterDrawableImage(id);
  }
}

void FrameChildPageHost::RegisterDrawableImageFirstFrameAvailable(
    int64_t image_id) {
  RegisterDrawableFirstFrameBridge(image_id);
}

void FrameChildPageHost::UnregisterDrawableImageFirstFrameAvailable(
    int64_t image_id) {
  UnregisterDrawableFirstFrameBridge(image_id);
}

clay::FrameView* FrameChildPageHost::FindFrameView() const {
  if (!parent_view_context_ || frame_view_id_ == 0) {
    return nullptr;
  }
  auto* view = parent_view_context_->FindViewByViewId(frame_view_id_);
  if (!view || !view->Is<clay::FrameView>()) {
    return nullptr;
  }
  return static_cast<clay::FrameView*>(view);
}

bool FrameChildPageHost::MatchesLoadedFrame(
    const clay::FrameView* frame_view) const {
  return frame_view && frame_view->id() == frame_view_id_ &&
         frame_view->url() == loaded_url_;
}

bool FrameChildPageHost::HasRunnableViewport(
    const clay::FrameView* frame_view) const {
  if (!frame_view || !MatchesLoadedFrame(frame_view) || !parent_page_view_ ||
      !parent_page_view_->Visible() || !frame_view->Visible()) {
    return false;
  }
  const auto viewport_size = frame_view->child_viewport_size();
  return viewport_size.width() > 0.f && viewport_size.height() > 0.f;
}

#if defined(OS_WIN) || defined(OS_MAC) || defined(ENABLE_HEADLESS)
clay::FloatPoint FrameChildPageHost::FrameContentOriginInParent() const {
  auto* frame_view = FindFrameView();
  if (!frame_view) {
    return {};
  }
  auto origin = frame_view->AbsoluteLocationWithScroll();
  origin.Move(frame_view->ContentInsetLeft(), frame_view->ContentInsetTop());
  return origin;
}

void FrameChildPageHost::MapChildRectToParent(float x, float y, float width,
                                              float height, float* mapped_x,
                                              float* mapped_y,
                                              float* mapped_width,
                                              float* mapped_height) const {
  const auto origin = FrameContentOriginInParent();
  if (mapped_x) {
    *mapped_x = origin.x() + x;
  }
  if (mapped_y) {
    *mapped_y = origin.y() + y;
  }
  if (mapped_width) {
    *mapped_width = width;
  }
  if (mapped_height) {
    *mapped_height = height;
  }
}

void FrameChildPageHost::RegisterTextInputBridge(int client_id) {
  if (!parent_page_view_) {
    return;
  }
  if (bridged_text_input_client_id_ &&
      *bridged_text_input_client_id_ == client_id) {
    return;
  }
  RemoveTextInputBridge();

  clay::InputClientManager::TextInputCallback callback;
  callback.on_update_edit_state =
      [this, client_id](uint64_t selection_base, uint64_t composing_extent,
                        const char* selection_affinity, const char* text,
                        uint64_t selection_extent, uint64_t composing_base) {
        if (destroyed_ || !child_page_view_ || !bridged_text_input_client_id_ ||
            *bridged_text_input_client_id_ != client_id) {
          return;
        }
        child_page_view_->GetInputClientManager()->InvokeUpdateEditState(
            client_id, selection_base, composing_extent, selection_affinity,
            text, selection_extent, composing_base);
      };
  callback.on_perform_action = [this, client_id]() {
    if (destroyed_ || !child_page_view_ || !bridged_text_input_client_id_ ||
        *bridged_text_input_client_id_ != client_id) {
      return;
    }
    child_page_view_->GetInputClientManager()->InvokePerformAction(client_id);
  };
  parent_page_view_->GetInputClientManager()->AddClientCallback(client_id,
                                                                callback);
  bridged_text_input_client_id_ = client_id;
}

void FrameChildPageHost::RemoveTextInputBridge() {
  if (!bridged_text_input_client_id_) {
    return;
  }
  if (parent_page_view_) {
    parent_page_view_->GetInputClientManager()->RemoveClientCallback(
        *bridged_text_input_client_id_);
  }
  bridged_text_input_client_id_.reset();
}

void FrameChildPageHost::ClearTextInputIfNeeded() {
  if (!text_input_active_) {
    RemoveTextInputBridge();
    return;
  }
  text_input_active_ = false;
  RemoveTextInputBridge();
  if (parent_page_view_) {
    parent_page_view_->ClearTextInputClient();
    parent_page_view_->HideTextInput();
  }
  if (frame_focus_enabled_by_child_) {
    if (auto* frame_view = FindFrameView()) {
      frame_view->ClearFocus();
      frame_view->SetFocusable(false);
    }
    frame_focus_enabled_by_child_ = false;
  }
}
#endif

}  // namespace tasm
}  // namespace lynx
