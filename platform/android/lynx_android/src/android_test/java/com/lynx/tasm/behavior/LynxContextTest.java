// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.util.DisplayMetrics;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.PageConfig;
import com.lynx.tasm.utils.DisplayMetricsHolder;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class LynxContextTest {
  private LynxContext mContext;

  @Before
  public void setUp() throws Exception {
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    DisplayMetricsHolder.updateOrInitDisplayMetrics(context, 3.0f);
    DisplayMetrics displayMetrics = new DisplayMetrics();
    displayMetrics.widthPixels = 1080;
    displayMetrics.heightPixels = 1920;
    displayMetrics.density = 3.0f;
    mContext = new LynxContext(context, displayMetrics) {
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
  public void currentTargetTouchPositionFollowsPageConfig() {
    assertFalse(mContext.getEnableCurrentTargetTouchPosition());

    JavaOnlyMap config = new JavaOnlyMap();
    config.putBoolean("enableCurrentTargetTouchPosition", true);
    mContext.onPageConfigDecoded(new PageConfig(config));

    assertTrue(mContext.getEnableCurrentTargetTouchPosition());
  }
}
