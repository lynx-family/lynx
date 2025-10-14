// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/rendering/render_object.h"

#include <algorithm>
#include <cmath>
#include <sstream>

#include "clay/fml/logging.h"
#include "clay/gfx/animation/animation_properties_util.h"
#include "clay/gfx/geometry/float_rounded_rect.h"
#include "clay/gfx/image/image_resource_client.h"
#include "clay/gfx/style/borders_data.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/css_property.h"
#include "clay/ui/compositing/pending_offset_layer.h"

namespace clay {

#define ENSURE_BACKGROUND()                                    \
  do {                                                         \
    if (!background_data_.has_value()) {                       \
      background_data_ = std::make_optional<BackgroundData>(); \
    }                                                          \
  } while (false)

#define ENSURE_MASKIMAGE()                         \
  do {                                             \
    if (!mask_data_.has_value()) {                 \
      mask_data_ = std::make_optional<MaskData>(); \
    }                                              \
  } while (false)

class BackgroundOrMaskImageClient : public ImageResourceClient {
 public:
  explicit BackgroundOrMaskImageClient(RenderObject* obj)
      : render_object_(obj) {}
  ~BackgroundOrMaskImageClient() override = default;

  DecodePriority GetDecodePriority() override {
    return DecodeUtils::GetDecodePriority(render_object_);
  }

  void RegisterUploadTask(OneShotCallback<>&& task, int image_id) override {
    if (render_object_->GetRenderer() &&
        render_object_->GetRenderer()->renderer_client()) {
      render_object_->GetRenderer()->renderer_client()->RegisterUploadTask(
          std::move(task), image_id);
    }
  }

 private:
  void RequestRenderImage(ImageResource* image_resource,
                          bool success) override {
    if (success && render_object_ &&
        (render_object_->HasBackground() || render_object_->HasMask())) {
      render_object_->MarkNeedsPaint();
    }
  }

  bool WillRenderImage() override {
    return render_object_ && render_object_->CanDisplay();
  }

  void OnImageChanged() override {}

