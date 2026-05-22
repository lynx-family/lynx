// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GEOMETRY_BOX_SHADOW_OPERATIONS_H_
#define CLAY_GFX_GEOMETRY_BOX_SHADOW_OPERATIONS_H_

#include <vector>

#include "clay/gfx/geometry/box_shadow_value.h"
#include "clay/gfx/style/shadow.h"
#include "clay/public/clay.h"
#include "clay/public/style_types.h"

namespace clay {

struct BoxShadowOperation {
  BoxShadowOperation() = default;

  explicit BoxShadowOperation(const BoxShadowValue& value);

  static bool BlendOperations(const BoxShadowOperation* from,
                              const BoxShadowOperation* to, float progress,
                              BoxShadowOperation* result);

  Shadow shadow;
};

class BoxShadowOperations {
 public:
  BoxShadowOperations() = default;
  ~BoxShadowOperations() = default;

  explicit BoxShadowOperations(const std::vector<BoxShadowValue>& values);

  BoxShadowOperations Blend(const BoxShadowOperations& other,
                            float fraction) const;

  bool BlendInternal(const BoxShadowOperations& from, float progress,
                     BoxShadowOperations* result) const;

  size_t MatchingPrefixLength(const BoxShadowOperations& other) const;

  void Append(BoxShadowOperation ops);

  std::vector<Shadow> apply() const {
    std::vector<Shadow> result;
    for (auto& o : operations_) {
      result.emplace_back(o.shadow);
    }
    return result;
  }

 private:
  std::vector<BoxShadowOperation> operations_;
};

}  // namespace clay

#endif  // CLAY_GFX_GEOMETRY_BOX_SHADOW_OPERATIONS_H_
