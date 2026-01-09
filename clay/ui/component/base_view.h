// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_BASE_VIEW_H_
#define CLAY_UI_COMPONENT_BASE_VIEW_H_

#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "base/include/fml/memory/weak_ptr.h"
#include "clay/gfx/animation/animation_data.h"
#include "clay/gfx/animation/animation_handler.h"
#include "clay/gfx/animation/animator_target.h"
#include "clay/gfx/animation/transition_data.h"
#include "clay/gfx/geometry/box_shadow_operations.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/geometry/float_size.h"
#include "clay/gfx/geometry/path.h"
#include "clay/gfx/geometry/sticky_info.h"
#include "clay/gfx/geometry/transform.h"
#include "clay/gfx/geometry/transform_operations.h"
#include "clay/gfx/geometry/transform_origin.h"
#include "clay/gfx/geometry/transform_raw.h"
#include "clay/gfx/style/borders_data.h"
#include "clay/gfx/style/length.h"
#include "clay/gfx/style/outline_data.h"
#include "clay/public/clay.h"
#include "clay/public/style_types.h"
#include "clay/ui/common/background_data.h"
#include "clay/ui/common/type_info.h"
#include "clay/ui/component/css_property.h"
#include "clay/ui/component/focus_event_handler.h"
#include "clay/ui/component/focus_node.h"
#include "clay/ui/component/key_event_handler.h"
#include "clay/ui/component/layout_controller.h"
#include "clay/ui/component/mouse_cursor.h"
#include "clay/ui/gesture/gesture_manager.h"
#include "clay/ui/gesture/gesture_recognizer.h"
#include "clay/ui/gesture/hit_test.h"
#include "clay/ui/lynx_module/lynx_ui_method_registrar.h"
#ifdef ENABLE_ACCESSIBILITY
#include "clay/ui/semantics/semantics_node.h"
#endif

#define SEND_EVENT(...) page_view()->SendEvent(id(), __VA_ARGS__);

#define UI_METHOD_DEF(name)                     \
  void name(const clay::LynxModuleValues& args, \
            const clay::LynxUIMethodCallback& callback);

