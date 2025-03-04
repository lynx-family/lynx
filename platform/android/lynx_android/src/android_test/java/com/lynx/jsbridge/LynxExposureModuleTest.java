// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import static org.mockito.Mockito.*;

import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.UIExposure;
import com.lynx.tasm.utils.UIThreadUtils;
import java.util.concurrent.Semaphore;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class LynxExposureModuleTest {
  public LynxExposureModule mModule;
  public LynxContext mLynxContext;
  public UIExposure mExposure;
  public Semaphore mSemaphore = new Semaphore(1);

  @Before
  public void setUp() throws IllegalAccessException, NoSuchFieldException, InterruptedException {
    mSemaphore.acquire(1);
    mLynxContext = mock(LynxContext.class);
    mExposure = mock(UIExposure.class);
    when(mLynxContext.getExposure()).thenReturn(mExposure);
    mModule = new LynxExposureModule(mLynxContext);
    mSemaphore.release(1);
  }

  @After
  public void tearDown() throws InterruptedException {
    mSemaphore.acquire(1);
    mModule = null;
    mLynxContext = null;
    mExposure = null;
    mSemaphore.release(1);
  }

  @Test
  public void testResumeExposure() throws InterruptedException {
    mSemaphore.acquire(1);
    mModule.resumeExposure();
    UIThreadUtils.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        verify(mExposure).resumeExposure();
        mSemaphore.release(1);
      }
    }, 10);
  }

  @Test
  public void testStopExposure() throws InterruptedException {
    mSemaphore.acquire(1);
    JavaOnlyMap options = new JavaOnlyMap();
    options.putBoolean("sendEvent", false);
    mModule.stopExposure(options);
    UIThreadUtils.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        verify(mExposure).stopExposure(options.asHashMap());
        mSemaphore.release(1);
      }
    }, 10);
  }
}
