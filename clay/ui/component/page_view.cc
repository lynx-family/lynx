// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/page_view.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/include/fml/macros.h"
#include "base/trace/native/trace_event.h"
#include "clay/common/graphics/screenshot.h"
#include "clay/flow/frame_timings.h"
#include "clay/flow/layers/layer_tree.h"
#include "clay/fml/logging.h"
#include "clay/gfx/animation/animation_handler.h"
#include "clay/gfx/animation/keyframe.h"
#include "clay/gfx/animation/keyframe_set.h"
#include "clay/gfx/geometry/box_shadow_operations.h"
#include "clay/gfx/geometry/box_shadow_value.h"
#include "clay/gfx/geometry/filter_operations.h"
#include "clay/gfx/geometry/filter_value.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/graphics_context.h"
#include "clay/gfx/image/image_data_cache.h"
#include "clay/gfx/image/image_upload_manager.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/net/loader/resource_loader_factory.h"
#include "clay/net/loader/resource_loader_intercept.h"
#include "clay/public/clay.h"
#include "clay/public/timing_collector_delegate.h"
#include "clay/shell/common/services/raster_frame_service.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/common/utils/watch_dog.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/image_view.h"
#include "clay/ui/component/inline_image_view.h"
#include "clay/ui/component/intersection_observer.h"
#include "clay/ui/component/native_view.h"
#include "clay/ui/component/nested_scroll/nested_scroll_manager.h"
#include "clay/ui/component/view_context.h"
#include "clay/ui/event/event_utils.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/event/key_code_converter.h"
#include "clay/ui/gesture/gesture_manager.h"
#include "clay/ui/gesture/long_press_gesture_recognizer.h"
#include "clay/ui/gesture/mouse_cursor_manager.h"
#include "clay/ui/gesture/tap_gesture_recognizer.h"
#include "clay/ui/painter/painting_context.h"
#include "clay/ui/rendering/render_page.h"
#include "clay/ui/resource/image_resource_fetcher.h"
#include "clay/version/version.h"

#ifdef ENABLE_SCREENSHOT_SERVICE
#include "clay/shell/common/services/screenshot_service.h"
#endif

