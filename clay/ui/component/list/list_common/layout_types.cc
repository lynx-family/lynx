// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/list_common/layout_types.h"

namespace clay {
namespace {
constexpr char kAlignNone[] = "none";
constexpr char kAlignTop[] = "top";
constexpr char kAlignBottom[] = "bottom";
constexpr char kAlignMiddle[] = "middle";
}  // namespace

AlignTo StringToAlign(const std::string& str) {
  if (str == kAlignNone) {
    return AlignTo::kNone;
  } else if (str == kAlignTop) {
    return AlignTo::kStart;
  } else if (str == kAlignMiddle) {
    return AlignTo::kMiddle;
  } else if (str == kAlignBottom) {
    return AlignTo::kEnd;
  } else {
    return AlignTo::kNone;
  }
}

}  // namespace clay
