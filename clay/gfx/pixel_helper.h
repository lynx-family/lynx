// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_PIXEL_HELPER_H_
#define CLAY_GFX_PIXEL_HELPER_H_

#include "build/build_config.h"

namespace clay {

// NOTE: Here we use enum instead of enum class for convenience.
enum PixelType {
  kPixelTypeLogical,
  kPixelTypePhysical,
};

// The pixel type used on the framework side (i.e. Lynx).
// Currently we use physical pixels on Android and logical pixels on others.
#if defined(OS_ANDROID)
static constexpr PixelType kPixelTypeFramework = kPixelTypePhysical;
#else
static constexpr PixelType kPixelTypeFramework = kPixelTypeLogical;
#endif

// The pixel type used on the Clay side, such as view bounds, touch positions.
// For now we use the same pixel type with the framework side.
static constexpr PixelType kPixelTypeClay = kPixelTypeFramework;

#if defined(OS_ANDROID)
static constexpr PixelType kPixelTypePlatform = kPixelTypePhysical;
#else
static constexpr PixelType kPixelTypePlatform = kPixelTypeLogical;
#endif

// A helper class to convert pixel values between different pixel types.
template <PixelType Current>
class PixelHelper {
 public:
  virtual ~PixelHelper() = default;
  virtual float DevicePixelRatio() const = 0;

  // Get the pixel ratio from one pixel type to another. For example
  // `GetPixelRatio<kPixelTypeLogical, kPixelTypePhysical>` returns the real
  // device pixel ratio.
  template <PixelType From, PixelType To>
  constexpr float GetPixelRatio() const {
    if constexpr (From == To) {
      return 1;
    } else if constexpr (From == kPixelTypeLogical) {
      static_assert(To == kPixelTypePhysical);
      return DevicePixelRatio();
    } else {
      static_assert(From == kPixelTypePhysical && To == kPixelTypeLogical);
      return 1 / DevicePixelRatio();
    }
  }

  // Convert the given value from one pixel type to another.
  template <PixelType From, PixelType To, typename T>
  constexpr T Convert(T value) const {
    return value * GetPixelRatio<From, To>();
  }

  // Convert multiple values from one pixel type to another.
  template <PixelType From, PixelType To, typename... Args>
  void ConvertValues(Args&... value) const {
    ((value = Convert<From, To>(value)), ...);
  }

  // Convert the given value from the clay pixel type to the given pixel type.
  template <PixelType To, typename T>
  constexpr T ConvertTo(T value) const {
    return Convert<Current, To>(value);
  }

  // Convert the given value from the given pixel type to the clay pixel type.
  template <PixelType From, typename T>
  constexpr T ConvertFrom(T value) const {
    return Convert<From, Current>(value);
  }

  // Round the given value by physical pixels regardless of the input pixel
  // type. For example, if we have a logical pixel value of 5.3 and the dpr is
  // 2, then we will get a result value of 5.5.
  template <PixelType Type = Current>
  float RoundPixels(float value) const {
    return Convert<kPixelTypePhysical, Type>(
        roundf(Convert<Type, kPixelTypePhysical>(value)));
  }
};

}  // namespace clay

#endif  // CLAY_GFX_PIXEL_HELPER_H_
