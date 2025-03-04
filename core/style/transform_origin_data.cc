// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/style/transform_origin_data.h"

namespace lynx {
namespace starlight {
TransformOriginData::TransformOriginData()
    : x(NLength::MakePercentageNLength(50.f)),
      y(NLength::MakePercentageNLength(50.f)) {}
void TransformOriginData::Reset() {
  x = NLength::MakePercentageNLength(50.f);
  y = NLength::MakePercentageNLength(50.f);
}

}  // namespace starlight
}  // namespace lynx
