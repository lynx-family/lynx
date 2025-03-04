// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_STYLE_LINEAR_DATA_H_
#define CORE_RENDERER_STARLIGHT_STYLE_LINEAR_DATA_H_

#include "base/include/fml/memory/ref_counted.h"
#include "core/renderer/starlight/style/css_type.h"

namespace lynx {
namespace starlight {

class LinearData : public fml::RefCountedThreadSafeStorage {
 public:
  void ReleaseSelf() const override { delete this; }
  static fml::RefPtr<LinearData> Create() {
    return fml::AdoptRef(new LinearData());
  }
  fml::RefPtr<LinearData> Copy() const {
    return fml::AdoptRef(new LinearData(*this));
  }

 public:
  LinearData();
  LinearData(const LinearData& data);
  ~LinearData() = default;
  void Reset();
  float linear_weight_sum_;
  float linear_weight_;
  LinearOrientationType linear_orientation_;
  LinearLayoutGravityType linear_layout_gravity_;
  LinearGravityType linear_gravity_;
  LinearCrossGravityType linear_cross_gravity_;
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_STYLE_LINEAR_DATA_H_
