// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SHELL_H_
#define CLAY_SHELL_COMMON_SHELL_H_

#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clay/common/graphics/drawable_image.h"
#include "clay/common/recyclable.h"
#include "clay/common/settings.h"
#include "clay/common/task_runners.h"
#include "clay/flow/frame_timings.h"
#include "clay/flow/layers/layer_tree.h"
#include "clay/flow/surface.h"
#include "clay/shell/common/services/animator_info_service.h"
#include "clay/shell/common/services/raster_frame_service.h"
#include "clay/shell/common/services/rasterizer_service.h"
#if OS_ANDROID || OS_MAC || OS_WIN
#include "clay/memory/memory_pressure_listener.h"
#endif
#include "base/include/closure.h"
#include "base/include/fml/macros.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/memory/weak_ptr.h"
#include "base/include/fml/synchronization/sync_switch.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/fml/thread.h"
#include "base/include/fml/time/time_point.h"
#include "clay/fml/status.h"
#include "clay/shell/common/devtools_instrumentation.h"
#include "clay/shell/common/display_manager.h"
#include "clay/shell/common/engine.h"
#include "clay/shell/common/platform_view.h"
#include "clay/shell/common/rasterizer.h"
#include "clay/shell/common/resource_cache_limit_calculator.h"
#include "clay/shell/common/timing_client_delegate.h"

namespace clay {
class PerfCollector;
class ResourceLoaderIntercept;
}  // namespace clay

