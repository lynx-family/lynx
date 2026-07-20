// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_FRAME_CHILD_PAGE_HOST_H_
#define CLAY_LYNX_ADAPTOR_FRAME_CHILD_PAGE_HOST_H_

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/memory/weak_ptr.h"
#include "clay/flow/frame_timings.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/float_size.h"
#include "clay/gfx/paint_image.h"
#include "clay/ui/component/frame_tick_client.h"
#include "clay/ui/render_delegate.h"
#include "platform/embedder/core/lynx_template_renderer.h"

namespace clay {
class BaseView;
class DrawableImage;
class FrameView;
class KeyEvent;
class LayerTree;
class PageView;
struct PointerEvent;
class ResourceLoaderIntercept;
class ShadowNode;
class ShadowNodeOwner;
class ViewContext;
}  // namespace clay

namespace lynx {
namespace pub {
class LynxResourceLoader;
}  // namespace pub
namespace embedder {
class LynxTemplateRenderer;
}  // namespace embedder
namespace tasm {

class LynxTemplateBundle;
class TemplateData;
class UIDelegateClay;

class FrameChildPageHost final : public clay::RenderDelegate,
                                 public clay::FrameTickClient {
 public:
  FrameChildPageHost(
      clay::ViewContext* parent_view_context,
      std::shared_ptr<pub::LynxResourceLoader> resource_loader,
      embedder::LynxTemplateRenderer::Settings parent_renderer_settings);
  ~FrameChildPageHost() override;

  FrameChildPageHost(const FrameChildPageHost&) = delete;
  FrameChildPageHost& operator=(const FrameChildPageHost&) = delete;

  bool LoadBundle(clay::FrameView* frame_view,
                  const std::shared_ptr<LynxTemplateBundle>& bundle,
                  const std::shared_ptr<TemplateData>& data,
                  const std::shared_ptr<TemplateData>& global_props);
  bool UpdateMetaData(const std::shared_ptr<TemplateData>& data,
                      const std::shared_ptr<TemplateData>& global_props);
  void UpdateViewport(clay::FrameView* frame_view);
  bool ForwardPointerEvent(const clay::PointerEvent& event);
  bool ForwardKeyEvent(const clay::KeyEvent* event);
  void Destroy();

  // clay::FrameTickClient
  bool BeginScheduledFrame(const clay::FrameTickInfo& tick,
                           bool forced) override;

  // clay::RenderDelegate
  void ScheduleFrame() override;
  void ForceBeginFrame() override;
  void OnFirstMeaningfulLayout() override;
  void ScheduleLayout() override;
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
      std::unique_ptr<clay::LayerTree> layer_tree,
      std::function<void(fml::RefPtr<clay::PaintImage>)> callback) override;
  fml::RefPtr<clay::PaintImage> MakeRasterSnapshot(
      clay::GrPicturePtr picture, skity::Vec2 picture_size) override;
  void SetClipboardData(const std::u16string& data) override;
  std::u16string GetClipboardData() override;
#if defined(OS_WIN) || defined(OS_MAC) || defined(ENABLE_HEADLESS)
  void SetTextInputClient(int client_id, const char* input_action,
                          const char* input_type) override;
  void ClearTextInputClient() override;
  void SetEditableTransform(const float transform_matrix[16]) override;
  void SetEditingState(uint64_t selection_base, uint64_t composing_extent,
                       const std::string& selection_affinity,
                       const std::string& text, bool selection_directional,
                       uint64_t selection_extent,
                       uint64_t composing_base) override;
  void SetCaretRect(float x, float y, float width, float height) override;
  void UpdateCaretPosition(float x, float y, float width,
                           float height) override;
  void setMarkedTextRect(float x, float y, float width, float height) override;
  void ShowTextInput() override;
  void HideTextInput() override;
  void SetCursorPosition(int position) override;
  void WindowMove() override;
  void ActivateSystemCursor(int type, const std::string& path) override;
#endif
  void FilterInputAsync(
      const std::string& input, const std::string& pattern,
      std::function<void(const std::string&)> callback) override;
  void ReportTiming(const std::unordered_map<std::string, int64_t>& timing,
                    const std::string& flag) override;
  clay::BaseView* FindViewById(int view_id) override;
  clay::ShadowNode* FindShadowNodeById(int node_id) override;
  void UpdateRootSize(int32_t width, int32_t height) override;
  void RegisterDrawableImage(
      std::shared_ptr<clay::DrawableImage> drawable_image) override;
  void UnregisterDrawableImage(int64_t id) override;
  void RegisterDrawableImageFirstFrameAvailable(int64_t image_id) override;
  void UnregisterDrawableImageFirstFrameAvailable(int64_t image_id) override;

