// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.PageConfig;
import com.lynx.tasm.TimingHandler;
import com.lynx.tasm.performance.fsp.FSPTracer;
import java.lang.reflect.Field;
import org.junit.Before;
import org.junit.Test;

public class PerformanceControllerTest {
  private PerformanceController performanceController;

  @Before
  public void setUp() {
    performanceController = spy(new PerformanceController());
  }

  @Test
  public void testSetExtraTiming() throws Exception {
    TimingHandler.ExtraTimingInfo extraTimingInfo = new TimingHandler.ExtraTimingInfo();
    extraTimingInfo.mOpenTime = 1000;
    extraTimingInfo.mContainerInitStart = 2000;
    extraTimingInfo.mContainerInitEnd = 3000;
    extraTimingInfo.mPrepareTemplateStart = 4000;
    extraTimingInfo.mPrepareTemplateEnd = 5000;

    performanceController.setExtraTiming(extraTimingInfo);
  }

  @Test
  public void startFSPTracerCreatesTracer() throws Exception {
    setEnableFSPField(true);

    performanceController.startFSPTracer(() -> null);

    assertNotNull(getFSPTracer());
  }

  @Test
  public void startFSPTracerUsesPageConfig() throws Exception {
    setEnableFSPField(false);
    performanceController.onPageConfigDecoded(createPageConfig(true));

    performanceController.startFSPTracer(() -> null);

    assertNotNull(getFSPTracer());
  }

  @Test
  public void startFSPTracerStopsForDisabledPageConfig() throws Exception {
    setEnableFSPField(true);
    performanceController.onPageConfigDecoded(createPageConfig(false));

    performanceController.startFSPTracer(() -> null);

    assertNull(getFSPTracer());
  }

  @Test
  public void stopFSPTracerByUserInteractionUsesPageConfig() throws Exception {
    FSPTracer fspTracer = mock(FSPTracer.class);
    setField(PerformanceController.class, performanceController, "mFSPTracer", fspTracer);

    performanceController.onPageConfigDecoded(createPageConfig(false));
    performanceController.stopFSPTracerByUserInteraction();
    verify(fspTracer, never()).cancelledByUserInteraction();

    performanceController.onPageConfigDecoded(createPageConfig(true));
    performanceController.stopFSPTracerByUserInteraction();
    verify(fspTracer).cancelledByUserInteraction();
  }

  private FSPTracer getFSPTracer() throws Exception {
    return (FSPTracer) getField(PerformanceController.class, performanceController, "mFSPTracer");
  }

  private PageConfig createPageConfig(boolean enableFSP) {
    JavaOnlyMap map = new JavaOnlyMap();
    map.putBoolean("enableFSP", enableFSP);
    return new PageConfig(map);
  }

  private void setEnableFSPField(boolean value) throws Exception {
    Field field = LynxEnv.class.getDeclaredField("mEnableFSP");
    field.setAccessible(true);
    field.set(LynxEnv.inst(), value);
  }

  private Object getField(Class clazz, Object target, String fieldName) throws Exception {
    Field field = clazz.getDeclaredField(fieldName);
    field.setAccessible(true);
    return field.get(target);
  }

  private void setField(Class clazz, Object target, String fieldName, Object value)
      throws Exception {
    Field field = clazz.getDeclaredField(fieldName);
    field.setAccessible(true);
    field.set(target, value);
  }
}
