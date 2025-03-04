// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance;

import static org.mockito.Mockito.*;

import com.lynx.tasm.TimingHandler;
import org.junit.Before;
import org.junit.Test;

public class TimingCollectorTest {
  private TimingCollector timingCollector;

  @Before
  public void setUp() {
    timingCollector = spy(new TimingCollector());
  }

  @Test
  public void testSetExtraTiming() throws Exception {
    TimingHandler.ExtraTimingInfo extraTimingInfo = new TimingHandler.ExtraTimingInfo();
    extraTimingInfo.mOpenTime = 1000;
    extraTimingInfo.mContainerInitStart = 2000;
    extraTimingInfo.mContainerInitEnd = 3000;
    extraTimingInfo.mPrepareTemplateStart = 4000;
    extraTimingInfo.mPrepareTemplateEnd = 5000;

    timingCollector.setExtraTiming(extraTimingInfo);

    verify(timingCollector).setMsTiming(TimingHandler.OPEN_TIME, 1000, null);
    verify(timingCollector).setMsTiming(TimingHandler.CONTAINER_INIT_START, 2000, null);
    verify(timingCollector).setMsTiming(TimingHandler.CONTAINER_INIT_END, 3000, null);
    verify(timingCollector).setMsTiming(TimingHandler.PREPARE_TEMPLATE_START, 4000, null);
    verify(timingCollector).setMsTiming(TimingHandler.PREPARE_TEMPLATE_END, 5000, null);
  }
}
