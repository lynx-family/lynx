// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PAINTER_GRADIENT_FACTORY_H_
#define CLAY_UI_PAINTER_GRADIENT_FACTORY_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/style/color_source.h"
#include "clay/ui/painter/gradient.h"
#include "third_party/googletest/googletest/include/gtest/gtest_prod.h"  // nogncheck

namespace clay {

class GradientFactory {
 public:
  static std::optional<Gradient> CreateLinear(
      const ClayLinearGradient& gradient_data);
  static std::optional<Gradient> CreateRadial(
      const ClayRadialGradient& gradient_data);
  static std::optional<Gradient> CreateConic(
      const ClayConicGradient& gradient_data);
  static std::optional<Gradient> Create(std::string raw_data);
  static std::shared_ptr<ColorSource> CreateShader(const Gradient& gradient,
                                                   const FloatRect& rect);

 private:
  FRIEND_TEST(GradientTest, DirectionToPoints);

  static std::vector<std::string_view> ParseArgs(const std::string_view& args);

  enum class ParseResult {
    kNotFound = 0,
    kSucceeded,
    kFailed,
  };

  static std::shared_ptr<ColorSource> CreateLinearShader(
      const Gradient& gradient, const FloatRect& rect);

  static std::shared_ptr<ColorSource> CreateRadialShader(
      const Gradient& gradient, const FloatRect& rect);

  static std::shared_ptr<ColorSource> CreateConicShader(
      const Gradient& gradient, const FloatRect& rect);

  static void ParseRadialCenter(const Gradient& gradient, const FloatRect& rect,
                                FloatPoint& at);

  static void ParseRadialRadiusAndMatrix(const Gradient& gradient,
                                         double& radius, const float& width,
                                         const float& height,
                                         const FloatPoint& at,
                                         skity::Matrix& matrix);

  static void ParseConicCenter(const Gradient& gradient, const FloatRect& rect,
                               FloatPoint& center);

  static ParseResult ParseDirection(const std::string_view& arg,
                                    Gradient& gradient);

  static ParseResult ParseColorPositions(
      const std::vector<std::string_view>& args, size_t start_idx,
      Gradient& gradient);

  static ParseResult ParseLinearGradientStartPoints(const Gradient& gradient,
                                                    const FloatRect& rect,
                                                    skity::Vec2* pts);
};

}  // namespace clay

#endif  // CLAY_UI_PAINTER_GRADIENT_FACTORY_H_