namespace clay {
namespace {

bool ShouldIgnoreFocus(BaseView* view) {
  if (view == nullptr) {
    return false;
  } else if (view->IgnoreFocus().has_value()) {
    return *view->IgnoreFocus();
  } else if (view->Parent() != nullptr) {
    return ShouldIgnoreFocus(view->Parent());
  } else {
    return false;
  }
}

clay::Value CreateExposeArray(
    ExposureDataList& data_arr,
    std::vector<fml::WeakPtr<BaseView>>& data_set_arr) {
  clay::Value::Array wrapper_array(data_arr.size());
  for (size_t i = 0; i < data_arr.size() && i < data_set_arr.size(); i++) {
    auto& exposure_event = *data_arr[i];
    BaseView* data_set_owner = data_set_arr[i].get();
    if (data_set_owner) {
      const auto& data_set = data_set_owner->GetDataSet();
      exposure_event["dataset"] = CloneClayValue(data_set);
      exposure_event["dataSet"] = CloneClayValue(data_set);
    }
    wrapper_array[i] = clay::Value(std::move(exposure_event));
  }
  data_arr.clear();
  data_set_arr.clear();
  return clay::Value(std::move(wrapper_array));
}

ClayEventType ToClayEventType(PointerEvent::EventType event_type,
                              PointerEvent::DeviceType device) {
  if (device == PointerEvent::DeviceType::kTouch) {
    switch (event_type) {
      case PointerEvent::EventType::kSignalEvent:
        return kClayEventTypeWheel;
      case PointerEvent::EventType::kUnkownEvent:
        return kClayEventTypeUnknown;
      case PointerEvent::EventType::kDownEvent:
        return kClayEventTypeTouchStart;
      case PointerEvent::EventType::kUpEvent:
        return kClayEventTypeTouchEnd;
      case PointerEvent::EventType::kMoveEvent:
        return kClayEventTypeTouchMove;
      case PointerEvent::EventType::kCancel:
        return kClayEventTypeTouchCancel;
      case PointerEvent::EventType::kHoverEvent:
        // TODO(yangliu): report hover event to lynx
        return kClayEventTypeUnknown;
      case PointerEvent::EventType::kPanZoomStartEvent:
        return kClayEventTypeUnknown;
      case PointerEvent::EventType::kPanZoomUpdateEvent:
        return kClayEventTypeWheel;
      case PointerEvent::EventType::kPanZoomEndEvent:
        return kClayEventTypeUnknown;
    }
  } else if (device == PointerEvent::DeviceType::kTrackpad) {
    switch (event_type) {
      case PointerEvent::EventType::kPanZoomStartEvent:
      case PointerEvent::EventType::kPanZoomUpdateEvent:
      case PointerEvent::EventType::kPanZoomEndEvent:
        return kClayEventTypePanZoom;
      default:
        return kClayEventTypeUnknown;
    }
  } else {  // mouse
    switch (event_type) {
      case PointerEvent::EventType::kSignalEvent:
        return kClayEventTypeWheel;
      case PointerEvent::EventType::kUnkownEvent:
        return kClayEventTypeUnknown;
      case PointerEvent::EventType::kDownEvent:
        return kClayEventTypeMouseDown;
      case PointerEvent::EventType::kUpEvent:
        return kClayEventTypeMouseUp;
      case PointerEvent::EventType::kMoveEvent:
        return kClayEventTypeMouseMove;
      case PointerEvent::EventType::kCancel:
        return kClayEventTypeUnknown;
      case PointerEvent::EventType::kHoverEvent:
        // TODO(yangliu): report hover event to lynx
        return kClayEventTypeUnknown;
      default:
        return kClayEventTypeUnknown;
    }
  }
  return kClayEventTypeUnknown;
}

ClayEventType ToClayEventType(const PointerEvent& event) {
  return ToClayEventType(event.type, event.device);
}

ClayEventType ToClayEventType(KeyEventType type) {
  if (type == KeyEventType::kDown || type == KeyEventType::kRepeat) {
    return kClayEventTypeKeyDown;
  } else if (type == KeyEventType::kUp) {
    return kClayEventTypeKeyUp;
  }
  return kClayEventTypeUnknown;
}

static constexpr int64_t kEventStateUpdateDelayTime = 1000;
static constexpr int64_t kEventStateUpdateIntervalTime = 100;
}  // namespace

#define DEBUG_KEYFRAMES 0

PageView::PageView(uint32_t id, fml::RefPtr<GPUUnrefQueue> unref_queue,
                   fml::RefPtr<fml::TaskRunner> ui_task_runner)
    : PageView(
          id,
          clay::ServiceManager::Create(
              {ui_task_runner ? ui_task_runner
                              : fml::MessageLoop::GetCurrent().GetTaskRunner(),
               ui_task_runner ? ui_task_runner
                              : fml::MessageLoop::GetCurrent().GetTaskRunner(),
               nullptr, nullptr}),
          unref_queue,
          clay::TaskRunners("", ui_task_runner, nullptr, ui_task_runner,
                            nullptr)) {}

PageView::PageView(uint32_t id, std::shared_ptr<ServiceManager> service_manager,
                   fml::RefPtr<GPUUnrefQueue> unref_queue,
                   clay::TaskRunners task_runners)
    : BaseView(id, "page", std::make_unique<RenderPage>(), this),
      task_runners_(std::move(task_runners)),
      gesture_manager_(std::make_unique<GestureManager>(
          task_runners_.GetUITaskRunner(), service_manager)),
      nested_scroll_manager_(std::make_unique<NestedScrollManager>(this)),
#if defined(OS_MAC) || defined(OS_WIN) || defined(ENABLE_HEADLESS)
      mouse_region_manager_(std::make_unique<MouseRegionManager>()),
#endif
      isolated_gesture_detector_(task_runners_.GetUITaskRunner()),
      focus_manager_(this),
      keyboard_bridge_(this),
      unref_queue_(unref_queue),
      service_manager_(service_manager),
      page_unique_id_(render_object()->element_id().unique_id()) {
  SetupIsolatedGestures();
  frame_builder_ = std::make_unique<FrameBuilder>(
      skity::Vec2{static_cast<int32_t>(metrics_.physical_width),
                  static_cast<int32_t>(metrics_.physical_height)},
      metrics_.device_pixel_ratio, unref_queue_);
  animation_handler_ = std::make_unique<AnimationHandler>();
  animation_handler_->SetOnNewAnimationCallback([this] { RequestNewFrame(); });
  renderer_ = std::make_unique<Renderer>(this, unref_queue_);
  renderer_->SetRoot(render_object_.get());

  InitManagers();

  layout_controller_ = std::make_unique<LayoutController>();

  attach_to_tree_ = true;

#ifdef ENABLE_ACCESSIBILITY
  accessibility_element_ = true;
  semantics_owner_ = std::make_unique<SemanticsOwner>(
      [weak = GetWeakPtr()](const SemanticsUpdateNodes& update_nodes) {
        if (weak) {
          auto page = static_cast<PageView*>(weak.get());
          page->SendUpdatedSemantics(update_nodes);
        }
      });
#endif

  FML_DLOG(INFO) << "PageView construction; id = " << id;
  FML_DLOG(INFO) << "Clay build version=" << clay::GetBuildVersion()
                 << " number=" << clay::GetBuildNumber();
}

PageView::~PageView() {
  FML_DLOG(INFO) << "PageView destruction; id = " << id_;
  UnRegisterUploadTask();
}

void PageView::OnDestroy() {
  FML_DLOG(INFO) << "Page OnDestroy; id = " << id_;
  animation_handler_->ClearCallbacks();
  DestroyAllChildren();
  image_resource_fetcher_ = nullptr;
  exposure_event_arr_.clear();
  disexposure_event_arr_.clear();
}

void PageView::CleanForRecycle() { ResetPageView(true); }

bool PageView::ShouldIgnoreFocusChange(const PointerEvent& event) {
  FloatPoint relative_pos;
  auto target = GetTopViewToAcceptEvent(event.position, &relative_pos);
  if (target == nullptr) {
    return false;
  }
  return ShouldIgnoreFocus(target);
}

void PageView::InitManagers() {
  // init focus manager
  focus_manager_.SetIsRootScope();
  gesture_manager_->SetListenerForNotCaredPointer(
      [this](const PointerEvent& event) {
        if (ShouldIgnoreFocusChange(event)) {
          return;
        }

        auto* focus_node = GetFocusManager()->GetLeafFocusedNode();
        if (focus_node) {
          focus_node->ClearFocus();
        }
      });

#if (defined(OS_MAC) || defined(OS_WIN)) || defined(ENABLE_HEADLESS)
  // init mouse region manager, mouse cursor manager
  MouseCursorManager::ActiveCursorCallback cursor_callback =
      [weak_view = weak_factory_.GetWeakPtr()](const Cursor& cursor) {
        PageView* page_view = static_cast<PageView*>(weak_view.get());

        if (!page_view) {
          FML_DCHECK(false)
              << "PageView : ActiveCursorCallback is called after "
                 "pageView released";
          return;
        }

        page_view->ActivateSystemCursor(static_cast<int>(cursor.type),
                                        cursor.val);
      };
  mouse_region_manager_->InitSubManager(cursor_callback);
  SetCursor({"default"});
#endif
}

void PageView::TriggerFirstPaintCallback() {
  if (!first_paint_ && event_delegate_) {
    first_paint_ = true;
    event_delegate_->OnFirstMeaningfulPaint();
  }
}

void PageView::OnPlatformViewCreated() {}

void PageView::OnOutputSurfaceCreated() {
  if (perf_collector_) {
    perf_collector_->InsertRecord(clay::Perf::kErrorCode, kPerfErrorCodeOK);
  }
  Isolate::Instance().GetResourceCache()->SetIsLowMemory(false);
  // Repaint page when LynxView enter foreground.
  // Must mark dirty to pass dirty_nodes check in BeginFrame
  RequestPaintBase();
}

void PageView::OnOutputSurfaceCreateFailed() {
  FML_LOG(ERROR) << "OnOutputSurfaceCreateFailed!";
  // Insert error code with empty perf record and trigger first perf.
  if (perf_collector_) {
    perf_collector_->InsertRecord(clay::Perf::kErrorCode,
                                  kPerfErrorCodeSurfaceFailed);
  }
}

void PageView::OnOutputSurfaceDestroyed() {
  // GpuResourceCache is unused now.
  // Isolate::Instance().GetResourceCache()->ClearCache();
  render_object()->MarkSubtreeDirty();
  frame_builder_->Reset();
}

void PageView::OnFirstMeaningfulLayout() {
  first_meaningful_layout_ = true;
  render_delegate_->OnFirstMeaningfulLayout();
}

bool PageView::BeginFrame(
    std::unique_ptr<clay::FrameTimingsRecorder> recorder) {
  if (!render_delegate_) {
    return false;
  }

  TRACE_EVENT("clay", "Clay::BeginFrame");
  auto target_time = recorder->GetVsyncTargetTime();
  // Ignore frame if `physical_size_` is empty. The `physical_size_` is set in
  // `SetViewportMetrics`.
  if (physical_size_.IsEmpty() || (width_ <= 0 || height_ <= 0)) {
    FML_DLOG(WARNING) << "PageView::BeginFrame has no size";
    return false;
  }

  if (!Visible()) {
    FML_DLOG(WARNING) << "PageView::BeginFrame no visible";
    return false;
  }

  // We only do the actual rendering when the first meaningful layout has been
  // received. Now the scheduler with state machine can already dispatch
  // `BeginFrame` based on `MeaningfulLayoutState`, but the calling of
  // `ForceBeginFrame` that outside the state machine need to depend on
  // `first_meaningful_layout_`.
  if (!first_meaningful_layout_) {
    FML_DLOG(WARNING) << "PageView::BeginFrame no meaningful layout";
    return false;
  }

  TriggerFirstPaintCallback();
  render_phase_ = RenderPhase::kLayout;
  if (perf_collector_ && !perf_collector_->IsRecordingFirstFramePerf()) {
    layout_and_animation_time_.Start();
  }

  // TODO(jinsong): Temporarily remove the engine old layer and rebuild it
  // always.
  frame_builder_->Reset();
  {
    TRACE_EVENT("clay", "Clay::Layout");
    recorder->RecordFrameTime(FrameTimingKey::kLayoutStart);
    LayoutInternal();
    LayoutUpdated();
    recorder->RecordFrameTime(FrameTimingKey::kLayoutEnd);
  }

  if (animation_handler_->GetAnimationCount() > 0) {
    recorder->RecordFrameTime(FrameTimingKey::kDoAnimationStart);
    TRACE_EVENT("clay", "Clay::Animation");
    RequestNewFrame();
    animation_handler_->DoAnimationFrame(
        recorder->GetVsyncTargetTime().ToEpochDelta().ToMilliseconds());
    recorder->RecordFrameTime(FrameTimingKey::kDoAnimationEnd);
  }

  if (perf_collector_ && !perf_collector_->IsRecordingFirstFramePerf()) {
    layout_and_animation_time_.Stop();
    perf_collector_->InsertLayoutAndAnimationRecord(layout_and_animation_time_);
  }

  if (render_settings_) {
    render_settings_->SetHasAnimation(animation_handler_->GetAnimationCount() >
                                      0);
  }

  if (intersection_observer_manager_) {
    intersection_observer_manager_->NotifyObservers();
  }

  GetFocusManager()->RestoreFocusSwitching();

  if (!force_raster_ && !renderer_->HasDirtyNodes()) {
    render_phase_ = RenderPhase::kIdle;
    FlushUIMethodTasks();
    FlushGapTaskIfNecessary(target_time);

#ifdef ENABLE_ACCESSIBILITY
    // We still need to update the semantics even though the UI has not changed.
    FlushSemantics();
#endif

    return false;
  }
  force_raster_ = false;

  render_phase_ = RenderPhase::kPaint;
  if (view_tree_observer_) {
    view_tree_observer_->DispatchOnPainting();
  }
  {
    TRACE_EVENT("clay", "Clay::Paint");
    recorder->RecordFrameTime(FrameTimingKey::kPaintStart);
    Paint();
    recorder->RecordFrameTime(FrameTimingKey::kPaintEnd);
  }

  render_phase_ = RenderPhase::kBuildFrame;

  {
    TRACE_EVENT("clay", "Clay::Composite");
    CompositeFrame(std::move(recorder));
  }

#ifdef ENABLE_ACCESSIBILITY
  FlushSemantics();
#endif

  render_phase_ = RenderPhase::kIdle;
  FlushUIMethodTasks();
  FlushGapTaskIfNecessary(target_time);
  SendGlobalExposureEvent();
  SendDrawEndEvent();
  return true;
}

void PageView::LayoutInternal() {
  if (perf_collector_ && perf_collector_->IsRecordingFirstFramePerf()) {
    perf_collector_->BeginRecord(Perf::kFirstLayoutCost);
  }

  layout_controller_->Layout();

  if (perf_collector_ && perf_collector_->IsRecordingFirstFramePerf()) {
    perf_collector_->EndRecord(Perf::kFirstLayoutCost);
  }
}

BaseView* PageView::FindViewByViewId(int view_id) {
  return view_context_->FindViewByViewId(view_id);
}

void PageView::SetRenderDelegate(RenderDelegate* delegate) {
  render_delegate_ = delegate;
}

void PageView::SendGlobalExposureEvent() {
  if (event_delegate_) {
    if (!exposure_event_arr_.empty()) {
      auto params =
          CreateExposeArray(exposure_event_arr_, exposure_event_data_set_);
      event_delegate_->OnSendGlobalEvent("exposure", std::move(params));
    }
    if (!disexposure_event_arr_.empty()) {
      auto params = CreateExposeArray(disexposure_event_arr_,
                                      disexposure_event_data_set_);
      event_delegate_->OnSendGlobalEvent("disexposure", std::move(params));
    }
  }
}

void PageView::CallJSIntersectionObserver(int observer_id, int callback_id,
                                          clay::Value params) {
  if (event_delegate_) {
    event_delegate_->CallJSIntersectionObserver(observer_id, callback_id,
                                                std::move(params));
  }
}

void PageView::SendDrawEndEvent() {
  if (event_delegate_) {
    event_delegate_->OnDrawEndEvent();
  }
}

void PageView::SetRenderSettings(fml::RefPtr<RenderSettings> render_settings) {
  render_settings_ = render_settings;
  render_settings_->SetHasAnimation(animation_handler_->GetAnimationCount() >
                                    0);
}

void PageView::SetViewportMetrics(const clay::ViewportMetrics& metrics) {
  if (metrics_ == metrics) {
    return;
  }

  // NOTE: Window insets are not considered in this place because they are not
  // used by now.
  bool needs_to_change_size = NeedsToChangeSize(metrics_, metrics);
  bool needs_to_change_system_params =
      NeedsToChangeSystemParameters(metrics_, metrics);
  metrics_ = metrics;
  if (!needs_to_change_size && !needs_to_change_system_params) {
    return;
  }
  if (needs_to_change_size) {
    auto physical_width = metrics_.physical_width;
    auto physical_height = metrics_.physical_height;
    auto pixel_ratio = metrics_.device_pixel_ratio;
    auto dpi = metrics_.device_density_dpi;
    frame_builder_->UpdateFrameSize(physical_width, physical_height,
                                    pixel_ratio);
    physical_size_.SetWidth(physical_width);
    physical_size_.SetHeight(physical_height);
    int logical_width = lround(physical_size_.width() / pixel_ratio);
    int logical_height = lround(physical_size_.height() / pixel_ratio);
    size_.SetWidth(logical_width);
    size_.SetHeight(logical_height);
    renderer_->SetPixelRatio(pixel_ratio);
    renderer_->SetDPI(dpi);
    if (gesture_manager_) {
      gesture_manager_->SetPixelRatio(DevicePixelRatio());
    }
    isolated_gesture_detector_.gesture_manager()->SetPixelRatio(
        DevicePixelRatio());
    renderer_->SetFrameSize(physical_size_);

    SetWidth(ConvertFrom<kPixelTypePhysical>(physical_width));
    SetHeight(ConvertFrom<kPixelTypePhysical>(physical_height));
    static_cast<RenderPage*>(render_object())
        ->SetScaleRatio(GetPixelRatio<kPixelTypeClay, kPixelTypePhysical>());
  }

  DispatchViewportMetricsUpdate();

  RequestPaintBase();
}

void PageView::UpdateRootSize(int32_t width, int32_t height) {
  fml::TaskRunner::RunNowOrPostTask(
      task_runners_.GetUITaskRunner(),
      [delegate = render_delegate_, width, height]() {
        delegate->UpdateRootSize(width, height);
      });
}

void PageView::Paint() {
  // Paint the dirty render objects and build the composited layer tree.
  if (perf_collector_ && perf_collector_->IsRecordingFirstFramePerf()) {
    perf_collector_->BeginRecord(Perf::kFirstPaintCost);
  }

  renderer_->Paint();

  if (perf_collector_ && perf_collector_->IsRecordingFirstFramePerf()) {
    perf_collector_->EndRecord(Perf::kFirstPaintCost);
  }
  ReportAfterPaint();
}

void PageView::ReportAfterPaint() {
  if (perf_collector_) {
    perf_collector_->InsertFocusChangedUntilFirstPaintFinish();
  }
}

void PageView::CompositeFrame(
    std::unique_ptr<clay::FrameTimingsRecorder> recorder) {
  // Build a frame for the composited layer tree.
  if (perf_collector_ && perf_collector_->IsRecordingFirstFramePerf()) {
    perf_collector_->BeginRecord(Perf::kFirstBuildFrameCost);
  }

  recorder->RecordFrameTime(FrameTimingKey::kBuildFrameStart);
  frame_builder_->BuildFrame(renderer_->GetLayer());
  recorder->RecordFrameTime(FrameTimingKey::kBuildFrameEnd);
  if (performance_overlay_enabled_) {
    frame_builder_->AddPerformanceOverlay(15, 0, physical_size_.width(), 0,
                                          physical_size_.height() / 5);
  }

  if (perf_collector_ && perf_collector_->IsRecordingFirstFramePerf()) {
    perf_collector_->EndRecord(Perf::kFirstBuildFrameCost);
  }

#ifndef NDEBUG
  // print rendering trees for debug styles and properties
  // DumpRenderingTrees();
#endif

  // Submit a frame to the engine.
  auto layer_tree = frame_builder_->TakeLayerTree();
  recorder->RecordVsyncTimeOnGeneration(recorder->GetVsyncStartTime());
  recorder->RecordVsyncSequenceId(recorder->GetVsyncSequenceId());

  if (layer_tree) {
    layer_tree->SetVsyncTimeOnGeneration(recorder->GetVsyncStartTime());
    layer_tree->AppendFrameTimings(recorder->TakeFrameTimings());
    if (timing_collector_delegate_ &&
        timing_collector_delegate_->HasPipelineIds()) {
      layer_tree->SetPipelineIdList(
          timing_collector_delegate_->GetPipelineIds());
      std::weak_ptr<TimingCollectorDelegate> weak_delegate =
          timing_collector_delegate_;
      layer_tree->SetPipelineEndCallback(
          [weak_delegate](const std::vector<FrameTimingItem>& timings,
                          const std::vector<std::string>& pipeline_ids) {
            if (auto delegate = weak_delegate.lock()) {
              std::vector<std::pair<std::string, uint64_t>> report_timings;
              report_timings.reserve(timings.size());

              for (const auto& timing_item : timings) {
                report_timings.emplace_back(
                    std::string(ToStringView(timing_item.key)),
                    timing_item.timestamp);
              }
              delegate->OnPipelineEnd(std::move(report_timings),
                                      std::move(pipeline_ids));
            }
          });
    }
  }
  if (!render_delegate_->Raster(std::move(layer_tree), std::move(recorder))) {
    // If the raster fails, we request a new frame to try to fix it and ensure
    // that the next raster is not skipped.
    force_raster_ = true;
    RequestNewFrame();
  }
}

#ifdef ENABLE_ACCESSIBILITY
void PageView::FlushSemantics() {
  FML_DCHECK(semantics_owner_);
  if (!IsSemanticsEnabled()) {
    return;
  }
  TRACE_EVENT("clay", "Clay::FlushSemantics");

  if (semantics_owner_->NeedRebuildSemanticsTree()) {
    // Prepare the whole semantics tree.
    semantics_owner_->semantics_nodes_to_update_descendants_.clear();
    std::vector<fml::RefPtr<SemanticsNode>> new_children;
    PrepareSemantics(nullptr, new_children, {});
    semantics_owner_->SetRebuildSemanticsTree(false);
  } else {
    // Only update the dirty nodes.
    std::unordered_set<BaseView*> node_to_process, node_process_descendants;
    node_process_descendants.swap(
        semantics_owner_->semantics_nodes_to_update_descendants_);

    for (auto* node : node_process_descendants) {
      if (node->IsAccessibilityElement()) {
        node_to_process.insert(node);
      }
      node->VisitChildren([&node_to_process](BaseView* child_view) {
        if (child_view->IsAccessibilityElement()) {
          node_to_process.insert(child_view);
        }
      });
    }
    for (auto* node : node_to_process) {
      if (node->IsAccessibilityElement()) {
        FML_DCHECK(node->GetSemantics());
        BaseView* parent_node_view =
            node->GetSemantics()->Parent()
                ? static_cast<SemanticsNode*>(node->GetSemantics()->Parent())
                      ->OwnerView()
                : nullptr;
        node->UpdateSemantics({}, parent_node_view, false);
      }
    }
  }
  semantics_owner_->SendSemanticsUpdate();
}

void PageView::SendUpdatedSemantics(const SemanticsUpdateNodes& update_nodes) {
  if (render_delegate_) {
    render_delegate_->UpdateSemantics(update_nodes);
  }
}

void PageView::PrepareSemantics(
    fml::RefPtr<SemanticsNode> parent_node,
    std::vector<fml::RefPtr<SemanticsNode>>& result,
    const std::vector<std::string>& ancestor_a11y_elements) {
  FML_DCHECK(semantics_owner_ && semantics_owner_->NeedRebuildSemanticsTree());
  FML_DCHECK(!parent_node);
  if (!semantics_) {
    semantics_ =
        fml::MakeRefCounted<SemanticsNode>(semantics_owner_.get(), this, 0);
  }
  // ResetPageView will let this semantics_ detach.
  if (!semantics_->Attached()) {
    semantics_->Attach(semantics_owner_.get());
  }
  std::vector<fml::RefPtr<SemanticsNode>> new_children;
  std::vector<std::string> a11y_elements =
      accessibility_elements_.value_or(std::vector<std::string>());
  for (auto* view : children_) {
    view->PrepareSemantics(semantics_, new_children, a11y_elements);
  }
  UpdateSemantics(new_children, nullptr, true, true);
}

void PageView::SetSemanticsEnabled(bool enabled) {
  FML_DCHECK(semantics_owner_);
  semantics_owner_->SetSemanticsEnabled(enabled);
  // Call a new frame and update the semantics node tree.
  if (enabled) {
    semantics_owner_->SetRebuildSemanticsTree(true);
    RequestNewFrameForSemantics();
  }
}

bool PageView::IsSemanticsEnabled() const {
  FML_DCHECK(semantics_owner_);
  return semantics_owner_->IsSemanticsEnabled();
}

void PageView::RequestNewFrameForSemantics() {
  FML_DCHECK(semantics_owner_);
  if (IsSemanticsEnabled()) {
    RequestNewFrame();
  }
}

void PageView::SetPageEnableAccessibilityElement(bool enabled) {
  FML_DCHECK(semantics_owner_);
  if (semantics_owner_->SetPageEnableAccessibilityElement(enabled)) {
    RequestNewFrameForSemantics();
  }
}

bool PageView::IsPageEnableAccessibilityElement() const {
  FML_DCHECK(semantics_owner_);
  return semantics_owner_->IsPageEnableAccessibilityElement();
}

void PageView::HandleA11yTapEvent(BaseView* view) {
  FloatRect local_bounds = GetBounds();
  FloatRect global_bounds = BoundsRelativeTo(nullptr);
  if (event_delegate_) {
    event_delegate_->OnTouchEvent(
        "tap", view->id(), local_bounds.Center().x(), local_bounds.Center().y(),
        global_bounds.Center().x(), global_bounds.Center().y());
  }

  OnA11yTap();
}

void PageView::HandleA11yLongPressEvent(BaseView* view) {
  FloatRect local_bounds = GetBounds();
  FloatRect global_bounds = BoundsRelativeTo(nullptr);

  if (event_delegate_) {
    event_delegate_->OnTouchEvent(
        "longpress", view->id(), local_bounds.Center().x(),
        local_bounds.Center().y(), global_bounds.Center().x(),
        global_bounds.Center().y());
  }

  OnA11yLongPress();
}

void PageView::DispatchSemanticsAction(int virtual_view_id, int action) {
  SemanticsNode::SemanticsAction semantics_action =
      static_cast<SemanticsNode::SemanticsAction>(action);
  BaseView* view = semantics_owner_->GetViewFromId(virtual_view_id);
  // If PageView just reset, this view may be nullptr;
  if (!view) {
    return;
  }
  if (!view->IsAccessibilityElement()) {
    FML_DLOG(ERROR)
        << "View: " << view->GetName()
        << " is not accessibility element. But dispatch semantics action: "
        << action;
    return;
  }

  switch (semantics_action) {
    case SemanticsNode::SemanticsAction::kTap: {
      HandleA11yTapEvent(view);
      break;
    }
    case SemanticsNode::SemanticsAction::kLongPress: {
      HandleA11yLongPressEvent(view);
      break;
    }
    case SemanticsNode::SemanticsAction::kShowOnScreen: {
      view->ScrollToMiddle(view);
      break;
    }
    default:
      FML_DLOG(ERROR) << "Unsupported semantics action: " << action;
      break;
  }
}

void PageView::EnsureSemanticsOwner() {
  if (!semantics_owner_) {
    semantics_owner_ = std::make_unique<SemanticsOwner>(
        [weak = GetWeakPtr()](const SemanticsUpdateNodes& update_nodes) {
          if (weak) {
            auto page = static_cast<PageView*>(weak.get());
            page->SendUpdatedSemantics(update_nodes);
          }
        });
  }
}
#endif

bool PageView::DispatchPointerEvent(std::vector<PointerEvent> events) {
  // The events data is in physical pixels, we need to convert them to clay
  // pixels.
  for (PointerEvent& event : events) {
    event.position = ConvertFrom<kPixelTypePhysical>(event.position);
    event.delta = ConvertFrom<kPixelTypePhysical>(event.delta);
    event.pan = ConvertFrom<kPixelTypePhysical>(event.pan);
    event.pan_delta = ConvertFrom<kPixelTypePhysical>(event.pan_delta);
    event.scroll_delta_x =
        ConvertFrom<kPixelTypePhysical>(event.scroll_delta_x);
    event.scroll_delta_y =
        ConvertFrom<kPixelTypePhysical>(event.scroll_delta_y);
  }

  bool consumed = gesture_manager_->HandlePointerEvents(this, events);
#if (defined(OS_MAC) || defined(OS_WIN)) || defined(ENABLE_HEADLESS)
  mouse_region_manager_->HandleEvents(this, events);
#endif

  // For Lynx event&gesture report
  if (consumed) {
    // if not consumed by clay elements, it should not be consumed by lynx as
    // well.
    isolated_gesture_detector_.DispatchPointerEvent(
        events, gesture_manager_->GetHitTestResponsiveResult());
    ReportTopViewRawEvents(events);
  } else {
#if defined(_WIN32) || defined(OS_OSX)
    for (PointerEvent& event : events) {
      if (event.type == PointerEvent::EventType::kHoverEvent) {
        ReportTopViewEvent(event);
      }
    }
#endif
  }

  if (render_settings_) {
    render_settings_->SetIsTouching(true);
    if (!event_watch_dog_) {
      event_watch_dog_ = std::make_unique<WatchDog>(GetTaskRunner());
    }

    if (event_watch_dog_->IsAlive()) {
      event_watch_dog_->FeedDog();
    } else {
      event_watch_dog_->Start(kEventStateUpdateDelayTime,
                              kEventStateUpdateIntervalTime,
                              [render_settings = render_settings_] {
                                render_settings->SetIsTouching(false);
                              });
    }
  }
  return consumed;
}

void PageView::DispatchEnterForeground() {
  std::list<BaseView*> to_visit{this};
  while (!to_visit.empty()) {
    BaseView* cur = to_visit.front();
    to_visit.pop_front();
    to_visit.insert(to_visit.end(), cur->GetChildren().begin(),
                    cur->GetChildren().end());
    cur->OnEnterForeground();
  }
}

void PageView::DispatchEnterBackground() {
  std::list<BaseView*> to_visit{this};
  while (!to_visit.empty()) {
    BaseView* cur = to_visit.front();
    to_visit.pop_front();
    to_visit.insert(to_visit.end(), cur->GetChildren().begin(),
                    cur->GetChildren().end());
    cur->OnEnterBackground();
  }
}

void PageView::ReportTopViewRawEvents(const std::vector<PointerEvent>& events) {
  for (const auto& event : events) {
    ReportTopViewEvent(event);
  }
}

void PageView::SetupIsolatedGestures() {
  auto tap_recognizer = std::make_unique<TapGestureRecognizer>(
      isolated_gesture_detector_.gesture_manager());
  tap_recognizer->SetTaskRunner(GetTaskRunner());
  tap_recognizer->SetTapUpCallback([this](const PointerEvent& event) {
    if (event.device == PointerEvent::DeviceType::kTouch) {
      ReportTopViewEvent(event, kClayEventTypeTap);
    } else {
      ReportTopViewEvent(event, kClayEventTypeMouseClick);
      // simulate primary mouse click as touch tap to adapt front-end code
      auto tap_event = event;
      tap_event.device = PointerEvent::DeviceType::kTouch;
      ReportTopViewEvent(tap_event, kClayEventTypeTap);
    }
  });
  isolated_gesture_detector_.AddRecognizer(std::move(tap_recognizer));

  auto long_press_recognizer = std::make_unique<LongPressGestureRecognizer>(
      isolated_gesture_detector_.gesture_manager());
  long_press_recognizer->SetLongPressStartCallback(
      [this](const PointerEvent& event) {
        if (event.device == PointerEvent::DeviceType::kTouch) {
          ReportTopViewEvent(event, kClayEventTypeLongPress);
        } else {  // mouse
          ReportTopViewEvent(event, kClayEventTypeMouseLongPress);
          // simulate primary mouse longpress as touch longpress to adapt
          // front-end code
          auto long_press_event = event;
          long_press_event.device = PointerEvent::DeviceType::kTouch;
          ReportTopViewEvent(long_press_event, kClayEventTypeLongPress);
        }
      });
  long_press_recognizer->SetTaskRunner(GetTaskRunner());
  isolated_gesture_detector_.AddRecognizer(std::move(long_press_recognizer));
}

bool PageView::HitTest(const PointerEvent& event, HitTestResult& result) {
  return BaseView::HitTest(event, result);
}

BaseView* PageView::GetTopViewToAcceptEvent(const FloatPoint& position,
                                            FloatPoint* relative_position) {
  return BaseView::GetTopViewToAcceptEvent(position, relative_position);
}

void PageView::ReportTopViewEvent(const PointerEvent& event,
                                  ClayEventType type) {
  auto position = event.position;
  BaseView* top_view = nullptr;
  FloatPoint transformed_position;
  if (type == kClayEventTypeTouchMove || type == kClayEventTypeTouchEnd ||
      type == kClayEventTypeTouchCancel) {
    auto it = touch_view_map_.find(event.pointer_id);
    if (it == touch_view_map_.end()) {
      FML_DLOG(WARNING) << "The touch down event of pointer_id "
                        << event.pointer_id
                        << " not existed or associated with a anonymous view.";
    }
    if (it != touch_view_map_.end()) {
      top_view = this->render_delegate_->FindViewById(it->second);
      if (type == kClayEventTypeTouchEnd || type == kClayEventTypeTouchCancel) {
        FML_DCHECK(touch_view_map_.find(event.pointer_id) !=
                   touch_view_map_.end());
        touch_view_map_.erase(it);
      }
    }
    if (!top_view || !top_view->attach_to_tree()) {
      return;
    }
    auto view_pos = top_view->AbsoluteLocationWithScroll();
    transformed_position = position;
    transformed_position.Move(-view_pos.x(), -view_pos.y());
  } else {
    top_view = GetTopViewToAcceptEvent(position, &transformed_position);
  }

  if (!top_view || top_view->IsAnonymousView()) {
    return;
  }

  switch (event.device) {
    case PointerEvent::DeviceType::kTouch: {
      if (type == kClayEventTypeTouchStart) {
        FML_DCHECK(touch_view_map_.find(event.pointer_id) ==
                   touch_view_map_.end());
        touch_view_map_[event.pointer_id] = top_view->id();
        ResignFirstResponderIfNeeded(top_view);
      }

      bool is_raw_events =
          type == kClayEventTypeTouchStart || type == kClayEventTypeTouchEnd ||
          type == kClayEventTypeTouchMove || type == kClayEventTypeTouchCancel;
      if (is_raw_events && UNLIKELY(top_view->Is<NativeView>())) {
        // Only dispatch the raw touch events to the NativeView. Generated
        // events such as tapping or long pressing should not be dispatched to
        // the NativeViews.
        static_cast<NativeView*>(top_view)->SendMotionEvent(
            event, transformed_position);
      }
      if (event_delegate_) {
        event_delegate_->OnTouchEvent(
            EventTypeToString(type), top_view->id(), transformed_position.x(),
            transformed_position.y(), position.x(), position.y());
      }
    } break;
    case PointerEvent::DeviceType::kMouse: {
      bool is_signal_event = type == kClayEventTypeWheel;
      bool is_raw_events = type == kClayEventTypeMouseDown ||
                           type == kClayEventTypeMouseUp ||
                           type == kClayEventTypeMouseMove;
      if (UNLIKELY(top_view->Is<NativeView>())) {
        auto native_view = static_cast<NativeView*>(top_view);
        if (is_raw_events ||
            (is_signal_event && native_view->IsScrollEnabled())) {
          native_view->SendMotionEvent(event, transformed_position);
        }
      }
      // update buttons state to detect which button is changed now
      if (buttons_state_ != event.buttons) {
        button_state_ = event.buttons ^ buttons_state_;
        buttons_state_ = event.buttons;
      }
      // wheel event
      if (event_delegate_) {
        if (is_signal_event) {
          if (event.signal_kind == PointerEvent::SignalKind::kStartScroll) {
            wheel_target_ = top_view;
          }
          FML_DCHECK(wheel_target_ != nullptr);
          event_delegate_->OnWheelEvent(
              EventTypeToString(type), wheel_target_->id(),
              transformed_position.x(), transformed_position.y(), position.x(),
              position.y(), event.scroll_delta_x, event.scroll_delta_y);
          if (event.signal_kind == PointerEvent::SignalKind::kEndScroll) {
            wheel_target_ = nullptr;
          }
        } else {
          event_delegate_->OnMouseEvent(
              EventTypeToString(type), top_view->id(), button_state_,
              buttons_state_, 1, transformed_position.x(),
              transformed_position.y(), position.x(), position.y());
        }
      }
    } break;
    case PointerEvent::DeviceType::kTrackpad: {
      if (event.type == PointerEvent::EventType::kPanZoomStartEvent) {
        pan_zoom_target_ = top_view;
        return;
      }
      if (event.type == PointerEvent::EventType::kPanZoomEndEvent) {
        pan_zoom_target_ = nullptr;
        return;
      }
      if (event.type != PointerEvent::EventType::kPanZoomUpdateEvent) {
        FML_DLOG(INFO) << "omit trackpad event: "
                       << static_cast<int>(event.type);
        return;
      }
      bool has_pan_data =
          event.pan_delta.width() > 0 || event.pan_delta.height() > 0;
      if (UNLIKELY(top_view->Is<NativeView>())) {
        auto native_view = static_cast<NativeView*>(top_view);
        if (native_view->IsScrollEnabled() && has_pan_data) {
          native_view->SendMotionEvent(event, transformed_position);
        }
      }
      bool is_scale = event.scale != 1;
      if (event_delegate_) {
        if (is_scale) {
          event_delegate_->OnMouseEvent(
              "zoom", pan_zoom_target_->id(), button_state_, buttons_state_,
              event.scale, transformed_position.x(), transformed_position.y(),
              position.x(), position.y());
        } else {
          event_delegate_->OnWheelEvent(
              "wheel", pan_zoom_target_->id(), transformed_position.x(),
              transformed_position.y(), position.x(), position.y(),
              event.pan_delta.width(), event.pan_delta.height());
        }
      }
    } break;
    default:
      // TODO(Chenfeng Pan): report event from *[Inverted]Stylus*
      break;
  }
}

void PageView::ReportTopViewEvent(const PointerEvent& event) {
  ReportTopViewEvent(event, ToClayEventType(event));
}

void PageView::ReportKeyEvent(const KeyEvent& event) {
  if (event_delegate_ && (event.GetType() == KeyEventType::kUp ||
                          event.GetType() == KeyEventType::kDown ||
                          event.GetType() == KeyEventType::kRepeat)) {
    int view_id;
    if (auto* node = focus_manager_.GetLeafFocusedNode()) {
      view_id = static_cast<BaseView*>(node)->id();
    } else {
      view_id = this->id();  // <page>
    }

    std::string web_key = KeyCodeConverter::ConvertToWebKey(
        event.GetLogical(), event.GetCharacter());
    if (web_key.empty() || web_key[0] == '\0') {
      return;
    }
    auto temp_type = ToClayEventType(event.GetType());
    event_delegate_->OnKeyEvent(EventTypeToString(temp_type), view_id,
                                web_key.c_str(),
                                event.GetType() == KeyEventType::kRepeat);
  }
}

void PageView::SetRefreshRate(uint32_t refresh_rate) {
  GetGapWorker()->SetRefreshRate(refresh_rate);
}

void PageView::ReportAnimationEvent(const AnimationParams& animation_params,
                                    int view_id) {
  if (event_delegate_) {
    event_delegate_->OnAnimationEvent(
        EventTypeToString(animation_params.event_type),
        animation_params.animation_name, view_id);
  }
}
void PageView::ReportTransitionEvent(const AnimationParams& animation_params,
                                     int view_id,
                                     ClayAnimationPropertyType property_type) {
  if (event_delegate_) {
    event_delegate_->OnTransitionEvent(
        EventTypeToString(animation_params.event_type),
        animation_params.animation_name, view_id, property_type);
  }
}
void PageView::DispatchAnimationEvent(const AnimationParams& animation_params,
                                      int view_id) {
  ReportAnimationEvent(animation_params, view_id);
}
void PageView::DispatchTransitionEvent(
    const AnimationParams& animation_params, int view_id,
    ClayAnimationPropertyType property_type) {
  ReportTransitionEvent(animation_params, view_id, property_type);
}
void PageView::RequestPaint() { Invalidate(); }
void PageView::RequestPaintBase() { BaseView::Invalidate(); }

void PageView::DispatchKeyEvent(
    std::unique_ptr<KeyEvent> event,
    std::function<void(bool /* handled */)> callback) {
  FML_DCHECK(event);

  // We need to first report the keyevent then handle the focus change
  ReportKeyEvent(*event);

  if (!keyevent_handled_ && focus_manager_.DispatchKeyEvent(event.get())) {
    keyevent_handled_ = true;
  }

#ifndef NDEBUG
  // In debug mode, we can press the 'D' key to print tree hierarchy (use scrcpy
  // or `adb shell input text d`)
  if (event->GetType() == KeyEventType::kDown &&
      static_cast<int>(event->GetLogical()) == 'd') {
    DumpRenderingTrees();
  }
#endif

  if (intercept_back_key_once_ && IsBackOrEscapeKey(event->GetLogical())) {
    keyevent_handled_ = true;
    if (event->GetType() != KeyEventType::kDown) {
      intercept_back_key_once_ = false;
    }
  }
  callback(keyevent_handled_);
  keyevent_handled_ = false;
}

GrDataPtr PageView::TakeScreenshotHardware(
    ScreenshotRequest screenshot_request) {
  if (!render_delegate_) {
    return nullptr;
  }
#ifdef ENABLE_SCREENSHOT_SERVICE
  screenshot_request.page_width_ = physical_size_.width();
  screenshot_request.page_height_ = physical_size_.height();
  clay::Puppet<clay::Owner::kPlatform, clay::ScreenshotService>
      screenshot_service =
          service_manager_->GetService<clay::ScreenshotService>();
  return screenshot_service->TakeScreenshotHardware(screenshot_request);
#else
  return nullptr;
#endif
}

void PageView::AddToKeyboardHostView(BaseView* keyboard_view) {
  AddChild(keyboard_view);
}

void PageView::RemoveFromKeyboardHostView(BaseView* keyboard_view) {
  RemoveChild(keyboard_view);
}

void PageView::OnKeyboardEvent(std::unique_ptr<KeyEvent> key_event) {
  if (!ime_listener_) {
    return;
  }
  if (key_event->GetType() == KeyEventType::kCommitText) {
    ime_listener_->OnCommitText(key_event->GetCharacter());
    return;
  }
  if (key_event->GetType() == KeyEventType::kCommitComposingText) {
    ime_listener_->OnComposingText(key_event->GetCharacter());
    return;
  }

  ime_listener_->OnKeyboardEvent(std::move(key_event));
}

void PageView::OnPerformAction(KeyboardAction action) {
  if (ime_listener_) {
    ime_listener_->OnPerformAction(action);
  }
}

void PageView::OnFinishInput() {
  if (ime_listener_) {
    ime_listener_->OnFinishInput();
  }
}

void PageView::OnDeleteSurroundingText(int before_length, int after_length) {
  if (!ime_listener_) {
    return;
  }
  ime_listener_->OnDeleteSurroundingText(before_length, after_length);
}

void PageView::PlatformShowSoftInput(int type, int action) {
  render_delegate_->ShowSoftInput(type, action);
}

void PageView::PlatformHideSoftInput() { render_delegate_->HideSoftInput(); }

std::string PageView::ShouldInterceptUrl(const std::string& origin_url,
                                         bool should_decode) {
  return render_delegate_->ShouldInterceptUrl(origin_url, should_decode);
}

std::shared_ptr<clay::ResourceLoaderIntercept>
PageView::GetResourceLoaderIntercept() {
  return render_delegate_->GetResourceLoaderIntercept();
}

InputClientManager* PageView::GetInputClientManager() {
  if (!input_client_manager_) {
    input_client_manager_ = std::make_unique<InputClientManager>();
  }
  return input_client_manager_.get();
}

void PageView::RequestInput(IMEListener* ime_listener, KeyboardInputType type,
                            KeyboardAction action) {
  ime_listener_ = ime_listener;
  keyboard_bridge_.RequestSoftKeyboard(type, action, this);
}

void PageView::StopInput(IMEListener* ime_listener) {
  if (ime_listener_ == ime_listener) {
    ime_listener_ = nullptr;
    keyboard_bridge_.HideSoftKeyboard();
  }
}

void PageView::Invalidate() { RequestNewFrame(); }

void PageView::RequestNewFrame() {
  if (LIKELY(render_delegate_)) {
    render_delegate_->ScheduleFrame();
  }
}

// It is used when decode with priority enabled to notify the scheduler that
// the image is decoded.
void PageView::RegisterUploadTask(OneShotCallback<>&& task, int image_id) {
  if (!ImageDecodeWithPriority()) {
    return;
  }

  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());
  // Register upload task.
  auto unique_id = PageUniqueId();
  ImageUploadManager::GetInstance().AddImageUploadTask(
      unique_id, std::move(task), image_id);

