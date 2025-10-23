// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_PAINT_H_
#define CLAY_GFX_PAINT_H_

#include <memory>

#include "clay/gfx/animation/picture_animation_type.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/gfx/style/color.h"
#include "clay/gfx/style/color_filter.h"
#include "clay/gfx/style/color_source.h"
#include "clay/gfx/style/image_filter.h"
#include "clay/gfx/style/mask_filter.h"
#include "clay/gfx/style/path_effect.h"

namespace clay {
enum class DrawStyle {
  kFill,           //!< fills interior of shapes
  kStroke,         //!< strokes boundary of shapes
  kStrokeAndFill,  //!< both strokes and fills shapes
#ifdef ENABLE_SKITY
  kStrokeThenFill_Style,  //!< set to stroke then fill geometry
#endif                    // ENABLE_SKITY

  kLastStyle = kStrokeAndFill,
  kDefaultStyle = kFill,
};

#ifndef ENABLE_SKITY
inline DrawStyle ToClay(SkPaint::Style style) {
  return static_cast<DrawStyle>(style);
}

inline SkPaint::Style ToSk(DrawStyle style) {
  return static_cast<SkPaint::Style>(style);
}
#else
inline DrawStyle ToClay(skity::Paint::Style style) {
  return static_cast<DrawStyle>(style);
}

inline skity::Paint::Style ToSk(DrawStyle style) {
  return static_cast<skity::Paint::Style>(style);
}
#endif  // ENABLE_SKITY

enum class StrokeCap {
  kButt,    //!< no stroke extension
  kRound,   //!< adds circle
  kSquare,  //!< adds square

  kLastCap = kSquare,
  kDefaultCap = kButt,
};

#ifndef ENABLE_SKITY
inline StrokeCap ToClay(SkPaint::Cap cap) {
  return static_cast<StrokeCap>(cap);
}

inline SkPaint::Cap ToSk(StrokeCap cap) {
  return static_cast<SkPaint::Cap>(cap);
}
#else
inline StrokeCap ToClay(skity::Paint::Cap cap) {
  return static_cast<StrokeCap>(cap);
}

inline skity::Paint::Cap ToSk(StrokeCap cap) {
  return static_cast<skity::Paint::Cap>(cap);
}
#endif  // ENABLE_SKITY

enum class StrokeJoin {
  kMiter,  //!< extends to miter limit
  kRound,  //!< adds circle
  kBevel,  //!< connects outside edges

  kLastJoin = kBevel,
  kDefaultJoin = kMiter,
};

#ifndef ENABLE_SKITY
inline StrokeJoin ToClay(SkPaint::Join join) {
  return static_cast<StrokeJoin>(join);
}

inline SkPaint::Join ToSk(StrokeJoin join) {
  return static_cast<SkPaint::Join>(join);
}
#else
inline StrokeJoin ToClay(skity::Paint::Join join) {
  return static_cast<StrokeJoin>(join);
}

inline skity::Paint::Join ToSk(StrokeJoin join) {
  return static_cast<skity::Paint::Join>(join);
}
#endif  // ENABLE_SKITY

class Paint {
 public:
  static constexpr Color kDefaultColor = Color::kBlack();
  static constexpr float kDefaultWidth = 0.0;
  static constexpr float kDefaultMiter = 4.0;

  Paint();

  bool isAntiAlias() const { return isAntiAlias_; }
  Paint& setAntiAlias(bool isAntiAlias) {
    isAntiAlias_ = isAntiAlias;
    return *this;
  }

  bool isDither() const { return isDither_; }
  Paint& setDither(bool isDither) {
    isDither_ = isDither;
    return *this;
  }

  bool isInvertColors() const { return isInvertColors_; }
  Paint& setInvertColors(bool isInvertColors) {
    isInvertColors_ = isInvertColors;
    return *this;
  }

  Color getColor() const { return color_; }
  Paint& setColor(Color color) {
    color_ = color;
    return *this;
  }

  uint8_t getAlpha() const { return color_.argb >> 24; }
  Paint& setAlpha(uint8_t alpha) {
    color_.argb = alpha << 24 | (color_.argb & 0x00FFFFFF);
    return *this;
  }
  Paint& setOpacity(float opacity) {
    setAlpha(static_cast<int>(opacity * 0xff));
    return *this;
  }

  BlendMode getBlendMode() const { return static_cast<BlendMode>(blendMode_); }
  Paint& setBlendMode(BlendMode mode) {
    blendMode_ = static_cast<unsigned>(mode);
    return *this;
  }

  DrawStyle getDrawStyle() const { return static_cast<DrawStyle>(drawStyle_); }
  Paint& setDrawStyle(DrawStyle style) {
    drawStyle_ = static_cast<unsigned>(style);
    return *this;
  }

  StrokeCap getStrokeCap() const { return static_cast<StrokeCap>(strokeCap_); }
  Paint& setStrokeCap(StrokeCap cap) {
    strokeCap_ = static_cast<unsigned>(cap);
    return *this;
  }

  StrokeJoin getStrokeJoin() const {
    return static_cast<StrokeJoin>(strokeJoin_);
  }
  Paint& setStrokeJoin(StrokeJoin join) {
    strokeJoin_ = static_cast<unsigned>(join);
    return *this;
  }

  float getStrokeWidth() const { return strokeWidth_; }
  Paint& setStrokeWidth(float width) {
    strokeWidth_ = width;
    return *this;
  }

  float getStrokeMiter() const { return strokeMiter_; }
  Paint& setStrokeMiter(float miter) {
    strokeMiter_ = miter;
    return *this;
  }

