// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_VIEW_CONTEXT_H_
#define CLAY_UI_COMPONENT_VIEW_CONTEXT_H_

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "clay/common/graphics/screenshot.h"
#include "clay/common/task_runners.h"
#include "clay/gfx/animation/animation_data.h"
#include "clay/gfx/animation/transition_data.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/pixel_helper.h"
#include "clay/gfx/style/borders_data.h"
#include "clay/public/clay.h"
#include "clay/public/event_delegate.h"
#include "clay/public/ui_component_delegate.h"
#include "clay/ui/component/text/text_style.h"
#include "clay/ui/lynx_module/lynx_ui_method_registrar.h"
#include "clay/ui/shadow/shadow_node.h"
#include "clay/ui/shadow/shadow_node_owner.h"
#include "clay/ui/window/viewport_metrics.h"

namespace clay {

enum BoxModelOffset {
  PAD_LEFT,
  PAD_TOP,
  PAD_RIGHT,
  PAD_BOTTOM,
  BORDER_LEFT,
  BORDER_TOP,
  BORDER_RIGHT,
  BORDER_BOTTOM,
  MARGIN_LEFT,
  MARGIN_TOP,
  MARGIN_RIGHT,
  MARGIN_BOTTOM,
  LAYOUT_LEFT,
  LAYOUT_TOP,
  LAYOUT_RIGHT,
  LAYOUT_BOTTOM
};
struct TransOffset {
  std::vector<float> left_top;
  std::vector<float> right_top;
  std::vector<float> right_bottom;
  std::vector<float> left_bottom;
};

using NetLoadCallback = std::function<void(
    const char* url, const char* method, const char* body,
    const char* headers[], size_t headers_size, size_t request_seq,
    ClayNetLoadResultCallback result_callback)>;

class TransformOperations;
class PageView;
struct BackgroundData;
class ServiceManager;
class CustomFilterDecoder;
class PipelineTimingDelegate;

class FrameObserver {
 public:
  virtual void OnBeginFrame() = 0;
};

class ViewContext : public std::enable_shared_from_this<ViewContext> {
 public:
  // The '#' in `id_selector` should not by passed.
  static BaseView* FindViewByIdSelector(std::string_view id_selector,
                                        BaseView* root);
  ViewContext(PageView* root, ShadowNodeOwner* shadow_node_owner);
  virtual ~ViewContext();

  void CleanLeakedViews();

  void OnOutputSurfaceDestroyed();

  bool CreateView(int id, const std::string& tag_name);

  int IndexOf(int id) const;

  void AddView(int id, int parent_id, int index);

  void RemoveView(int id, int parent_id, bool is_temporarily_removed = false);

  bool DestroyView(int id);

  void ResetPageView();

  ShadowNode* CreateShadowNode(int id, const std::string& tag_name,
                               bool is_parent_inline_container);

  int32_t GetTagInfo(const std::string& tag_name);

  void AddShadowNode(int id, int parent_id, int index);

  void RemoveShadowNode(int id);

  void InsertListItemPaintingNode(int list_id, int child_id);

  void RemoveListItemPaintingNode(int list_id, int child_id);

  bool DestroyShadowNode(int id);

  void DestroyAllShadowNode();

  void NotifyLowMemory();

  void OnLayout(int id, float width, TextMeasureMode width_mode, float height,
                TextMeasureMode height_mode,
                const std::array<float, 4>& paddings,
                const std::array<float, 4>& borders);

  void UpdateRootSize(int32_t width, int32_t height);

  void SetShadowNodeAttribute(int id, const char* attr,
                              const clay::Value& value);

  void ScheduleLayout();

  void Alignment(int id);

  // Anonymous view is views which created by internal, and its id is
  // invalid (-1). For anonymous view self, just destroy and delete it.
  // But there maybe some views created by outside (by RKCreateView),
  // for those views who has valid id, call `DestroyView` so that
  // id in view_map_ can be erased.
  void DestroyAnonymousView(BaseView* view);

  void Invalidate();

  void SetCustomFilterDecoder(std::unique_ptr<CustomFilterDecoder> decoder) {
    custom_filter_decoder_ = std::move(decoder);
  }

  CustomFilterDecoder* GetCustomFilterDecoder() {
    if (custom_filter_decoder_) {
      return custom_filter_decoder_.get();
    }
    return nullptr;
  }

  void SetBounds(int id, float left, float top, float width, float height);

  void SetPaddings(int id, float padding_left, float padding_top,
                   float padding_right, float padding_bottom);

  void SetMargins(int id, float margin_left, float margin_top,
                  float margin_right, float margin_bottom);

  void SetOpacity(int id, float opacity);

  void SetOverflow(int id, int overflow);

  void SetBorderStyle(int id, BorderStyleType left, BorderStyleType top,
                      BorderStyleType right, BorderStyleType bottom);

  void SetBorderWidth(int id, int left_width, int top_width, int right_width,
                      int bottom_width);

  void SetBorderColor(int id, unsigned int left_color, unsigned int top_color,
                      unsigned int right_color, unsigned int bottom_color);

  void SetBorderRadius(int id, const FloatSize& left_top,
                       const FloatSize& right_top,
                       const FloatSize& right_bottom,
                       const FloatSize& left_bottom);

  void SetOutlineStyle(int id, BorderStyleType style);

  void SetOutlineWidth(int id, int width);

  void SetOutlineColor(int id, unsigned int color);

