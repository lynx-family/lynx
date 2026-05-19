// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_ANIMATION_PROPERTIES_UTIL_H_
#define CLAY_GFX_ANIMATION_ANIMATION_PROPERTIES_UTIL_H_

#include <type_traits>

#include "clay/public/clay.h"
#include "clay/public/style_types.h"

namespace clay {

using AnimationPropertyUnderlyingType =
    std::underlying_type<ClayAnimationPropertyType>::type;

constexpr inline AnimationPropertyUnderlyingType ToUnderlying(
    ClayAnimationPropertyType value) {
  return static_cast<AnimationPropertyUnderlyingType>(value);
}

constexpr inline AnimationPropertyUnderlyingType
BitmaskAnimationPropertyMask() {
  return ToUnderlying(ClayAnimationPropertyType::kAll) - 1;
}

constexpr inline AnimationPropertyUnderlyingType MakeValid(
    AnimationPropertyUnderlyingType value) {
  if (value == ToUnderlying(ClayAnimationPropertyType::kAll)) {
    return value;
  }
  return value & BitmaskAnimationPropertyMask();
}

constexpr inline ClayAnimationPropertyType ToAnimationProperty(
    AnimationPropertyUnderlyingType value) {
  return static_cast<ClayAnimationPropertyType>(MakeValid(value));
}

constexpr inline bool IsSingleBitAnimationPropertyValue(
    AnimationPropertyUnderlyingType value) {
  return value != 0 && (value & (value - 1)) == 0 &&
         value < ToUnderlying(ClayAnimationPropertyType::kAll);
}

constexpr inline bool IsSequentialAnimationProperty(
    ClayAnimationPropertyType value) {
  const AnimationPropertyUnderlyingType raw_value = ToUnderlying(value);
  return raw_value >= ToUnderlying(ClayAnimationPropertyType::kPaddingLeft) &&
         raw_value < ToUnderlying(ClayAnimationPropertyType::kAll);
}

constexpr inline bool AnimationPropertyTest(ClayAnimationPropertyType value,
                                            ClayAnimationPropertyType to_test) {
  if (value == ClayAnimationPropertyType::kAll) {
    return to_test != ClayAnimationPropertyType::kNone &&
           to_test != ClayAnimationPropertyType::kAll;  // kAll is not a real
                                                        // animation property.
  }
  const AnimationPropertyUnderlyingType raw_value = ToUnderlying(value);
  const AnimationPropertyUnderlyingType raw_to_test = ToUnderlying(to_test);
  if (IsSequentialAnimationProperty(value) ||
      !IsSingleBitAnimationPropertyValue(raw_to_test)) {
    return value == to_test;
  }
  return raw_value & raw_to_test;
}

constexpr inline void AnimationPropertySet(ClayAnimationPropertyType& value,
                                           ClayAnimationPropertyType to_set) {
  value = ToAnimationProperty(ToUnderlying(value) | ToUnderlying(to_set));
}

constexpr inline void AnimationPropertyUnset(
    ClayAnimationPropertyType& value, ClayAnimationPropertyType to_unset) {
  value = ToAnimationProperty(ToUnderlying(value) & ~ToUnderlying(to_unset));
}

constexpr inline void AnimationPropertySetIf(ClayAnimationPropertyType& value,
                                             ClayAnimationPropertyType prop,
                                             bool set) {
  (set ? AnimationPropertySet : AnimationPropertyUnset)(value, prop);
}

constexpr inline bool IsRasterAnimationProperty(
    ClayAnimationPropertyType type) {
  return type == ClayAnimationPropertyType::kOpacity ||
         type == ClayAnimationPropertyType::kTransform ||
         type == ClayAnimationPropertyType::kBackgroundColor ||
         type == ClayAnimationPropertyType::kColor;
}

template <typename Func, typename = std::enable_if_t<std::is_invocable_v<
                             Func, ClayAnimationPropertyType>>>
constexpr inline void ForEachRasterAnimationProperty(Func&& func) {
  func(ClayAnimationPropertyType::kOpacity);
  func(ClayAnimationPropertyType::kTransform);
  func(ClayAnimationPropertyType::kBackgroundColor);
  func(ClayAnimationPropertyType::kColor);
}

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_ANIMATION_PROPERTIES_UTIL_H_