namespace clay {

/// Values for |Shell::SetGpuAvailability|.
enum class GpuAvailability {
  /// Indicates that GPU operations should be permitted.
  kAvailable = 0,
  /// Indicates that the GPU is about to become unavailable, and to attempt to
  /// flush any GPU related resources now.
  kFlushAndMakeUnavailable = 1,
  /// Indicates that the GPU is unavailable, and that no attempt should be made
  /// to even flush GPU objects until it is available again.
  kUnavailable = 2
};

//------------------------------------------------------------------------------
/// Perhaps the single most important class in the Flutter engine repository.
/// When embedders create a Flutter application, they are referring to the
/// creation of an instance of a shell. Creation and destruction of the shell is
/// synchronous and the embedder only holds a unique pointer to the shell. The
/// shell does not create the threads its primary components run on. Instead, it
/// is the embedder's responsibility to create threads and give the shell task
/// runners for those threads. Due to deterministic destruction of the shell,
/// the embedder can terminate all threads immediately after collecting the
/// shell. The shell must be created and destroyed on the same thread, but,
/// different shells (i.e. a separate instances of a Flutter application) may be
/// run on different threads simultaneously. The task runners themselves do not
/// have to be unique. If all task runner references given to the shell during
/// shell creation point to the same task runner, the Flutter application is
/// effectively single threaded.
///
/// The shell is the central nervous system of the Flutter application. None of
/// the shell components are thread safe and must be created, accessed and
/// destroyed on the same thread. To interact with one another, the various
/// components delegate to the shell for communication. Instead of using back
/// pointers to the shell, a delegation pattern is used by all components that
/// want to communicate with one another. Because of this, the shell implements
/// the delegate interface for all these components.
///
/// All shell methods accessed by the embedder may only be called on the
/// platform task runner. In case the embedder wants to directly access a shell
/// subcomponent, it is the embedder's responsibility to acquire a weak pointer
/// to that component and post a task to the task runner used by the component
/// to access its methods. The shell must also be destroyed on the platform
/// task runner.
///
class Shell final : public PlatformView::Delegate,
                    public Engine::Delegate,
                    public ResourceCacheLimitItem,
                    public clay::Recyclable {
 public:
  template <class T>
  using CreateCallbackFnPtr = std::unique_ptr<T> (*)(
      std::shared_ptr<clay::ServiceManager> service_manager);
  template <class T>
  using CreateCallback = std::function<std::unique_ptr<T>(
      std::shared_ptr<clay::ServiceManager> service_manager, Shell&)>;
  typedef std::function<std::unique_ptr<Engine>(
      std::shared_ptr<clay::ServiceManager> service_manager,
      TaskRunners task_runners, Settings settings,
      fml::RefPtr<GPUUnrefQueue> unref_queue, Engine::Delegate* delegate)>
      EngineCreateCallback;

  //----------------------------------------------------------------------------
  /// @brief      Creates a shell instance using the provided settings. The
  ///             callbacks to create the various shell subcomponents will be
  ///             called on the appropriate threads before this method returns.
  ///
  /// @param[in]  task_runners             The task runners
  /// @param[in]  settings                 The settings
  /// @param[in]  on_create_platform_view  The callback that must return a
  ///                                      platform view. This will be called on
  ///                                      the platform task runner before this
  ///                                      method returns.
  /// @param[in]  on_create_rasterizer     That callback that must provide a
  ///                                      valid rasterizer. This will be called
  ///                                      on the render task runner before this
  ///                                      method returns.
  /// @param[in]  is_gpu_disabled          The default value for the switch that
  ///                                      turns off the GPU.
  ///
  /// @return     A full initialized shell if the settings and callbacks are
  ///             valid. The root isolate has been created but not yet launched.
  ///             It may be launched by obtaining the engine weak pointer and
  ///             posting a task onto the UI task runner with a valid run
  ///             configuration to run the isolate. The embedder must always
  ///             check the validity of the shell (using the IsSetup call)
  ///             immediately after getting a pointer to it.
  ///
  static std::unique_ptr<Shell> Create(
      std::shared_ptr<clay::ServiceManager> service_manager,
      const TaskRunners& task_runners, Settings settings,
      const CreateCallback<PlatformView>& on_create_platform_view,
      const CreateCallbackFnPtr<Rasterizer>& on_create_rasterizer,
      bool is_gpu_disabled = false);

  //----------------------------------------------------------------------------
  /// @brief      Destroys the shell. This is a synchronous operation and
  ///             synchronous barrier blocks are introduced on the various
  ///             threads to ensure shutdown of all shell sub-components before
  ///             this method returns.
  ///
  ~Shell();

  //----------------------------------------------------------------------------
  /// @brief      Creates one Shell from another Shell where the created Shell
  ///             takes the opportunity to share any internal components it can.
  ///             This results is a Shell that has a smaller startup time cost
  ///             and a smaller memory footprint than an Shell created with the
  ///             Create function.
  ///
  /// @see        http://flutter.dev/go/multiple-engines
  std::unique_ptr<Shell> Spawn(
      std::shared_ptr<clay::ServiceManager> service_manager,
      const std::string& initial_route,
      const CreateCallback<PlatformView>& on_create_platform_view,
      const CreateCallbackFnPtr<Rasterizer>& on_create_rasterizer) const;

  std::shared_ptr<clay::ServiceManager> SpawnServiceManager() const;

  //------------------------------------------------------------------------------
  /// @return     The settings used to launch this shell.
  ///
  const Settings& GetSettings() const;

  //------------------------------------------------------------------------------
  /// @brief      If callers wish to interact directly with any shell
  ///             subcomponents, they must (on the platform thread) obtain a
  ///             task runner that the component is designed to run on and a
  ///             weak pointer to that component. They may then post a task to
  ///             that task runner, do the validity check on that task runner
  ///             before performing any operation on that component. This
  ///             accessor allows callers to access the task runners for this
  ///             shell.
  ///
  /// @return     The task runners current in use by the shell.
  ///
  const TaskRunners& GetTaskRunners() const;

  //------------------------------------------------------------------------------
  /// @brief      Engines may only be accessed on the UI thread. This method is
  ///             deprecated, and implementers should instead use other API
  ///             available on the Shell or the PlatformView.
  ///
  /// @return     A weak pointer to the engine.
  ///
  fml::WeakPtr<Engine> GetEngine();
  //----------------------------------------------------------------------------
  /// @brief      Platform views may only be accessed on the platform task
  ///             runner.
  ///
  /// @return     A weak pointer to the platform view.
  ///
  fml::WeakPtr<PlatformView> GetPlatformView();

  //----------------------------------------------------------------------------
  /// @brief      The IO Manager may only be accessed on the IO task runner.
  ///
  /// @return     A weak pointer to the IO manager.
  ///

  // Embedders should call this under low memory conditions to free up
  // internal caches used.
  //
  // This method posts a task to the raster threads to signal the Rasterizer to
  // free resources.

  //----------------------------------------------------------------------------
  /// @brief      Used by embedders to notify that there is a low memory
  ///             warning. The shell will attempt to purge caches. Current, only
  ///             the rasterizer cache is purged.
  void NotifyLowMemoryWarning() const;

  //----------------------------------------------------------------------------
  /// @brief      Used by embedders to check if all shell subcomponents are
  ///             initialized. It is the embedder's responsibility to make this
  ///             call before accessing any other shell method. A shell that is
  ///             not set up must be discarded and another one created with
  ///             updated settings.
  ///
  /// @return     Returns if the shell has been set up. Once set up, this does
  ///             not change for the life-cycle of the shell.
  ///
  bool IsSetup() const;

  //----------------------------------------------------------------------------
  /// @brief      Captures a screenshot and optionally Base64 encodes the data
  ///             of the last layer tree rendered by the rasterizer in this
  ///             shell.
  ///
  /// @param[in]  type           The type of screenshot to capture.
  /// @param[in]  base64_encode  If the screenshot data should be base64
  ///                            encoded.
  ///
  /// @return     The screenshot result.
  ///
  ScreenshotData ScreenshotSync(ScreenshotData::ScreenshotType type,
                                uint32_t background_color);

  void ScreenshotAsync(ScreenshotData::ScreenshotType type,
                       uint32_t background_color,
                       std::function<void(ScreenshotData)> callback);

  //----------------------------------------------------------------------------
  /// @brief      Pauses the calling thread until the first frame is presented.
  ///
  /// @param[in]  timeout  The duration to wait before timing out. If this
  ///                      duration would cause an overflow when added to
  ///                      std::chrono::steady_clock::now(), this method will
  ///                      wait indefinitely for the first frame.
  ///
  /// @return     'kOk' when the first frame has been presented before the
  ///             timeout successfully, 'kFailedPrecondition' if called from the
  ///             GPU or UI thread, 'kDeadlineExceeded' if there is a timeout.
  ///
  fml::Status WaitForFirstFrame(fml::TimeDelta timeout);

  //----------------------------------------------------------------------------
  /// @brief      Used by embedders to reload the system fonts in
  ///             FontCollection.
  ///             It also clears the cached font families and send system
  ///             channel message to framework to rebuild affected widgets.
  ///
  /// @return     Returns if shell reloads system fonts successfully.
  ///
  bool ReloadSystemFonts();

  //----------------------------------------------------------------------------
  /// @brief      Used by embedders to get the last error if one exists.
  ///
  /// @return     Returns the last error code from the UI Isolate.
  ///
  //----------------------------------------------------------------------------
  /// @brief     Accessor for the disable GPU SyncSwitch.
  std::shared_ptr<const fml::SyncSwitch> GetIsGpuDisabledSyncSwitch() const;

  //----------------------------------------------------------------------------
  /// @brief     Marks the GPU as available or unavailable.
  void SetGpuAvailability(GpuAvailability availability);

  //----------------------------------------------------------------------------
  /// @brief      Notifies the display manager of the updates.
  ///
  void OnDisplayUpdates(DisplayUpdateType update_type,
                        std::vector<std::unique_ptr<Display>> displays);

  //----------------------------------------------------------------------------
  /// @brief Queries the `DisplayManager` for the main display refresh rate.
  ///
  double GetMainDisplayRefreshRate();

  //----------------------------------------------------------------------------
  /// @brief      Install a new factory that can match against and decode image
  ///             data.
  /// @param[in]  factory   Callback that produces `ImageGenerator`s for
  ///                       compatible input data.
  /// @param[in]  priority  The priority used to determine the order in which
  ///                       factories are tried. Higher values mean higher
  ///                       priority. The built-in Skia decoders are installed
  ///                       at priority 0, and so a priority > 0 takes precedent
  ///                       over the builtin decoders. When multiple decoders
  ///                       are added with the same priority, those which are
  ///                       added earlier take precedent.
  /// @see        `CreateCompatibleGenerator`
  // void RegisterImageDecoder(ImageGeneratorFactory factory, int32_t priority);

  // |Engine::Delegate|

  const std::weak_ptr<VsyncWaiter> GetVsyncWaiter() const;

  // Engine::Delegate
  void ShowSoftInput(int type, int action) override;
  void HideSoftInput() override;

  std::string ShouldInterceptUrl(const std::string& origin_url,
                                 bool should_decode) override;
  std::shared_ptr<clay::ResourceLoaderIntercept> GetResourceLoaderIntercept()
      override;

  void MakeRasterSnapshot(
      std::unique_ptr<LayerTree> layer_tree,
      std::function<void(fml::RefPtr<PaintImage>)> callback) override;
  fml::RefPtr<PaintImage> MakeRasterSnapshot(GrPicturePtr picture,
                                             skity::Vec2 picture_size) override;

  void OnDrawHardware(bool force_draw);

  void SetClipboardData(const std::u16string& data) override {
    platform_view_->SetClipboardData(data);
  }

  std::u16string GetClipboardData() override {
    return platform_view_->GetClipboardData();
  }

#if defined(OS_WIN) || defined(OS_MAC) || defined(ENABLE_HEADLESS)
  // Text input related functions Begin.
  void SetTextInputClient(int client_id, const char* input_action,
                          const char* input_type) override {
    platform_view_->SetTextInputClient(client_id, input_action, input_type);
  }

  void ClearTextInputClient() override {
    platform_view_->ClearTextInputClient();
  }

  void SetEditableTransform(const float transform_matrix[16]) override {
    platform_view_->SetEditableTransform(transform_matrix);
  }

  void SetEditingState(uint64_t selection_base, uint64_t composing_extent,
                       const std::string& selection_affinity,
                       const std::string& text, bool selection_directional,
                       uint64_t selection_extent,
                       uint64_t composing_base) override {
    platform_view_->SetEditingState(
        selection_base, composing_extent, selection_affinity, text,
        selection_directional, selection_extent, composing_base);
  }

  void SetCaretRect(float x, float y, float width, float height) override {
    platform_view_->SetCaretRect(x, y, width, height);
  }

  void setMarkedTextRect(float x, float y, float width, float height) override {
    platform_view_->setMarkedTextRect(x, y, width, height);
  }

  void ShowTextInput() override { platform_view_->ShowTextInput(); }

  void HideTextInput() override { platform_view_->HideTextInput(); }
  // Text input related functions End.
  void WindowMove() override { platform_view_->WindowMove(); }
  void ActivateSystemCursor(int type, const std::string& path) override {
    platform_view_->ActivateSystemCursor(type, path);
  }
#endif

  std::string GetRasterCacheInfo();

  void DumpInfoToDevtoolEnabled(bool enabled) override;

  void UpdateRasterInfo(const std::vector<RasterCacheInfo>& raster_cache_info);
  void SetDevtoolInstrumentation(
      std::unique_ptr<clay::DevtoolsInstrumentation> devtool_instrumentation) {
    devtool_instrumentation_ = std::move(devtool_instrumentation);
  }
  clay::DevtoolsInstrumentation* GetDevtoolInstrumentation() {
    return devtool_instrumentation_.get();
  }

  void CreateTimingClientDelegate();
  void ReportTiming(const std::unordered_map<std::string, int64_t>& timing,
                    const std::string& flag) override;

  void UpdateRootSize(int32_t width, int32_t height) override;

  void CleanForRecycle() override;
  void PrepareForRecycle() override;

  void RegisterDrawableImage(
      std::shared_ptr<DrawableImage> drawable_image) override;
  void UnregisterDrawableImage(int64_t image_id) override;

  const std::shared_ptr<clay::ServiceManager>& GetServiceManager() const {
    return service_manager_;
  }

  const std::shared_ptr<clay::PerfCollector>& GetPerfCollector() const {
    return perf_collector_;
  }

  double GetDevicePixelRatio() const { return device_pixel_ratio_; }

#if OS_ANDROID || OS_WIN || OS_MAC
  void SetUpMemoryPressureListener();
#endif

#ifdef ENABLE_ACCESSIBILITY
  void SetSemanticsEnabled(bool enabled);
  void SetPageEnableAccessibilityElement(bool enabled);
#endif

  void OnPostInvalidate(bool is_raster_frame);

 private:
  const std::shared_ptr<clay::ServiceManager> service_manager_;
  const TaskRunners task_runners_;
  const clay::Puppet<clay::Owner::kPlatform, AnimatorInfoService>
      animator_info_service_;
  const clay::Puppet<clay::Owner::kPlatform, RasterizerService>
      rasterizer_service_;
  std::shared_ptr<ResourceCacheLimitCalculator>
      resource_cache_limit_calculator_;
  size_t resource_cache_limit_;
  const Settings settings_;
  std::unique_ptr<Engine> engine_;
  std::unique_ptr<PlatformView> platform_view_;  // on platform task runner

  std::shared_ptr<fml::SyncSwitch> is_gpu_disabled_sync_switch_;

  std::shared_ptr<clay::PerfCollector> perf_collector_;
  std::unique_ptr<clay::TimingClientDelegate> timing_client_delegate_;
  std::unique_ptr<clay::DevtoolsInstrumentation> devtool_instrumentation_;

  fml::WeakPtr<Engine> weak_engine_;  // to be shared across threads
  fml::WeakPtr<PlatformView>
      weak_platform_view_;  // to be shared across threads

  bool is_setup_ = false;
  bool is_added_to_service_protocol_ = false;
  uint64_t next_pointer_flow_id_ = 0;

  std::shared_ptr<std::atomic<bool>> waiting_for_first_frame_ =
      std::make_shared<std::atomic<bool>>(true);
  std::mutex waiting_for_first_frame_mutex_;
  std::condition_variable waiting_for_first_frame_condition_;
  std::shared_ptr<std::atomic<bool>> screenshot =
      std::make_shared<std::atomic<bool>>(true);

  // Whether there's a task scheduled to report the timings.
  bool frame_timings_report_scheduled_ = false;

  // Vector of FrameTiming::kCount * n timestamps for n frames whose timings
  // have not been reported yet.
  std::vector<int64_t> unreported_timings_;

  /// Manages the displays. This class is thread safe, can be accessed from any
  /// of the threads.
  std::shared_ptr<DisplayManager> display_manager_;

  // protects expected_frame_size_ which is set on platform thread and read on
  // raster thread
  std::shared_ptr<std::mutex> resize_mutex_ = std::make_shared<std::mutex>();

  // used to discard wrong size layer tree produced during interactive resizing
  std::shared_ptr<skity::Vec2> expected_frame_size_ =
      std::make_shared<skity::Vec2>();

  // Used to communicate the right frame bounds via service protocol.
  double device_pixel_ratio_ = 0.0;

  // How many frames have been timed since last report.
  size_t UnreportedFramesCount() const;

  bool has_set_gr_context_ = false;

#if OS_ANDROID || OS_WIN || OS_MAC
  std::unique_ptr<clay::MemoryPressureListener> memory_pressure_listener_ =
      nullptr;
#endif

  bool devtools_instrumentation_enabled_ = false;

  Shell(std::shared_ptr<clay::ServiceManager> service_manager,
        const TaskRunners& task_runners,
        const std::shared_ptr<ResourceCacheLimitCalculator>&
            resource_cache_limit_calculator,
        const Settings& settings, bool is_gpu_disabled);

  static std::unique_ptr<Shell> CreateShellOnPlatformThread(
      std::shared_ptr<clay::ServiceManager> service_manager,
      const std::shared_ptr<ResourceCacheLimitCalculator>&
          resource_cache_limit_calculator,
      const TaskRunners& task_runners, const Settings& settings,
      const Shell::CreateCallback<PlatformView>& on_create_platform_view,
      const Shell::CreateCallbackFnPtr<Rasterizer>& on_create_rasterizer,
      const EngineCreateCallback& on_create_engine, bool is_gpu_disabled);

  static std::unique_ptr<Shell> CreateWithSnapshot(
      std::shared_ptr<clay::ServiceManager> service_manager,
      const TaskRunners& task_runners,
      const std::shared_ptr<ResourceCacheLimitCalculator>&
          resource_cache_limit_calculator,
      Settings settings,
      const CreateCallback<PlatformView>& on_create_platform_view,
      const CreateCallbackFnPtr<Rasterizer>& on_create_rasterizer,
      const EngineCreateCallback& on_create_engine, bool is_gpu_disabled);

  bool Setup(std::unique_ptr<PlatformView> platform_view,
             std::unique_ptr<Engine> engine);

  void ReportTimings();

  // |PlatformView::Delegate|
  void OnPlatformViewCreated() override;

  // |PlatformView::Delegate|
  void OnPlatformViewDestroyed() override;

  // |PlatformView::Delegate|
  void OnPlatformViewScheduleFrame() override;

  // |PlatformView::Delegate|
  void OnPlatformViewSetViewportMetrics(
      const ViewportMetrics& metrics) override;

  // |PlatformView::Delegate|
  bool OnPlatformViewDispatchPointerDataPacket(
      std::unique_ptr<PointerDataPacket> packet) override;

  void OnPlatformViewDispatchKeyEvent(
      std::unique_ptr<clay::KeyEvent> key_event,
      std::function<void(bool /* handled */)> callback) override;
  // |PlatformView::Delegate|
  void OnPlatformViewSendKeyboardEvent(
      std::unique_ptr<clay::KeyEvent> key_event) override;

  // |PlatformView::Delegate|
  void OnPlatformViewPerformEditorAction(
      clay::KeyboardAction action_code) override;

  // |PlatformView::Delegate|
  void OnPlatformViewDeleteSurroundingText(int before_length,
                                           int after_length) override;

  // |PlatformView::Delegate|
  void OnPlatformViewOnEnterForeground() override;

  // |PlatformView::Delegate|
  void OnPlatformViewOnEnterBackground() override;

  // |PlatformView::Delegate|
  void OnPlatformViewSetDefaultFocusRingEnabled(bool enable) override;

  // |PlatformView::Delegate|
  void OnPlatformViewSetPerformanceOverlayEnabled(bool enable) override;

  // |PlatformView::Delegate|
  void OnDefaultOverflowVisibleChanged(bool visible) override;

  // |PlatformView::Delegate|
  void OnExposurePropsChanged(int freq,
                              bool enable_exposure_ui_margin) override;

  // |PlatformView::Delegate|
  void OnPlatformViewSetNextFrameCallback(const fml::closure& closure) override;

  // |PlatformView::Delegate|
  const Settings& OnPlatformViewGetSettings() const override;

#ifdef ENABLE_ACCESSIBILITY
  void OnPlatformViewDispatchSemanticsAction(int virtual_view_id,
                                             int action) override;
  void UpdateSemantics(const clay::SemanticsUpdateNodes& update_nodes) override;
#endif

  // |ResourceCacheLimitItem|
  size_t GetResourceCacheLimit() override { return resource_cache_limit_; }

  fml::WeakPtrFactory<Shell> weak_factory_;

  friend class testing::ShellTest;

  BASE_DISALLOW_COPY_AND_ASSIGN(Shell);
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SHELL_H_
