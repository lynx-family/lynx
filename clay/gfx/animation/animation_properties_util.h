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

constexpr inline AnimationPropertyUnderlyingType MakeValid(
    AnimationPropertyUnderlyingType value) {
  return value & ToUnderlying(ClayAnimationPropertyType::kAll);
}

constexpr inline ClayAnimationPropertyType ToAnimationProperty(
    AnimationPropertyUnderlyingType value) {
  return static_cast<ClayAnimationPropertyType>(MakeValid(value));
}

constexpr inline bool AnimationPropertyTest(ClayAnimationPropertyType value,
                                            ClayAnimationPropertyType to_test) {
  return ToUnderlying(value) & ToUnderlying(to_test);
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
