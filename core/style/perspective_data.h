// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_STYLE_PERSPECTIVE_DATA_H_
#define CORE_STYLE_PERSPECTIVE_DATA_H_

#include "core/renderer/css/css_value.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/starlight/types/nlength.h"

namespace lynx {
namespace starlight {
struct PerspectiveData {
  NLength length_;
  mutable tasm::CSSValuePattern pattern_;
  PerspectiveData();
  ~PerspectiveData() = default;

  void Reset();

  bool operator==(const PerspectiveData& rhs) const {
    return length_ == rhs.length_ && pattern_ == rhs.pattern_;
  }
};

}  // namespace starlight
}  // namespace lynx
#endif  // CORE_STYLE_PERSPECTIVE_DATA_H_
