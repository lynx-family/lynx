// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow;

public interface MeasureFunc {
  long measure(
      LayoutNode node, float width, MeasureMode widthMode, float height, MeasureMode heightMode);
}
