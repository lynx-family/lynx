// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_PLATFORM_VIEW_H_
#define CLAY_SHELL_COMMON_PLATFORM_VIEW_H_

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/fml/macros.h"
#include "base/include/fml/memory/weak_ptr.h"
#include "clay/common/task_runners.h"
#include "clay/flow/layers/layer_tree.h"
#include "clay/flow/surface.h"
#include "clay/fml/mapping.h"
#include "clay/shell/common/pointer_data_dispatcher.h"
#include "clay/shell/common/vsync_waiter.h"
#include "clay/ui/event/key_event.h"
#include "clay/ui/platform/keyboard_types.h"
#ifdef ENABLE_ACCESSIBILITY
#include "clay/ui/semantics/semantics_update_node.h"
#endif
#include "clay/ui/window/key_data_packet.h"
#include "clay/ui/window/pointer_data_packet.h"
#include "clay/ui/window/pointer_data_packet_converter.h"
#include "clay/ui/window/viewport_metrics.h"

namespace clay {
class ServiceManager;
class ResourceLoaderIntercept;
}  // namespace clay

namespace clay {

class OutputSurface;

//------------------------------------------------------------------------------
/// @brief      Platform views are created by the shell on the platform task
///             runner. Unless explicitly specified, all platform view methods
///             are called on the platform task runner as well. Platform views
///             are usually sub-classed on a per platform basis and the bulk of
///             the window system integration happens using that subclass. Since
///             most platform window toolkits are usually only safe to access on
///             a single "main" thread, any interaction that requires access to
///             the underlying platform's window toolkit is routed through the
///             platform view associated with that shell. This involves
///             operations like settings up and tearing down the render surface,
///             interacting with accessibility features on the platform, input
///             events, etc.
///
class PlatformView {
 public:
  //----------------------------------------------------------------------------
  /// @brief      Used to forward events from the platform view to interested
  ///             subsystems. This forwarding is done by the shell which sets
  ///             itself up as the delegate of the platform view.
  ///
  class Delegate {
   public:
    using KeyDataResponse = std::function<void(bool)>;

    //--------------------------------------------------------------------------
    /// @brief      Notifies the delegate that the platform view was created
    ///             with the given render surface. This surface is platform
    ///             (iOS, Android) and client-rendering API (OpenGL, Software,
    ///             Metal, Vulkan) specific. This is usually a sign to the
    ///             rasterizer to set up and begin rendering to that surface.
    ///
    ///
    virtual void OnPlatformViewCreated() = 0;

    //--------------------------------------------------------------------------
    /// @brief      Notifies the delegate that the platform view was destroyed.
    ///             This is usually a sign to the rasterizer to suspend
    ///             rendering a previously configured surface and collect any
    ///             intermediate resources.
    ///
    virtual void OnPlatformViewDestroyed() = 0;

    //--------------------------------------------------------------------------
    /// @brief      Notifies the delegate that the platform needs to schedule a
    ///             frame to regenerate the layer tree and redraw the surface.
    ///
    virtual void OnPlatformViewScheduleFrame() = 0;

    //--------------------------------------------------------------------------
    /// @brief      Notifies the delegate that the specified callback needs to
    ///             be invoked after the rasterizer is done rendering the next
    ///             frame. This callback will be called on the render thread and
    ///             it is caller responsibility to perform any re-threading as
    ///             necessary. Due to the asynchronous nature of rendering in
    ///             Flutter, embedders usually add a placeholder over the
    ///             contents in which Flutter is going to render when Flutter is
    ///             first initialized. This callback may be used as a signal to
    ///             remove that placeholder.
    ///
    /// @attention  The callback will be invoked on the render thread and not
    ///             the calling thread.
    ///
    /// @param[in]  closure  The callback to execute on the next frame.
    ///
    virtual void OnPlatformViewSetNextFrameCallback(
        const fml::closure& closure) = 0;

    //--------------------------------------------------------------------------
    /// @brief      Notifies the delegate the viewport metrics of the platform
    ///             view have been updated. The rasterizer will need to be
    ///             reconfigured to render the frame in the updated viewport
    ///             metrics.
    ///
    /// @param[in]  metrics  The updated viewport metrics.
    ///
    virtual void OnPlatformViewSetViewportMetrics(
        const ViewportMetrics& metrics) = 0;

