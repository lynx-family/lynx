// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.utils;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

import android.content.Context;
import android.util.DisplayMetrics;
import androidx.test.platform.app.InstrumentationRegistry;
import org.junit.Test;

public class DisplayMetricsHolderTest {
  @Test
  public void getScreenDisplayMetricsReturnsIsolatedSnapshot() {
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    float resourcesDensity = context.getResources().getDisplayMetrics().density;
    DisplayMetricsHolder.updateOrInitDisplayMetrics(context, 3.0f);
    DisplayMetrics snapshot = DisplayMetricsHolder.getScreenDisplayMetrics();

    assertNotNull(snapshot);
    assertEquals(3.0f, snapshot.density, 0);
    assertEquals(resourcesDensity, context.getResources().getDisplayMetrics().density, 0);
    int globalWidth = snapshot.widthPixels;
    snapshot.widthPixels = globalWidth + 1;

    DisplayMetrics globalMetrics = DisplayMetricsHolder.getScreenDisplayMetrics();
    assertNotNull(globalMetrics);
    assertEquals(globalWidth, globalMetrics.widthPixels);
  }
}
