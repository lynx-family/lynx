// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_BASIC_SHAPE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_BASIC_SHAPE_H_

#include <string>

#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {
namespace harmony {
class BasicShape {
 public:
  explicit BasicShape(const lepus::Value& value, float density);
  void ParsePathWithParentSize(float view_width, float view_height);
  const std::string& PathString();

 private:
  std::string path_string_;
  float density_{1.f};
  fml::RefPtr<lepus::CArray> shape_data_{nullptr};
  starlight::BasicShapeType basic_shape_type_{
      starlight::BasicShapeType::kUnknown};
  const std::string ScaleSvgPath(const std::string& path);
  const std::string CircleToSvgPath(float radius, float cx, float cy);
  const std::string EllipseToSvgPath(float rx, float ry, float cx, float cy);
  const std::string SuperEllipseToSvgPath(float rx, float ry, float cx,
                                          float cy, float exponents_x,
                                          float exponents_y);
  void AddLameCurveToPath(std::ostringstream& oss, float rx, float ry, float cx,
                          float cy, float exponents_x, float exponents_y,
                          int32_t quadrant);
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_BASIC_SHAPE_H_
