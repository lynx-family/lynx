// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import java.lang.reflect.Method;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxHostScriptViewHooksTest {
  private Method mSetEnabled;
  private boolean mWasEnabled;
  private final AtomicInteger mNotifications = new AtomicInteger();

  @Before
  public void setUp() throws Exception {
    mSetEnabled = LynxHostScriptViewHooks.class.getDeclaredMethod("setEnabled", boolean.class);
    mSetEnabled.setAccessible(true);
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      mWasEnabled = LynxHostScriptViewHooks.isEnabled();
      setEnabled(false);
    });
  }

  @After
  public void tearDown() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> setEnabled(mWasEnabled));
  }

  @Test
  public void queuedCreationIsDiscardedAfterDisableAndReenable() {
    queueCreation(true, () -> {
      setEnabled(false);
      setEnabled(true);
    });
    assertEquals(0, mNotifications.get());
  }

  @Test
  public void queuedCreationIsDiscardedAfterDisable() {
    queueCreation(true, () -> setEnabled(false));
    assertEquals(0, mNotifications.get());
  }

  @Test
  public void creationWhileDisabledIsNotReplayedAfterEnable() {
    queueCreation(false, () -> setEnabled(true));
    assertEquals(0, mNotifications.get());
  }

  @Test
  public void queuedCreationIsDeliveredWithinEnabledPeriod() {
    queueCreation(true, () -> {});
    assertEquals(1, mNotifications.get());
  }

  @Test
  public void repeatedEnableKeepsQueuedCreation() {
    queueCreation(true, () -> setEnabled(true));
    assertEquals(1, mNotifications.get());
  }

  @Test
  public void newCreationIsDeliveredAfterReenable() {
    queueCreation(true, () -> {
      setEnabled(false);
      setEnabled(true);
    });
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxHostScriptViewHooks.postCreationNotification(mNotifications::incrementAndGet);
      assertEquals(1, mNotifications.get());
    });
    assertEquals(1, mNotifications.get());
  }

  private void queueCreation(boolean enabled, Runnable beforeDelivery) {
    AtomicReference<Throwable> failure = new AtomicReference<>();
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      setEnabled(enabled);
      // Hold the UI queue until the worker has posted and the state has changed.
      Thread worker = new Thread(() -> {
        try {
          LynxHostScriptViewHooks.postCreationNotification(mNotifications::incrementAndGet);
        } catch (Throwable throwable) {
          failure.set(throwable);
        }
      });
      worker.start();
      try {
        worker.join(3000);
      } catch (InterruptedException e) {
        Thread.currentThread().interrupt();
        throw new AssertionError(e);
      }
      assertFalse(worker.isAlive());
      beforeDelivery.run();
    });
    InstrumentationRegistry.getInstrumentation().waitForIdleSync();
    assertNull(failure.get());
  }

  private void setEnabled(boolean enabled) {
    try {
      mSetEnabled.invoke(null, enabled);
    } catch (ReflectiveOperationException e) {
      throw new AssertionError(e);
    }
  }
}
