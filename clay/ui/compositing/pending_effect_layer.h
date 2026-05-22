// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPOSITING_PENDING_EFFECT_LAYER_H_
#define CLAY_UI_COMPOSITING_PENDING_EFFECT_LAYER_H_

#include <memory>
#include <optional>
#include <string>

#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/float_rounded_rect.h"
#include "clay/gfx/geometry/path.h"
#include "clay/gfx/style/color_filter.h"
#include "clay/gfx/style/color_source.h"
#include "clay/gfx/style/image_filter.h"
#include "clay/ui/compositing/frame_builder.h"
#include "clay/ui/compositing/pending_container_layer.h"

namespace clay {

enum class EffectType {
  kOpacity = 0,
  kColorFilter,
  kImageFilter,
  kBackdropFilter,
  kShaderMask,

  kLast = kShaderMask,
};

// A pending effect layer that applies an effect [Opacity, Filter ...] to its
// children.
class PendingEffectLayer : public PendingContainerLayer {
 public:
  PendingEffectLayer();
  ~PendingEffectLayer() override;

  std::string GetName() const override { return "PendingEffectLayer"; }

  void SetOffset(const FloatPoint& offset) {
    if (offset != offset_) {
      offset_ = offset;
      MarkNeedsAddToFrame();
    }
  }
  const FloatPoint& GetOffset() const { return offset_; }

  void SetOpacity(int opacity) {
    if (!opacity_.has_value() || *opacity_ != opacity) {
      opacity_ = opacity;
      MarkNeedsAddToFrame();
    }
  }
  int GetOpacity() const { return *opacity_; }

  void SetColorFilter(std::shared_ptr<ColorFilter> color_filter) {
    color_filter_ = color_filter;
    MarkNeedsAddToFrame();
  }

  void SetImageFilter(std::shared_ptr<ImageFilter> image_filter) {
    image_filter_ = image_filter;
    MarkNeedsAddToFrame();
  }

  void SetBackdropFilter(std::shared_ptr<ImageFilter> backdrop_filter) {
    backdrop_filter_ = backdrop_filter;
    MarkNeedsAddToFrame();
  }

  void SetClipRect(const FloatRect& clip_rect) {
    clip_rect_ = clip_rect;
    MarkNeedsAddToFrame();
  }

  void SetClipRRect(const FloatRoundedRect& clip_rrect) {
    clip_rrect_ = clip_rrect;
    MarkNeedsAddToFrame();
  }

  void SetClipPath(const GrPath& path) {
    clip_path_ = path;
    MarkNeedsAddToFrame();
  }

  void SetShaderMask(std::shared_ptr<ColorSource> color_source,
                     const FloatRect& mask_rect, BlendMode blend_mode) {
    color_source_ = color_source;
    blend_mode_ = blend_mode;
    mask_rect_ = mask_rect;
    MarkNeedsAddToFrame();
  }

#ifndef NDEBUG
  std::string ToString() const override;
#endif

 private:
  void AddToFrame(FrameBuilder* builder, const FloatPoint& offset) override;

  // Offset from parent in the parent's coordinate system.
  FloatPoint offset_;
  // For opacity effect.
  std::optional<int> opacity_ = std::nullopt;

  // For shader mask
  std::shared_ptr<ColorSource> color_source_;
  std::optional<FloatRect> mask_rect_;
  BlendMode blend_mode_ = BlendMode::kClear;

  std::shared_ptr<ColorFilter> color_filter_;
  std::shared_ptr<ImageFilter> image_filter_;
  std::shared_ptr<ImageFilter> backdrop_filter_;
  std::optional<FloatRect> clip_rect_ = std::nullopt;
  std::optional<FloatRoundedRect> clip_rrect_ = std::nullopt;
  std::optional<GrPath> clip_path_ = std::nullopt;
};

}  // namespace clay

#endif  // CLAY_UI_COMPOSITING_PENDING_EFFECT_LAYER_H_
