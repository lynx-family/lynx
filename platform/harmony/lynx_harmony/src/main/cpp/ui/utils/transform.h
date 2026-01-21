// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_TRANSFORM_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_TRANSFORM_H_

#include <node_api.h>

#include <cstddef>
#include <vector>

#include "base/include/value/base_value.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/style/transform/matrix44.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/platform_length.h"

namespace lynx {
namespace tasm {
namespace harmony {

struct TransformRaw {
  starlight::TransformType func_type = starlight::TransformType::kNone;
  PlatformLength params_[3];
};

struct TransformOrigin {
  PlatformLength x = {0.5, PlatformLengthType::kPercentage};
  PlatformLength y = {0.5, PlatformLengthType::kPercentage};
};

class Transform {
 public:
  constexpr static size_t kIndexTranslationZ = 14;
  explicit Transform(const lepus::Value& value);
  transforms::Matrix44 GetTransformMatrix(
      float width, float height, float scaled_density = 1.0f,
      bool with_transform_origin = false) const;
  void SetTransformOrigin(const TransformOrigin& transform_origin);

 private:
  std::vector<TransformRaw> raw_;
  TransformOrigin transform_origin_;
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_TRANSFORM_H_