  // Notify scheduler.
  clay::Puppet<Owner::kUI, RasterFrameService> raster_frame_service =
      service_manager_->GetService<RasterFrameService>();
  raster_frame_service.Act(
      [](auto& impl) { impl.NotifyUploadTaskRegistered(); });
}

void PageView::UnRegisterUploadTask() {
  if (!ImageDecodeWithPriority()) {
    return;
  }

  auto unique_id = PageUniqueId();
  ImageUploadManager::GetInstance().RemoveImageUploadTaskByQueueId(unique_id);
}

void PageView::DispatchViewportMetricsUpdate() {
  std::list<BaseView*> to_visit{this};
  while (!to_visit.empty()) {
    BaseView* cur = to_visit.front();
    to_visit.pop_front();
    to_visit.insert(to_visit.end(), cur->GetChildren().begin(),
                    cur->GetChildren().end());
    cur->OnViewportMetricsUpdated(physical_size_.width(),
                                  physical_size_.height(),
                                  metrics_.device_pixel_ratio);
  }
  if (event_delegate_) {
    event_delegate_->OnViewportMetricsChanged(
        metrics_.device_pixel_ratio, metrics_.device_density_dpi, size_.width(),
        size_.height(), metrics_.physical_screen_width,
        metrics_.physical_screen_height, metrics_.font_scale,
        metrics_.night_mode);
  }
}