    //--------------------------------------------------------------------------
    /// @brief      Notifies the delegate that the platform view has encountered
    ///             a pointer event. This pointer event needs to be forwarded to
    ///             the running root isolate hosted by the engine on the UI
    ///             thread.
    ///
    /// @param[in]  packet  The pointer data packet containing multiple pointer
    ///                     events.
    ///
    virtual bool OnPlatformViewDispatchPointerDataPacket(
        std::unique_ptr<PointerDataPacket> packet) = 0;

    //--------------------------------------------------------------------------
    /// @brief      Notifies the delegate that the platform view has encountered
    ///             a key event. This key event and the callback needs to be
    ///             forwarded to the running root isolate hosted by the engine
    ///             on the UI thread.
    ///
    /// @param[in]  key_event    The key data containing one key event.
    /// @param[in]  callback  Called when the framework has decided whether
    ///                       to handle this key data.
    ///
    virtual void OnPlatformViewDispatchKeyEvent(
        std::unique_ptr<clay::KeyEvent> key_event,
        std::function<void(bool /* handled */)> callback) = 0;

    virtual void OnPlatformViewSendKeyboardEvent(
        std::unique_ptr<clay::KeyEvent> key_event) = 0;

    virtual void OnPlatformViewPerformEditorAction(
        clay::KeyboardAction action_code) = 0;

    virtual void OnPlatformViewDeleteSurroundingText(int before_length,
                                                     int after_length) = 0;

    virtual void OnPlatformViewOnEnterForeground() = 0;
    virtual void OnPlatformViewOnEnterBackground() = 0;

    virtual void OnPlatformViewSetDefaultFocusRingEnabled(bool enable) = 0;
    virtual void OnPlatformViewSetPerformanceOverlayEnabled(bool enable) = 0;
    virtual void OnDefaultOverflowVisibleChanged(bool visible) = 0;
    virtual void OnExposurePropsChanged(int freq,
                                        bool enable_dump_element_tree) = 0;

#ifdef ENABLE_ACCESSIBILITY
    virtual void OnPlatformViewDispatchSemanticsAction(int virtual_view_id,
                                                       int action) = 0;
#endif

    //--------------------------------------------------------------------------
    /// @brief      Called by the platform view on the platform thread to get
    ///             the settings object associated with the platform view
    ///             instance.
    ///
    /// @return     The settings.
    ///
    virtual const Settings& OnPlatformViewGetSettings() const = 0;
  };

  //----------------------------------------------------------------------------
  /// @brief      Creates a platform view with the specified delegate and task
  ///             runner. The base class by itself does not do much but is
  ///             suitable for use in test environments where full platform
  ///             integration may not be necessary. The platform view may only
  ///             be created, accessed and destroyed on the platform task
  ///             runner.
  ///
  /// @param      delegate      The delegate. This is typically the shell.
  /// @param[in]  task_runners  The task runners used by this platform view.
  ///
  PlatformView(std::shared_ptr<clay::ServiceManager> service_manager,
               Delegate& delegate, const TaskRunners& task_runners);

  //----------------------------------------------------------------------------
  /// @brief      Destroys the platform view. The platform view is owned by the
  ///             shell and will be destroyed by the same on the platform tasks
  ///             runner.
  ///
  virtual ~PlatformView();

  //----------------------------------------------------------------------------
  /// @brief      Used by embedders to specify the updated viewport metrics. In
  ///             response to this call, on the raster thread, the rasterizer
  ///             may need to be reconfigured to the updated viewport
  ///             dimensions. On the UI thread, the framework may need to start
  ///             generating a new frame for the updated viewport metrics as
  ///             well.
  ///
  /// @param[in]  metrics  The updated viewport metrics.
  ///
  void SetViewportMetrics(const ViewportMetrics& metrics);

