// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.image.model;

import android.graphics.drawable.Drawable;

public interface AnimationListener {
  /**
   * Called when the animation is started for the given drawable.
   *
   * @param drawable the affected drawable
   */
  void onAnimationStart(Drawable drawable);

  void onAnimationFinalLoop(Drawable drawable);

  void onAnimationCurrentLoop(Drawable drawable);
}