void PageView::SetKeyframesData(KeyframesData* keyframes_data) {
  for (int i = 0; i < keyframes_data->size; i++) {
    ClayKeyframesRule* rule = keyframes_data->keyframe_rules + i;
    if (!rule) {
      continue;
    }
    KeyframesMap keyframes_map;
    for (int j = 0; j < rule->size; j++) {
      ClayKeyframe* keyframe = rule->keyframes + j;
      if (!keyframe) {
        continue;
      }
      float fraction = keyframe->percentage;
      for (int k = 0; k < keyframe->size; k++) {
        ClayAnimationPropertyValue* prop = keyframe->properties + k;
        if (!prop) {
          continue;
        }
        FloatKeyframeSet* float_keyframe_set;
        ColorKeyframeSet* color_keyframe_set;
        RawTransformKeyframeSet* transform_keyframe_set;
        ClayTransform* clay_transform;
        auto search = keyframes_map.find(prop->type);
        switch (prop->type) {
          case ClayAnimationPropertyType::kFilter: {
            if (search == keyframes_map.end()) {
              search = keyframes_map
                           .try_emplace(prop->type, FilterKeyframeSet::Create())
                           .first;
            }
            FilterKeyframeSet* filter_keyframe_set =
                static_cast<FilterKeyframeSet*>(search->second.get());
            auto values = static_cast<std::vector<FilterValue>*>(
                prop->value.GetPointer());
            filter_keyframe_set->AddKeyframe(FilterKeyframe::Create(
                fraction, FilterOperations(*values),
                Interpolator::CreateDefaultInterpolator()));
            break;
          }
          case ClayAnimationPropertyType::kBoxShadow: {
            if (search == keyframes_map.end()) {
              search =
                  keyframes_map
                      .try_emplace(prop->type, BoxShadowKeyframeSet::Create())
                      .first;
            }
            BoxShadowKeyframeSet* box_shadow_keyframe_set =
                static_cast<BoxShadowKeyframeSet*>(search->second.get());
            auto values = static_cast<std::vector<BoxShadowValue>*>(
                prop->value.GetPointer());
            box_shadow_keyframe_set->AddKeyframe(BoxShadowKeyframe::Create(
                fraction, BoxShadowOperations(*values),
                Interpolator::CreateDefaultInterpolator()));
            break;
          }
          case ClayAnimationPropertyType::kOpacity: {
            if (search == keyframes_map.end()) {
              search = keyframes_map
                           .try_emplace(prop->type,
                                        FloatKeyframeSet::Create(prop->type))
                           .first;
            }
            float_keyframe_set =
                static_cast<FloatKeyframeSet*>(search->second.get());
            float_keyframe_set->AddKeyframe(FloatKeyframe::Create(
                fraction, prop->value.GetFloat(),
                Interpolator::CreateDefaultInterpolator()));
            break;
          }
          case ClayAnimationPropertyType::kBackgroundColor: {
            if (search == keyframes_map.end()) {
              search = keyframes_map
                           .try_emplace(prop->type,
                                        ColorKeyframeSet::Create(prop->type))
                           .first;
            }
            color_keyframe_set =
                static_cast<ColorKeyframeSet*>(search->second.get());
            color_keyframe_set->AddKeyframe(ColorKeyframe::Create(
                fraction, Color(prop->value.GetUint()),
                Interpolator::CreateDefaultInterpolator()));
            break;
          }
          case ClayAnimationPropertyType::kColor: {
            if (search == keyframes_map.end()) {
              search = keyframes_map
                           .try_emplace(prop->type,
                                        ColorKeyframeSet::Create(prop->type))
                           .first;
            }
            color_keyframe_set =
                static_cast<ColorKeyframeSet*>(search->second.get());
            color_keyframe_set->AddKeyframe(ColorKeyframe::Create(
                fraction, Color(prop->value.GetUint()),
                Interpolator::CreateDefaultInterpolator()));
            break;
          }
          case ClayAnimationPropertyType::kTransform: {
            if (search == keyframes_map.end()) {
              search = keyframes_map
                           .try_emplace(prop->type,
                                        RawTransformKeyframeSet::Create())
                           .first;
            }
            transform_keyframe_set =
                static_cast<RawTransformKeyframeSet*>(search->second.get());
            clay_transform =
                static_cast<ClayTransform*>(prop->value.GetPointer());
            transform_keyframe_set->AddKeyframe(RawTransformKeyframe::Create(
                fraction, *clay_transform,
                Interpolator::CreateDefaultInterpolator()));
            break;
          }
          default:
            FML_DLOG(ERROR) << "SetKeyframesData unsupported property type";
            break;
        }
      }
    }
    if (!keyframes_map.empty()) {
      auto ret = keyframes_data_.insert_or_assign(rule->name,
                                                  std::move(keyframes_map));
      if (!ret.second) {
        FML_DLOG(WARNING) << "SetKeyframesData duplicated name:" << rule->name;
      }
    }
  }

#if (DEBUG_KEYFRAMES)
  FML_LOG(ERROR) << "PageView::SetKeyframesData size="
                 << keyframes_data_.size();
  for (const auto& node : keyframes_data_) {
    FML_LOG(ERROR) << "PageView::SetKeyframesData \t name=" << node.first
                   << " property size=" << node.second.size();
    for (const auto& keyframe_set : node.second) {
      FML_LOG(ERROR) << "PageView::SetKeyframesData \t\t keyframe_set="
                     << keyframe_set.second->ToString();
    }
  }
#endif
}

