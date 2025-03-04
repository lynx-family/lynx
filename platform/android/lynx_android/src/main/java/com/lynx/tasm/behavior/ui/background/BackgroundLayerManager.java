// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.background;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Path;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.utils.BackgroundDrawable;
import com.lynx.tasm.utils.PixelUtils;
import java.util.ArrayList;
import java.util.List;

public class BackgroundLayerManager extends LayerManager {
  public BackgroundLayerManager(LynxContext context, Drawable drawable, float curFontSize) {
    super(context, drawable, curFontSize);
  }

  @Override
  protected boolean isMask() {
    return false;
  }

  public boolean hasBackgroundLayers() {
    return hasImageLayers();
  }
}
