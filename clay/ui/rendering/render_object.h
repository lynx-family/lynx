// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RENDERING_RENDER_OBJECT_H_
#define CLAY_UI_RENDERING_RENDER_OBJECT_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "clay/common/element_id.h"
#include "clay/gfx/animation/keyframes_manager.h"
#include "clay/gfx/animation/transition_manager.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/transform.h"
#include "clay/gfx/style/borders_data.h"
#include "clay/gfx/style/box_data.h"
#include "clay/gfx/style/outline_data.h"
#include "clay/gfx/style/shadow.h"
#include "clay/public/clay.h"
#include "clay/public/style_types.h"
#include "clay/ui/common/background_data.h"
#include "clay/ui/component/css_property.h"
#include "clay/ui/compositing/pending_container_layer.h"
#include "clay/ui/compositing/pending_effect_layer.h"
#include "clay/ui/compositing/pending_layer.h"
#include "clay/ui/painter/painting_context.h"
#include "clay/ui/rendering/abstract_node.h"
#include "clay/ui/rendering/render_object_child_list.h"
#include "clay/ui/rendering/renderer.h"

namespace clay {

using CacheStrategy = clay::CacheStrategy;
using PaintFunction = std::function<void(PaintingContext&, const FloatPoint&)>;
class BackgroundOrMaskImageClient;

// An object in the render tree.
class RenderObject : public AbstractNode {
  friend class MockRenderObject;
  friend class RenderObjectChildList;

 public:
  RenderObject();
  RenderObject(const RenderObject&) = delete;
  RenderObject& operator=(const RenderObject&) = delete;
  virtual ~RenderObject();

  virtual const char* GetName() const { return "RenderObject"; }

  std::string DebugName() const;

  int ID() const { return id_.view_id(); }
  const ElementId& element_id() const { return id_; }
  void SetID(int id) { id_.UpdateViewId(id); }
  bool HasValidID() const { return id_.view_id() != -1; }

  bool IsDescendantOf(const RenderObject*) const;

  RenderObject* PreviousSibling() const { return previous_; }
  RenderObject* NextSibling() const { return next_; }

  RenderObject* SlowFirstChild() const {
    return VirtualChildren().FirstChild();
  }

  RenderObject* SlowLastChild() const { return VirtualChildren().LastChild(); }

  static void ChangeDefaultOverflowValue(bool default_overflow_visible) {
    default_overflow_ = default_overflow_visible ? CSSProperty::OVERFLOW_XY
                                                 : CSSProperty::OVERFLOW_HIDDEN;
  }

  static uint8_t DefaultOverflowValue() { return default_overflow_; }

  virtual bool HasClip() const { return false; }
  bool HasOverflowClip() const { return overflow_ != CSSProperty::OVERFLOW_XY; }
  virtual PaintFunction FixupPainterIfNeeded(const PaintFunction& painter) {
    return painter;
  }
  bool HasClipOrOverflowClip() const { return HasClip() || HasOverflowClip(); }
  uint8_t Overflow() const { return overflow_; }

  // RenderObject tree traverse.
  RenderObject* NextInPreOrder() const;
  RenderObject* NextInPreOrder(const RenderObject* stay_within) const;
  RenderObject* NextInPreOrderAfterChildren() const;
  RenderObject* NextInPreOrderAfterChildren(
      const RenderObject* stay_within) const;
  RenderObject* PreviousInPreOrder() const;
  RenderObject* PreviousInPreOrder(const RenderObject* stay_within) const;
  RenderObject* ChildAt(unsigned) const;

  RenderObject* LastLeafChild() const;

  RenderObjectChildList& VirtualChildren() { return children_; }
  const RenderObjectChildList& VirtualChildren() const { return children_; }

  // RenderObject tree manipulation.
  bool CanHaveChildren() const { return VirtualChildren().FirstChild(); }
  void AddChild(RenderObject* new_child, RenderObject* before_child = nullptr);
  void RemoveChild(RenderObject*);
  void RemoveAllChildren();
  void RemoveAndCollectChildren(std::vector<RenderObject*>&);

  void Remove() {
    if (Parent()) {
      static_cast<RenderObject*>(Parent())->RemoveChild(this);
    }
  }

  void BringChildToFront(RenderObject* child);

  // The box model information.
  void SetLeft(float left);
  void SetTop(float top);
  void SetWidth(float width);
  void SetHeight(float height);
  float Left() const { return box_data_.Left(); }
  float Top() const { return box_data_.Top(); }
  float Width() const { return box_data_.Width(); }
  float Height() const { return box_data_.Height(); }

