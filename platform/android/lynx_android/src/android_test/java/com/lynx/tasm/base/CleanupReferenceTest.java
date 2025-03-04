// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.base;

import static org.junit.Assert.*;

import androidx.test.filters.SmallTest;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;
import java.util.Calendar;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.function.BooleanSupplier;
import org.junit.Before;
import org.junit.Test;

/** Test suite for {@link CleanupReference}. */
public class CleanupReferenceTest {
  private static AtomicInteger sObjectCount = new AtomicInteger();

  private static class ReferredObject {
    private CleanupReference mRef;

    // Remember: this MUST be a static class, to avoid an implicit ref back to the
    // owning ReferredObject instance which would defeat GC of that object.
    private static class DestroyRunnable implements Runnable {
      private final boolean mCleanupOnUiThread;

      public DestroyRunnable(boolean cleanupOnUiThread) {
        super();
        mCleanupOnUiThread = cleanupOnUiThread;
      }

      @Override
      public void run() {
        assertEquals(mCleanupOnUiThread, UIThreadUtils.isOnUiThread());
        sObjectCount.decrementAndGet();
      }
    }

    public ReferredObject(boolean cleanupOnUiThread) {
      sObjectCount.incrementAndGet();
      mRef = new CleanupReference(this, new DestroyRunnable(cleanupOnUiThread), cleanupOnUiThread);
    }

    private static class DestroyRunnableDelayFinish implements Runnable {
      private final long mDelayFinishMs;

      public DestroyRunnableDelayFinish(long delayFinishMs) {
        super();
        mDelayFinishMs = delayFinishMs;
      }

      @Override
      public void run() {
        assertEquals(true, UIThreadUtils.isOnUiThread());
        if (mDelayFinishMs > 0) {
          try {
            Thread.sleep(mDelayFinishMs);
          } catch (Throwable e) {
          }
        }
        sObjectCount.decrementAndGet();
      }
    }

    public ReferredObject(long delayFinishMs) {
      sObjectCount.incrementAndGet();
      mRef = new CleanupReference(this, new DestroyRunnableDelayFinish(delayFinishMs), true);
    }

    public CleanupReference getRef() {
      return mRef;
    }
  }

  @Before
  public void setUp() {
    sObjectCount.set(0);
  }

  private static void collectGarbage() {
    // While this is only a 'hint' to the VM, it's generally effective and sufficient on
    // dalvik. If this changes in future, maybe try allocating a few gargantuan objects
    // too, to force the GC to work.
    Runtime.getRuntime().gc();
  }

  private static void testCreateSingle(boolean cleanupOnUiThread) {
    assertEquals(0, sObjectCount.get());

    ReferredObject instance = new ReferredObject(cleanupOnUiThread);
    assertEquals(1, sObjectCount.get());

    CleanupReference ref = instance.getRef();

    instance = null;
    // Ensure compiler / instrumentation does not strip out the assignment.
    assertNull(instance);
    collectGarbage();
    assertEquals(true, pollCheckGc(() -> true == ref.hasCleanedUp(), sMaxTimeoutMs));
    assertEquals(0, sObjectCount.get());
  }

  @Test
  @SmallTest
  public void testCreateSingleHandleOnUiThread() {
    testCreateSingle(true);
  }

  @Test
  @SmallTest
  public void testCreateSingleHandleOnWorkerThread() {
    testCreateSingle(false);
  }

  @Test
  @SmallTest
  public void testLongTask() {
    assertEquals(0, sObjectCount.get());

    ReferredObject instance = new ReferredObject(sMaxTimeoutMs * 3 / 4);
    assertEquals(1, sObjectCount.get());

    CleanupReference ref = instance.getRef();

    instance = null;
    // Ensure compiler / instrumentation does not strip out the assignment.
    assertNull(instance);
    collectGarbage();
    // Has started
    assertEquals(true, pollCheckGc(() -> true == ref.hasCleanedUp(), sMaxTimeoutMs / 2));
    // May not finish, but should finish after a while
    assertEquals(true, pollCheckGc(() -> 0 == sObjectCount.get(), sMaxTimeoutMs));
  }

  @Test
  @SmallTest
  public void testCreateMany() {
    assertEquals(0, sObjectCount.get());

    final int instanceCount = 20;
    ReferredObject[] instances = new ReferredObject[instanceCount];
    CleanupReference[] refs = new CleanupReference[instanceCount];

    for (int i = 0; i < instanceCount; ++i) {
      instances[i] = new ReferredObject(true);
      assertEquals(i + 1, sObjectCount.get());
      refs[i] = instances[i].getRef();
    }

    instances = null;
    // Ensure compiler / instrumentation does not strip out the assignment.
    assertNull(instances);
    // Calling sObjectCount.get() before collectGarbage() seems to be required for the objects
    // to be GC'ed only when building using GN.
    assertNotEquals(sObjectCount.get(), -1);
    collectGarbage();
    assertEquals(true, pollCheckGc(() -> {
      for (CleanupReference ref : refs) {
        if (!ref.hasCleanedUp()) {
          return false;
        }
      }
      return true;
    }, sMaxTimeoutMs));
    assertEquals(0, sObjectCount.get());
  }

