// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.fluency;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.Map;
import junit.framework.TestCase;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxFpsTracerTest extends TestCase {
  private LynxFpsTracer mTracer;
  private LynxFluencyCallback mFluencyCallback;

  private static class LynxFluencyCallback implements LynxFpsTracer.IFluencyCallback {
    private final String mScene;
    private final String mTag;

    private final Map<String, Object> metrics = new HashMap<>();
    public LynxFluencyCallback(String scene, String tag) {
      mScene = scene;
      mTag = tag;
    }

    @Override
    public void report(LynxFpsTracer.LynxFpsRawMetrics rawMetrics) {
      // mock duration
      rawMetrics.duration = 462;

      metrics.put("lynxsdk_fluency_scene", mScene);
      metrics.put("lynxsdk_fluency_frames_number", rawMetrics.frames);
      metrics.put("lynxsdk_fluency_fps", rawMetrics.fps);
      metrics.put("lynxsdk_fluency_drop1_count", rawMetrics.drop1);
      metrics.put("lynxsdk_fluency_drop1_duration", rawMetrics.drop1Duration);
      metrics.put("lynxsdk_fluency_drop3_count", rawMetrics.drop3);
      metrics.put("lynxsdk_fluency_drop3_duration", rawMetrics.drop3Duration);
      metrics.put("lynxsdk_fluency_drop7_count", rawMetrics.drop7);
      metrics.put("lynxsdk_fluency_drop7_duration", rawMetrics.drop7Duration);
      metrics.put("lynxsdk_fluency_drop25_count", rawMetrics.drop25);
      metrics.put("lynxsdk_fluency_drop25_duration", rawMetrics.drop25Duration);
    }
    // LynxFpsRawMetrics{frames=3, fps=7, maximumFrames=60, duration=462, drop1=3,
    // drop1Duration=333, drop3=3, drop3Duration=333, drop7=2, drop7Duration=250, drop25=0,
    // drop25Duration=0}
    public boolean checkResult() {
      boolean result = true;
      result &= (int) metrics.get("lynxsdk_fluency_frames_number") == 3;
      result &= (int) metrics.get("lynxsdk_fluency_fps") == 7;
      result &= (int) metrics.get("lynxsdk_fluency_drop1_count") == 3;
      result &= (long) metrics.get("lynxsdk_fluency_drop1_duration") == 333;
      result &= (int) metrics.get("lynxsdk_fluency_drop3_count") == 3;
      result &= (long) metrics.get("lynxsdk_fluency_drop3_duration") == 333;
      result &= (int) metrics.get("lynxsdk_fluency_drop7_count") == 2;
      result &= (long) metrics.get("lynxsdk_fluency_drop7_duration") == 250;
      result &= (int) metrics.get("lynxsdk_fluency_drop25_count") == 0;
      result &= (long) metrics.get("lynxsdk_fluency_drop25_duration") == 0;
      return result;
    }
  }

  public LynxFpsTracerTest() {}

  @Before
  public void setUp() throws Exception {
    super.setUp();

    LynxContext lynxContext = TestingUtils.getLynxContext();
    mTracer = new LynxFpsTracer(lynxContext);
    mFluencyCallback = new LynxFluencyCallback("scroll", null);
    mTracer.setFluencyCallback(mFluencyCallback);
  }

  @Test
  public void testFluencyCallback() throws Exception {
    // mock data
    Method doDropComputeMethod =
        mTracer.getClass().getDeclaredMethod("doDropCompute", long.class, long.class);
    doDropComputeMethod.setAccessible(true);
    Field lastFrameField = mTracer.getClass().getDeclaredField("mLastFrameNanos");
    lastFrameField.setAccessible(true);
    lastFrameField.setLong(mTracer, 3399526448707L);
    doDropComputeMethod.invoke(mTracer, 3399526448707L, 3399643115369L);
    lastFrameField.setLong(mTracer, 3399643115369L);
    doDropComputeMethod.invoke(mTracer, 3399643115369L, 3399743115365L);
    lastFrameField.setLong(mTracer, 3399743115365L);
    doDropComputeMethod.invoke(mTracer, 3399743115365L, 3399909782025L);
    lastFrameField.setLong(mTracer, 3399909782025L);

    Field startTimeNanosField = mTracer.getClass().getDeclaredField("mStartTimeNanos");
    startTimeNanosField.setAccessible(true);
    startTimeNanosField.setLong(mTracer, 3399526448707L);
    Field counterField = mTracer.getClass().getDeclaredField("mCounter");
    counterField.setAccessible(true);
    counterField.setInt(mTracer, 4);

    Method doReportMethod = mTracer.getClass().getDeclaredMethod("doReport");
    doReportMethod.setAccessible(true);
    doReportMethod.invoke(mTracer);

    // check callback
    assertTrue(mFluencyCallback.checkResult());
  }

  @Test
  public void testGetRoundedRate() throws Exception {
    Method doReportMethod = mTracer.getClass().getDeclaredMethod("getRoundedRate", float.class);
    doReportMethod.setAccessible(true);
    int rate = (int) doReportMethod.invoke(mTracer, 60.1f);

    assertEquals(rate, 60);
  }
}