namespace clay {
class BaseViewAnimationMutator;
class InlineSpecStyles;
class KeyEvent;
class KeyframesManager;
class PageView;
class RenderObject;
class Scrollable;
class FilterOperations;
struct PointerEvent;
struct Shadow;
class TransitionManager;
struct TransOffset;

class BaseView : public TypeIdentifiable<BaseView>,
                 public HitTestable,
                 public HitTestTarget,
                 public KeyEventHandler,
                 public FocusEventHandler,
                 public FocusNode {
 public:
  BaseView(std::unique_ptr<RenderObject> render_object, PageView* page_view);

  BaseView(int id, std::string tag, std::unique_ptr<RenderObject> render_object,
           PageView* page_view);

  ~BaseView() override;

  void SetDestructListener(const std::function<void(BaseView*)>& func) {
    destruct_listener_ = func;
  }

  bool ShouldPassEventToNative() const override {
    return should_pass_event_for_hittest_;
  }

  bool IsAppRegionDraggable();
  // TODO(wangchen) to do better
  bool IsInternalView() const { return id_ == -1; }
  // Override the destroy function to delete the custom children if they are not
  // in the view tree
  void Destroy();
  virtual void AddChild(BaseView* child);
  virtual void AddChild(BaseView* child, int index);
  virtual void RemoveChild(BaseView* child);
  // Remove the view and do not trigger the detach lifecycle. You MUST re-add
  // the view to the tree subsequently. This is useful for moving a view to a
  // new place without triggering the detach lifecycle, which could lead to
  // focus loss or other problems.
  void RemoveChildTemporarily(BaseView* child);
  virtual void DestroyAllChildren();
  void BringChildToFront(BaseView* child);

  virtual void OnEnterForeground() {}
  virtual void OnEnterBackground() {}

  virtual void OnMouseHoverChange();
  void OnMouseEvent(const ClayEventType type, const PointerEvent& event);

  // For some internally created view events, the callback id needs to be
  // returned.
  virtual int GetCallbackId() { return id_; }
  // add for layoutchange event
  void OnLayoutChange();
  virtual void OnLayoutUpdated() {}
  virtual FloatPoint GetScrollOffset() const { return FloatPoint(); }
  // FIXME(ZhuChengCheng) this is a workaround method for rtl scrollview, we
  // will fix this soon by refactoring scrollview's rlt logic.
  virtual FloatPoint GetScrollOffsetForPaint() const {
    return GetScrollOffset();
  }

  virtual void OnNodeReady() {}

  void UpdateInlineImageInfo();

  void OnMouseEnter(const PointerEvent& event);
  void OnMouseLeave(const PointerEvent& event);
  void OnMouseHover(const PointerEvent& event);

  void MarkDirty();

  void SetFilterOperations(const FilterOperations& value);

  // Some components may have child order which is diffferent from insertions
  // like view-pager
  virtual int GetChildIndex(BaseView* child);

  float GetTranslateZ() const;

  void SetPaintingOrder(int value);
  float GetPaintingOrder() const;

  void SetID(int id);
  int id() const { return id_; }
  const std::string& GetName() const { return tag_; }
  std::vector<BaseView*>& GetChildren() { return children_; }
  size_t child_count() const { return children_.size(); }

  // An anonymous view is a view created internally and does not have a
  // corresponding Lynx element.
  bool IsAnonymousView() const { return id_ < 0; }

  const std::string& GetComponentName() const { return name_; }
  void SetComponentName(std::string name) { name_ = std::move(name); }

  const std::string& GetIdSelector() const { return id_selector_; }
  void SetIdSelector(std::string selector) {
    id_selector_ = std::move(selector);
  }

  BaseView* Parent() const { return parent_; }

  // Check if `a_view` is descendant of the current node.
  // Note: if a_view is the current view, return true.
  bool IsDescendant(BaseView* a_view) const;

  void SetRepaintBoundary(bool repaint_boundary);
  virtual void SetDirection(int type){};
  virtual void SetX(float x);
  virtual void SetY(float y);
  virtual void SetWidth(float width);
  virtual void SetHeight(float height);
  virtual void SetBound(float left, float top, float width, float height);
  virtual void SetPaddings(float padding_left, float padding_top,
                           float padding_right, float padding_bottom);
  void SetMargins(float margin_left, float margin_top, float margin_right,
                  float margin_bottom);
  void SetOpacity(float opacity);
  float Opacity();
  void SetOverflowWithMask(uint8_t mask, int overflow);
  virtual void SetOverflow(int overflow);
  int GetOverflow() { return overflow_; }
  virtual void SetBorder(const BordersData& data);
  void OnBorderChanged(const BordersData& data);

  void AppendShadow(const Shadow& shadow);
  void SetShadows(std::vector<Shadow>&& shadows);
  virtual void SetBorderStyle(std::vector<Side> sides,
                              std::vector<BorderStyleType> styles);
  virtual void SetBorderWidth(std::vector<Side> sides,
                              std::vector<float> widths);
  virtual void SetBorderColor(std::vector<Side> sides,
                              std::vector<uint32_t> colors);
  virtual void SetBorderRadius(const FloatSize& left_top,
                               const FloatSize& right_top,
                               const FloatSize& right_bottom,
                               const FloatSize& left_bottom);
  void SetBorderRadius(size_t index, const std::vector<Length>& array);
  bool IsDelayDestroy() { return delay_destroy_; }
  std::optional<bool> IgnoreFocus() { return ignore_focus_; }
  void SetOutline(const OutlineData& outline);
  void SetOutlineStyle(const BorderStyleType style);
  void SetOutlineWidth(int width);
  void SetOutlineOffset(int offset);
  void SetOutlineColor(unsigned int color);
  void SetCursor(const std::vector<std::string>& vec);
  void SetCursor(const clay::Value::Array& array);
  void SetClipOffsetPath(const clay::Value::Array& array, bool is_clip_path);
  void ClearClipPath();
  void ClearOffsetPath();
  void SetOffsetRotate(float rotate);
  void SetOffsetDistance(float distance);
  void SetFilter(const clay::Value::Array& array);
  void ClearFilter();
  void SetImageRendering(ClayImageRendering image_rendering);
  void SetBoxShadowOperations(const BoxShadowOperations& value);
  MouseCursor* GetMouseCursor();

  virtual void SetBackground(const BackgroundData& background);

  enum class BackgroundPropType {
    kColor,
    kImage,
    kClip,
    kOrigin,
    kPosition,
    kRepeat,
    kSize
  };
  struct BackgroundUpdate {
    BackgroundPropType type;
    std::variant<Color, const clay::Value::Array*,
                 const std::vector<BackgroundPosition>*,
                 const std::vector<BackgroundSize>*>
        payload;
  };

  virtual bool OnBackgroundProperty(const BackgroundUpdate& update) {
    return false;
  }

  void SetBackgroundColor(const Color& color);
  void SetBackgroundImage(const clay::Value::Array& array);
  void SetBackgroundClip(const clay::Value::Array& array);
  void SetBackgroundOrigin(const clay::Value::Array& array);
  void SetBackgroundPosition(const std::vector<BackgroundPosition>& positions);
  void SetBackgroundRepeat(const clay::Value::Array& array);
  void SetBackgroundSize(const std::vector<BackgroundSize>& sizes);

  void SetMaskImage(const clay::Value::Array& array);
  void SetMaskPosition(const std::vector<MaskPosition>& positions);
  void SetMaskOrigin(const clay::Value::Array& array);
  void SetMaskRepeat(const clay::Value::Array& array);
  void SetMaskSize(const std::vector<MaskSize>& mask_sizes);
  void SetMaskClip(const clay::Value::Array& array);

  TransitionManager* TransitionMgr();
  void TransitionTo(ClayAnimationPropertyType type, float value);
  void TransitionTo(ClayAnimationPropertyType type, const Color& value);
  void TransitionTo(ClayAnimationPropertyType type,
                    const TransformOperations& ops);

  const clay::Value& GetDataSet() { return data_set_; }

  KeyframesManager* KeyframesMgr();
  void SetAnimation(const std::vector<AnimationData>& data);
  void SetAnimation(const clay::Value::Array& array);
  bool HasAnimation() const { return animation_.has_value(); }
  const std::vector<AnimationData>& Animation() const { return *animation_; }

  void AppendTransition(const TransitionData& data);
  void SetTransition(const std::vector<TransitionData>& data);
  void SetTransition(const clay::Value::Array& array);
  void SetTransform(const TransformOperations& ops, const FloatPoint& origin);
  void SetTransform(const std::vector<TransformRaw>& transform_row);
  void SetTransformOrigin(std::optional<TransformOrigin> origin);
  void SetTransformOrigin(const std::vector<Length>& array);
  void SetPerspective(const clay::Value::Array& array);
  void SetInteractable(bool enable) { is_interactable_ = enable; }
  void SetBlockNativeEvent(bool enable) { should_block_native_event_ = enable; }
  void SetConsumeSlideEventDirection(const clay::Value::Array& array);
  void SetEnableNewAnimator(bool enable) { enable_new_animator_ = enable; }
  bool IsInteractable() const { return is_interactable_; }
  const TransformOperations& GetTransformOps() const { return transform_ops_; }
  FloatPoint GetTransformOrigin() const;
  Transform GetTransform() const;

  bool ConsumeSlideEvent(float angle) override;

  virtual void DidUpdateAttributes() {
    // Consistent with Lynx-Native behavior, flag dirty nodes after attribute
    // updates.
    Invalidate();
  }
  virtual void OnViewPostionUpdate(FloatPoint scroll_offset);
  virtual void NotifyLowMemory() {}

  virtual void SetAttribute(const char* attr, const clay::Value& value);
  virtual void AddEventCallback(const char* event);

  // Indicate whether all children should be restricted in this view.
  virtual bool CanChildrenEscape() const {
    return overflow_ != CSSProperty::OVERFLOW_HIDDEN;
  }
  bool HitTest(const PointerEvent& event, HitTestResult& result) override;
  void HandleEvent(const PointerEvent& event) override;
  bool HasDragGestureRecognizer(ScrollDirection direction) const override;
  bool HasTapGestureRecognizer() const override;
  bool HasLongPressGestureRecognizer() const override;
  bool HasTapEvent() const override;
  bool HasLongPressEvent() const override;
  bool ShouldBlockNativeEvent() const override {
    return should_block_native_event_;
  }
  bool HasConsumeSlideEventAngles() const override {
    return !consume_slide_event_ranges_.empty();
  }

  // By default, focus scope self cannot accept focus while traversal.
  FocusBehavior GetFocusBehavior() const override {
    if (Visible()) {
      bool skip_focus_traversal = true;
      if (skip_focus_traversal_.has_value()) {
        skip_focus_traversal = *skip_focus_traversal_;
      } else {
        skip_focus_traversal = IsFocusScope();
      }
      return skip_focus_traversal ? FocusBehavior::kStepIntoChild
                                  : FocusBehavior::kFocus;
    }
    return FocusBehavior::kSkip;
  }

  void SetVisible(bool visible);
  bool Visible() const;

  float Left() const { return left_; }
  float Top() const { return top_; }
  float Width() const { return width_; }
  float Height() const { return height_; }

  FloatPoint AbsoluteLocationWithScroll() const;

  float LeftWithScroll() const;
  float TopWithScroll() const;
  // TODO(yulitao): consider box-sizing.
  float HorizontalThickness() const {
    return PaddingLeft() + PaddingRight() + BorderLeft() + BorderRight();
  }
  float VerticalThickness() const {
    return PaddingTop() + PaddingBottom() + BorderTop() + BorderBottom();
  }
  virtual float ContentWidth() const { return Width() - HorizontalThickness(); }
  virtual float ContentHeight() const { return Height() - VerticalThickness(); }
  void SetContentWidth(float content_width) {
    SetWidth(content_width + HorizontalThickness());
  }
  void SetContentHeight(float content_height) {
    SetHeight(content_height + VerticalThickness());
  }
  float MarginLeft() const { return margin_left_; }
  float MarginTop() const { return margin_top_; }
  float MarginRight() const { return margin_right_; }
  float MarginBottom() const { return margin_bottom_; }

  float PaddingLeft() const { return padding_left_; }
  float PaddingTop() const { return padding_top_; }
  float PaddingRight() const { return padding_right_; }
  float PaddingBottom() const { return padding_bottom_; }

  float BorderLeft() const;
  float BorderTop() const;
  float BorderRight() const;
  float BorderBottom() const;
  FloatRect GetBounds() const {
    return FloatRect(left_, top_, width_, height_);
  }

  float ContentInsetLeft() const;
  float ContentInsetTop() const;

  virtual bool CanAcceptEvent() const;
  virtual BaseView* GetTopViewToAcceptEvent(const FloatPoint& position,
                                            FloatPoint* relative_position);
  // Content bounds (without border and padding) in the viewport.
  FloatRect ContentBoundsInViewport() const;
  // Bounds (including border and padding) relative to an other view (or the
  // viewport if the view parameter is null)
  FloatRect BoundsRelativeTo(BaseView* view) const;

  FloatPoint GetPointBySelf(const FloatPoint& point_by_page) const;
  Transform LocalToGlobalTransform() const;

  // FocusNode
  FocusManager* GetParentFocusManager() override;
  const std::string& FocusId() const override { return id_selector_; }

  void FocusHasChanged(bool focus, bool is_leaf) override;
  // Return true if need to propagate scrolling, or false to stop propagate.
  virtual bool OnScrollToVisible() { return true; }
  virtual bool OnScrollToMiddle(BaseView* target_view) { return true; }
  // Make a view visible in scrollable container.
  void ScrollToVisible() {
    if (OnScrollToVisible() && Parent()) {
      Parent()->ScrollToVisible();
    }
  }
  void ScrollToMiddle(BaseView* target_view) {
    if (OnScrollToMiddle(target_view) && Parent()) {
      Parent()->ScrollToMiddle(target_view);
    }
  }
  void SetHasDefaultFocusRing(bool has_focus_ring) override;
  FloatRect CalcFocusRect() const override;
  bool DispatchKeyEventOnFocusNode(const KeyEvent* event) override;
  FloatRect GetContentVisibleRect() override;
  FloatSize GetThicknessOffset() override;

  // Layout related. Used by TextView and BaseListView.
  // Assume this flag won't change during the lifetime.
  virtual bool IsLayoutRootCandidate() const { return false; }
  void SetInLayoutTree(bool in_layout_tree);
  // A node is in a layout tree if one of its ancestor is a layout root
  // candidate or the node itself is a layout root candidate.
  bool InLayoutTree() const {
    return IsLayoutRootCandidate() || in_layout_tree_;
  }

  // `view_to_layout` is the node that initiates the layout. It can be the view
  // itself or its descendant.
  // When `view_to_layout` is nullptr, it means it the view itself.
  // NOTE: If this is called when a descendant is removed from the layout tree,
  // `view_to_layout` can be no longer in the layout tree.
  void MarkNeedsLayout(BaseView* view_to_layout = nullptr);
  bool NeedsLayout() const { return needs_layout_; }
  bool NeedsLayoutUpdated() const { return needs_layout_updated_; }

  bool ShouldIgnoreLayoutRequest() const {
    return ignore_layout_request_count_ > 0;
  }

  void PushIgnoreLayoutRequest() {
    FML_DCHECK(ignore_layout_request_count_ >= 0);
    ignore_layout_request_count_++;
  }

  void PopIgnoreLayoutRequest() {
    ignore_layout_request_count_--;
    FML_DCHECK(ignore_layout_request_count_ >= 0);
  }

  void VisitChildren(const std::function<void(BaseView*)>& visitor);

  class LayoutIgnoreHelper {
   public:
    explicit LayoutIgnoreHelper(BaseView* view) : view_(view) {
      FML_DCHECK(view_);
      view_->PushIgnoreLayoutRequest();
    }

    ~LayoutIgnoreHelper() { view_->PopIgnoreLayoutRequest(); }

   private:
    BaseView* view_ = nullptr;
  };

  // context is nullable.
  void Layout(LayoutContext* context = nullptr);
  virtual void OnLayout(LayoutContext* context);

  virtual void OnContentSizeChanged(const FloatRect& old_rect,
                                    const FloatRect& new_rect);
  virtual void OnBoundsChanged(const FloatRect& old_bounds,
                               const FloatRect& new_bounds);
  virtual void OnChildSizeChanged(BaseView* child) {}

  fml::WeakPtr<BaseView> GetWeakPtr() const {
    return weak_factory_.GetWeakPtr();
  }
  PageView* page_view() const { return page_view_; }
  bool attach_to_tree() const { return attach_to_tree_; }

  // physical_width & physical_height can be zero.
  // It happens when the app just starts, the activity is not created but the
  // pixel ratio is known. Then zero physical size is passed.
  virtual void OnViewportMetricsUpdated(int physical_width, int physical_height,
                                        float device_pixel_ratio) {}

#ifndef NDEBUG
  virtual void DumpViewTree(int depth) const;
  virtual std::string ToString() const;
#endif

  // Lynx ui method
#define UI_METHOD_LIST_DECLARATION(V) \
  V(setFocus)                         \
  V(interceptBackKeyOnce)             \
  V(cancelInterceptBackKey)           \
  V(boundingClientRect)               \
  V(scrollIntoView)                   \
  V(takeScreenshot)
  UI_METHOD_LIST_DECLARATION(UI_METHOD_DEF);
#undef UI_METHOD_LIST_DECLARATION

  // animation event and transition event
  void OnAnimationEvent(const AnimationParams& animation_params);
  void OnTransitionEvent(const AnimationParams& animation_params,
                         ClayAnimationPropertyType property_type);

  bool HasAnimationEvent(const ClayEventType& event_type) const;
  virtual bool HasEvent(const std::string& event) const {
    return events_ &&
           std::find(events_->begin(), events_->end(), event) != events_->end();
  }
  const std::string& ItemKey() const { return item_key_; }

  Scrollable* FindAncestorScrollableView(BaseView* child);

  RenderObject* render_object() const { return render_object_.get(); }
  void AddGestureRecognizer(std::unique_ptr<GestureRecognizer> recognizer);
  void RemoveGestureRecognizer(GestureRecognizer* recognizer);
  void ClearGestureRecognizers();

  void EndAllTransitionsRecursively();

  void GetTransformValue(float left, float right, float top, float bottom,
                         TransOffset& res);

  void GetLocationOnScreen(float in_out_location_x, float in_out_location_y,
                           std::vector<float>& res);

  std::vector<float> TransformFromViewToRootView(BaseView* view,
                                                 float& in_out_location_x,
                                                 float& in_out_location_y);

  void UpdateTransitionRasterAnimation(ClayAnimationPropertyType type);

  void UpdateKeyframesRasterAnimation();

  bool IsRasterAnimationEnabled() const;

  void UpdateSticky(std::optional<StickyInfo> sticky);
  void CheckStickyOnParentScrollAndReset(int left, int top);

  // this means whether the entire page through the touch events.
  std::optional<bool> CanEventThrough() const { return event_through_; }
  // this means whether this view node pass through the events to the nodes
  // behind it.
  virtual bool CanEventsPassThroughToViewsBehind() const { return false; }

  AnimationHandler* GetAnimationHandler();

  virtual void OnLayoutFinish(BaseView* view) {}

  float FromLogical(float value) const;
  float ToLogical(float value) const;

  // AnimatorTarget related interfaces
  void GetProperty(ClayAnimationPropertyType type, float& value);
  void GetProperty(ClayAnimationPropertyType type, Color& value);
  void GetProperty(ClayAnimationPropertyType type, TransformOperations& value);
  void GetProperty(ClayAnimationPropertyType type, FilterOperations& value);
  // SetProperty will modify value without triggering transition animation.
  virtual void SetProperty(ClayAnimationPropertyType type, float value,
                           bool skip_update_for_raster_animation);
  void SetProperty(ClayAnimationPropertyType type, const Color& value,
                   bool skip_update_for_raster_animation);
  void SetProperty(ClayAnimationPropertyType type,
                   const TransformOperations& value,
                   bool skip_update_for_raster_animation);
  void SetProperty(ClayAnimationPropertyType type,
                   const FilterOperations& value);
  const KeyframesMap* GetKeyframesMap(const std::string& animation_name);
  FloatSize PercentageResolutionSize() { return {Width(), Height()}; }

  void DecodeImagesRecursively();

#ifdef ENABLE_ACCESSIBILITY
  virtual FloatRect GetSemanticsBounds() const;
  // Whether enables a11y for this view.
  void SetAccessibilityElement(bool value);
  virtual bool IsAccessibilityElement() const;
  void SetAccessibilityLabel(const std::string& value);
  void SetAccessibilityElements(const std::string& value);
  void MarkRebuildSemanticsTree();
  void UpdateSemantics(
      const std::vector<fml::RefPtr<SemanticsNode>>& new_children,
      BaseView* parent_node_view, bool need_check_children,
      bool force_update = false);
  fml::RefPtr<SemanticsNode> GetSemantics() const { return semantics_; }
  Transform AccumulateTransformFromView(BaseView* view) const;
  // Prepre SemanticsNode for each BaseView.
  virtual void PrepareSemantics(
      fml::RefPtr<SemanticsNode> parent_node,
      std::vector<fml::RefPtr<SemanticsNode>>& result,
      const std::vector<std::string>& ancestor_a11y_elements);
  // Some views may need to dispatch tap/long_press action in clay.
  virtual void OnA11yTap() {}
  virtual void OnA11yLongPress() {}
  virtual std::u16string GetAccessibilityLabel() const;
#endif

 protected:
  friend class BaseListView;
  friend class BaseViewWithChildrenTest;
  friend class BaseViewAnimationMutator;

  BaseViewAnimationMutator* GetAnimationMutator();

  void DestroyChildrenRecursively(BaseView* view);
  virtual bool IsIndependentSubViewTree() const { return false; }

  virtual void OnDestroy() {}
  virtual void OnDetachFromTree();
  virtual void OnAttachToTree();
  virtual void Invalidate();
  void LayoutUpdated();
  void ScrollToFocus();
  virtual void ScrollChildViewToVisible(const FloatRect& rect) {}
  int GetCurrentImageLoaderToken() const { return bg_image_loader_token_; }
  int GetCurrentMaskImageLoaderToken() const {
    return mask_image_loader_token_;
  }
  bool should_pass_event_for_hittest_ = false;
  void LoadBackgroundOrMaskImage(const std::string& uri, size_t index,
                                 bool background = true);

  // Handle common attributes and styles for all views. Return true if the
  // attribute is handled.
  bool HandleCommonAttribute(const char* attr, const clay::Value& value);

  void UpdateCacheStrategy();
  void UpdateChildrenBounds();
  void SetTransformOperations(const TransformOperations& operations,
                              bool is_from_animation);

  void UpdateRenderObjectTransformOrigin();

#ifdef ENABLE_ACCESSIBILITY
  // Some views can update its specific semantics data.
  void UpdateSemanticsData(BaseView* parent_node_view);
  virtual int32_t GetA11yScrollChildren() const;
  virtual int32_t GetSemanticsActions() const;
  virtual int32_t GetSemanticsFlags() const;

  SemanticsOwner* GetSemanticsOwner() const;

  // Whether enable a11y for this view. Only ImageView and TextView are set to
  // true by default.
  std::optional<bool> accessibility_element_ = std::nullopt;
  std::optional<std::u16string> accessibility_label_ = std::nullopt;
  std::optional<std::vector<std::string>> accessibility_elements_ =
      std::nullopt;
  fml::RefPtr<SemanticsNode> semantics_ = nullptr;
#endif

  int id_ = -1;
  std::string id_selector_;
  std::string name_;
  std::string app_region_;
  bool can_draggable_ = false;
  PointerEvent event_draggable_;
  // left_ and top_ are relative to the upper left
  // corner of the parent borderbox
  float left_ = 0.f;
  float top_ = 0.f;
  float width_ = 0.f;
  float height_ = 0.f;
  float padding_left_ = 0.f;
  float padding_top_ = 0.f;
  float padding_right_ = 0.f;
  float padding_bottom_ = 0.f;
  float margin_left_ = 0.f;
  float margin_top_ = 0.f;
  float margin_right_ = 0.f;
  float margin_bottom_ = 0.f;
  clay::Value data_set_ = clay::Value(clay::Value::Map());
  std::string tag_;
  // FIXME(baiqiang): remove focus&text list then move to component
  std::string item_key_;
  bool attach_to_tree_ = false;
  std::optional<bool> ignore_focus_;
  BaseView* parent_ = nullptr;
  PageView* page_view_ = nullptr;
  std::vector<BaseView*> children_;
  std::unique_ptr<RenderObject> render_object_;
  int bg_image_loader_token_ = 0;
  int mask_image_loader_token_ = 0;
  std::unique_ptr<KeyframesManager> keyframes_mgr_;
  std::unique_ptr<TransitionManager> transition_mgr_;
  std::optional<std::vector<AnimationData>> animation_;
  bool in_layout_tree_ = false;
  bool needs_layout_ = false;
  bool needs_layout_updated_ = false;
  int ignore_layout_request_count_ = 0;
  fml::WeakPtrFactory<BaseView> weak_factory_;
  TransformOperations transform_ops_;
  FilterOperations color_matrix_ops_;
  BoxShadowOperations box_shadow_ops_;
  std::optional<TransformOrigin> transform_origin_;
  std::optional<std::vector<TransformRaw>> transform_raw_;
  std::optional<float> perspective_value_;
  std::optional<std::vector<std::string>> events_;
  std::unique_ptr<MouseCursor> cursor_ = nullptr;
  bool is_mouse_hover_ = false;
  uint8_t overflow_;
  std::function<void(BaseView*)> destruct_listener_ = nullptr;
  std::vector<std::unique_ptr<GestureRecognizer>> gesture_recognizers_;
  bool enable_layout_change_event_ = false;
  bool is_interactable_ = true;
  bool should_block_native_event_ = false;
  bool has_intersection_observer_ = false;
  std::optional<bool> event_through_;
  // all slop values means extend x px
  float hit_slop_top_ = 0.f;
  float hit_slop_left_ = 0.f;
  float hit_slop_right_ = 0.f;
  float hit_slop_bottom_ = 0.f;
  std::vector<std::pair<float, float>> consume_slide_event_ranges_;
  DirectionType layout_direction_ = DirectionType::kLtr;
  std::optional<StickyInfo> sticky_ = std::nullopt;
  std::optional<FloatPoint> post_translation_;
  bool enable_new_animator_ = false;
  bool remove_temporarily_ = false;
  std::optional<ClipPathData> clip_path_data_;
  std::optional<OffsetPathData> offset_path_data_;
  bool delay_destroy_ = false;
  std::unique_ptr<BaseViewAnimationMutator> animation_mutator_;

 private:
  template <typename... Args>
  void NotifyBgImageLoadStatus(bool success,
                               const std::vector<std::string>& keys,
                               Args&&... args);
  bool UpdateExposeAttrs(const char* attr, const clay::Value& value);
  void DirtyChildrenPaintingOrder() { sorted_children_.clear(); }
  void RebuildSortedChildrenIfNeeded();
  void NotifyBoundChangeIfNeeded(const FloatRect& old_bounds);
  void DrawClipPath(bool is_clip_path);

  std::vector<BaseView*> sorted_children_;
  bool ignore_size_change_checks_ = false;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_BASE_VIEW_H_