  void SetCursor(int id, const char* src[], int size);

  void SetBackgroundColor(int id, unsigned int color);

  void SetBackground(int id, const BackgroundData& background);

  void AppendShadow(int id, const Shadow& shadow);

  void SetAttribute(int id, const char* attr, const clay::Value& value);
  const clay::ViewportMetrics& GetViewportMetrics() const;

  void DidUpdateAttributes(int id);

  void AddEventProp(int id, const char* event);

  void AddShadowNodeEventProp(int id, const char* event);

  void SetTransform(int id, const TransformOperations& ops,
                    const FloatPoint& origin);

  void SetTransition(int id,
                     const std::vector<TransitionData>& transition_data);

  void SetAnimation(int id, const std::vector<AnimationData>& animation_data);

  void SetKeyframes(const clay::Value& keyframes_value);

  void SetRepaintBoundary(int id, bool repaint_boundary);

  void TextMeasure(int id, float width, TextMeasureMode width_mode,
                   float height, TextMeasureMode height_mode, float& out_width,
                   float& out_height);

  void SetFontFace(const char* font_family, const char* src[], int size);
  void SetNetLoadCallback(NetLoadCallback net_load_callback);

  void GetTransformValue(int id, const float* pad_border_margin_layout,
                         int size, float* res);

  void OnFirstMeaningfulLayout();

  void UpdateNodeReadyPatching(std::vector<int32_t> ready_ids,
                               std::vector<int32_t> remove_ids);

  fml::RefPtr<fml::TaskRunner> GetUITaskRunner() const;
  const clay::TaskRunners& GetTaskRunners() const;
  const std::shared_ptr<ServiceManager>& GetServiceManager() const;

  PageView* GetPageView() const { return page_view_; }
  BaseView* FindViewByViewId(int view_id);
  ShadowNode* FindShadowNodeByNodeId(int node_id);
  BaseView* FindViewByComponentId(const std::string& component_id);
  void ShowToast(const char* message, const char* type, float duration);
  void GetAbsolutePosition(int id, float& top, float& left);
  int GetViewIdForLocation(int x, int y);

  void InvokeUIMethod(int view_id, const std::string& method,
                      const LynxModuleValues& params,
                      const LynxUIMethodCallback& callback);

  uint32_t unique_id() { return unique_id_; }

  BaseView* GetViewById(int id);

  void UpdateSticky(int id, const float* sticky);

  float GetBaseline(int id) const;

  void SetDefaultOverflowVisible(bool value);
  void SetExposureProps(int freq, bool exposure_ui_margin_enabled);
  void SetEventDelegate(EventDelegate* delegate);
  void SetUIComponentDelegate(UIComponentDelegate* delegate);
  void SetLayoutDelegate(LayoutDelegate* delegate);

  void SetPipelineTimingDelegate(
      std::shared_ptr<PipelineTimingDelegate> delegate);

  void SetExternalScreenshotCallback(ExternalScreenshotCallback callback);

  // Lynx list containers.
  void UpdateContentOffsetForListContainer(int id, float content_size,
                                           float target_content_offset_x,
                                           float target_content_offset_y);
  void UpdateScrollInfo(int id, bool smooth, float estimated_offset,
                        bool scrolling);
  void FinishLayoutOperation(int child_view_id, int parent_view_id);
  void SetDefaultOverflowAlwaysVisible(bool visible) {
    default_overflow_always_visible = visible;
  }

  bool DefaultOverflowAlwaysVisible() {
    return default_overflow_always_visible;
  }

  TextUpdateBundle* GetTextBundle(int32_t id);
  void UpdateExtraData(int id, TextUpdateBundle* bundle);

  bool UsesLogicalPixels() const {
    return clay::kPixelTypeFramework == clay::kPixelTypeLogical;
  }

  // Sync native view tags. should be called before view tree created.
  // tag in this set will be created as native view.
  // these tags has a higher priority than builtin tags.
  void SyncNativeViewTags(std::unordered_set<std::string> tags);

  std::weak_ptr<ViewContext> GetWeakPtr() { return shared_from_this(); }

  void SetFrameObserver(FrameObserver* observer) { frame_observer_ = observer; }

  FrameObserver* GetFrameObserver() const { return frame_observer_; }

 protected:
  // FIXME(Xietong):
  // In Lynx, the initial properties are passed during view creation. That's
  // missing for now.
  void ConsumeInitialAttributes(BaseView* view);

  bool default_overflow_always_visible = true;

  PageView* page_view_;

  std::unordered_map<int, BaseView*> view_map_;
  std::set<std::string> events_;
  uint32_t unique_id_ = 0;
  ShadowNodeOwner* shadow_node_owner_;

  // component_id_to_ui_id_map_ is used to map radon component id to element id.
  // In method invokeUIMethod, we need to use this map and radon(js) component
  // id to find related views.
  std::unordered_map<std::string, int> component_id_to_ui_id_map_;

  fml::WeakPtrFactory<ViewContext> weak_factory_;
  std::unique_ptr<CustomFilterDecoder> custom_filter_decoder_;
  FrameObserver* frame_observer_ = nullptr;
};

class CustomFilterDecoder {
 public:
  virtual ~CustomFilterDecoder() = default;
  virtual bool Decode(const clay::Value::Array& array, BaseView* base_view) = 0;
};

}  // namespace clay
#endif  // CLAY_UI_COMPONENT_VIEW_CONTEXT_H_
