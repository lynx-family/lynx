// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/style/transform_raw_data.h"

namespace lynx {
namespace starlight {

TransformRawData::TransformRawData()
    : type(TransformType::kNone),
      p0(NLength::MakeUnitNLength(0.0f)),
      p1(NLength::MakeUnitNLength(0.0f)),
      p2(NLength::MakeUnitNLength(0.0f)) {}

void TransformRawData::Reset() {
  type = TransformType::kNone;
  p0 = NLength::MakeUnitNLength(0.0f);
  p1 = NLength::MakeUnitNLength(0.0f);
  p2 = NLength::MakeUnitNLength(0.0f);
}
}  // namespace starlight
}  // namespace lynx