  virtual float ScrollLeft() const { return 0.0f; }
  virtual float ScrollTop() const { return 0.0f; }
  void ClearFilter();
  void SetOpacity(float opacity, bool is_from_animation = false);
  void SetBlurRadius(float radius);
  void AppendGrayScale(float scale);
  void SetBackdropBlurRadius(float radius);
  void SetOverflow(uint8_t overflow);
  void SetClipPath(const FloatRoundedRect& rrect);
  void SetClipPath(const GrPath& path);
  void ClearClipPath();
  virtual bool IsRenderBounceView() const { return false; }
  virtual bool IsScrollable() const { return false; }

  float Opacity() const { return *opacity_; }
  float BlurRadius() const { return blur_radius_; }
  using ClipPathType = std::variant<std::monostate, FloatRoundedRect, GrPath>;
  ClipPathType& ClipPath();

  bool HasOpacity() const { return opacity_.has_value() && Opacity() != 1.0f; }
  bool HasBlur() const { return blur_radius_ > 0.f; }

  void AppendBrightness(float brightness);
  void AppendContrast(float contrast);
  void AppendSaturate(float saturate);
  void AppendHueRotate(float deg);
  void AppendCustomColorMatrix(const std::array<float, 20>& matrix);
  bool HasColorFilter() const { return !current_color_filter_matrix_.empty(); }
  const std::vector<float>& ColorFilterMatrix() const {
    return current_color_filter_matrix_;
  }

  bool HasClipPath() const {
    return !std::holds_alternative<std::monostate>(clip_shape_);
  }

  virtual bool IsExternalView() const { return false; }
  void SetOverlay(bool is_overlay) { overlay_ = is_overlay; }
  bool IsOverlay() const { return overlay_; }
  void SetOverlayLevel(int level) { overlay_level_ = level; }
  int OverlayLevel() const { return overlay_level_; }

  float HorizontalThickness() const {
    return PaddingLeft() + PaddingRight() + BorderLeft() + BorderRight();
  }
  float VerticalThickness() const {
    return PaddingTop() + PaddingBottom() + BorderTop() + BorderBottom();
  }

  FloatPoint location() const {
    return FloatPoint(box_data_.Left(), box_data_.Top());
  }

  FloatRect GetFrameRect() const {
    return FloatRect(box_data_.Left(), box_data_.Top(), box_data_.Width(),
                     box_data_.Height());
  }

  virtual bool CanDisplay() {
    if (!Visible()) {
      return false;
    }
    return (box_data_.Width() > 0.f && box_data_.Height() > 0.f) ||
           !HasOverflowClip();
  }

  // When the render object has a layer, use this bounds to construct the
  // |PaintingContext|.  Usually FrameRect is equal to BoxRect, but when the
  // render object has Shadow or Outline, FrameRect is the union of BoxRect and
  // ShadowRect and OutlineRect.
  FloatRect GetFrameRectEx() const;

  FloatPoint OffsetInLayer() const;

  FloatPoint PaintOffset() const {
    return FloatPoint(BorderLeft() + PaddingLeft(), BorderTop() + PaddingTop());
  }

  FloatPoint ClipOffset() const {
    return FloatPoint(BorderLeft(), BorderTop());
  }

  float PaddingLeft() const { return box_data_.PaddingLeft(); }
  float PaddingRight() const { return box_data_.PaddingRight(); }
  float PaddingTop() const { return box_data_.PaddingTop(); }
  float PaddingBottom() const { return box_data_.PaddingBottom(); }
  void SetPaddingLeft(float left);
  void SetPaddingRight(float right);
  void SetPaddingTop(float top);
  void SetPaddingBottom(float bottom);

  float MarginLeft() const { return box_data_.MarginLeft(); }
  float MarginRight() const { return box_data_.MarginRight(); }
  float MarginTop() const { return box_data_.MarginTop(); }
  float MarginBottom() const { return box_data_.MarginBottom(); }
  void SetMarginLeft(float left) { box_data_.SetMarginLeft(left); }
  void SetMarginRight(float right) { box_data_.SetMarginRight(right); }
  void SetMarginTop(float top) { box_data_.SetMarginTop(top); }
  void SetMarginBottom(float bottom) { box_data_.SetMarginBottom(bottom); }

  void SetBorders(const BordersData& borders_data);
  bool HasBorder() const { return border_.has_value(); }
  const BordersData& Border() const { return *border_; }
  BordersData& MutableBorder() {
    if (!border_.has_value()) {
      border_ = std::make_optional<BordersData>(BordersData());
    }
    return *border_;
  }

