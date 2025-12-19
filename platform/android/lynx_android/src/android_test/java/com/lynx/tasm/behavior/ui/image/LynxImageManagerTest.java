// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.image;

import static org.junit.Assert.*;

import android.graphics.Path;
import android.graphics.Rect;
import android.graphics.RectF;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.render.RoundedRectangle;
import com.lynx.tasm.behavior.ui.utils.BackgroundDrawable;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class LynxImageManagerTest {
  private LynxContext mContext;

  @Before
  public void setUp() {
    mContext = TestingUtils.getLynxContext();
  }

  @After
  public void tearDown() {}

  private BackgroundDrawable.RoundRectPath getRoundRectPath(LynxImageManager manager) {
    BackgroundDrawable.RoundRectPath path = null;
    try {
      Field field = LynxImageManager.class.getDeclaredField("mInnerClipPathForBorderRadius");
      field.setAccessible(true);
      path = (BackgroundDrawable.RoundRectPath) field.get(manager);
    } catch (NoSuchFieldException | IllegalAccessException e) {
      fail(e.toString());
    }
    return path;
  }

  private Path getPath(BackgroundDrawable.RoundRectPath roundRectPath) {
    Path path = null;
    try {
      Field field = BackgroundDrawable.RoundRectPath.class.getDeclaredField("path");
      field.setAccessible(true);
      path = (Path) field.get(roundRectPath);
    } catch (NoSuchFieldException | IllegalAccessException e) {
      fail(e.toString());
    }
    return path;
  }

  private RectF convertToRectF(Rect rect) {
    return new RectF(rect.left, rect.top, rect.right, rect.bottom);
  }

  private boolean isPathEqual(Path p1, Path p2) {
    if (p1 == p2)
      return true;

    Path result = new Path();
    // Perform XOR operation between p1 and p2, store the result in 'result'
    // If the two shapes are identical, the XOR result should be empty
    boolean success = result.op(p1, p2, Path.Op.XOR);

    if (success) {
      return result.isEmpty();
    }
    return false;
  }

  @Test
  public void updateInnerClipPathForBorderRadius() {
    RectF rectF = new RectF(1.1f, 1.1f, 10.2f, 10.2f);
    float[] borders = new float[8];
    borders[0] = 1.0f;
    borders[1] = 1.0f;
    borders[2] = 1.0f;
    borders[3] = 1.0f;
    borders[4] = 1.0f;
    borders[5] = 1.0f;
    borders[6] = 1.0f;
    borders[7] = 1.0f;
    RoundedRectangle roundedRect = new RoundedRectangle(rectF, borders);

    LynxImageManager manager = new LynxImageManager(mContext);
    manager.updateInnerClipPathForBorderRadius(roundedRect);
    BackgroundDrawable.RoundRectPath roundRectPath = getRoundRectPath(manager);
    assertNotNull(roundRectPath);

    Path path = getPath(roundRectPath);
    Path expectPath = new Path();
    expectPath.addRoundRect(
        convertToRectF(roundedRect.getRect()), roundedRect.getBorderRadii(), Path.Direction.CW);
    assertTrue(isPathEqual(path, expectPath));

    manager.updateInnerClipPathForBorderRadius(null);
    roundRectPath = getRoundRectPath(manager);
    assertNull(roundRectPath);
  }
}
