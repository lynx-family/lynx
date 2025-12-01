// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import android.graphics.PointF;

public interface IPaintingContext {
  // this func will be execed on main thread.
  void destroy();

  long getNativePaintingContextPtr();

  PointF convertPointInViewToScreen(int sign, PointF point);

  int getTargetWidth(int sign);

  int getTargetHeight(int sign);
}
