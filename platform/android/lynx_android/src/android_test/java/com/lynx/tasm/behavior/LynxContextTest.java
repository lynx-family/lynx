// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.util.DisplayMetrics;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.utils.DisplayMetricsHolder;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class LynxContextTest {
  private LynxContext mContext;
  private Context mAndroidContext;
  private DisplayMetrics mDisplayMetrics;

  @Before
  public void setUp() throws Exception {
    mAndroidContext =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    DisplayMetricsHolder.updateOrInitDisplayMetrics(mAndroidContext, 3.0f);
    mDisplayMetrics = new DisplayMetrics();
    mDisplayMetrics.widthPixels = 1080;
    mDisplayMetrics.heightPixels = 1920;
    mDisplayMetrics.density = 3.0f;
    mContext = new LynxContext(mAndroidContext, mDisplayMetrics) {
      @Override
      public void handleException(Exception e) {}
    };
  }

  @After
  public void tearDown() throws Exception {}

  @Test
  public void isFallbackProcess() {
    assertFalse(mContext.isFallbackProcess());
  }

  @Test
  public void markFallbackProcess() {
    mContext.markFallbackProcess(true);
    assertTrue(mContext.isFallbackProcess());

    mContext.markFallbackProcess(false);
    assertFalse(mContext.isFallbackProcess());
  }

  @Test
  public void takeScreenMetricsOwnership() {
    LynxContext context = new LynxContext(mAndroidContext, mDisplayMetrics, true) {
      @Override
      public void handleException(Exception e) {}
    };

    assertSame(mDisplayMetrics, context.getScreenMetrics());
  }
}