 private:
  void EnsureChildPage(clay::FrameView* frame_view);
  embedder::LynxTemplateRenderer::Settings BuildChildRendererSettings(
      const clay::FrameView* frame_view) const;
  bool BeginChildFrame(const clay::FrameTickInfo& tick, bool forced);
  bool TryProduceInitialChildFrameNow();
  void ScheduleParentFrame(bool forced);
  void RegisterDrawableFirstFrameBridge(int64_t image_id);
  void UnregisterDrawableFirstFrameBridge(int64_t image_id);
  void UnregisterDrawableFirstFrameBridges();
  clay::FrameView* FindFrameView() const;
  bool MatchesLoadedFrame(const clay::FrameView* frame_view) const;
  bool HasRunnableViewport(const clay::FrameView* frame_view) const;
#if defined(OS_WIN) || defined(OS_MAC) || defined(ENABLE_HEADLESS)
  clay::FloatPoint FrameContentOriginInParent() const;
  void MapChildRectToParent(float x, float y, float width, float height,
                            float* mapped_x, float* mapped_y,
                            float* mapped_width, float* mapped_height) const;
  void RegisterTextInputBridge(int client_id);
  void RemoveTextInputBridge();
  void ClearTextInputIfNeeded();
#endif

  clay::ViewContext* parent_view_context_ = nullptr;
  clay::PageView* parent_page_view_ = nullptr;
  std::shared_ptr<pub::LynxResourceLoader> resource_loader_;
  embedder::LynxTemplateRenderer::Settings parent_renderer_settings_;
  int frame_view_id_ = 0;
  std::string loaded_url_;
  clay::FloatSize child_viewport_size_;
  float child_viewport_device_pixel_ratio_ = 0.f;
  int child_viewport_width_mode_ = -1;
  int child_viewport_height_mode_ = -1;
  bool child_viewport_initialized_ = false;
  bool pending_layout_ = false;
  bool inside_begin_frame_ = false;
  bool child_first_meaningful_layout_ = false;
  bool has_submitted_child_surface_ = false;
  bool producing_initial_child_frame_ = false;
  bool destroyed_ = false;
  std::shared_ptr<clay::LayerTree> previous_child_layer_tree_;
  std::unordered_set<int64_t> bridged_drawable_image_ids_;
  std::unordered_set<int64_t> dispatching_drawable_first_frame_bridge_ids_;
  std::unordered_set<int64_t> pending_unbridged_drawable_image_ids_;
#if defined(OS_WIN) || defined(OS_MAC) || defined(ENABLE_HEADLESS)
  std::optional<int> bridged_text_input_client_id_;
  bool text_input_active_ = false;
  bool frame_focus_enabled_by_child_ = false;
#endif

  std::shared_ptr<clay::ViewContext> child_view_context_;
  std::unique_ptr<clay::PageView> child_page_view_;
  std::unique_ptr<clay::ShadowNodeOwner> child_shadow_node_owner_;
  std::unique_ptr<UIDelegateClay> ui_delegate_;
  std::unique_ptr<embedder::LynxTemplateRenderer> renderer_;
  fml::WeakPtrFactory<FrameChildPageHost> weak_factory_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CLAY_LYNX_ADAPTOR_FRAME_CHILD_PAGE_HOST_H_
