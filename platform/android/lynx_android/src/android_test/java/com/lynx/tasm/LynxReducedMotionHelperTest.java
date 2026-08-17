// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;

import android.content.Context;
import androidx.test.core.app.ApplicationProvider;
import java.lang.reflect.Field;
import java.util.WeakHashMap;
import org.junit.Test;

public class LynxReducedMotionHelperTest {
  @Test
  public void animationScaleMapping() {
    assertFalse(LynxReducedMotionHelper.isReducedMotionEnabled(1.0f, 1.0f));
    assertTrue(LynxReducedMotionHelper.isReducedMotionEnabled(0.0f, 1.0f));
    assertTrue(LynxReducedMotionHelper.isReducedMotionEnabled(1.0f, 0.0f));
    assertTrue(LynxReducedMotionHelper.isReducedMotionEnabled(0.0f, 0.0f));
  }

  @Test
  public void sharesSystemObserversAcrossTemplateRenders() throws Exception {
    Context context = ApplicationProvider.getApplicationContext();
    LynxTemplateRender firstRender = mock(LynxTemplateRender.class);
    LynxTemplateRender secondRender = mock(LynxTemplateRender.class);

    LynxReducedMotionHelper helper =
        LynxReducedMotionHelper.getInstance(context.getContentResolver());
    assertSame(helper, LynxReducedMotionHelper.getInstance(context.getContentResolver()));
    helper.start(firstRender);
    helper.start(secondRender);

    assertTrue(isStarted(helper));
    assertEquals(2, listenerCount(helper));

    helper.stop(firstRender);
    assertTrue(isStarted(helper));
    assertEquals(1, listenerCount(helper));

    helper.stop(secondRender);
    assertFalse(isStarted(helper));
    assertEquals(0, listenerCount(helper));
  }

  private static boolean isStarted(LynxReducedMotionHelper helper) throws Exception {
    Field field = LynxReducedMotionHelper.class.getDeclaredField("mStarted");
    field.setAccessible(true);
    return field.getBoolean(helper);
  }

  private static int listenerCount(LynxReducedMotionHelper helper) throws Exception {
    Field field = LynxReducedMotionHelper.class.getDeclaredField("mTemplateRenders");
    field.setAccessible(true);
    return ((WeakHashMap<?, ?>) field.get(helper)).size();
  }
}
