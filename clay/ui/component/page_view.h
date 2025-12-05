// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_PAGE_VIEW_H_
#define CLAY_UI_COMPONENT_PAGE_VIEW_H_

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/closure.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/time/time_point.h"
#include "build/build_config.h"
#include "clay/common/graphics/screenshot.h"
#include "clay/common/recyclable.h"
#include "clay/common/service/service_manager.h"
#include "clay/flow/frame_timings.h"
#include "clay/gfx/geometry/size.h"
#include "clay/gfx/pixel_helper.h"
#include "clay/public/event_delegate.h"
#include "clay/public/ui_component_delegate.h"
#include "clay/public/value.h"
#include "clay/ui/common/gap_worker.h"
#include "clay/ui/common/input_client_manager.h"
#include "clay/ui/common/isolate.h"
#include "clay/ui/common/perf_collector.h"
#include "clay/ui/common/render_settings.h"
#include "clay/ui/common/value_utils.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/editable/ime_listener.h"
#include "clay/ui/component/intersection_observer_manager.h"
#include "clay/ui/component/isolated_gesture_detector.h"
#include "clay/ui/component/keyframes_data.h"
#include "clay/ui/component/layout_controller.h"
#include "clay/ui/component/nested_scroll/nested_scroll_manager.h"
#include "clay/ui/component/view_context.h"
#include "clay/ui/component/view_tree_observer.h"
#include "clay/ui/compositing/frame_builder.h"
#include "clay/ui/event/focus_manager.h"
#include "clay/ui/event/key_event.h"
#include "clay/ui/gesture/gesture_manager.h"
#include "clay/ui/gesture/mouse_region_manager.h"
#include "clay/ui/platform/keyboard_bridge.h"
#include "clay/ui/render_delegate.h"
#include "clay/ui/rendering/renderer.h"
#include "clay/ui/resource/image_resource_fetcher.h"
#include "clay/ui/window/viewport_metrics.h"
#ifdef ENABLE_ACCESSIBILITY
#include "clay/ui/semantics/semantics_owner.h"
#endif
#ifdef ENABLE_SKITY
#include "clay/ui/resource/image_fetcher.h"
#endif