const KeyframesMap* PageView::GetKeyframesMap(
    const std::string& animation_name) {
  auto iter = keyframes_data_.find(animation_name);
  if (iter == keyframes_data_.end()) {
    return nullptr;
  }
  return &(iter->second);
}

#ifndef ENABLE_SKITY
fml::RefPtr<ImageResourceFetcher> PageView::GetImageResourceFetcher() {
  if (!image_resource_fetcher_) {
    FML_DCHECK(GetTaskRunner()->RunsTasksOnCurrentThread());
    image_resource_fetcher_ = fml::MakeRefCounted<ImageResourceFetcher>(
        GetResourceLoaderIntercept(), GetTaskRunners(), unref_queue_,
        GetServiceManager());
  }
  return image_resource_fetcher_;
}

void PageView::SetImageResourceFetcher(
    fml::RefPtr<ImageResourceFetcher> image_resource_fetcher) {
  image_resource_fetcher_ = image_resource_fetcher;
}
#else
fml::RefPtr<ImageFetcher> PageView::GetImageResourceFetcher() {
  if (!image_resource_fetcher_) {
    FML_DCHECK(GetTaskRunner()->RunsTasksOnCurrentThread());
    image_resource_fetcher_ =
        ImageFetcher::Create(GetTaskRunners(), unref_queue_);
  }
  return image_resource_fetcher_;
}

