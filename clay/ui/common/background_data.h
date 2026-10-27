// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMMON_BACKGROUND_DATA_H_
#define CLAY_UI_COMMON_BACKGROUND_DATA_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/include/fml/memory/ref_ptr.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/float_size.h"
#include "clay/gfx/graphics_context.h"
#include "clay/gfx/image/image_resource.h"
#include "clay/gfx/style/color.h"
#include "clay/public/clay.h"
#include "clay/public/style_types.h"
#include "clay/ui/painter/gradient.h"

namespace clay {

class BackgroundImage {
 public:
  BackgroundImage() = default;
  BackgroundImage(const BackgroundImage& background_image);
  ~BackgroundImage() = default;

#ifndef ENABLE_SKITY
  void SetImageResource(std::unique_ptr<ImageResource> image_resource);
  const ImageResource* GetImageResource() const {
    return image_resource_.get();
  }
#else
  void SetImageResource(std::shared_ptr<BaseImage> image_resource);
  const BaseImage* GetImageResource() const { return image_resource_.get(); }
#endif  // ENABLE_SKITY

  void SetGradient(const Gradient& gradient);
  const Gradient& GetGradient() const { return *gradient_; }

  bool IsEmpty() const;
  bool IsSkImage() const;
  int Width() const;
  int Height() const;

 private:
  // The background of a box can have multiple layers in CSS3. Each layer cannot
  // have image and gradient at the same time.
  // https://www.w3.org/TR/css-backgrounds-3/#layering
#ifndef ENABLE_SKITY
  std::unique_ptr<ImageResource> image_resource_;
#else
  std::shared_ptr<BaseImage> image_resource_;
#endif
  std::optional<Gradient> gradient_ = std::nullopt;
};

struct BackgroundImageData {
  ClayBackgroundImageType type = ClayBackgroundImageType::kNone;
  // only for ClayBackgroundImageType::kUrl.
  std::string src_str;
  // only for ClayBackgroundImageType::kLinearGradient.
  // non-empty |gradient_str| overrides |gradient_data|.
  std::string gradient_str;
  ClayLinearGradient gradient_data = {};

  // BackgroundImage image;
  ClayBackgroundSizeType size = ClayBackgroundSizeType::kContain;
  // TODO(heke): Align position to RKBackgroundPositionType.
  // Background-position, relative to ClayBackgroundOriginType.
  FloatPoint position;
  ClayBackgroundOriginType origin = ClayBackgroundOriginType::kBorderBox;
  ClayBackgroundRepeatType repeat = ClayBackgroundRepeatType::kRepeat;
  ClayBackgroundClipType clip = ClayBackgroundClipType::kBorderBox;
};

class BackgroundPosition {
 public:
  BackgroundPosition() = default;
  BackgroundPosition(float value, ClayPlatformLengthUnit type)
      : value_(value), type_(type) {}
  float apply(float parent_value) const {
    if (type_ == ClayPlatformLengthUnit::kPercentage) {
      return parent_value * value_;
    } else {
      return value_;
    }
  }

 private:
  float value_ = 0.f;
  ClayPlatformLengthUnit type_ = ClayPlatformLengthUnit::kNumber;
};

class BackgroundSize {
 public:
  BackgroundSize() = default;
  BackgroundSize(float value, int type)
      : value_(value), type_(static_cast<ClayPlatformLengthUnit>(type)) {}
  BackgroundSize(float value, ClayPlatformLengthUnit type)
      : value_(value), type_(type) {}
  bool IsCover() const {
    return static_cast<ClayBackgroundSizeType>(-value_) ==
           ClayBackgroundSizeType::kCover;
  }
  bool IsContain() const {
    return static_cast<ClayBackgroundSizeType>(-value_) ==
           ClayBackgroundSizeType::kContain;
  }
  bool IsAuto() const {
    return static_cast<ClayBackgroundSizeType>(-value_) ==
           ClayBackgroundSizeType::kAuto;
  }
  float apply(float parent_value, float current_value) const {
    if (type_ == ClayPlatformLengthUnit::kPercentage) {
      return value_ * parent_value;
    } else if (IsAuto()) {
      return current_value;
    } else {
      return value_;
    }
  }

 private:
  float value_ = -static_cast<float>(ClayBackgroundSizeType::kAuto);
  ClayPlatformLengthUnit type_ = ClayPlatformLengthUnit::kNumber;
};

struct BackgroundData {
  Color background_color = Color::RGBOColor(0, 0, 0, 0);  // The compatible code
  std::vector<BackgroundImageData> background_images;     // The compatible code
  std::vector<BackgroundImage> images;
  std::vector<ClayBackgroundOriginType> origins;
  std::vector<ClayBackgroundRepeatType> repeats;
  std::vector<ClayBackgroundClipType> clips;
  std::vector<BackgroundSize> sizes;
  std::vector<BackgroundPosition> positions;
};

// Mask
using MaskImage = BackgroundImage;
using MaskPosition = BackgroundPosition;
using MaskSize = BackgroundSize;

struct MaskData {
  std::vector<MaskImage> images;
  std::vector<ClayMaskOriginType> origins;
  std::vector<ClayMaskRepeatType> repeats;
  std::vector<ClayMaskClipType> clips;
  std::vector<MaskSize> sizes;
  std::vector<MaskPosition> positions;
};

};  // namespace clay

#endif  // CLAY_UI_COMMON_BACKGROUND_DATA_H_
