// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;

import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.behavior.LynxContext;
import java.util.concurrent.Semaphore;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class LynxTextInfoModuleTest {
  public LynxTextInfoModule mModule;
  public LynxContext mLynxContext;
  public Semaphore mSemaphore = new Semaphore(1);

  @Before
  public void setUp() throws IllegalAccessException, NoSuchFieldException, InterruptedException {
    mSemaphore.acquire(1);
    mLynxContext = mock(LynxContext.class);
    mModule = new LynxTextInfoModule(mLynxContext);
    mSemaphore.release(1);
  }

  @After
  public void tearDown() throws InterruptedException {
    mSemaphore.acquire(1);
    mModule = null;
    mLynxContext = null;
    mSemaphore.release(1);
  }

  @Test
  public void testGetTextInfo() throws InterruptedException {
    mSemaphore.acquire(1);
    ReadableMap ret = mModule.getTextInfo("aaaaa", new JavaOnlyMap());
    assertTrue(ret != null);
    assertTrue(ret.getInt("width") == 0);
    assertTrue(ret.getArray("content") == null);

    JavaOnlyMap textInfo = new JavaOnlyMap();
    textInfo.putString("fontSize", "12px");
    ret = mModule.getTextInfo("aaaaa", textInfo);
    assertTrue(ret != null);
    assertTrue(ret.getInt("width") >= 0);
    assertTrue(ret.getArray("content") != null);
    mSemaphore.release(1);
  }
}
