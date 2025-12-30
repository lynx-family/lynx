// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_ENGINE_H_
#define CLAY_SHELL_COMMON_ENGINE_H_

#include <future>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/fml/memory/weak_ptr.h"
#include "clay/common/element_id.h"
#include "clay/common/recyclable.h"
#include "clay/common/task_runners.h"
#include "clay/flow/frame_timings.h"
#include "clay/gfx/animation/animation_data.h"
#include "clay/gfx/gpu_object.h"
#include "clay/shell/common/services/raster_frame_service.h"
#include "clay/shell/common/services/ui_frame_service.h"
#include "clay/shell/common/vsync_waiter.h"
#include "clay/ui/common/frame_timing_collector.h"
#include "clay/ui/common/isolate.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/view_context.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/event/key_event.h"
#include "clay/ui/platform/keyboard_types.h"
#include "clay/ui/render_delegate.h"
#include "clay/ui/resource/font_collection.h"
#include "clay/ui/shadow/shadow_node_owner.h"
#include "clay/ui/window/viewport_metrics.h"

namespace clay {
class ResourceLoaderIntercept;
}  // namespace clay

namespace clay {

using clay::GPUUnrefQueue;

namespace testing {
class ShellTest;
}

class LayerTree;

class Engine : public clay::RenderDelegate, public clay::Recyclable {
 public:
  class Delegate {
   public:
    virtual void ShowSoftInput(int type, int action) = 0;
    virtual void HideSoftInput() = 0;
    virtual void FilterInputAsync(
        const std::string& input, const std::string& pattern,
        std::function<void(const std::string&)> callback) = 0;
    virtual std::string ShouldInterceptUrl(const std::string& origin_url,
                                           bool should_decode) = 0;
    virtual std::shared_ptr<clay::ResourceLoaderIntercept>
    GetResourceLoaderIntercept() {
      return nullptr;
    }

    virtual void MakeRasterSnapshot(
        std::unique_ptr<LayerTree> layer_tree,
        std::function<void(fml::RefPtr<PaintImage>)> callback) = 0;
    virtual fml::RefPtr<PaintImage> MakeRasterSnapshot(
        GrPicturePtr picture, skity::Vec2 picture_size) = 0;
    virtual void DumpInfoToDevtoolEnabled(bool enabled) {}
    virtual void SetClipboardData(const std::u16string& data) = 0;
    virtual std::u16string GetClipboardData() = 0;
#if defined(OS_WIN) || defined(OS_MAC) || defined(ENABLE_HEADLESS)
    // Text input related functions Begin.
    virtual void SetTextInputClient(int client_id, const char* input_action,
                                    const char* input_type) = 0;
    virtual void ClearTextInputClient() = 0;
    virtual void SetEditableTransform(const float transform_matrix[16]) = 0;
    virtual void SetEditingState(uint64_t selection_base,
                                 uint64_t composing_extent,
                                 const std::string& selection_affinity,
                                 const std::string& text,
                                 bool selection_directional,
                                 uint64_t selection_extent,
                                 uint64_t composing_base) = 0;
    virtual void SetCaretRect(float x, float y, float width, float height) = 0;
    virtual void setMarkedTextRect(float x, float y, float width,
                                   float height) = 0;
    virtual void ShowTextInput() = 0;
    virtual void HideTextInput() = 0;
    // Text input related functions End.
    virtual void WindowMove() = 0;
    virtual void ActivateSystemCursor(int type, const std::string& path) = 0;
#endif
    virtual void ReportTiming(
        const std::unordered_map<std::string, int64_t>& timing,
        const std::string& flag) = 0;
    virtual void UpdateRootSize(int32_t width, int32_t height) = 0;

#ifdef ENABLE_ACCESSIBILITY
    virtual void UpdateSemantics(
        const clay::SemanticsUpdateNodes& update_nodes) = 0;
#endif

    virtual void RegisterDrawableImage(
        std::shared_ptr<DrawableImage> drawable_image) = 0;
    virtual void UnregisterDrawableImage(int64_t id) = 0;
  };

  Engine(std::shared_ptr<clay::ServiceManager> service_manager,
         TaskRunners task_runners, Settings settings,
         fml::RefPtr<GPUUnrefQueue> unref_queue, Delegate* delegate);
  ~Engine();

  fml::WeakPtr<Engine> GetWeakPtr() const;
  std::unique_ptr<Engine> Spawn(
      std::shared_ptr<clay::ServiceManager> service_manager, Settings settings,
      fml::RefPtr<GPUUnrefQueue> unref_queue, Delegate* delegate) const;

  bool BeginFrame(std::unique_ptr<FrameTimingsRecorder> recorder);

  void SetViewportMetrics(const ViewportMetrics& metrics);
  void SetupDefaultFontManager();
  bool DispatchPointerEvent(std::vector<clay::PointerEvent> events);
  void DispatchKeyEvent(std::unique_ptr<clay::KeyEvent> event,
                        std::function<void(bool /* handled */)> callback);
  void SendKeyboardEvent(std::unique_ptr<clay::KeyEvent> key_event);
  void PerformEditorAction(clay::KeyboardAction action_code);
  void DeleteSurroundingText(int before_length, int after_length);
  void SetRenderSettings(fml::RefPtr<clay::RenderSettings> render_settings);

  void OnEnterForeground();
  void OnEnterBackground();

  void SetVisible(bool enable);
  void RequestPaint();

  void SetDefaultFocusRingEnabled(bool enable);
  void SetPerformanceOverlayEnabled(bool enable);
  void SetDefaultOverflowVisible(bool visible);
  void SetExposureProps(int freq, bool enable_exposure_ui_margin);