void PageView::SetImageResourceFetcher(
    fml::RefPtr<ImageFetcher> image_resource_fetcher) {
  image_resource_fetcher_ = image_resource_fetcher;
}
#endif  // ENABLE_SKITY

ViewTreeObserver* PageView::GetViewTreeObserver() {
  if (!view_tree_observer_) {
    view_tree_observer_ = std::make_unique<ViewTreeObserver>();
  }
  return view_tree_observer_.get();
}

bool PageView::HasIntersectionObserverManager() {
  return intersection_observer_manager_ ? true : false;
}

void PageView::NotifyLowMemory() {
  Isolate::Instance().GetResourceCache()->ClearCache();
  ImageDataCache::GetInstance().ClearCache();
#ifndef ENABLE_SKITY
  if (image_resource_fetcher_) {
    image_resource_fetcher_->ClearCache();
  }
#endif  // ENABLE_SKITY
  render_delegate_->ClearTextCache();
}

void PageView::PostUIMethodTask(std::function<void()> task) {
  ui_method_tasks_.emplace_back(std::move(task));
  RequestPaint();
}

void PageView::FlushUIMethodTasks() {
  auto tasks = std::move(ui_method_tasks_);
  for (const auto& task : tasks) {
    task();
  }
}

void PageView::ResetPageView(bool recycle) {
  DestroyAllChildren();
  padding_left_ = 0.f;
  padding_top_ = 0.f;
  padding_right_ = 0.f;
  padding_bottom_ = 0.f;
  keyframes_mgr_.reset();
  transition_mgr_.reset();
#ifdef ENABLE_ACCESSIBILITY
  semantics_owner_->Reset();
#endif
  render_object_ = std::make_unique<RenderPage>();
  render_object_->SetWidth(width_);
  render_object_->SetHeight(height_);
  render_object_->SetTop(top_);
  render_object_->SetLeft(left_);
  static_cast<RenderPage*>(render_object_.get())
      ->SetScaleRatio(GetPixelRatio<kPixelTypeClay, kPixelTypePhysical>());
  renderer_ = std::make_unique<Renderer>(this, unref_queue_);
  layout_controller_ = std::make_unique<LayoutController>();
  renderer_->SetRoot(render_object_.get());
  renderer_->SetPixelRatio(metrics_.device_pixel_ratio);
  renderer_->SetDPI(metrics_.device_density_dpi);
  renderer_->SetFrameSize({static_cast<int32_t>(metrics_.physical_width),
                           static_cast<int32_t>(metrics_.physical_height)});
  animation_handler_->ClearCallbacks();
  animation_handler_->SetOnNewAnimationCallback([this] { RequestNewFrame(); });
  frame_builder_ = std::make_unique<FrameBuilder>(
      skity::Vec2{static_cast<int32_t>(metrics_.physical_width),
                  static_cast<int32_t>(metrics_.physical_height)},
      metrics_.device_pixel_ratio, unref_queue_);
  focus_manager_ = FocusManager(this);
  focus_manager_.SetIsRootScope();
  view_tree_observer_.reset();
  UnRegisterUploadTask();

  if (recycle) {
    first_paint_ = false;
    first_meaningful_layout_ = false;
    perf_collector_.reset();
    image_resource_fetcher_ = nullptr;
  }

  exposure_event_arr_.clear();
  disexposure_event_arr_.clear();
  RequestPaintBase();
}

