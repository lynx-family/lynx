// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.*;

import android.os.Looper;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.LynxEnv;
import java.util.concurrent.FutureTask;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.InOrder;

/** Exercises the native renderer, not a Java-only approximation of deferred consumption. */
@RunWith(AndroidJUnit4.class)
public class DeferredRendererPreparationTest {
  private static final int SIGN = 101;
  private PlatformRendererContext context;
  private Runnable completion;
  private long nativePtr;

  @Before
  public void setUp() throws Exception {
    LynxEnv.inst().initNativeLibraries(System::loadLibrary);
    context = mock(PlatformRendererContext.class);
    completion = mock(Runnable.class);
    doAnswer(invocation -> {
      assertEquals(Looper.getMainLooper(), Looper.myLooper());
      return null;
    })
        .when(completion)
        .run();
    // Use the real Java handoff, but no UIOwner or actual Android host is needed.
    doCallRealMethod().when(context).createPlatformExtendedRendererWithPreparation(
        SIGN, "deferred-test", null, completion);
    runOnMainSync(() -> nativePtr = nativeCreate(context, completion));
  }

  @After
  public void tearDown() throws Exception {
    runOnMainSync(() -> {
      nativeDestroy(nativePtr);
      nativePtr = 0;
    });
  }

  private static void runOnMainSync(Runnable runnable) throws Exception {
    // Report assertion failures to JUnit instead of crashing the Android main looper.
    FutureTask<Void> task = new FutureTask<>(runnable, null);
    InstrumentationRegistry.getInstrumentation().runOnMainSync(task);
    task.get();
  }

  private void assertNativeOnlyWorkIsDeferred() {
    assertTrue(nativeHasPendingPreparations(nativePtr));
    assertEquals("Construction must not claim preparation", 0, nativePreparationRuns(nativePtr));
    assertTrue("Renderer must be registered and cache layout", nativeCacheLayout(nativePtr));
    assertEquals("Layout caching must not claim preparation", 0, nativePreparationRuns(nativePtr));
    // In particular, eager createPlatformExtendedRendererWithPreparation fails here.
    verifyNoInteractions(context, completion);
  }

  @Test
  public void firstHostOperationCompletesPreparationOnceBeforeFrameUpdate() throws Exception {
    runOnMainSync(() -> {
      assertNativeOnlyWorkIsDeferred();
      nativeUpdateDisplayList(nativePtr);
      assertFalse(nativeHasPendingPreparations(nativePtr));
      nativeUpdateDisplayList(nativePtr);
      assertEquals(1, nativePreparationRuns(nativePtr));

      InOrder order = inOrder(context, completion);
      order.verify(context).createPlatformExtendedRendererWithPreparation(
          SIGN, "deferred-test", null, completion);
      order.verify(completion).run();
      order.verify(context, times(2))
          .updatePlatformRendererFrame(
              SIGN, false, 20, 30, 100, 200, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
      verifyNoMoreInteractions(context, completion);
    });
  }

  @Test
  public void consumingOnePreparationKeepsOtherPendingUntilAbort() throws Exception {
    runOnMainSync(() -> {
      nativeAddAbortedPreparation(nativePtr);
      nativeUpdateDisplayList(nativePtr);
      assertEquals(1, nativePreparationRuns(nativePtr));
      assertTrue(
          "Another renderer still needs preparation", nativeHasPendingPreparations(nativePtr));
      nativeConsumeAbortedPreparation(nativePtr);
      assertFalse(
          "An aborted result also releases the gate", nativeHasPendingPreparations(nativePtr));
      nativeConsumeAbortedPreparation(nativePtr);
      assertFalse(nativeHasPendingPreparations(nativePtr));
      verify(completion).run();
    });
  }

  @Test
  public void droppingOnePreparationKeepsOtherPending() throws Exception {
    runOnMainSync(() -> {
      nativeAddAbortedPreparation(nativePtr);
      assertTrue(nativeTearDownRenderer(nativePtr, false));
      assertTrue(nativeHasPendingPreparations(nativePtr));
      nativeConsumeAbortedPreparation(nativePtr);
      assertFalse(nativeHasPendingPreparations(nativePtr));
      assertEquals(0, nativePreparationRuns(nativePtr));
      verifyNoInteractions(context, completion);
    });
  }

  @Test
  public void reentrantTeardownDuringFinalizationSkipsFrameUpdate() throws Exception {
    runOnMainSync(() -> {
      assertNativeOnlyWorkIsDeferred();
      doAnswer(invocation -> {
        assertFalse(
            "Clear the gate before Java finalization", nativeHasPendingPreparations(nativePtr));
        assertTrue(nativeTearDownRenderer(nativePtr, true));
        return null;
      })
          .when(completion)
          .run();
      nativeUpdateDisplayList(nativePtr);
      assertEquals(1, nativePreparationRuns(nativePtr));
      verify(context).createPlatformExtendedRendererWithPreparation(
          SIGN, "deferred-test", null, completion);
      verify(completion).run();
      verifyNoMoreInteractions(context, completion);
    });
  }

  @Test
  public void rendererTeardownBeforeFirstHostOperationDoesNotFinalize() throws Exception {
    assertTeardownDoesNotFinalize(false);
  }

  @Test
  public void contextTeardownRejectsLateHostCreation() throws Exception {
    assertTeardownDoesNotFinalize(true);
  }

  private void assertTeardownDoesNotFinalize(boolean destroyContext) throws Exception {
    runOnMainSync(() -> {
      assertNativeOnlyWorkIsDeferred();
      assertTrue("Teardown must unregister the renderer",
          nativeTearDownRenderer(nativePtr, destroyContext));
      assertEquals(0, nativePreparationRuns(nativePtr));
      assertFalse(nativeHasPendingPreparations(nativePtr));
      verifyNoInteractions(context, completion);
    });
  }

  // Direct JNI exports live in testing/lynx/android/deferred_renderer_preparation_unittest.cc.
  private static native long nativeCreate(PlatformRendererContext context, Runnable completion);
  private static native boolean nativeCacheLayout(long ptr);
  private static native int nativePreparationRuns(long ptr);
  private static native boolean nativeHasPendingPreparations(long ptr);
  private static native void nativeAddAbortedPreparation(long ptr);
  private static native void nativeConsumeAbortedPreparation(long ptr);
  private static native void nativeUpdateDisplayList(long ptr);
  private static native boolean nativeTearDownRenderer(long ptr, boolean destroyContext);
  private static native void nativeDestroy(long ptr);
}