  RenderObject* render_object_;
};

uint8_t RenderObject::default_overflow_ = CSSProperty::OVERFLOW_XY;

RenderObject::RenderObject()
    : raster_transition_properties_(ClayAnimationPropertyType::kNone),
      raster_animation_properties_(ClayAnimationPropertyType::kNone),
      transition_manager_(nullptr),
      keyframes_manager_(nullptr),
      previous_(nullptr),
      next_(nullptr) {}

RenderObject::~RenderObject() {
  // Remove this layer from the composite layer tree.
  {
    std::unique_ptr<PendingContainerLayer> temp = nullptr;
    std::swap(temp, compositor_layer_);
    if (temp) {
      temp->Detach();
    }
  }

  {
    std::unique_ptr<PendingContainerLayer> temp = nullptr;
    std::swap(temp, container_layer_);
    if (temp) {
      temp->Detach();
    }
  }

  for (auto& effect_layer : effect_layers_) {
    if (effect_layer) {
      effect_layer->Remove();
    }
  }
}

std::string RenderObject::DebugName() const { return GetName(); }

void RenderObject::SetLeft(float left) {
  if (Left() == left) {
    return;
  }

  if (IsRepaintBoundary() && Parent()) {
    // TODO(jinsong): If there is a parent, let the parent re-composite this
    // object, otherwise re-composite ourselves. If the cost of repainting
    // parent is high, this is a bad case.
    static_cast<RenderObject*>(Parent())->MarkNeedsPaint();
  } else {
    MarkNeedsPaint();
  }

  box_data_.SetLeft(left);
}

void RenderObject::SetTop(float top) {
  if (Top() == top) {
    return;
  }

  if (IsRepaintBoundary() && Parent()) {
    // TODO(jinsong): If there is a parent, let the parent re-composite this
    // object, otherwise re-composite ourselves. If the cost of repainting
    // parent is high, this is a bad case.
    static_cast<RenderObject*>(Parent())->MarkNeedsPaint();
  } else {
    MarkNeedsPaint();
  }

  box_data_.SetTop(top);
}

void RenderObject::SetWidth(float width) {
  if (Width() == width) {
    return;
  }

  MarkNeedsPaint();
  box_data_.SetWidth(width);
}

void RenderObject::SetHeight(float height) {
  if (Height() == height) {
    return;
  }

  MarkNeedsPaint();
  box_data_.SetHeight(height);
}

void RenderObject::SetPaddingLeft(float left) {
  if (PaddingLeft() == left) {
    return;
  }

  MarkNeedsPaint();
  box_data_.SetPaddingLeft(left);
}

void RenderObject::SetPaddingRight(float right) {
  if (PaddingRight() == right) {
    return;
  }

  MarkNeedsPaint();
  box_data_.SetPaddingRight(right);
}

void RenderObject::SetPaddingTop(float top) {
  if (PaddingTop() == top) {
    return;
  }

  MarkNeedsPaint();
  box_data_.SetPaddingTop(top);
}

void RenderObject::SetPaddingBottom(float bottom) {
  if (PaddingBottom() == bottom) {
    return;
  }

  MarkNeedsPaint();
  box_data_.SetPaddingBottom(bottom);
}

void RenderObject::SetBorders(const BordersData& borders_data) {
  border_ = std::make_optional<BordersData>(borders_data);
  MarkNeedsPaint();
}

void RenderObject::SetOutline(const OutlineData& outline_data) {
  outline_ = std::make_optional<OutlineData>(outline_data);
  MarkNeedsPaint();
}

void RenderObject::SetVisible(bool visible) {
  if (visible == visible_) {
    return;
  }

  visible_ = visible;
  MarkSubtreeDirty();
}

void RenderObject::SetShadow(const Shadow& shadow) {
  if (!shadows_.has_value()) {
    shadows_ = std::make_optional<std::vector<Shadow>>();
  }

  shadows_->push_back(shadow);
  MarkNeedsPaint();
}

void RenderObject::SetShadows(std::vector<Shadow>&& shadows) {
  if (!shadows_.has_value() || shadows != shadows_) {
    shadows_ = std::move(shadows);
    MarkNeedsPaint();
  }
}

FloatRect RenderObject::GetFrameRectEx() const {
  FloatRect box_rect(0, 0, Width(), Height());
  if (!HasShadow() && !HasOutline() && !HasDefaultFocusRing()) {
    return box_rect;
  }

  FloatRect bounds = box_rect;

  if (HasShadow()) {
    // Union a series of shadows with BoxRect.
    for (const auto& shadow : shadows_.value()) {
      if (shadow.inset) {
        continue;
      }

      FloatRect shadow_rect = box_rect;
      auto radius = 2.0 * shadow.blur_radius + shadow.spread_radius;
      shadow_rect.Move(shadow.offset_x - radius, shadow.offset_y - radius);
      shadow_rect.Expand(2 * radius, 2 * radius);
      shadow_rect.SetLocation(
          {std::min(0.f, shadow_rect.location().x() - box_data_.Left()),
           std::min(0.f, shadow_rect.location().y() - box_data_.Top())});
      bounds.ExpandToInclude(shadow_rect);
    }
  }

  if (HasOutline()) {
    FloatRect outline_rect = box_rect;
    outline_rect.Move(-outline_->width_, -outline_->width_);
    outline_rect.Expand(2 * outline_->width_, 2 * outline_->width_);
    bounds.ExpandToInclude(outline_rect);
  }

  if (HasDefaultFocusRing()) {
    FloatRect focus_ring_rect = box_rect;
    focus_ring_rect.Move(-num_value::kFocusRingThickness,
                         -num_value::kFocusRingThickness);
    focus_ring_rect.Expand(2 * num_value::kFocusRingThickness,
                           2 * num_value::kFocusRingThickness);
    bounds.ExpandToInclude(focus_ring_rect);
  }

  return bounds;
}

FloatPoint RenderObject::OffsetInLayer() const {
  if (!HasShadow() && !HasOutline() && !HasDefaultFocusRing()) {
    return location();
  }

  FloatPoint offset = location();
  FloatPoint offset_ex = GetFrameRectEx().location();

  // The figure below illustrates how to calculate the layer offset.
  //     |
  //     |    offset   _____________
  //     |<---------->|    View     |
  // |   |            | case1       |
  // |   |    | case2 |<---->|      |
  // |   |    |<----->|      |      |
  // |<--|----------->|             |
  //     |  case3     |             |
  //     |            |_____________|
  //
  // case1: offset_ex > 0, layer_offset = offset;
  // case2: offset_ex < 0 & abs(offset_ex) < offset,
  //        layer_offset = offset_ex + offset;
  // case3: offset_ex < 0 & abs(offset_ex) > offset,
  //        layer_offset = 0;
  float offset_x = 0;
  if (offset_ex.x() >= 0) {
    offset_x = offset.x();
  } else {
    offset_x = std::max(0.f, offset.x() + offset_ex.x());
  }

  float offset_y = 0;
  if (offset_ex.y() >= 0) {
    offset_y = offset.y();
  } else {
    offset_y = std::max(0.f, offset.y() + offset_ex.y());
  }
  return FloatPoint(offset_x, offset_y);
}

FloatPoint RenderObject::PaintOffsetEx() const {
  if (!HasShadow() && !HasOutline() && !HasDefaultFocusRing()) {
    return FloatPoint();
  }

  FloatPoint offset = location();
  FloatPoint offset_ex = GetFrameRectEx().location();
  float offset_x = 0;
  if (offset_ex.x() >= 0) {
    offset_x = 0;
  } else if (std::abs(offset_ex.x()) < offset.x()) {
    offset_x = std::abs(offset_ex.x());
  } else {
    offset_x = offset.x();
  }
  float offset_y = 0;
  if (offset_ex.y() >= 0) {
    offset_y = 0;
  } else if (std::abs(offset_ex.y()) < offset.y()) {
    offset_y = std::abs(offset_ex.y());
  } else {
    offset_y = offset.y();
  }
  return FloatPoint(offset_x, offset_y);
}

void RenderObject::SetBackgroundData(const BackgroundData& background_data) {
  background_data_ = std::make_optional<BackgroundData>();
  background_data_->background_color = background_data.background_color;
  background_data_->images.resize(background_data.background_images.size());
  for (const auto& bg : background_data.background_images) {
    background_data_->clips.emplace_back(bg.clip);
    background_data_->origins.emplace_back(bg.origin);
    background_data_->repeats.emplace_back(bg.repeat);  // repeat-x
    background_data_->repeats.emplace_back(bg.repeat);  // repeat-y
  }
  MarkNeedsPaint();
}

void RenderObject::ResizeBackground(size_t size) {
  ENSURE_BACKGROUND();
  if (background_data_->images.size() != size) {
    background_data_->images.resize(size);
    MarkNeedsPaint();
  }
}

void RenderObject::ResizeMask(size_t size) {
  ENSURE_MASKIMAGE();
  if (mask_data_->images.size() != size) {
    mask_data_->images.resize(size);
    MarkNeedsPaint();
  }
}

#ifndef ENABLE_SKITY
void RenderObject::SetBackgroundImage(size_t index,
                                      std::unique_ptr<ImageResource> resource) {
  if (!HasBackground() || index >= background_data_->images.size()) {
    return;
  }

  if (resource) {
    if (!bg_image_client_) {
      bg_image_client_ = std::make_unique<BackgroundOrMaskImageClient>(this);
    }
    resource->AddImageResourceClient(bg_image_client_.get());
    background_data_->images[index].SetImageResource(std::move(resource));
  }
  MarkNeedsPaint();
}

void RenderObject::SetMaskImage(size_t index,
                                std::unique_ptr<ImageResource> resource) {
  if (!HasMask() || index >= mask_data_->images.size()) {
    return;
  }

  if (resource) {
    if (!mask_image_client_) {
      mask_image_client_ = std::make_unique<BackgroundOrMaskImageClient>(this);
    }
    resource->AddImageResourceClient(mask_image_client_.get());
    mask_data_->images[index].SetImageResource(std::move(resource));
  }
  MarkNeedsPaint();
}
#else
void RenderObject::SetBackgroundImage(size_t index,
                                      std::shared_ptr<BaseImage> image) {
  if (!HasBackground() || index >= background_data_->images.size()) {
    return;
  }
  if (image) {
    background_data_->images[index].SetImageResource(image);
  }
  MarkNeedsPaint();
}

void RenderObject::SetMaskImage(size_t index,
                                std::shared_ptr<BaseImage> image) {
  if (!HasMask() || index >= mask_data_->images.size()) {
    return;
  }
  if (image) {
    mask_data_->images[index].SetImageResource(image);
  }
  MarkNeedsPaint();
}
#endif  // ENABLE_SKITY

void RenderObject::SetBackgroundColor(const Color& color,
                                      bool skip_update_for_raster_animation) {
  ENSURE_BACKGROUND();
  if (background_data_->background_color == color) {
    return;
  }
  background_data_->background_color = color;
  if (!skip_update_for_raster_animation) {
    MarkNeedsPaint();
  }
}

void RenderObject::SetBackgroundImage(size_t index, const Gradient& gradient) {
  if (!HasBackground() || index >= background_data_->images.size()) {
    return;
  }
  background_data_->images[index].SetGradient(gradient);
  MarkNeedsPaint();
}

void RenderObject::SetBackgroundClip(
    const std::vector<ClayBackgroundClipType>& clips) {
  ENSURE_BACKGROUND();
  background_data_->clips = clips;
  MarkNeedsPaint();
}

void RenderObject::SetBackgroundOrigin(
    const std::vector<ClayBackgroundOriginType>& origins) {
  ENSURE_BACKGROUND();
  background_data_->origins = origins;
  MarkNeedsPaint();
}

void RenderObject::SetBackgroundPosition(
    const std::vector<BackgroundPosition>& positions) {
  ENSURE_BACKGROUND();
  background_data_->positions = positions;
  MarkNeedsPaint();
}

void RenderObject::SetBackgroundRepeat(
    const std::vector<ClayBackgroundRepeatType>& repeats) {
  ENSURE_BACKGROUND();
  background_data_->repeats = repeats;
  MarkNeedsPaint();
}

void RenderObject::SetBackgroundSize(const std::vector<BackgroundSize>& sizes) {
  ENSURE_BACKGROUND();
  background_data_->sizes = sizes;
  MarkNeedsPaint();
}

void RenderObject::SetMaskImage(size_t index, const Gradient& gradient) {
  ENSURE_MASKIMAGE();
  if (!HasMask() || index >= mask_data_->images.size()) {
    return;
  }
  mask_data_->images[index].SetGradient(gradient);
  MarkNeedsPaint();
}

void RenderObject::SetMaskPosition(const std::vector<MaskPosition>& positions) {
  ENSURE_MASKIMAGE();
  mask_data_->positions = positions;
  MarkNeedsPaint();
}

void RenderObject::SetMaskRepeat(
    const std::vector<ClayMaskRepeatType>& repeats) {
  ENSURE_MASKIMAGE();
  mask_data_->repeats = repeats;
  MarkNeedsPaint();
}

void RenderObject::SetMaskOrigin(
    const std::vector<ClayMaskOriginType>& origins) {
  ENSURE_MASKIMAGE();
  mask_data_->origins = origins;
  MarkNeedsPaint();
}

void RenderObject::SetMaskSize(const std::vector<MaskSize>& mask_sizes) {
  ENSURE_MASKIMAGE();
  mask_data_->sizes = mask_sizes;
  MarkNeedsPaint();
}

void RenderObject::SetMaskClip(const std::vector<ClayMaskClipType>& clips) {
  ENSURE_MASKIMAGE();
  mask_data_->clips = clips;
  MarkNeedsPaint();
}

void RenderObject::SetHasDefaultFocusRing(bool has_default_focus_ring) {
  if (has_default_focus_ring == has_default_focus_ring_) {
    return;
  }
  has_default_focus_ring_ = has_default_focus_ring;
  MarkNeedsPaint();
}

void RenderObject::SetImageFilterMode(FilterMode mode) {
  if (image_filter_mode_ != mode) {
    image_filter_mode_ = mode;
    MarkNeedsPaint();
  }
}

bool RenderObject::HasTransformOperations() const {
  return transform_.has_value();
}

void RenderObject::SetTransformOperations(const TransformOperations& transform,
                                          bool is_from_animation) {
  if (transform_ &&
      transform_->ApproximatelyEqual(
          transform, TransformOperations::kApproximatelyEqualTolerance)) {
    return;
  }
  float translate_z = transform.GetTranslateZ();
  if (std::abs(translate_z_ - translate_z) > 1e-6) {
    // translate_z_ changes.
    translate_z_ = translate_z;
    if (auto* parent = static_cast<RenderObject*>(Parent())) {
      parent->DirtyChildrenPaintingOrder();
    }
  }
  transform_ = transform;
  if (!HasTransform()) {
    // Resets transform_ to identity and needs mark dirty to repaint.
    MarkNeedsPaint();
  } else {
    // Don't dirty this node, only update transform effect.
    // Don't ever update transform effect while using rasterizer animation
    if (!is_from_animation) {
      MarkNeedsEffect();
    }
  }
}

void RenderObject::SetPerspective(float value) {
  if (value != perspective_.value_or(0.f)) {
    if (!HasPerspective()) {
      MarkNeedsPaint();
    } else {
      MarkNeedsEffect();
    }
    perspective_ = value;
  }
}

void RenderObject::RebuildSortedChildrenIfNeeded() {
  if (!sorted_children_.empty()) {
    return;
  }

  bool needs_reorder = false;
  for (auto* child = VirtualChildren().FirstChild(); child != nullptr;
       child = child->NextSibling()) {
    needs_reorder |= child->GetTranslateZ() || child->GetPaintingOrder();
    sorted_children_.push_back(child);
  }

  if (!needs_reorder) {
    return;
  }

  std::stable_sort(
      sorted_children_.begin(), sorted_children_.end(),
      [](RenderObject* node1, RenderObject* node2) {
        if (node1->GetTranslateZ() < node2->GetTranslateZ()) {
          return true;
        } else if (node1->GetTranslateZ() == node2->GetTranslateZ()) {
          return node1->GetPaintingOrder() < node2->GetPaintingOrder();
        }
        return false;
      });
}

void RenderObject::VisitChildren(
    const std::function<void(RenderObject*)> visitor) {
  RebuildSortedChildrenIfNeeded();
  for (auto* child : sorted_children_) {
    visitor(child);
  }
}

void RenderObject::SetTransformOrigin(const FloatPoint& origin) {
  transform_origin_ = origin;
  MarkNeedsEffect();
}

void RenderObject::SetOpacity(float opacity, bool is_from_animation) {
  if (opacity_.has_value() && opacity_.value() == opacity) {
    return;
  }

  opacity_ = opacity;
  if (!HasOpacity()) {
    // The latest opacity is 1.0, which means it changes from translucent to
    // opaque and needs mark dirty to repaint.
    MarkNeedsPaint();
  } else {
    // Don't dirty this node, only update opacity effect.
    // Don't ever update opacity effect while using rasterizer animation
    if (!is_from_animation) {
      MarkNeedsEffect();
    }
  }
}

void RenderObject::ClearFilter() {
  ClearColorFilter();
  ClearImageFilter();
  MarkNeedsPaint();
}

void RenderObject::ClearColorFilter() { current_color_filter_matrix_.clear(); }

void RenderObject::ClearImageFilter() { blur_radius_ = 0.f; }

void RenderObject::SetBlurRadius(float radius) {
  if (blur_radius_ == radius) {
    return;
  }
  blur_radius_ = radius;
  if (!HasBlur()) {
    MarkNeedsPaint();
  } else {
    MarkNeedsEffect();
  }
}

void RenderObject::CombineColorFilter(const std::vector<float>& matrix) {
  if (current_color_filter_matrix_.empty()) {
    current_color_filter_matrix_ = matrix;
  } else {
    std::vector<float> target(20);

    int index = 0;
    for (int j = 0; j < 20; j += 5) {
      for (int i = 0; i < 4; i++) {
        target[index++] = matrix[j + 0] * current_color_filter_matrix_[i + 0] +
                          matrix[j + 1] * current_color_filter_matrix_[i + 5] +
                          matrix[j + 2] * current_color_filter_matrix_[i + 10] +
                          matrix[j + 3] * current_color_filter_matrix_[i + 15];
      }
      target[index++] = matrix[j + 0] * current_color_filter_matrix_[4] +
                        matrix[j + 1] * current_color_filter_matrix_[9] +
                        matrix[j + 2] * current_color_filter_matrix_[14] +
                        matrix[j + 3] * current_color_filter_matrix_[19] +
                        matrix[j + 4];
    }
    current_color_filter_matrix_ = target;
  }
}

void RenderObject::AppendGrayScale(float gray_scale) {
  FML_DCHECK(gray_scale >= 0 && gray_scale <= 1);
  if (gray_scale != 0.f) {
    // ref: cc/paint/render_surface_filters.cc
    auto v = 1 - gray_scale;
    auto m01 = 0.2126f + 0.7874f * v;
    auto m02 = 0.7152f - 0.7152f * v;
    auto m10 = 0.2126f - 0.2126f * v;
    auto m11 = 0.7152f + 0.2848f * v;
    std::vector<float> m{m01,
                         m02,
                         1.f - (m01 + m02),
                         0.f,
                         0.f,
                         m10,
                         m11,
                         1.f - (m10 + m11),
                         0.f,
                         0.f,
                         m10,
                         m02,
                         1.f - (m10 + m02),
                         0.f,
                         0.f,
                         0.f,
                         0.f,
                         0.f,
                         1.f,
                         0.f};
    CombineColorFilter(m);
    MarkNeedsEffect();
  }
}

void RenderObject::AppendBrightness(float brightness) {
  FML_DCHECK(brightness >= 0);
  if (brightness != 1.0) {
    // ref: cc/paint/render_surface_filters.cc
    // [0, +inf]
    std::vector<float> m{brightness, 0.f, 0.f, 0.f, 0.f, 0.f,        brightness,
                         0.f,        0.f, 0.f, 0.f, 0.f, brightness, 0.f,
                         0.f,        0.f, 0.f, 0.f, 1.f, 0.f};
    CombineColorFilter(m);
    MarkNeedsEffect();
  };
}

void RenderObject::AppendContrast(float contrast) {
  FML_DCHECK(contrast >= 0);
  if (contrast != 1.0f) {
    // ref: cc/paint/render_surface_filters.cc
    auto v = -0.5f * contrast + 0.5f;
    std::vector<float> m{
        contrast, 0.f, 0.f,      0.f, v, 0.f, contrast, 0.f, 0.f, v,
        0.f,      0.f, contrast, 0.f, v, 0.f, 0.f,      0.f, 1.f, 0.f,
    };
    CombineColorFilter(m);
    MarkNeedsEffect();
  }
}

void RenderObject::AppendSaturate(float saturate) {
  FML_DCHECK(saturate >= 0);
  if (saturate != 1.0f) {
    auto m01 = 0.213f + 0.787f * saturate;
    auto m02 = 0.715f - 0.715f * saturate;
    auto m10 = 0.213f - 0.213f * saturate;
    auto m11 = 0.715f + 0.285f * saturate;
    std::vector<float> m{m01,
                         m02,
                         1.f - (m01 + m02),
                         0.f,
                         0.f,
                         m10,
                         m11,
                         1.f - (m10 + m11),
                         0.f,
                         0.f,
                         m10,
                         m02,
                         1.f - (m10 + m02),
                         0.f,
                         0.f,
                         0.f,
                         0.f,
                         0.f,
                         1.f,
                         0.f};
    CombineColorFilter(m);
    MarkNeedsEffect();
  }
}
void RenderObject::AppendHueRotate(float deg) {
  FML_CHECK(deg >= 0);
  if (deg != 0) {
    float rad = deg * 3.1415926 / 180;
    float cos_hue = std::cosf(rad);
    float sin_hue = std::sinf(rad);
    std::vector<float> m{0.213f + cos_hue * 0.787f - sin_hue * 0.213f,
                         0.715f - cos_hue * 0.715f - sin_hue * 0.715f,
                         0.072f - cos_hue * 0.072f + sin_hue * 0.928f,
                         0.f,
                         0.f,
                         0.213f - cos_hue * 0.213f + sin_hue * 0.143f,
                         0.715f + cos_hue * 0.285f + sin_hue * 0.140f,
                         0.072f - cos_hue * 0.072f - sin_hue * 0.283f,
                         0.f,
                         0.f,
                         0.213f - cos_hue * 0.213f - sin_hue * 0.787f,
                         0.715f - cos_hue * 0.715f + sin_hue * 0.715f,
                         0.072f + cos_hue * 0.928f + sin_hue * 0.072f,
                         0.f,
                         0.f,
                         0.f,
                         0.f,
                         0.f,
                         1.f,
                         0.f};
    CombineColorFilter(m);
    MarkNeedsEffect();
  }
}

void RenderObject::AppendCustomColorMatrix(
    const std::array<float, 20>& matrix) {
  std::vector<float> m(matrix.begin(), matrix.end());
  CombineColorFilter(m);
  MarkNeedsEffect();
}

void RenderObject::SetOverflow(uint8_t overflow) {
  overflow_ = overflow;
  MarkNeedsPaint();
}

void RenderObject::SetClipPath(const FloatRoundedRect& rrect) {
  if (auto ptr = std::get_if<FloatRoundedRect>(&clip_shape_)) {
    if (*ptr == rrect) {
      return;
    }
  }
  clip_shape_ = rrect;
  MarkNeedsPaint();
}

void RenderObject::SetClipPath(const GrPath& path) {
  if (auto ptr = std::get_if<GrPath>(&clip_shape_)) {
    if (*ptr == path) {
      return;
    }
  }
  clip_shape_ = path;
  MarkNeedsPaint();
}

void RenderObject::ClearClipPath() {
  if (auto ptr = std::get_if<std::monostate>(&clip_shape_)) {
    return;
  }
  clip_shape_ = std::monostate{};
  MarkNeedsPaint();
}

RenderObject::ClipPathType& RenderObject::ClipPath() { return clip_shape_; }

bool RenderObject::IsDescendantOf(const RenderObject* parent) const {
  for (const RenderObject* object = this; object;
       object = static_cast<const RenderObject*>(object->Parent())) {
    if (object == parent) {
      return true;
    }
  }
  return false;
}

void RenderObject::RedepthChildren() {
  RenderObject* child = SlowFirstChild();
  while (child) {
    RedepthChild(child);
    child = child->NextSibling();
  }
}

void RenderObject::AddChild(RenderObject* new_child,
                            RenderObject* before_child) {
  FML_DCHECK(!new_child->Parent());
  AdoptChild(new_child);

  VirtualChildren().InsertChild(this, new_child, before_child);

  // Set Renderer for child.
  new_child->SetRenderer(renderer_);
  new_child->MarkSubtreeDirty();
  MarkNeedsPaint();
}

void RenderObject::RemoveChild(RenderObject* old_child) {
  VirtualChildren().RemoveChild(this, old_child);

  DropChild(old_child);

  old_child->SetRenderer(nullptr);
  MarkNeedsPaint();
}

void RenderObject::RemoveAllChildren() {
  RenderObject* child = VirtualChildren().FirstChild();
  while (child) {
    VirtualChildren().RemoveChild(this, child);
    DropChild(child);
    child->SetRenderer(nullptr);
    child = VirtualChildren().FirstChild();
  }
  MarkNeedsPaint();
}

void RenderObject::RemoveAndCollectChildren(
    std::vector<RenderObject*>& objects) {
  while (VirtualChildren().FirstChild()) {
    RenderObject* child = VirtualChildren().FirstChild();
    objects.push_back(child);
    VirtualChildren().RemoveChild(this, child);
    DropChild(child);
  }
}

void RenderObject::BringChildToFront(RenderObject* child) {
  if (VirtualChildren().LastChild() == child) {
    return;
  }
  VirtualChildren().RemoveChild(this, child);
  VirtualChildren().InsertChild(this, child, nullptr);
  MarkNeedsPaint();
}

RenderObject* RenderObject::NextInPreOrder() const {
  if (RenderObject* object = SlowFirstChild()) {
    return object;
  }

  return NextInPreOrderAfterChildren();
}

RenderObject* RenderObject::NextInPreOrderAfterChildren() const {
  const RenderObject* object = NextSibling();
  if (!object) {
    object = static_cast<const RenderObject*>(Parent());
    while (object && !object->NextSibling()) {
      object = static_cast<const RenderObject*>(object->Parent());
    }
    if (object) {
      object = object->NextSibling();
    }
  }

  return const_cast<RenderObject*>(object);
}

RenderObject* RenderObject::NextInPreOrder(
    const RenderObject* stay_within) const {
  if (RenderObject* object = SlowFirstChild()) {
    return object;
  }

  return NextInPreOrderAfterChildren(stay_within);
}

RenderObject* RenderObject::NextInPreOrderAfterChildren(
    const RenderObject* stay_within) const {
  if (this == stay_within) {
    return nullptr;
  }

  const RenderObject* current = this;
  RenderObject* next = current->NextSibling();
  for (; !next; next = current->NextSibling()) {
    current = static_cast<const RenderObject*>(current->Parent());
    if (!current || current == stay_within) {
      return nullptr;
    }
  }
  return next;
}

RenderObject* RenderObject::PreviousInPreOrder() const {
  if (RenderObject* object = PreviousSibling()) {
    while (RenderObject* last = object->SlowLastChild()) {
      object = last;
    }
    return object;
  }

  return const_cast<RenderObject*>(static_cast<const RenderObject*>(Parent()));
}

RenderObject* RenderObject::PreviousInPreOrder(
    const RenderObject* stay_within) const {
  if (this == stay_within) {
    return nullptr;
  }

  return PreviousInPreOrder();
}

RenderObject* RenderObject::ChildAt(unsigned index) const {
  RenderObject* child = SlowFirstChild();
  for (unsigned i = 0; child && i < index; i++) {
    child = child->NextSibling();
  }
  return child;
}

RenderObject* RenderObject::LastLeafChild() const {
  RenderObject* leaf = SlowLastChild();
  while (leaf) {
    RenderObject* object = nullptr;
    object = leaf->SlowLastChild();
    if (!object) {
      break;
    }
    leaf = object;
  }
  return leaf;
}

void RenderObject::Destroy() {
  SetRenderer(nullptr);
  for (auto& effect_layer : effect_layers_) {
    if (effect_layer) {
      effect_layer->Remove();
    }
  }
  std::fill(effect_layers_.begin(), effect_layers_.end(), nullptr);
  Remove();
}

void RenderObject::ValidateForPaint(bool parent_visibility) {
  bool visible = parent_visibility && visible_;
  if (!visible && (needs_paint_ || needs_effect_)) {
    renderer_->RemoveDirtyNode(this);
    needs_paint_ = false;
    needs_effect_ = false;
  }
  RenderObject* child = VirtualChildren().FirstChild();
  while (child) {
    child->ValidateForPaint(visible);
    child = child->NextSibling();
  }
}

bool RenderObject::IsActualVisible() {
  // TODO(dongjiajian): Wait for image deferred decode
  if (!renderer_ || !Visible()) {
    return false;
  }
  if (!Parent()) {
    return true;
  }
  return static_cast<RenderObject*>(Parent())->IsActualVisible();
}

void RenderObject::SetRepaintBoundary(bool repaint_boundary) {
  if (repaint_boundary_ == repaint_boundary) {
    return;
  }
  // Don't flatten RenderObjects with raster animations.
  if (!repaint_boundary && HasRasterAnimation()) {
    return;
  }

  repaint_boundary_ = repaint_boundary;
  MarkNeedsPaint();
}

bool RenderObject::WasRepaintBoundary() {
  // This is a hint for this RenderObject with repaint boundary.
  if (GetContainerLayer()) {
    return true;
  }

  return renderer_ && renderer_->Contains(this);
}

void RenderObject::SetRenderer(Renderer* renderer) {
  if (renderer_ == renderer) {
    return;
  }

  if (renderer_) {
    // Remove from old one.
    renderer_->RemoveDirtyNode(this);
  }

  renderer_ = renderer;

  if (renderer_ && needs_paint_) {
    needs_paint_ = false;
    MarkNeedsPaint();
  }

  if (CanHaveChildren()) {
    RenderObject* child = VirtualChildren().FirstChild();
    while (child) {
      child->SetRenderer(renderer_);
      child = child->NextSibling();
    }
  }
}

void RenderObject::MarkSubtreeDirty() {
  MarkNeedsPaint();
  auto* child = VirtualChildren().FirstChild();
  while (child) {
    child->MarkSubtreeDirty();
    child = child->NextSibling();
  }
}

void RenderObject::MarkNeedsPaint(bool force_repaint) {
  if (IsPainting()) {
    return;
  }

  bool was_repaint_boundary = WasRepaintBoundary();
  if (!force_repaint && needs_paint_ &&
      was_repaint_boundary == IsRepaintBoundary()) {
    // The current render object has been marked as dirty, no need to traverse,
    // only request repaint.
    if (renderer_) {
      renderer_->RequestPaint();
    }
    return;
  }

  needs_paint_ = true;

  if (was_repaint_boundary && !IsRepaintBoundary()) {
    // When the render object changes from a repaint boundary to a non-boundary,
    // we should move its descendant boundary layers (i.e. `OffsetLayer`s) to
    // its parent (Here we just remove all children, the `OffsetLayer`s will be
    // added to its parent automatically in next paint) and remove its layers
    auto layer = GetLayer();
    if (layer) {
      layer->RemoveAllChildren();
      layer->Remove();
    }
    ResetLayer();

    if (renderer_) {
      renderer_->RemoveDirtyNode(this);
    }
  }

  if (IsRepaintBoundary()) {
    // If we always have our own picture layer, then we can just repaint
    // ourselves without involving any other nodes.
    if (renderer_) {
      if (!GetContainerLayer() && Parent()) {
        // If there is no content layer on it, means it was not a repaint
        // boundary before. And we should mark its parent to paint to rebuild
        // the layer tree.
        static_cast<RenderObject*>(Parent())->MarkNeedsPaint();
      }

      if (!needs_effect_) {
        renderer_->AddNeedPaint(this);
        renderer_->RequestPaint();
      }
    }
    DestroyContainerLayer();
  } else if (Parent()) {
    static_cast<RenderObject*>(Parent())->MarkNeedsPaint();
  } else {
    // If we're the root of the render tree, then we have to paint ourselves.
    if (renderer_) {
      renderer_->RequestPaint();
    }
  }
}

void RenderObject::MarkNeedsEffect() {
  // Changes of state of repaint_boundary will change the structure of UI layer
  // tree. In this case we need to repaint the render object instead of just
  // updating effect.
  if (WasRepaintBoundary() != IsRepaintBoundary()) {
    needs_effect_ = false;
    MarkNeedsPaint();
    return;
  }

  if (needs_effect_) {
    return;
  }

  needs_effect_ = true;
  if (IsRepaintBoundary()) {
    // If we always have our own offset layer, then we can just update effect
    // layers without involving any other nodes.
    if (renderer_) {
      if (!needs_paint_) {
        renderer_->AddNeedPaint(this);
        renderer_->RequestPaint();
      }
    }
  } else if (Parent()) {
    static_cast<RenderObject*>(Parent())->MarkNeedsEffect();
  } else {
    // If we're the root of the render tree, then we have to paint ourselves.
    if (renderer_) {
      renderer_->RequestPaint();
    }
  }
}

void RenderObject::DestroyContainerLayer() {
  RemoveContainerLayerFromParent();
  container_layer_.reset();
  for (auto& effect_layer : effect_layers_) {
    if (effect_layer) {
      effect_layer->Remove();
    }
  }
  std::fill(effect_layers_.begin(), effect_layers_.end(), nullptr);
  if (compositor_layer_) {
    compositor_layer_->RemoveAllChildren();
    compositor_layer_->RetainLayer(nullptr);
  }
}

void RenderObject::RemoveContainerLayerFromParent() {
  if (!GetContainerLayer()) {
    return;
  }

  PendingContainerLayer* parent =
      static_cast<PendingContainerLayer*>(GetContainerLayer()->Parent());
  if (parent) {
    parent->RemoveChild(GetContainerLayer());
  }
}

void RenderObject::PaintWithContext(PaintingContext& context,
                                    const FloatPoint& offset) {
  if (!CanDisplay()) {
    // Resume painting status.
    painting_ = false;
    needs_paint_ = false;
    needs_effect_ = false;
    return;
  }

  painting_ = true;
  needs_paint_ = false;
  needs_effect_ = false;
  // FIXME(fangzheyuan): Read IsRepaintBoundary before actual rendering starts.
  // Some render object's properties(like 'repaint_boundary_') can be mutable in
  // painting, this is a temporary workaround to avoid this scenario.
  bool is_repaint_boundary = IsRepaintBoundary();

  if (is_repaint_boundary && GetContainerLayer()) {
    context.AddLayer(GetContainerLayer());
  } else {
    // If this object has shadow, should add shadow offset to show shadow.
    Paint(context, offset + PaintOffsetEx());

    if (is_repaint_boundary && !GetContainerLayer() &&
        context.GetContainerLayer()) {
      SetContainerLayer(
          std::unique_ptr<PendingContainerLayer>(context.GetContainerLayer()));
    }
  }

  // Check that the Paint() method didn't mark us dirty again.
  FML_DCHECK(!needs_paint_);
  FML_DCHECK(!needs_effect_);
  painting_ = false;
#ifdef ENABLE_RASTER_CACHE_SCALE
  if (strategy_ == CacheStrategy::NotCache) {
    // Reset strategy for self and descendant render objects.
    SetCacheStrategyRecursively(CacheStrategy::None, false);
  }
#endif
}

void RenderObject::SetCacheStrategyRecursively(CacheStrategy strategy,
                                               bool set_layer) {
  strategy_ = strategy;
  if (set_layer && compositor_layer_) {
    compositor_layer_->SetCacheStrategyRecursively(strategy);
  }
  auto child = VirtualChildren().FirstChild();
  while (child) {
    child->SetCacheStrategyRecursively(strategy, set_layer);
    child = child->NextSibling();
  }
}

void RenderObject::WillPaint() {
  RenderObject* child = VirtualChildren().FirstChild();
  while (child) {
    child->WillPaint();
    child = child->NextSibling();
  }
}

#ifndef NDEBUG
void RenderObject::DumpRenderTree() const {
  std::string intent;
  intent.append(2 * Depth(), ' ');
  FML_LOG(ERROR) << intent << "[" << GetName() << "] " << this << ToString();

  RenderObject* child = VirtualChildren().FirstChild();
  while (child) {
    child->DumpRenderTree();
    child = child->NextSibling();
  }
}

std::string RenderObject::ToString() const {
  std::stringstream ss;
  ss << " id=" << id_.ToString();
  if (!Visible()) {
    ss << " visible=false";
  }
  ss << " frame=(" << Left() << "," << Top() << "," << Width() << ","
     << Height() << ")";
  if (PaddingLeft() != 0 || PaddingTop() != 0 || PaddingRight() != 0 ||
      PaddingBottom() != 0) {
    ss << " padding=(" << PaddingLeft() << "," << PaddingTop() << ","
       << PaddingRight() << "," << PaddingBottom() << ")";
  }
  if (GetLayer()) {
    ss << " " << GetLayer()->GetName() << "(" << GetLayer() << ")";
  }
  if (GetContainerLayer()) {
    std::string children_info = "children layers:[";
    auto* child = GetContainerLayer()->FirstChild();
    while (child) {
      children_info += child->GetName() + ",";
      child = child->NextSibling();
    }
    children_info += "]";
    ss << " Container(" << GetContainerLayer() << ")," << children_info;
  }
  if (IsRepaintBoundary()) {
    ss << " (RepaintBoundary)";
  }
  if (HasShadow()) {
    ss << " shadow=(";
    for (const auto& shadow : Shadows()) {
      if (shadow.inset) {
        ss << "inset ";
      }
      ss << shadow.offset_x << " " << shadow.offset_y << " "
         << shadow.blur_radius << " " << shadow.spread_radius << " "
         << shadow.color.ToString();
    }
    ss << ")";
  }
  if (HasBorder()) {
    ss << " " << Border().ToString();
  }
  if (HasBackground()) {
    if (Background().background_color.Alpha() != 0) {
      ss << " background-color=" << Background().background_color.ToString();
    }
    if (!Background().background_images.empty() &&
        Background().background_images.at(0).type ==
            ClayBackgroundImageType::kUrl) {
      ss << " background-image=("
         << Background().background_images.at(0).src_str << ")";
    }
    if (!Background().images.empty()) {
      if (Background().images.at(0).GetImageResource() != nullptr) {
#ifndef ENABLE_SKITY
        ss << " images=("
           << Background().images.at(0).GetImageResource()->GetUrl() << ")";
#endif  // ENABLE_SKITY
      } else {
        ss << " images=("
           << "gradient"
           << ")";
      }
    }
  }
  return ss.str();
}
#endif

bool RenderObject::HasTransition(ClayAnimationPropertyType type) const {
  return AnimationPropertyTest(raster_transition_properties_, type);
}

void RenderObject::MarkHasTransition(ClayAnimationPropertyType type,
                                     bool value) {
  AnimationPropertySetIf(raster_transition_properties_, type, value);
}

bool RenderObject::HasAnimation(ClayAnimationPropertyType type) const {
  return AnimationPropertyTest(raster_animation_properties_, type);
}

void RenderObject::MarkHasAnimation(ClayAnimationPropertyType type,
                                    bool value) {
  AnimationPropertySetIf(raster_animation_properties_, type, value);
}

void RenderObject::SetPaintingOrder(int painting_order) {
  if (painting_order_ == painting_order) {
    return;
  }
  painting_order_ = painting_order;

  if (auto* parent = static_cast<RenderObject*>(Parent())) {
    parent->DirtyChildrenPaintingOrder();
  }
}

bool RenderObject::HasTransformExpansion() const {
  const RenderObject* object = this;
  skity::Matrix transform;
  while (object) {
    transform.PostConcat(object->GetTransform().matrix());
    object = static_cast<const RenderObject*>(object->Parent());
  }
  if (transform.Get(0, 0) > 1 || transform.Get(1, 1) > 1) {
    return true;
  }
  return false;
}

bool RenderObject::HasBackgroundClipText() const {
  if (!HasBackground()) {
    return false;
  }
  const BackgroundData& background = Background();
  for (auto type : background.clips) {
    if (ClayBackgroundClipType::kText == type) {
      return true;
    }
  }
  return false;
}

bool RenderObject::CanApplyAA() const {
  // Currently if a node has mask-image, then it can not apply AA when drawing
  // background.
  const RenderObject* node = this;
  do {
    if (node->IsRepaintBoundary()) {
      return !node->HasMask();
    }
    node = static_cast<const RenderObject*>(node->Parent());
  } while (node);
  return true;
}

void RenderObject::DecodeImages() {
#ifndef ENABLE_SKITY
  if (!HasBackground()) {
    return;
  }
  // Decode background images.
  for (auto& image : Background().images) {
    if (image.IsSkImage() &&
        image.GetImageResource()->GetImage()->NeedDecode()) {
      DecodePriority priority = DecodeUtils::GetDecodePriority(this);
      image.GetImageResource()->GetGraphicsImage(priority);
    }
  }
#endif  // ENABLE_SKITY
}

}  // namespace clay
