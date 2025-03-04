// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.utils;

import static org.junit.Assert.*;

import android.content.Context;
import android.util.DisplayMetrics;
import androidx.test.platform.app.InstrumentationRegistry;
import org.junit.Before;
import org.junit.Test;

public class PixelUtilsTest {
  @Before
  public void setUp() {
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    DisplayMetricsHolder.updateOrInitDisplayMetrics(context, 3.0f);
    DisplayMetricsHolder.updateDisplayMetrics(1080, 1920);
    DisplayMetrics dm = DisplayMetricsHolder.getScreenDisplayMetrics();
    if (dm != null) {
      dm.scaledDensity = 3;
    }
  }

  @Test
  public void dipToPx() {
    assertEquals(300, PixelUtils.dipToPx(100f), 0);
    assertEquals(300, PixelUtils.dipToPx(100d), 0);
  }

  @Test
  public void pxToDip() {
    assertEquals(100, PixelUtils.pxToDip(300f), 0);
  }
}