  //----------------------------------------------------------------------------
  /// @brief      Used by embedders to notify the shell that a platform view
  ///             has been created. This notification is used to create a
  ///             rendering surface and pick the client rendering API to use to
  ///             render into this surface. No frames will be scheduled or
  ///             rendered before this call. The surface must remain valid till
  ///             the corresponding call to NotifyDestroyed.
  ///
  void NotifyCreated();

  //----------------------------------------------------------------------------
  /// @brief      Used by embedders to notify the shell that the platform view
  ///             has been destroyed. This notification used to collect the
  ///             rendering surface and all associated resources. Frame
  ///             scheduling is also suspended.
  ///
  /// @attention  Subclasses may choose to override this method to perform
  ///             platform specific functions. However, they must call the base
  ///             class method at some point in their implementation.
  ///
  virtual void NotifyDestroyed();

  //----------------------------------------------------------------------------
  /// @brief      Used by embedders to schedule a frame. In response to this
  ///             call, the framework may need to start generating a new frame.
  ///
  void ScheduleFrame();

  //--------------------------------------------------------------------------
  /// @brief      Returns a platform-specific PointerDataDispatcherMaker so the
  ///             `Engine` can construct the PointerDataPacketDispatcher based
  ///             on platforms.
  virtual PointerDataDispatcherMaker GetDispatcherMaker();

  //----------------------------------------------------------------------------
  /// @brief      Returns a weak pointer to the platform view. Since the
  ///             platform view may only be created, accessed and destroyed
  ///             on the platform thread, any access to the platform view
  ///             from a non-platform task runner needs a weak pointer to
  ///             the platform view along with a reference to the platform
  ///             task runner. A task must be posted to the platform task
  ///             runner with the weak pointer captured in the same. The
  ///             platform view method may only be called in the posted task
  ///             once the weak pointer validity has been checked. This
  ///             method is used by callers to obtain that weak pointer.
  ///
  /// @return     The weak pointer to the platform view.
  ///
  fml::WeakPtr<PlatformView> GetWeakPtr() const;

  //----------------------------------------------------------------------------
  /// @brief      Gives embedders a chance to react to a "cold restart" of the
  ///             running isolate. The default implementation of this method
  ///             does nothing.
  ///
  ///             While a "hot restart" patches a running isolate, a "cold
  ///             restart" restarts the root isolate in a running shell.
  ///
  virtual void OnPreEngineRestart() const;

  //----------------------------------------------------------------------------
  /// @brief      Sets a callback that gets executed when the rasterizer renders
  ///             the next frame. Due to the asynchronous nature of
  ///             rendering in Flutter, embedders usually add a placeholder
  ///             over the contents in which Flutter is going to render when
  ///             Flutter is first initialized. This callback may be used as
  ///             a signal to remove that placeholder. The callback is
  ///             executed on the render task runner and not the platform
  ///             task runner. It is the embedder's responsibility to
  ///             re-thread as necessary.
  ///
  /// @attention  The callback is executed on the render task runner and not the
  ///             platform task runner. Embedders must re-thread as necessary.
  ///
  /// @param[in]  closure  The callback to execute on the render thread when the
  ///                      next frame gets rendered.
  ///
  void SetNextFrameCallback(const fml::closure& closure);

  //----------------------------------------------------------------------------
  /// @brief      Dispatches pointer events from the embedder to the
  ///             framework. Each pointer data packet may contain multiple
  ///             pointer input events. Each call to this method wakes up
  ///             the UI thread.
  ///
  /// @param[in]  packet  The pointer data packet to dispatch to the framework.
  ///
  bool DispatchPointerDataPacket(std::unique_ptr<PointerDataPacket> packet);
  //----------------------------------------------------------------------------
  /// @brief      Dispatches key events from the embedder to the framework. Each
  ///             key data packet contains one physical event and multiple
  ///             logical key events. Each call to this method wakes up the UI
  ///             thread.
  ///
  /// @param[in]  key_event  The key data to dispatch to the framework.
  ///
  void DispatchKeyEvent(std::unique_ptr<clay::KeyEvent> key_event,
                        Delegate::KeyDataResponse callback);

