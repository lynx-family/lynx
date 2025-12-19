// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.graphics.Rect;
import android.graphics.RectF;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class RoundedRectangleTest {
  @Before
  public void setUp() {}

  @After
  public void tearDown() {}

  @Test
  public void testNewRoundedRectangle() {
    RectF rectF = new RectF(1.1f, 1.1f, 2.2f, 2.2f);
    RoundedRectangle roundedRect = new RoundedRectangle(rectF, null);
    assertEquals(rectF, roundedRect.getRectF());
    assertNull(roundedRect.getBorderRadii());
    assertFalse(roundedRect.hasBorderRadius());
    assertEquals(new Rect(1, 1, 2, 2), roundedRect.getRect());

    float[] borders = new float[9];
    borders[0] = 1.0f;
    borders[1] = 1.0f;
    borders[2] = 1.0f;
    borders[3] = 1.0f;
    borders[4] = 1.0f;
    borders[5] = 1.0f;
    borders[6] = 1.0f;
    borders[7] = 1.0f;
    borders[8] = 1.0f;
    roundedRect = new RoundedRectangle(rectF, borders);
    assertFalse(roundedRect.hasBorderRadius());

    borders = new float[8];
    borders[0] = 1.0f;
    borders[1] = 1.0f;
    borders[2] = 1.0f;
    borders[3] = 1.0f;
    borders[4] = 1.0f;
    borders[5] = 1.0f;
    borders[6] = 1.0f;
    borders[7] = 1.0f;
    roundedRect = new RoundedRectangle(rectF, borders);
    assertTrue(roundedRect.hasBorderRadius());
  }
}