  /// Schedule a frame with the default parameter of regenerating the layer
  /// tree.
  void ScheduleLayout() override;

  // clay::RenderDelegate
  void ScheduleFrame() override;
  void ForceBeginFrame() override;
  void OnFirstMeaningfulLayout() override;
  bool Raster(std::unique_ptr<clay::LayerTree> layer_tree,
              std::unique_ptr<clay::FrameTimingsRecorder> recorder = nullptr,
              bool force = false) override;
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
  clay::BaseView* FindViewById(int view_id) override;
  clay::ShadowNode* FindShadowNodeById(int node_id) override;

  // compositor animation.
  void OnAnimationEvent(const clay::ElementId& element_id,
                        const clay::AnimationParams& animation_params);
  void OnTransitionEvent(const clay::ElementId& element_id,
                         const clay::AnimationParams& animation_params,
                         ClayAnimationPropertyType property_type);

  void DumpInfoToDevtoolEnabled(bool enabled) override {
    delegate_->DumpInfoToDevtoolEnabled(enabled);
  }

  std::shared_ptr<clay::FontCollection> GetFontCollection();
  clay::ViewContext* GetViewContext() { return view_context_.get(); }
  std::shared_ptr<clay::ViewContext> view_context() const {
    return view_context_;
  }
  clay::PageView* GetPageView() { return page_view_.get(); }
  const std::shared_ptr<clay::ServiceManager>& GetServiceManager() const {
    return service_manager_;
  }

  void SetFrameTimingCollector(
      std::shared_ptr<clay::FrameTimingCollector> frame_timing_collector);

  void OnPlatformViewCreated();

  void OnOutputSurfaceCreated();
  void OnOutputSurfaceCreateFailed();
  void OnOutputSurfaceDestroyed();

  void SetFontFaceCache(const char* font_family, const char* local_path);

  void UpdateMemoryCacheOptions();

  bool MarkDrawableImageFrameAvailable(int64_t texture_id);

  void SetClipboardData(const std::u16string& data) override {
    delegate_->SetClipboardData(data);
  }

  std::u16string GetClipboardData() override {
    return delegate_->GetClipboardData();
  }

#if defined(OS_WIN) || defined(OS_MAC) || defined(ENABLE_HEADLESS)
  // Text input related functions Begin.
  void SetTextInputClient(int client_id, const char* input_action,
                          const char* input_type) override {
    delegate_->SetTextInputClient(client_id, input_action, input_type);
  }

  void ClearTextInputClient() override { delegate_->ClearTextInputClient(); }

  void SetEditableTransform(const float transform_matrix[16]) override {
    delegate_->SetEditableTransform(transform_matrix);
  }

  void SetEditingState(uint64_t selection_base, uint64_t composing_extent,
                       const std::string& selection_affinity,
                       const std::string& text, bool selection_directional,
                       uint64_t selection_extent,
                       uint64_t composing_base) override {
    delegate_->SetEditingState(selection_base, composing_extent,
                               selection_affinity, text, selection_directional,
                               selection_extent, composing_base);
  }

  void SetCaretRect(float x, float y, float width, float height) override {
    delegate_->SetCaretRect(x, y, width, height);
  }

  void setMarkedTextRect(float x, float y, float width, float height) override {
    delegate_->setMarkedTextRect(x, y, width, height);
  }

  void ShowTextInput() override { delegate_->ShowTextInput(); }

  void HideTextInput() override { delegate_->HideTextInput(); }
  // Text input related functions End.
  void WindowMove() override { delegate_->WindowMove(); }
  void ActivateSystemCursor(int type, const std::string& path) override {
    delegate_->ActivateSystemCursor(type, path);
  }
#endif

  void FilterInputAsync(
      const std::string& input, const std::string& pattern,
      std::function<void(const std::string&)> callback) override {
    delegate_->FilterInputAsync(input, pattern, callback);
  }
  void ReportTiming(const std::unordered_map<std::string, int64_t>& timing,
                    const std::string& flag) override;

  const std::weak_ptr<VsyncWaiter> GetVsyncWaiter() const;

  void ClearTextCache() override;

  void UpdateRootSize(int32_t width, int32_t height) override;

#ifdef ENABLE_ACCESSIBILITY
  void UpdateSemantics(const clay::SemanticsUpdateNodes& update_nodes) override;
  void SetSemanticsEnabled(bool enabled);
  void SetPageEnableAccessibilityElement(bool enabled);
  void DispatchSemanticsAction(int virtual_view_id, int action);
#endif

  void RegisterDrawableImage(
      std::shared_ptr<DrawableImage> drawable_image) override;

  void UnregisterDrawableImage(int64_t id) override;

  void CleanForRecycle() override;
  void PrepareForRecycle() override;

 private:
  const std::shared_ptr<clay::ServiceManager> service_manager_;
  bool pending_layout_;
  TaskRunners task_runners_;
  const Settings settings_;
  std::shared_ptr<clay::ViewContext> view_context_;
  clay::Puppet<clay::Owner::kUI, UIFrameService> ui_frame_service_;
  clay::Puppet<clay::Owner::kUI, RasterFrameService> raster_frame_service_;
  std::unique_ptr<clay::PageView> page_view_;
  std::unique_ptr<clay::ShadowNodeOwner> shadow_node_owner_;
  Delegate* delegate_;

  fml::WeakPtrFactory<Engine> weak_factory_;
  friend class testing::ShellTest;
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_ENGINE_H_
