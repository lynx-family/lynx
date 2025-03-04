// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.utils;

import static org.junit.Assert.*;

import org.junit.Test;

public class FloatUtilsTest {
  @Test
  public void floatsEqual() {
    assertTrue(FloatUtils.floatsEqual(1f, 1f));
    assertTrue(FloatUtils.floatsEqual(1f, 1.000001f));
    assertFalse(FloatUtils.floatsEqual(1f, 1.00001f));
    assertFalse(FloatUtils.floatsEqual(Float.POSITIVE_INFINITY, Float.NaN));
    assertTrue(FloatUtils.floatsEqual(Float.NaN, Float.NaN));
    assertFalse(FloatUtils.floatsEqual(Float.POSITIVE_INFINITY, Float.POSITIVE_INFINITY));
    assertFalse(FloatUtils.floatsEqual(Float.POSITIVE_INFINITY, 1f));
  }

  @Test
  public void isContainOnlyZero() {
    float[] arrayWithOne = {1f, 0f, 1f, 0f};
    assertFalse(FloatUtils.isContainOnlyZero(arrayWithOne));
    float[] arrayOnlyZero = {0f, 0f, 0f, 0f};
    assertTrue(FloatUtils.isContainOnlyZero(arrayOnlyZero));
    float[] arrayEmpty = {};
    assertTrue(FloatUtils.isContainOnlyZero(arrayEmpty));
  }

  @Test
  public void sanitizeFloatPropertyValue() {
    assertEquals(
        Float.MAX_VALUE, FloatUtils.sanitizeFloatPropertyValue(Float.POSITIVE_INFINITY), 0);
    assertEquals(
        -Float.MAX_VALUE, FloatUtils.sanitizeFloatPropertyValue(Float.NEGATIVE_INFINITY), 0);
    assertEquals(0, FloatUtils.sanitizeFloatPropertyValue(Float.NaN), 0);
    assertEquals(1.0f, FloatUtils.sanitizeFloatPropertyValue(1.0f), 0);
  }

  @Test
  public void getValue() {
    assertEquals(1.0, FloatUtils.getValue("1.0"), 0.0001);
    assertEquals(0.1, FloatUtils.getValue("10.0%"), 0.0001);
    assertEquals(-0.1, FloatUtils.getValue("-10.0%"), 0.0001);
    assertEquals(Float.NaN, FloatUtils.getValue("%"), 0.0001);
    assertEquals(Float.NaN, FloatUtils.getValue("error"), 0.0001);
  }
}