void PageView::AddGlobalExposureEvent(bool exposure,
                                      std::unique_ptr<clay::Value::Map> params,
                                      BaseView* view) {
  if (exposure) {
    // should keep following vector size equal
    exposure_event_arr_.emplace_back(std::move(params));
    exposure_event_data_set_.push_back(view->GetWeakPtr());
  } else {
    // should keep following vector size equal
    disexposure_event_arr_.emplace_back(std::move(params));
    disexposure_event_data_set_.push_back(view->GetWeakPtr());
  }
}

void PageView::MoveWindow() {
#if (defined(OS_MAC) || defined(OS_WIN))
  WindowMove();
#endif
}

void PageView::MakeRasterSnapshot(
    BaseView* target, bool compress_jpeg, float scale,
    std::function<void(GrDataPtr, int32_t, int32_t)> callback) {
#ifndef ENABLE_SKITY
  if (!render_delegate_) {
    callback(nullptr, 0, 0);
    return;
  }
  if (target->render_object()->NeedsPaint() ||
      target->render_object()->NeedsEffect()) {
    // Needs to repaint first if the RenderObject has been marked dirty.
    render_delegate_->ForceBeginFrame();
  }
  FML_DCHECK(!(target->render_object()->NeedsPaint() ||
               target->render_object()->NeedsEffect()));
  FML_DCHECK(target->render_object()->IsRepaintBoundary());
  auto* layer =
      static_cast<PendingOffsetLayer*>(target->render_object()->GetLayer());
  if (!layer) {
    callback(nullptr, 0, 0);
    return;
  }
  auto offset = layer->Offset();
  // Reset the offset of the target view to (0, 0) to take the snapshot, and
  // will be restored after.
  layer->SetOffset(FloatPoint(0, 0));
  // Get the frame size with physical pixel.
  int32_t width = static_cast<int32_t>(
      ConvertTo<kPixelTypePhysical>(target->Width()) * scale);
  int32_t height = static_cast<int32_t>(
      ConvertTo<kPixelTypePhysical>(target->Height()) * scale);
  // Create a temporary FrameBuilder to generate the layer tree.
  std::unique_ptr<FrameBuilder> frame_builder = std::make_unique<FrameBuilder>(
      skity::Vec2{width, height}, metrics_.device_pixel_ratio, unref_queue_);
  // Apply the scale ratio using a transform layer to get the final image.
  float scale_ratio =
      GetPixelRatio<kPixelTypeClay, kPixelTypePhysical>() * scale;
  skity::Matrix matrix;
  matrix.Reset();
  matrix.SetScaleX(scale_ratio);
  matrix.SetScaleY(scale_ratio);
  auto transform_layer = std::make_shared<PendingTransformLayer>(matrix);
  frame_builder->PushStaticTransform(matrix, transform_layer.get());
  frame_builder->BuildSubtreeFrame(layer);
  // Restore the offset of the target view.
  layer->SetOffset(offset);
  // Take the subtree of the target view.
  auto layer_tree = frame_builder->TakeLayerTree();
  if (!layer_tree) {
    callback(nullptr, 0, 0);
    return;
  }
  // Make raster snapshot with the subtree, resulting in a CPU-backed DlImage.
  render_delegate_->MakeRasterSnapshot(
      std::move(layer_tree),
      [compress_jpeg, callback](fml::RefPtr<PaintImage> paint_image) {
        // The callback should run on IO thread.
        auto image = paint_image ? paint_image->gr_image() : nullptr;
        if (!image) {
          callback(nullptr, 0, 0);
          return;
        }
        // The image must be CPU-backed.
        FML_DCHECK(!image->isTextureBacked());
        GrDataPtr gr_data = nullptr;
        // The scale factor has already been applied to the image with transform
        // layer, so we don't need to scale the image here again. Just compress
        // the image to png or jpeg format.
        if (compress_jpeg) {
          SkJpegEncoder::Options options;
          gr_data = SkJpegEncoder::Encode(nullptr, image.get(), options);
        } else {
          SkPngEncoder::Options options;
          gr_data = SkPngEncoder::Encode(nullptr, image.get(), options);
        }
        callback(gr_data, image->width(), image->height());
      });
#else
  // TODO: The jpeg/png encoder is not supported on Skity by default for
  // binary size, so we have no need to call MakeRasterSnapshot now, should be
  // implemented in future.
  FML_UNIMPLEMENTED();
  callback(nullptr, 0, 0);
#endif  // ENABLE_SKITY
}