  private static class ReferredFinalizer {
    private CleanupReference mRef;

    // Remember: this MUST be a static class, to avoid an implicit ref back to the
    // owning ReferredObject instance which would defeat GC of that object.
    private static class DestroyRunnable implements Runnable {
      private final boolean mCleanupOnUiThread;
      private final boolean mCleanupWhenDestroy;
      WeakReference<ReferredFinalizer> mWeakReferredFinalizer;

      public DestroyRunnable(boolean cleanupOnUiThread, boolean cleanupWhenDestroy,
          ReferredFinalizer referredFinalizer) {
        super();
        mCleanupOnUiThread = cleanupOnUiThread;
        mCleanupWhenDestroy = cleanupWhenDestroy;
        mWeakReferredFinalizer = new WeakReference<>(referredFinalizer);
      }

      @Override
      public void run() {
        assertEquals(mCleanupOnUiThread, UIThreadUtils.isOnUiThread());
        // not always run after finalize
        sObjectCount.decrementAndGet();
        if (!mCleanupWhenDestroy) {
          // referred object can't be accessed again
          assertNull(mWeakReferredFinalizer.get());
        }
      }
    }

    private void destroy() {
      mRef.cleanupNow();
    }

    @Override
    protected void finalize() throws Throwable {
      super.finalize();
      sObjectCount.decrementAndGet();
    }

    public ReferredFinalizer(boolean cleanupOnUiThread, boolean cleanupWhenDestroy) {
      sObjectCount.addAndGet(2);
      mRef = new CleanupReference(this,
          new DestroyRunnable(cleanupOnUiThread, cleanupWhenDestroy, this), cleanupOnUiThread);
    }
  }

  public void testCreateFinalizer(boolean cleanupOnUiThread, boolean cleanupWhenDestroy) {
    assertEquals(0, sObjectCount.get());
    ReferredFinalizer instance = new ReferredFinalizer(cleanupOnUiThread, cleanupWhenDestroy);
    assertEquals(2, sObjectCount.get());

    if (cleanupWhenDestroy) {
      instance.destroy();
    }
    instance = null;

    // Ensure compiler / instrumentation does not strip out the assignment.
    assertNull(instance);
    collectGarbage();
    assertEquals(true, pollCheckGc(() -> 0 == sObjectCount.get(), sMaxTimeoutMs));
    assertEquals(0, sObjectCount.get());
    // assert cleanup task run once only
    assertEquals(false, pollCheckGc(() -> 0 != sObjectCount.get(), sCheckIntervalMs * 10));
  }

  @Test
  @SmallTest
  public void testCleanupOnUiThreadWhenGc() {
    testCreateFinalizer(true, false);
  }

  @Test
  @SmallTest
  public void testCleanupOnWorkerThreadWhenGc() {
    testCreateFinalizer(false, false);
  }

  @Test
  @SmallTest
  public void testCleanupOnUiThreadWhenDestroyOnTestThread() {
    UIThreadUtils.assertNotOnUiThread();
    testCreateFinalizer(true, true);
  }

  @Test
  @SmallTest
  public void testCleanupOnWorkerThreadWhenDestroyOnTestThread() {
    UIThreadUtils.assertNotOnUiThread();
    testCreateFinalizer(false, true);
  }

  @Test
  @SmallTest
  public void testCleanupOnUiThreadWhenDestroyOnUiThread() {
    UIThreadUtils.runOnUiThreadImmediately(() -> {
      UIThreadUtils.assertOnUiThread();
      testCreateFinalizer(true, true);
    });
  }

  @Test
  @SmallTest
  public void testCleanupOnWorkerThreadWhenDestroyOnUiThread() {
    UIThreadUtils.runOnUiThreadImmediately(() -> {
      UIThreadUtils.assertOnUiThread();
      // When cleanup on worker thread is set, cleanupNow on ui thread will be ignored, instead will
      // cleanup on worker thread when gc.
      testCreateFinalizer(false, true);
    });
  }

  private static final long sMaxTimeoutMs = 3000L;
  private static final long sCheckIntervalMs = 50L;

  private static boolean pollCheckGc(BooleanSupplier supplier, long timeoutMs) {
    long startMs = Calendar.getInstance().getTimeInMillis();
    while ((Calendar.getInstance().getTimeInMillis() - startMs) < timeoutMs) {
      try {
        collectGarbage();
        Thread.sleep(sCheckIntervalMs);
        if (supplier.getAsBoolean()) {
          return true;
        }
      } catch (Throwable e) {
      }
    }
    return false;
  }
}
