// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/style/perspective_data.h"

namespace lynx {
namespace starlight {
PerspectiveData::PerspectiveData()
    : length_(NLength::MakeAutoNLength()),
      pattern_(tasm::CSSValuePattern::EMPTY) {}
void PerspectiveData::Reset() {
  length_ = NLength::MakeAutoNLength();
  pattern_ = tasm::CSSValuePattern::EMPTY;
}

}  // namespace starlight
}  // namespace lynx