  float BorderLeft() const { return HasBorder() ? border_->width_left_ : 0.f; }
  float BorderTop() const { return HasBorder() ? border_->width_top_ : 0.f; }
  float BorderRight() const {
    return HasBorder() ? border_->width_right_ : 0.f;
  }
  float BorderBottom() const {
    return HasBorder() ? border_->width_bottom_ : 0.f;
  }

  void SetOutline(const OutlineData& outline_data);
  bool HasOutline() const { return outline_.has_value(); }
  const OutlineData& Outline() const { return *outline_; }
  OutlineData& MutableOutline() {
    if (!outline_.has_value()) {
      outline_ = std::make_optional<OutlineData>(OutlineData());
    }
    return *outline_;
  }

  bool IsPainting() const { return painting_; }
  void SetVisible(bool visible);
  bool Visible() const { return visible_; }
  bool IsActualVisible();

  void SetShadow(const Shadow& shadow);
  void SetShadows(std::vector<Shadow>&& shadows);
  bool HasShadow() const { return shadows_.has_value() && !shadows_->empty(); }
  const std::vector<Shadow>& Shadows() const { return *shadows_; }

  void SetBackgroundColor(const Color& color,
                          bool skip_update_for_raster_animation = false);
  void SetBackgroundData(const BackgroundData& background_data);
  bool HasBackground() const { return background_data_.has_value(); }
  const BackgroundData& Background() const { return *background_data_; }
  void ResizeBackground(size_t size);
  void ResizeMask(size_t size);

  bool HasMask() const { return mask_data_.has_value(); }
  const MaskData& Mask() const { return *mask_data_; }

  virtual void DecodeImages();

#ifndef ENABLE_SKITY
  void SetBackgroundImage(size_t index,
                          std::unique_ptr<ImageResource> accessor);
  void SetMaskImage(size_t index, std::unique_ptr<ImageResource> accessor);
#else
  void SetBackgroundImage(size_t index, std::shared_ptr<BaseImage> image);
  void SetMaskImage(size_t index, std::shared_ptr<BaseImage> image);
#endif  // ENABLE_SKITY
  void SetBackgroundImage(size_t index, const Gradient& gradient);
  void SetBackgroundClip(const std::vector<ClayBackgroundClipType>& clips);
  void SetBackgroundOrigin(
      const std::vector<ClayBackgroundOriginType>& origins);
  void SetBackgroundPosition(const std::vector<BackgroundPosition>& positions);
  void SetBackgroundRepeat(
      const std::vector<ClayBackgroundRepeatType>& repeats);
  void SetBackgroundSize(const std::vector<BackgroundSize>& sizes);

  void SetMaskImage(size_t index, const Gradient& gradient);
  void SetMaskPosition(const std::vector<MaskPosition>& positions);
  void SetMaskRepeat(const std::vector<ClayMaskRepeatType>& repeats);
  void SetMaskOrigin(const std::vector<ClayMaskOriginType>& origins);
  void SetMaskSize(const std::vector<MaskSize>& mask_sizes);
  void SetMaskClip(const std::vector<ClayMaskClipType>& clips);

  void SetHasDefaultFocusRing(bool has_default_focus_ring);
  bool HasDefaultFocusRing() const { return has_default_focus_ring_; }

  void SetImageFilterMode(FilterMode mode);
  FilterMode ImageFilterMode() const { return image_filter_mode_; }

  void SetTransformOperations(const TransformOperations& transform,
                              bool is_from_animation = false);
  void SetTransformOrigin(const FloatPoint& origin);
  void SetPerspective(float value);
  bool HasTransform() const {
    return transform_.has_value() && !transform_->IsIdentity();
  }
  bool HasTransformOperations() const;
  const TransformOperations& GetTransformOperations() const {
    return *transform_;
  }
  Transform GetTransform() const {
    return transform_.has_value() ? (*transform_).Apply() : Transform();
  }
  const FloatPoint& GetTransformOrigin() const { return transform_origin_; }
  bool HasPerspective() const { return perspective_.has_value(); }
  float GetPerspective() const { return perspective_.value_or(0.f); }

  void RedepthChildren() override;

  void SetLayer(std::unique_ptr<PendingContainerLayer> layer) {
    compositor_layer_ = std::move(layer);
    compositor_layer_->Attach(this);
  }
  void ResetLayer() {
    RemoveContainerLayerFromParent();
    compositor_layer_.reset();
    if (NeedsPaint()) {
      container_layer_.reset();
    }
  }
  PendingContainerLayer* GetLayer() const { return compositor_layer_.get(); }

  void SetContainerLayer(std::unique_ptr<PendingContainerLayer> layer) {
    container_layer_ = std::move(layer);
    container_layer_->Attach(this);
  }

