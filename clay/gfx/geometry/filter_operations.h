// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GEOMETRY_FILTER_OPERATIONS_H_
#define CLAY_GFX_GEOMETRY_FILTER_OPERATIONS_H_

#include <array>
#include <vector>

#include "clay/gfx/geometry/filter_value.h"
#include "clay/public/clay.h"
#include "clay/public/style_types.h"

namespace clay {

struct FilterOperation {
  enum Type {
    kTypeNone = 0,
    kTypeGrayscale = 1,
    kTypeBlur = 2,
    kTypeBrightness = 3,
    kTypeContrast = 4,
    kTypeSaturate = 5,
    kTypeHueRotate = 6,
    kTypeMatrix = 7,
    kTypeIdentity = 8,
  };

  FilterOperation() = default;

  FilterOperation(Type type, float value);

  Type type = kTypeIdentity;

  // Is similar to Skia's SkColorMatrix.
  std::array<float, 20> color_matrix = {0.f};

  bool IsIdentity() const { return type == kTypeIdentity; }

  float value;

  void Bake();

  void MakeDefault(Type type);

  static bool BlendOperations(const FilterOperation* from,
                              const FilterOperation* to, float progress,
                              FilterOperation* result);
};

class FilterOperations {
 public:
  FilterOperations() = default;
  ~FilterOperations() = default;

  explicit FilterOperations(const std::vector<FilterValue>& values);

  std::array<float, 20> Apply() const;

  void Grayscale(float value);
  void Contrast(float value);
  void Brightness(float value);
  void Saturate(float value);
  void HueRotate(float value);

  FilterOperations Blend(const FilterOperations& other, float fraction) const;

  bool BlendInternal(const FilterOperations& from, float progress,
                     FilterOperations* result) const;

  size_t MatchingPrefixLength(const FilterOperations& other) const;

  void Append(const FilterOperations& ops);

  void Append(const FilterOperation& operation);

  float GetBlurRadius() const { return blur_radius_; }

 private:
  std::vector<FilterOperation> operations_;
  float blur_radius_ = 0.f;
};

}  // namespace clay

#endif  // CLAY_GFX_GEOMETRY_FILTER_OPERATIONS_H_
