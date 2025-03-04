// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.background;

import android.graphics.Bitmap;
import android.graphics.ColorFilter;
import android.graphics.Path;
import android.graphics.PixelFormat;
import android.graphics.drawable.Drawable;
import androidx.annotation.Nullable;
import com.lynx.tasm.behavior.ui.LynxBaseUI;

/*
 * This is the base class for background-image, gradient(linear-gradient, radial-gradient)
 * and background-color. For each one, it needs to rewrite the following methods:
 * isReady, getWidth, getHeight, and draw.
 */
public abstract class BackgroundLayerDrawable extends Drawable {
  public abstract boolean isReady();
  public abstract int getImageWidth();
  public abstract int getImageHeight();
  public abstract void onAttach();
  public abstract void onDetach();
  public abstract void onSizeChanged(int width, int height);
  private Path mPath;

  // Default implementations for the Drawable abstract functions
  @Override
  public void setAlpha(int alpha) {}

  @Override
  public void setColorFilter(@Nullable ColorFilter colorFilter) {}

  @Override
  public int getOpacity() {
    return PixelFormat.TRANSPARENT;
  }

  public void setPathEffect(Path path) {
    mPath = path;
  }

  public Path getPathEffect() {
    return mPath;
  }

  public void setBitmapConfig(@Nullable Bitmap.Config config) {}

  public void setEnableBitmapGradient(boolean enable) {}

  public void setLynxUI(LynxBaseUI ui) {}
}