  PendingContainerLayer* GetContainerLayer() const {
    return container_layer_.get();
  }

  PendingEffectLayer* GetOrCreateEffectLayer(EffectType type) {
    size_t type_index = static_cast<size_t>(type);
    if (!effect_layers_[type_index]) {
      std::unique_ptr<PendingEffectLayer> layer =
          std::make_unique<PendingEffectLayer>();
      layer->Attach(this);
      effect_layers_[type_index] = std::move(layer);
    }
    return effect_layers_[type_index].get();
  }

  void UnloadAllEffectLayers() {
    for (auto& layer : effect_layers_) {
      if (layer) {
        layer->Remove();
        layer->RemoveAllChildren();
      }
    }
  }

  // Only used for when a view is added, the whole subtree should mark dirty.
  void MarkSubtreeDirty();
  void MarkNeedsPaint(bool force_repaint = false);
  bool NeedsPaint() const { return needs_paint_; }

  void MarkNeedsEffect();
  bool NeedsEffect() const { return needs_effect_; }

  Renderer* GetRenderer() { return renderer_; }
  void SetRenderer(Renderer* renderer);
  void SetRepaintBoundary(bool repaint_boundary);
  virtual bool IsRepaintBoundary() const {
    return repaint_boundary_ || HasShadow() || HasTransform() ||
           HasPerspective() || HasOpacity() || HasClipOrOverflowClip() ||
           HasBlur() || HasColorFilter() || HasClipPath() ||
           HasBackgroundClipText() || HasMask();
  }

  bool CanApplyAA() const;

  void SetCacheStrategyRecursively(CacheStrategy strategy,
                                   bool set_layer = true);
  CacheStrategy GetCacheStrategy() const { return strategy_; }

  // Don't override this function instead using Paint.
  void PaintWithContext(PaintingContext& context, const FloatPoint& offset);

  // Just paint self.
  //            Layer [Canvas]
  //    -----------------------------
  //    |\                          |
  //    | \ Offset                  |
  //    |  \                        |
  //    |   \___________            |
  //    |   |\_________/|PaintOffset|
  //    |   ||         ||           |
  //    |   || content ||           |
  //    |   ||         ||           |
  //    |   ||_________||           |
  //    |   |/_________\|           |
  //    |                           |
  //    |                           |
  //    |                           |
  //    |___________________________|
  //
  //
  // Offset = the distance to Canvas, that is, the offset of the layer.
  // PaintOffset = Border + Padding.
  // Paint order:
  // 1. Translate canvas by Offset;
  // 2. Paint background, shadow, border ...
  // 3. Translate canvas by PaintOffset;
  // 4. Paint content;
  // 5. Paint Children.
  virtual void Paint(PaintingContext& context, const FloatPoint& offset) = 0;
  // Paint children and their children.
  virtual void PaintChildren(PaintingContext& context,
                             const FloatPoint& offset) {}
  virtual void WillPaint();
  void Destroy();
  // Validate if this node is qualified for paint.
  // Once a node is not visible, all its descendants are not qualified,
  // meanwhile this node should be removed from renderer_.
  void ValidateForPaint(bool parent_visibility);

#ifndef NDEBUG
  void DumpRenderTree() const;
  virtual std::string ToString() const;
  bool EnableRepaintBoundaryBorders() const { return false; }
#endif

  bool HasTransition(ClayAnimationPropertyType type) const;
  void MarkHasTransition(ClayAnimationPropertyType type, bool value);

  bool HasAnimation(ClayAnimationPropertyType type) const;
  void MarkHasAnimation(ClayAnimationPropertyType type, bool value);

  bool HasOpacityRasterAnimation() const {
    return HasTransition(ClayAnimationPropertyType::kOpacity) ||
           HasAnimation(ClayAnimationPropertyType::kOpacity);
  }
  bool HasTransformRasterAnimation() const {
    return HasTransition(ClayAnimationPropertyType::kTransform) ||
           HasAnimation(ClayAnimationPropertyType::kTransform);
  }
  bool HasBackgroundColorRasterAnimation() const {
    return HasTransition(ClayAnimationPropertyType::kBackgroundColor) ||
           HasAnimation(ClayAnimationPropertyType::kBackgroundColor);
  }
  bool HasColorRasterAnimation() const {
    return HasTransition(ClayAnimationPropertyType::kColor) ||
           HasAnimation(ClayAnimationPropertyType::kColor);
  }

  bool HasRasterAnimation() const {
    return HasOpacityRasterAnimation() || HasTransformRasterAnimation() ||
           HasBackgroundColorRasterAnimation() || HasColorRasterAnimation();
  }

