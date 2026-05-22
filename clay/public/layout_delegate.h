// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_PUBLIC_LAYOUT_DELEGATE_H_
#define CLAY_PUBLIC_LAYOUT_DELEGATE_H_

#include <cstddef>
#include <cstdint>

#include "clay/public/style_types.h"

namespace clay {
class LayoutDelegate {
 public:
  virtual void OnTriggerLayout() = 0;
  virtual void OnMarkDirty(int32_t id) = 0;
  virtual void OnAlignNativeNode(int32_t id, float, float) = 0;
  virtual ClayMeasureOutput OnMeasureNativeNode(int32_t, float, int, float,
                                                int) = 0;
  virtual ClayLayoutStyles OnGetLayoutStyles(int32_t id) = 0;
};
}  // namespace clay

#endif  // CLAY_PUBLIC_LAYOUT_DELEGATE_H_