  void DispatchKeyDataPacket(std::unique_ptr<clay::KeyDataPacket> packet,
                             Delegate::KeyDataResponse callback);

  //--------------------------------------------------------------------------
  /// @brief      Directly invokes platform-specific APIs to compute the
  ///             locale the platform would have natively resolved to.
  ///
  /// @param[in]  supported_locale_data  The vector of strings that represents
  ///                                    the locales supported by the app.
  ///                                    Each locale consists of three
  ///                                    strings: languageCode, countryCode,
  ///                                    and scriptCode in that order.
  ///
  /// @return     A vector of 3 strings languageCode, countryCode, and
  ///             scriptCode that represents the locale selected by the
  ///             platform. Empty strings mean the value was unassigned. Empty
  ///             vector represents a null locale.
  ///
  virtual std::unique_ptr<std::vector<std::string>>
  ComputePlatformResolvedLocales(
      const std::vector<std::string>& supported_locale_data);

  virtual void ShowSoftInput(int type, int action) {}
  virtual void HideSoftInput() {}

  virtual std::string ShouldInterceptUrl(const std::string& origin_url,
                                         bool should_decode) {
    return origin_url;
  }

  virtual std::shared_ptr<clay::ResourceLoaderIntercept>
  GetResourceLoaderIntercept() {
    return nullptr;
  }

  virtual void PostInvalidate() {}

  virtual void SubmitFrameFuture(std::future<bool> future) {}

  virtual void UpdateRasterCacheInfo(const std::string& result) {}
  virtual void SetClipboardData(const std::u16string& data) {}
  virtual std::u16string GetClipboardData() { return std::u16string(); }
  virtual void SetTextInputClient(int client_id, const char* input_action,
                                  const char* input_type) {}
  virtual void ClearTextInputClient() {}
  virtual void SetEditableTransform(const float transform_matrix[16]) {}
  virtual void SetEditingState(uint64_t selection_base,
                               uint64_t composing_extent,
                               const std::string& selection_affinity,
                               const std::string& text,
                               bool selection_directional,
                               uint64_t selection_extent,
                               uint64_t composing_base) {}
  virtual void SetCaretRect(float x, float y, float width, float height) {}
  virtual void setMarkedTextRect(float x, float y, float width, float height) {}
  virtual void ShowTextInput() {}
  virtual void HideTextInput() {}
  virtual std::string InputFilter(const std::string& input,
                                  const std::string& pattern) {
    return input;
  };

  virtual void WindowMove() {}
  virtual void ActivateSystemCursor(int type, const std::string& path) {}

  const TaskRunners& GetTaskRunners() const { return task_runners_; }

  virtual void OnTimingSetup(
      const std::unordered_map<std::string, int64_t>& timing) {}
  virtual void OnTimingUpdate(
      const std::unordered_map<std::string, int64_t>& timing,
      const std::string& flag) {}

  virtual void ReleasePlatformInstanceManager() {}

  virtual void UpdateRootSize(int32_t width, int32_t height) {}
#ifdef ENABLE_ACCESSIBILITY
  void DispatchClaySemanticsAction(int virtual_view_id, int action);
  virtual void UpdateSemantics(const clay::SemanticsUpdateNodes& update_nodes) {
  }
#endif

  const std::shared_ptr<clay::ServiceManager>& GetServiceManager() const {
    return service_manager_;
  }

  // If the output surface is thread safe, it's safe to destruct
  // independently, otherwise the shell must wait for platform surface teardown
  // before destructing platform view.
  // The default implementation returns false.
  virtual bool IsOutputSurfaceThreadSafe() const { return false; }

  virtual fml::RefPtr<OutputSurface> GetOutputSurface() const = 0;

 protected:
  const std::shared_ptr<clay::ServiceManager> service_manager_;
  PlatformView::Delegate& delegate_;
  const TaskRunners task_runners_;
  PointerDataPacketConverter pointer_data_packet_converter_;
  fml::WeakPtrFactory<PlatformView> weak_factory_;  // Must be the last member.

 private:
  BASE_DISALLOW_COPY_AND_ASSIGN(PlatformView);
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_PLATFORM_VIEW_H_
