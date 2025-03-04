// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.background;

import android.graphics.Canvas;
import androidx.annotation.NonNull;

public class BackgroundNoneLayer extends BackgroundLayerDrawable {
  public BackgroundNoneLayer() {}

  @Override
  public boolean isReady() {
    return false;
  }

  @Override
  public int getImageWidth() {
    return 0;
  }

  @Override
  public int getImageHeight() {
    return 0;
  }

  @Override
  public void onAttach() {}

  @Override
  public void onDetach() {}

  @Override
  public void onSizeChanged(int width, int height) {}

  @Override
  public void draw(@NonNull Canvas canvas) {
    // nothing to do here
  }
}
