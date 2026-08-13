// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior;

import static org.junit.Assert.assertEquals;

import android.graphics.Rect;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxUIRendererPixelCopyTest {
  @Test
  public void popupSurfaceInsetsDoNotClipRightEdge() {
    int[] locationInSurface = {36, 24};
    int[] rootLocationInSurface = {36, 24};

    Rect result = LynxUIRenderer.calculatePixelCopyRect(
        locationInSurface, 1080, 1200, rootLocationInSurface, 1080, 1200);

    assertEquals(new Rect(36, 24, 1116, 1224), result);
    assertEquals(1080, result.width());
  }

  @Test
  public void viewOutsideRootIsClippedAndKeepsSurfaceCoordinates() {
    int[] locationInSurface = {-20, -30};
    int[] rootLocationInSurface = {0, 0};

    Rect result = LynxUIRenderer.calculatePixelCopyRect(
        locationInSurface, 1120, 1300, rootLocationInSurface, 1080, 1200);

    assertEquals(new Rect(0, 0, 1080, 1200), result);
  }

  @Test
  public void viewOutsideRootReturnsEmptyRect() {
    int[] locationInSurface = {1200, 0};
    int[] rootLocationInSurface = {0, 0};

    Rect result = LynxUIRenderer.calculatePixelCopyRect(
        locationInSurface, 100, 100, rootLocationInSurface, 1080, 1200);

    assertEquals(new Rect(), result);
  }
}