fml::RefPtr<PaintImage> PageView::MakeRasterSnapshot(GrPicturePtr picture,
                                                     skity::Vec2 picture_size) {
  return render_delegate_->MakeRasterSnapshot(std::move(picture), picture_size);
}

int PageView::GetViewIdForLocation(int x, int y) {
  FloatPoint unused;
  BaseView* top_view = GetTopViewToAcceptEvent(FloatPoint(x, y), &unused);
  if (top_view) {
    return top_view->id();
  }
  return -1;
}

#ifndef NDEBUG
std::string PageView::ToString() const {
  std::stringstream ss;
  ss << BaseView::ToString();
  ss << " physical_size=(" << physical_size().width() << ","
     << physical_size().height() << ")";
  ss << " logical_size=(" << logical_size().width() << ","
     << logical_size().height() << ")";
  ss << " DevicePixelRatio=" << DevicePixelRatio();
  return ss.str();
}

void PageView::DumpRenderingTrees() const {
  FML_LOG(ERROR) << ">>>>>>>>>> DumpViewTree";
  DumpViewTree(0);
  FML_LOG(ERROR) << ">>>>>>>>>> DumpRenderTree";
  render_object()->DumpRenderTree();
  FML_LOG(ERROR) << ">>>>>>>>>> DumpLayerTree";
  render_object()->GetLayer()->DumpPendingLayerTree();
  FML_LOG(ERROR) << ">>>>>>>>>> DumpEngineLayerTree";
  frame_builder_->DumpLayerTree();
}
#endif

void PageView::FlushGapTaskIfNecessary(const fml::TimePoint& target_end_time) {
  if (GetGapWorker()->HasGapTask() && fml::TimePoint::Now() < target_end_time) {
    // We still have time, try flush gap work at ui thread.
    task_runners_.GetUITaskRunner()->PostTask(
        [weak = GetWeakPtr(), target_end_time]() {
          if (weak) {
            auto page = static_cast<PageView*>(weak.get());
            page->GetGapWorker()->FlushTask(target_end_time);
            if (page->GetGapWorker()->HasGapTask()) {
              // Check if there are still tasks that need to be executed at
              // subsequent frame intervals. Because sometimes the rendering
              // state may remain static or nothing new needs to be rendered, it
              // is also possible to run some tasks in idle time.
              page->RequestNewFrame();
            }
          }
        });
  }
}

GapWorker* PageView::GetGapWorker() {
  if (!gap_worker_) {
    gap_worker_ = std::make_unique<GapWorker>(task_runners_.GetUITaskRunner());
  }
  return gap_worker_.get();
}

void PageView::ReportTiming(
    const std::unordered_map<std::string, int64_t>& timing,
    const std::string& flag) {
  if (render_delegate_) {
    render_delegate_->ReportTiming(timing, flag);
  }
}

void PageView::RegisterFirstFrameAvailable(int64_t image_id,
                                           const fml::closure& callback) {
  first_frame_callbacks_[image_id] = callback;
}

void PageView::UnRegisterFirstFrameAvailable(int64_t image_id) {
  first_frame_callbacks_.erase(image_id);
}

bool PageView::MarkDrawableImageFrameAvailable(int64_t image_id) {
  auto it = first_frame_callbacks_.find(image_id);
  if (it == first_frame_callbacks_.end()) {
    return false;
  }
  // The first frame has been available, call callback to do something.
  // Finally, a new frame must be requested.
  it->second();
  return true;
}

void PageView::SetEditingPlatformView(NativeView* view) {
  editing_native_view_ = view;
}

void PageView::OnPlatformUpdateEditState(int client_id, uint64_t selection_base,
                                         uint64_t composing_extent,
                                         const char* selection_affinity,
                                         const char* text,
                                         uint64_t selection_extent,
                                         uint64_t composing_base) {
  input_client_manager_->InvokeUpdateEditState(
      client_id, selection_base, composing_extent, selection_affinity, text,
      selection_extent, composing_base);
}

void PageView::OnPlatformPerformInputAction(int client_id) {
  input_client_manager_->InvokePerformAction(client_id);
}

void PageView::ResignFirstResponderIfNeeded(BaseView* current_responder) {
  if (editing_native_view_ && editing_native_view_ != current_responder) {
    editing_native_view_->ResignFirstResponder();
    editing_native_view_ = nullptr;
  }
}

void PageView::RegisterDrawableImage(
    std::shared_ptr<DrawableImage> drawable_image) {
  render_delegate_->RegisterDrawableImage(drawable_image);
}

void PageView::UnregisterDrawableImage(int64_t id) {
  render_delegate_->UnregisterDrawableImage(id);
}

void PageView::SetExternalScreenshotCallback(
    ExternalScreenshotCallback callback) {
#ifdef ENABLE_SCREENSHOT_SERVICE
  clay::Puppet<clay::Owner::kPlatform, clay::ScreenshotService>
      screenshot_service =
          service_manager_->GetService<clay::ScreenshotService>();
  screenshot_service->SetExternalScreenshotCallback(std::move(callback));
#endif
}

}  // namespace clay
