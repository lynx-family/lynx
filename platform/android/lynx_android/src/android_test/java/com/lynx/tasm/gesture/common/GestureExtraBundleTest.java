// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.gesture.common;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class GestureExtraBundleTest {
  private GestureExtraBundle gestureExtraBundle;

  @Before
  public void setUp() {
    gestureExtraBundle = new GestureExtraBundle();
  }

  @Test
  public void testInitialState() {
    assertEquals(
        "Initial gesture direction should be 0", 0, gestureExtraBundle.getGestureDirection());
    assertEquals("Initial simultaneousDeltaX should be 0", 0,
        gestureExtraBundle.getSimultaneousDeltaX(), 0.0f);
    assertEquals("Initial simultaneousDeltaY should be 0", 0,
        gestureExtraBundle.getSimultaneousDeltaY(), 0.0f);
    assertFalse(
        "Initial isConsumedGesture should be false", gestureExtraBundle.isConsumedGesture());
  }

  @Test
  public void testGestureDirection() {
    gestureExtraBundle.setGestureDirection(1);
    assertEquals(
        "Gesture direction should be set to 1", 1, gestureExtraBundle.getGestureDirection());

    gestureExtraBundle.setGestureDirection(-1);
    assertEquals(
        "Gesture direction should be set to -1", -1, gestureExtraBundle.getGestureDirection());
  }

  @Test
  public void testSimultaneousDeltaX() {
    gestureExtraBundle.setSimultaneousDeltaX(10.5f);
    assertEquals("simultaneousDeltaX should be set to 10.5f", 10.5f,
        gestureExtraBundle.getSimultaneousDeltaX(), 0.0f);
  }

  @Test
  public void testSimultaneousDeltaY() {
    gestureExtraBundle.setSimultaneousDeltaY(20.5f);
    assertEquals("simultaneousDeltaY should be set to 20.5f", 20.5f,
        gestureExtraBundle.getSimultaneousDeltaY(), 0.0f);
  }

  @Test
  public void testConsumedGesture() {
    gestureExtraBundle.setConsumedGesture(true);
    assertTrue(
        "isConsumedGesture should be true after setting", gestureExtraBundle.isConsumedGesture());

    gestureExtraBundle.setConsumedGesture(false);
    assertFalse(
        "isConsumedGesture should be false after setting", gestureExtraBundle.isConsumedGesture());
  }

  @Test
  public void testReset() {
    gestureExtraBundle.setGestureDirection(1);
    gestureExtraBundle.setSimultaneousDeltaX(15.0f);
    gestureExtraBundle.setSimultaneousDeltaY(25.0f);
    gestureExtraBundle.setConsumedGesture(true);

    gestureExtraBundle.reset();

    // The reset method does not reset gestureDirection, which seems intentional.
    assertEquals(
        "gestureDirection should not be reset", 1, gestureExtraBundle.getGestureDirection());
    assertEquals("simultaneousDeltaX should be reset to 0", 0,
        gestureExtraBundle.getSimultaneousDeltaX(), 0.0f);
    assertEquals("simultaneousDeltaY should be reset to 0", 0,
        gestureExtraBundle.getSimultaneousDeltaY(), 0.0f);
    assertFalse(
        "isConsumedGesture should be reset to false", gestureExtraBundle.isConsumedGesture());
  }
}
