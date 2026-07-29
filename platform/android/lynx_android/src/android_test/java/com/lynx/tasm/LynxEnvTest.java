// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.app.Application;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.utils.UIThreadUtils;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxEnvTest {
  private LynxEnv mEnv;

  @Before
  public void setUp() {
    mEnv = LynxEnv.inst();
    resetLynxEnvReadyState();
  }

  @After
  public void tearDown() {
    resetLynxEnvReadyState();
  }

  @Test
  public void hasInitedDoesNotMeanInitCompleted() {
    mEnv.hasInit.set(true);

    assertTrue(mEnv.hasInited());
    assertFalse(mEnv.isInitCompleted());
  }

  @Test
  public void isInitCompletedRequiresNativeUiThreadInitialized() {
    mEnv.hasInit.set(true);
    mEnv.mIsNativeLibraryLoaded = true;
    mEnv.mHasInitCompleted = true;

    mEnv.mIsNativeUIThreadInited = false;
    assertFalse(mEnv.isInitCompleted());

    mEnv.mIsNativeUIThreadInited = true;
    assertTrue(mEnv.isInitCompleted());
  }

  @Test
  public void initDoesNotReenterBeforeInitStartedIsMarked() {
    Application application = ApplicationProvider.getApplicationContext();
    ReentrantLynxEnv env = new ReentrantLynxEnv(application);

    env.init(application, libName -> {}, null, null, null);

    assertEquals(1, env.mInitDevtoolComponentAttachSwitchCount);
    assertEquals(1, env.mInitNativeLibrariesCount);
  }

  @Test
  public void isInitCompletedReturnsFalseAcrossThreadsWhileInitIsIncomplete() throws Exception {
    mEnv.hasInit.set(true);
    mEnv.mIsNativeLibraryLoaded = true;
    mEnv.mIsNativeUIThreadInited = true;
    mEnv.mHasInitCompleted = false;

    int threadCount = 8;
    CountDownLatch start = new CountDownLatch(1);
    CountDownLatch done = new CountDownLatch(threadCount);
    AtomicBoolean sawInitCompleted = new AtomicBoolean(false);
    AtomicBoolean failed = new AtomicBoolean(false);
    List<Thread> threads = new ArrayList<>();

    for (int i = 0; i < threadCount; i++) {
      Thread thread = new Thread(() -> {
        try {
          start.await();
          for (int j = 0; j < 1000; j++) {
            if (mEnv.isInitCompleted()) {
              sawInitCompleted.set(true);
            }
          }
        } catch (Throwable t) {
          failed.set(true);
        } finally {
          done.countDown();
        }
      });
      threads.add(thread);
      thread.start();
    }

    start.countDown();

    assertTrue(done.await(5, TimeUnit.SECONDS));
    for (Thread thread : threads) {
      thread.join();
    }
    assertFalse(failed.get());
    assertFalse(sawInitCompleted.get());

    mEnv.mHasInitCompleted = true;
    assertTrue(mEnv.isInitCompleted());
  }

  @Test
  public void initCompletionHandlerSeesInitCompletedOnInitialInit() throws Exception {
    Application application = ApplicationProvider.getApplicationContext();
    CountDownLatch callbackLatch = new CountDownLatch(1);
    AtomicBoolean callbackSawInitCompleted = new AtomicBoolean(false);

    mEnv.init(application, System::loadLibrary, null, null, null, () -> {
      callbackSawInitCompleted.set(mEnv.isInitCompleted());
      callbackLatch.countDown();
    });

    assertTrue(callbackLatch.await(5, TimeUnit.SECONDS));
    assertTrue(callbackSawInitCompleted.get());
  }

  @Test
  public void initCompletionHandlerRunsOnMainThreadAfterInitCompleted() throws Exception {
    mEnv.hasInit.set(true);
    mEnv.mIsNativeLibraryLoaded = true;
    mEnv.mIsNativeUIThreadInited = true;
    mEnv.mHasInitCompleted = true;
    Application application = ApplicationProvider.getApplicationContext();
    CountDownLatch callbackLatch = new CountDownLatch(1);
    CountDownLatch mainQueueDrainedLatch = new CountDownLatch(1);
    AtomicBoolean callbackRanOnMainThread = new AtomicBoolean(false);
    AtomicBoolean callbackSawInitCompleted = new AtomicBoolean(false);
    AtomicInteger callbackCount = new AtomicInteger(0);

    Thread initThread = new Thread(() -> {
      mEnv.init(application, null, null, null, null, () -> {
        callbackCount.incrementAndGet();
        callbackRanOnMainThread.set(UIThreadUtils.isOnUiThread());
        callbackSawInitCompleted.set(mEnv.isInitCompleted());
        callbackLatch.countDown();
      });
    });
    initThread.start();
    initThread.join();

    UIThreadUtils.runOnUiThread(mainQueueDrainedLatch::countDown);
    assertTrue(callbackLatch.await(5, TimeUnit.SECONDS));
    assertTrue(mainQueueDrainedLatch.await(5, TimeUnit.SECONDS));
    assertEquals(1, callbackCount.get());
    assertTrue(callbackRanOnMainThread.get());
    assertTrue(callbackSawInitCompleted.get());
  }

  private void resetLynxEnvReadyState() {
    mEnv.hasInit.set(false);
    mEnv.mIsNativeLibraryLoaded = false;
    mEnv.mIsNativeUIThreadInited = false;
    mEnv.mHasInitCompleted = false;
  }

  private static class ReentrantLynxEnv extends LynxEnv {
    private final Application mApplication;
    private boolean mHasReentered;
    private int mInitDevtoolComponentAttachSwitchCount;
    private int mInitNativeLibrariesCount;

    private ReentrantLynxEnv(Application application) {
      mApplication = application;
    }

    @Override
    protected void initDevtoolComponentAttachSwitch() {
      mInitDevtoolComponentAttachSwitchCount++;
      if (!mHasReentered) {
        mHasReentered = true;
        init(mApplication, libName -> {}, null, null, null);
      }
    }

    @Override
    public synchronized boolean initNativeLibraries(INativeLibraryLoader loader) {
      mInitNativeLibrariesCount++;
      return false;
    }

    @Override
    protected void setAppTracingAllowed() {}
  }
}