  std::shared_ptr<const ColorSource> getColorSource() const {
    return colorSource_;
  }
  const ColorSource* getColorSourcePtr() const { return colorSource_.get(); }
  Paint& setColorSource(std::shared_ptr<const ColorSource> source) {
    colorSource_ = source;
    return *this;
  }
  Paint& setColorSource(const ColorSource* source) {
    colorSource_ = source ? source->shared() : nullptr;
    return *this;
  }

  std::shared_ptr<const ColorFilter> getColorFilter() const {
    return colorFilter_;
  }
  const ColorFilter* getColorFilterPtr() const { return colorFilter_.get(); }
  Paint& setColorFilter(const std::shared_ptr<const ColorFilter> filter) {
    colorFilter_ = filter ? filter->shared() : nullptr;
    return *this;
  }
  Paint& setColorFilter(const ColorFilter* filter) {
    colorFilter_ = filter ? filter->shared() : nullptr;
    return *this;
  }

  std::shared_ptr<const ImageFilter> getImageFilter() const {
    return imageFilter_;
  }
  const ImageFilter* getImageFilterPtr() const { return imageFilter_.get(); }
  Paint& setImageFilter(const std::shared_ptr<const ImageFilter> filter) {
    imageFilter_ = filter;
    return *this;
  }
  Paint& setImageFilter(const ImageFilter* filter) {
    imageFilter_ = filter ? filter->shared() : nullptr;
    return *this;
  }

  std::shared_ptr<const MaskFilter> getMaskFilter() const {
    return maskFilter_;
  }
  const MaskFilter* getMaskFilterPtr() const { return maskFilter_.get(); }
  Paint& setMaskFilter(std::shared_ptr<MaskFilter> filter) {
    maskFilter_ = filter;
    return *this;
  }
  Paint& setMaskFilter(const MaskFilter* filter) {
    maskFilter_ = filter ? filter->shared() : nullptr;
    return *this;
  }

  std::shared_ptr<const PathEffect> getPathEffect() const {
    return pathEffect_;
  }
  const PathEffect* getPathEffectPtr() const { return pathEffect_.get(); }
  Paint& setPathEffect(std::shared_ptr<PathEffect> pathEffect) {
    pathEffect_ = pathEffect;
    return *this;
  }

  GrPaint gr_object() const {
    GrPaint paint;
    PAINT_SET_BLEND_MODE(paint, getBlendMode());
    PAINT_SET_STYLE(paint, getDrawStyle());
    PAINT_SET_STROKE_CAP(paint, ToSk(getStrokeCap()));
    PAINT_SET_STROKE_JOIN(paint, ToSk(getStrokeJoin()));
    PAINT_SET_ANTI_ALIAS(paint, isAntiAlias());
#ifndef ENABLE_SKITY
    // Skity paint not supports dither yet.
    paint.setDither(isDither());
#endif  // ENABLE_SKITY
    PAINT_SET_COLOR(paint, getColor());
    PAINT_SET_STROKE_WIDTH(paint, getStrokeWidth());
    PAINT_SET_STROKE_MITER(paint, getStrokeMiter());
    if (colorSource_) {
      PAINT_SET_SHADER(paint, colorSource_->gr_object());
    }
    if (colorFilter_) {
      PAINT_SET_COLOR_FILTER(paint, colorFilter_->gr_object());
    }
    if (imageFilter_) {
      PAINT_SET_IMAGE_FILTER(paint, imageFilter_->gr_object());
    }
    if (maskFilter_) {
      PAINT_SET_MASK_FILTER(paint, maskFilter_->gr_object());
    }
    if (pathEffect_) {
      PAINT_SET_PATH_EFFECT(paint, pathEffect_->gr_object());
    }
    return paint;
  }

  bool operator==(Paint const& other) const;
  bool operator!=(Paint const& other) const { return !(*this == other); }

  void setDynamicOpType(DynamicOpType type) { dynamic_op_type_ = type; }

  const DynamicOpType& getDynamicOpType() const { return dynamic_op_type_; }

 private:
#define ASSERT_ENUM_FITS(last_enum, num_bits)                    \
  static_assert(static_cast<int>(last_enum) < (1 << num_bits) && \
                static_cast<int>(last_enum) * 2 >= (1 << num_bits))

  static constexpr int kBlendModeBits = 5;
  static constexpr int kDrawStyleBits = 2;
  static constexpr int kStrokeCapBits = 2;
  static constexpr int kStrokeJoinBits = 2;
  ASSERT_ENUM_FITS(BlendMode::kLastMode, kBlendModeBits);
  ASSERT_ENUM_FITS(DrawStyle::kLastStyle, kDrawStyleBits);
  ASSERT_ENUM_FITS(StrokeCap::kLastCap, kStrokeCapBits);
  ASSERT_ENUM_FITS(StrokeJoin::kLastJoin, kStrokeJoinBits);

  union {
    struct {
      unsigned blendMode_ : kBlendModeBits;
      unsigned drawStyle_ : kDrawStyleBits;
      unsigned strokeCap_ : kStrokeCapBits;
      unsigned strokeJoin_ : kStrokeJoinBits;
      unsigned isAntiAlias_ : 1;
      unsigned isDither_ : 1;
      unsigned isInvertColors_ : 1;
    };
  };

  Color color_;
  float strokeWidth_;
  float strokeMiter_;
  DynamicOpType dynamic_op_type_ = DynamicOpType::kNone;

  std::shared_ptr<const ColorSource> colorSource_;
  std::shared_ptr<const ColorFilter> colorFilter_;
  std::shared_ptr<const ImageFilter> imageFilter_;
  std::shared_ptr<const MaskFilter> maskFilter_;
  std::shared_ptr<const PathEffect> pathEffect_;
  // missing (as compared to SkPaint):
  // DlBlender - not planning on using that object in a pure DisplayList world
};

}  // namespace clay

#endif  // CLAY_GFX_PAINT_H_