  bool HasBackgroundClipText() const;

  TransitionManager* GetTransitionManager() const {
    return transition_manager_;
  }

  void SetTransitionManager(TransitionManager* manager) {
    transition_manager_ = manager;
  }

  KeyframesManager* GetKeyframesManager() const { return keyframes_manager_; }

  void SetKeyframesManager(KeyframesManager* manager) {
    keyframes_manager_ = manager;
  }

  float GetTranslateZ() const { return translate_z_; }

  void DirtyChildrenPaintingOrder() {
    if (sorted_children_.empty()) {
      return;
    }
    sorted_children_.clear();
    MarkNeedsPaint();
  }

  void SetPaintingOrder(int painting_order);
  int GetPaintingOrder() const { return painting_order_; }

  void CombineColorFilter(const std::vector<float>& matrix);

  void SetImageDecodeWithPriority(bool value) {
    image_decode_with_priority_ = value;
  }
  bool ImageDecodeWithPriority() const { return image_decode_with_priority_; }

 protected:
  // Helper functions.
  void SetPreviousSibling(RenderObject* previous) { previous_ = previous; }
  void SetNextSibling(RenderObject* next) { next_ = next; }

  void VisitChildren(const std::function<void(RenderObject*)> visitor);

  bool HasTransformExpansion() const;

  Renderer* renderer_ = nullptr;

 private:
  FloatPoint PaintOffsetEx() const;

  bool WasRepaintBoundary();

  // Destroy the old container layer for the dirty render object.
  void DestroyContainerLayer();

  // Remove the container layer from Parent for reusing.
  void RemoveContainerLayerFromParent();

  void RebuildSortedChildrenIfNeeded();
  void ClearColorFilter();
  void ClearImageFilter();

  ElementId id_{-1};

  // Whether this render object repaints separately from its parent.
  bool repaint_boundary_ = false;
  // Whether this render object's paint information is dirty.
  bool needs_paint_ = true;
  // Whether this render object's paint effect is updated, if needs_paint_ is
  // false, we only update effects and reuse the last draw recording.
  bool needs_effect_ = false;

  bool visible_ = true;

  bool painting_ = false;

  bool overlay_ = false;
  bool is_overlay_ng_ = false;
  //  1 to 4 . 4 levels totally. level 4 is the top level, and level 1 is the
  //  bottom level.
  int overlay_level_ = 1;

  // hack for demo
  bool has_default_focus_ring_ = false;

  CacheStrategy strategy_ = CacheStrategy::None;

  // The width/height of the contents + borders + padding.  The left/top
  // location is relative to our container (which is not always our parent).
  BoxData box_data_;
  std::optional<BackgroundData> background_data_;
  std::optional<MaskData> mask_data_;
  std::optional<BordersData> border_;
  std::optional<OutlineData> outline_;
  std::optional<std::vector<Shadow>> shadows_;
  std::optional<TransformOperations> transform_;
  std::optional<float> perspective_;
  FloatPoint transform_origin_;
  std::optional<float> opacity_ = std::nullopt;
  float blur_radius_ = 0.f;
  std::vector<float> current_color_filter_matrix_;
  ClipPathType clip_shape_;
  FilterMode image_filter_mode_ = FilterMode::kLinear;
  ClayAnimationPropertyType raster_transition_properties_;
  ClayAnimationPropertyType raster_animation_properties_;
  TransitionManager* transition_manager_;
  KeyframesManager* keyframes_manager_;

  RenderObject* previous_;
  RenderObject* next_;

  RenderObjectChildList children_;

  std::unique_ptr<BackgroundOrMaskImageClient> bg_image_client_;
  std::unique_ptr<BackgroundOrMaskImageClient> mask_image_client_;
  // The compositor_layer_ is the root of the composited layer subtree.
  std::unique_ptr<PendingContainerLayer> compositor_layer_;
  // The container_layer_ is the root of the content layer and other subtree.
  std::unique_ptr<PendingContainerLayer> container_layer_;
  std::array<std::unique_ptr<PendingEffectLayer>,
             static_cast<size_t>(EffectType::kLast) + 1u>
      effect_layers_;
  uint8_t overflow_ = DefaultOverflowValue();

  static uint8_t default_overflow_;

  float translate_z_ = 0.f;
  int painting_order_ = 0;
  std::vector<RenderObject*> sorted_children_;

  bool image_decode_with_priority_ = false;
};

}  // namespace clay

#endif  // CLAY_UI_RENDERING_RENDER_OBJECT_H_
