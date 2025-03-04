// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.featurecount;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.service.ILynxEventReporterService;
import com.lynx.tasm.service.LynxServiceCenter;
import java.util.Map;
import java.util.concurrent.CountDownLatch;
import junit.framework.TestCase;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxFeatureCounterTest extends TestCase {
  private CountDownLatch mLock;
  private boolean mRet = false;

  @Before
  public void setUp() throws Exception {
    super.setUp();
    mLock = new CountDownLatch(1);
    mRet = false;
    LynxServiceCenter.inst().registerService(
        ILynxEventReporterService.class, new ILynxEventReporterService() {
          @Override
          public void onReportEvent(@NonNull String eventName, int instanceId,
              @NonNull Map<String, ? extends Object> data,
              @Nullable Map<String, ? extends Object> extraData) {
            try {
              Object value = data.get("java_hardware_layer");
              if (eventName.equals("lynxsdk_feature_count_event") && value != null
                  && value instanceof Boolean && (boolean) value == true) {
                mRet = true;
              }
              mLock.countDown();
            } catch (Exception e) {
              e.printStackTrace();
              mLock.countDown();
            }
          }
        });
  }

  @After
  public void tearDown() throws Exception {
    super.tearDown();
    LynxServiceCenter.inst().unregisterAllService();
  }

  @Test
  public void count() {
    LynxFeatureCounter.count(LynxFeatureCounter.JAVA_HARDWARE_LAYER, 1);
    // Test in c++, java api ignore.
  }
}