namespace clay {

class WatchDog;
class GapWorker;
typedef std::vector<std::unique_ptr<clay::Value::Map>> ExposureDataList;

class ServiceManager;
class ViewContext;
class NativeView;
class TimingCollectorDelegate;

class PageView : public BaseView,
                 public RendererClient,
                 public KeyboardClient,
                 public Recyclable,
                 public PixelHelper<kPixelTypeClay> {
 public:
  // Only for unittests compatiblity.
  PageView(uint32_t id, fml::RefPtr<GPUUnrefQueue> unref_queue,
           fml::RefPtr<fml::TaskRunner> ui_task_runner);

  PageView(uint32_t id, std::shared_ptr<ServiceManager> service_manager,
           fml::RefPtr<GPUUnrefQueue> unref_queue,
           clay::TaskRunners task_runners);
  ~PageView() override;

  bool IsLayoutRootCandidate() const override { return true; }

  void SetEventDelegate(EventDelegate* delegate) { event_delegate_ = delegate; }
  EventDelegate* GetEventDelegate() { return event_delegate_; }
  UIComponentDelegate* GetUIComponentDelegate() {
    return ui_component_delegate_;
  }
  BaseView* FindViewByViewId(int view_id);

  BaseView* FindViewByIdSelector(std::string_view id_selector) {
    if (view_context_ == nullptr) {
      return nullptr;
    }
    return view_context_->FindViewByIdSelector(id_selector, this);
  }
  void SetRenderDelegate(RenderDelegate* delegate);

  // BaseView
  FocusManager* GetFocusManager() override { return &focus_manager_; }
  AnimationHandler* GetAnimationHandler() { return animation_handler_.get(); }
  void DispatchEnterForeground();
  void DispatchEnterBackground();
  void FlushGapTaskIfNecessary(const fml::TimePoint& target_end_time);

  CustomFilterDecoder* GetCustomFilterDecoder() {
    return view_context_ ? view_context_->GetCustomFilterDecoder() : nullptr;
  }

  void RequestPaint();
  void RequestPaintBase();
  bool BeginFrame(std::unique_ptr<clay::FrameTimingsRecorder>);
  void SetRefreshRate(uint32_t refresh_rate);
  void SetRenderSettings(fml::RefPtr<RenderSettings> render_settings);
  fml::RefPtr<RenderSettings> GetRenderSettings() const {
    return render_settings_;
  }
  void SetViewportMetrics(const clay::ViewportMetrics& metrics);
  const clay::ViewportMetrics& GetViewportMetrics() const { return metrics_; }
  // The real pixel ratio.
  float DevicePixelRatio() const override {
    return metrics_.device_pixel_ratio;
  }
  float DeviceDpi() const { return metrics_.device_density_dpi; }

  void UpdateRootSize(int32_t width, int32_t height);

  bool DispatchPointerEvent(std::vector<PointerEvent> events);
  virtual void DispatchKeyEvent(
      std::unique_ptr<KeyEvent> event,
      std::function<void(bool /* handled */)> callback);
  void DispatchAnimationEvent(const AnimationParams& animation_params,
                              int view_id);
  void DispatchTransitionEvent(const AnimationParams& animation_params,
                               int view_id,
                               ClayAnimationPropertyType property_type);
  void CallJSIntersectionObserver(int observer_id, int callback_id,
                                  clay::Value params);
  void ReportAfterPaint();

  GrDataPtr TakeScreenshotHardware(
      ScreenshotRequest screenshot_request = ScreenshotRequest());

  Size physical_size() const { return physical_size_; }
  Size logical_size() const { return size_; }

  GestureManager* gesture_manager() { return gesture_manager_.get(); }
  NestedScrollManager* nested_scroll_manager() {
    return nested_scroll_manager_.get();
  }
  MouseRegionManager* mouse_region_manager() {
    return mouse_region_manager_.get();
  }

  ViewTreeObserver* GetViewTreeObserver();

  bool HasIntersectionObserverManager();
  IntersectionObserverManager* intersection_observer_manager() {
    if (!intersection_observer_manager_) {
      intersection_observer_manager_ =
          std::make_unique<IntersectionObserverManager>(this);
    }
    return intersection_observer_manager_.get();
  }

  const clay::TaskRunners& GetTaskRunners() const { return task_runners_; }

  fml::RefPtr<GPUUnrefQueue> GetUnrefQueue() const { return unref_queue_; }

  // KeyboardClient
  fml::RefPtr<fml::TaskRunner> GetTaskRunner() override {
    return task_runners_.GetUITaskRunner();
  }

  FloatSize KeyboardHostViewSize() override {
    return FloatSize(Width(), Height());
  }
  void AddToKeyboardHostView(BaseView* keyboard_view) override;
  void RemoveFromKeyboardHostView(BaseView* keyboard_view) override;
  void OnKeyboardEvent(std::unique_ptr<KeyEvent> key_event) override;
  void OnDeleteSurroundingText(int before_length, int after_length) override;
  void OnPerformAction(KeyboardAction action) override;
  void OnFinishInput() override;

  void RequestInput(IMEListener* ime_listener, KeyboardInputType type,
                    KeyboardAction action);
  void StopInput(IMEListener* ime_listener);

  void TriggerFirstPaintCallback();

  BaseView* GetTouchableViewOnTop(const FloatPoint& point);
  void SetDefaultFocusRingEnabled(bool enable) {
    default_focus_ring_enable_ = enable;
  }
  bool DefaultFocusRingEnabled() const { return default_focus_ring_enable_; }
  void SetPerformanceOverlayEnabled(bool enable) {
    performance_overlay_enabled_ = enable;
  }

  void SetKeyframesData(KeyframesData* keyframes_data);
  const KeyframesMap* GetKeyframesMap(const std::string& animation_name);

  LayoutController* GetLayoutController() const {
    return layout_controller_.get();
  }

#ifndef ENABLE_SKITY
  fml::RefPtr<ImageResourceFetcher> GetImageResourceFetcher();
  void SetImageResourceFetcher(
      fml::RefPtr<ImageResourceFetcher> image_resource_fetcher);
#else
  fml::RefPtr<ImageFetcher> GetImageResourceFetcher();
  void SetImageResourceFetcher(
      fml::RefPtr<ImageFetcher> image_resource_fetcher);
#endif

  virtual void OnPlatformViewCreated();

  void OnOutputSurfaceCreated();
  void OnOutputSurfaceCreateFailed();
  void OnOutputSurfaceDestroyed();
  void OnFirstMeaningfulLayout();

  void SetPerfCollector(std::shared_ptr<PerfCollector> perf_collector) {
    perf_collector_ = perf_collector;
  }
  PerfCollector* GetPerfCollector() const { return perf_collector_.get(); }
  void ReportTiming(const std::unordered_map<std::string, int64_t>& timing,
                    const std::string& flag);

  std::string ShouldInterceptUrl(const std::string& origin_url,
                                 bool should_decode = false);
  std::shared_ptr<ResourceLoaderIntercept> GetResourceLoaderIntercept();

  InputClientManager* GetInputClientManager();

  template <typename... Args>
  void SendEvent(int id, const char* event_name,
                 const std::vector<std::string>& keys, Args&&... args) {
    SendCustomEvent(id, event_name, CreateClayMap(keys, args...));
  }

  void SendCustomEvent(int id, const char* event_name,
                       clay::Value::Map params) {
    if (event_delegate_) {
      event_delegate_->OnSendCustomEvent(id, event_name, std::move(params));
    }
  }

  void AddGlobalExposureEvent(bool exposure,
                              std::unique_ptr<clay::Value::Map> params,
                              BaseView* view);

  void SetInterceptBackKeyOnce(bool intercept) {
    intercept_back_key_once_ = intercept;
  }

  void MoveWindow();

  void PostUIMethodTask(std::function<void()> task);
  void NotifyLowMemory() override;

  void MakeRasterSnapshot(
      BaseView* target, bool compress_jpeg, float scale,
      std::function<void(GrDataPtr, int32_t, int32_t)> callback);
  fml::RefPtr<PaintImage> MakeRasterSnapshot(GrPicturePtr picture,
                                             skity::Vec2 picture_size) override;

  void DumpInfoToDevtoolEnabled(bool enabled) {
    render_delegate_->DumpInfoToDevtoolEnabled(enabled);
  }

  void SetClipboardData(const std::u16string& data) {
    render_delegate_->SetClipboardData(data);
  }

  std::u16string GetClipboardData() const {
    return render_delegate_->GetClipboardData();
  }

#if defined(OS_WIN) || defined(OS_MAC) || defined(ENABLE_HEADLESS)
  // Text input related functions Begin.
  void SetTextInputClient(int client_id, const char* input_action,
                          const char* input_type) {
    render_delegate_->SetTextInputClient(client_id, input_action, input_type);
  }

  void ClearTextInputClient() { render_delegate_->ClearTextInputClient(); }

  void SetEditableTransform(const float transform_matrix[16]) {
    render_delegate_->SetEditableTransform(transform_matrix);
  }

  void SetEditingState(uint64_t selection_base, uint64_t composing_extent,
                       const std::string& selection_affinity,
                       const std::string& text, bool selection_directional,
                       uint64_t selection_extent, uint64_t composing_base) {
    render_delegate_->SetEditingState(
        selection_base, composing_extent, selection_affinity, text,
        selection_directional, selection_extent, composing_base);
  }

  void SetCaretRect(float x, float y, float width, float height) {
    render_delegate_->SetCaretRect(x, y, width, height);
  }

  void setMarkedTextRect(float x, float y, float width, float height) {
    render_delegate_->setMarkedTextRect(x, y, width, height);
  }

  void ShowTextInput() { render_delegate_->ShowTextInput(); }
  void HideTextInput() { render_delegate_->HideTextInput(); }
  // Text input related functions End.

  void WindowMove() { render_delegate_->WindowMove(); }

  void ActivateSystemCursor(int type, const std::string& path) {
    render_delegate_->ActivateSystemCursor(type, path);
  }
#endif

  void DispatchLynxLayout();
  virtual void ResetPageView(bool recycle = false);

  bool IsRasterAnimationEnabled() const {
    if (ui_component_delegate_) {
      return ui_component_delegate_->OnEnableRasterAnimation();
    }
    return raster_animation_enabled_;
  }

  // The current switch of ui and raster animation is normally set by Lynx.
  // For the purpose of local test, add a interface to enable raster
  // animation.
  void SetRasterAnimationEnabled(bool value) {
    raster_animation_enabled_ = value;
  }

  bool HitTest(const PointerEvent& event, HitTestResult& result) override;
  BaseView* GetTopViewToAcceptEvent(const FloatPoint& position,
                                    FloatPoint* relative_position) override;
  int GetViewIdForLocation(int x, int y);
#ifndef NDEBUG
  std::string ToString() const override;
  void DumpRenderingTrees() const;
#endif

  void RegisterFirstFrameAvailable(int64_t image_id,
                                   const fml::closure& callback);
  void UnRegisterFirstFrameAvailable(int64_t image_id);
  bool MarkDrawableImageFrameAvailable(int64_t image_id);

  GapWorker* GetGapWorker();

  bool UseTextureBackend() { return use_texture_backend_; }
  void SetUseTextureBackend(bool use_texture_backend) {
    use_texture_backend_ = use_texture_backend;
  }

  void SetExposureProps(int freq, bool exposure_ui_margin_enabled) {
    intersection_observer_manager()->SetExposureFrequency(freq);
    intersection_observer_manager()->SetExposureUIMarginEnabled(
        exposure_ui_margin_enabled);
  }

  // Enable deferred image decode by default.
  bool DeferredImageDecode() const {
#ifndef ENABLE_SKITY
    return false;
#else
    return true;
#endif  // ENABLE_SKITY
  }

  // Enable new image decoding strategies.
  // Will not affect skity when using platform decoders.
  bool ImageDecodeWithPriority() const { return false; }

  void RegisterDrawableImage(std::shared_ptr<DrawableImage> drawable_image);
  void UnregisterDrawableImage(int64_t id);

#ifdef ENABLE_ACCESSIBILITY
  SemanticsOwner* GetSemanticsOwner() const { return semantics_owner_.get(); }
  void SetSemanticsEnabled(bool enabled);
  bool IsSemanticsEnabled() const;
  void SetPageEnableAccessibilityElement(bool enabled);
  bool IsPageEnableAccessibilityElement() const;

  void RequestNewFrameForSemantics();
  void DispatchSemanticsAction(int virtual_view_id, int action);
#endif

  const std::shared_ptr<ServiceManager>& GetServiceManager() const {
    return service_manager_;
  }

  ShadowNode* GetShadowNodeById(int node_id) {
    return render_delegate_->FindShadowNodeById(node_id);
  }

  void SetExternalScreenshotCallback(ExternalScreenshotCallback callback);

  void CleanForRecycle() override;

  // When using `input-view` or `textarea-view` backed by platform view, we
  // record corresponding platform view when it begins to edit. If user touch
  // any places other than `input-view` or `textarea-view`, we notity the
  // platform view to resign first responder. Maybe we can combine this feature
  // with focus system in the furture.
  void SetEditingPlatformView(NativeView* view);

  void OnPlatformUpdateEditState(int client_id, uint64_t selection_base,
                                 uint64_t composing_extent,
                                 const char* selection_affinity,
                                 const char* text, uint64_t selection_extent,
                                 uint64_t composing_base);

  void OnPlatformPerformInputAction(int client_id);

  virtual void OnFlingStart() {}
  virtual void OnFlingEnd() {}

  void RegisterUploadTask(OneShotCallback<>&& task, int image_id) override;

  uint64_t PageUniqueId() const { return page_unique_id_; }

 protected:
  void OnDestroy() override;

  bool ShouldIgnoreFocusChange(const PointerEvent& event);
  void InitManagers();
  void Invalidate() override;
  void LayoutInternal();
  void Paint();
  void CompositeFrame(std::unique_ptr<clay::FrameTimingsRecorder> recorder);
  void CompositeFrame();
#ifdef ENABLE_ACCESSIBILITY
  void FlushSemantics();
  void PrepareSemantics(
      fml::RefPtr<SemanticsNode> parent_node,
      std::vector<fml::RefPtr<SemanticsNode>>& result,
      const std::vector<std::string>& ancestor_a11y_elements) override;
  void SendUpdatedSemantics(const SemanticsUpdateNodes& update_nodes);
#endif
  void RequestNewFrame() override;
  RenderPhase GetRenderPhase() const override { return render_phase_; }
  void DispatchViewportMetricsUpdate();

  void PlatformShowSoftInput(int type, int action) override;
  void PlatformHideSoftInput() override;

  void SetupIsolatedGestures();
  // Report the deepest leaf view in the position to lynx.
  void ReportTopViewRawEvents(const std::vector<PointerEvent>& events);
  // Report pointer event with specified type
  virtual void ReportTopViewEvent(const PointerEvent& event,
                                  ClayEventType type);
  // Report pointer event with the type deduced by event.device and
  // event.type
  void ReportTopViewEvent(const PointerEvent& event);
  // Report key event and current focused view
  void ReportKeyEvent(const KeyEvent& event);
  void ReportAnimationEvent(const AnimationParams& animation_params,
                            int view_id);
  void ReportTransitionEvent(const AnimationParams& animation_params,
                             int view_id,
                             ClayAnimationPropertyType property_type);
  void FlushUIMethodTasks();
  void SendGlobalExposureEvent();

  void SendDrawEndEvent();
  void HandleA11yTapEvent(BaseView* view);
  void HandleA11yLongPressEvent(BaseView* view);

  void EnsureSemanticsOwner();
  void ResignFirstResponderIfNeeded(BaseView* current_responder);

  void UnRegisterUploadTask();

  Size physical_size_;
  Size size_;
  clay::ViewportMetrics metrics_;
  bool first_paint_ = false;
  bool first_meaningful_layout_ = false;
  bool default_focus_ring_enable_ = false;
  bool performance_overlay_enabled_ = false;
  // Frontend may want to intercept kGoBack key from the default behavior.
  // As kGoBack is a sensitive key, just intercept once for each calling.
  bool intercept_back_key_once_ = false;
  // If set to true, we will do raster in this frame even if there are no
  // nodes need to paint.
  bool force_raster_ = false;
  int button_state_ = 0;   // the one button changed recently
  int buttons_state_ = 0;  // bit field, all buttons pressed
  RenderPhase render_phase_ = RenderPhase::kIdle;
  const clay::TaskRunners task_runners_;

  std::unique_ptr<Renderer> renderer_;
  std::unique_ptr<LayoutController> layout_controller_;
  std::unique_ptr<FrameBuilder> frame_builder_;
  std::unique_ptr<AnimationHandler> animation_handler_;
#ifndef ENABLE_SKITY
  fml::RefPtr<ImageResourceFetcher> image_resource_fetcher_;
#else
  fml::RefPtr<ImageFetcher> image_resource_fetcher_;
#endif
  std::unique_ptr<GestureManager> gesture_manager_;
  std::unique_ptr<NestedScrollManager> nested_scroll_manager_;
  std::unique_ptr<MouseRegionManager> mouse_region_manager_;
  IsolatedGestureDetector isolated_gesture_detector_;
  FocusManager focus_manager_;
  KeyboardBridge keyboard_bridge_;
  IMEListener* ime_listener_ = nullptr;
  KeyframesMapData keyframes_data_;
  fml::RefPtr<GPUUnrefQueue> unref_queue_;
  std::unique_ptr<WatchDog> event_watch_dog_;
  fml::RefPtr<RenderSettings> render_settings_;
  std::unique_ptr<InputClientManager> input_client_manager_;
  std::unique_ptr<ViewTreeObserver> view_tree_observer_;
  std::shared_ptr<PerfCollector> perf_collector_;
  std::unique_ptr<IntersectionObserverManager> intersection_observer_manager_;
  std::vector<std::function<void()>> ui_method_tasks_;
  bool raster_animation_enabled_ = true;
  std::string base_uri_;
  clay::FixedRefreshRateStopwatch layout_and_animation_time_;
  std::unordered_map<int64_t, fml::closure> first_frame_callbacks_;
  // This records the views that received touch down events. Subsequent
  // touch events with the same pointer_id should be dispatched to the same
  // view, regardless of whether the touch point remains within the view's
  // boundaries.
  std::unordered_map<int, int> touch_view_map_;

  std::unique_ptr<GapWorker> gap_worker_;

  ExposureDataList exposure_event_arr_;
  ExposureDataList disexposure_event_arr_;

  std::vector<fml::WeakPtr<BaseView>> exposure_event_data_set_;
  std::vector<fml::WeakPtr<BaseView>> disexposure_event_data_set_;

  bool keyevent_handled_ = false;
  bool use_texture_backend_ = true;
  EventDelegate* event_delegate_ = nullptr;
  UIComponentDelegate* ui_component_delegate_ = nullptr;
  RenderDelegate* render_delegate_ = nullptr;

  std::shared_ptr<TimingCollectorDelegate> timing_collector_delegate_;

  friend class ViewContext;
  ViewContext* view_context_ = nullptr;

#ifdef ENABLE_ACCESSIBILITY
  std::unique_ptr<SemanticsOwner> semantics_owner_;
#endif

  const std::shared_ptr<ServiceManager> service_manager_;
  NativeView* editing_native_view_ = nullptr;

  const uint64_t page_unique_id_;
  BaseView* pan_zoom_target_ = nullptr;
  BaseView* wheel_target_ = nullptr;
};

}  // namespace clay
#endif  // CLAY_UI_COMPONENT_PAGE_VIEW_H_
