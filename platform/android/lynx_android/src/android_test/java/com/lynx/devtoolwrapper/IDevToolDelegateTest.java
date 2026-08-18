// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtoolwrapper;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;

import android.graphics.Bitmap;
import com.lynx.react.bridge.ReadableMap;
import org.junit.Test;

public class IDevToolDelegateTest {
  private final IDevToolDelegate mDelegate = new DefaultDevToolDelegate();

  @Test
  public void isClayRendererDefaultsToFalse() {
    assertFalse(mDelegate.isClayRenderer());
  }

  @Test
  public void getLynxUITreeDefaultsToEmptyString() {
    assertEquals("", mDelegate.getLynxUITree());
  }

  @Test
  public void getUINodeInfoDefaultsToEmptyString() {
    assertEquals("", mDelegate.getUINodeInfo(14));
  }

  @Test
  public void setUIStyleDefaultsToFailure() {
    assertEquals(-1, mDelegate.setUIStyle(14, "opacity", "0.5"));
  }

  // Implement only the required methods so the tests exercise the interface's real defaults.
  private static final class DefaultDevToolDelegate implements IDevToolDelegate {
    @Override
    public void onDispatchMessageEvent(ReadableMap map) {}

    @Override
    public void takeScreenshot(ScreenshotBitmapHandler handler, String screenShotMode) {}

    @Override
    public void scrollIntoViewFromUI(int nodeId) {}

    @Override
    public String getActualScreenshotMode() {
      return "";
    }

    @Override
    public int getNodeForLocation(float x, float y, String mode) {
      return -1;
    }

    @Override
    public float[] getTransformValue(int id, float[] padBorderMarginLayout) {
      return padBorderMarginLayout;
    }

    @Override
    public Bitmap getBitmapOfView() {
      return null;
    }
  }
}
