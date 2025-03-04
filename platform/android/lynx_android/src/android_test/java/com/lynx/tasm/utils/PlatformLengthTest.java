// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.utils;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;

import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.ui.utils.PlatformLength;
import org.junit.Before;
import org.junit.Test;

public class PlatformLengthTest {
  private PlatformLength mLength1;
  private PlatformLength mLength2;
  private PlatformLength mLength3;
  private PlatformLength mLength4;

  @Before
  public void setUp() {
    JavaOnlyArray array = new JavaOnlyArray();
    array.pushDouble(0);
    array.pushDouble(0.5);
    JavaOnlyArray calc = new JavaOnlyArray();
    calc.pushDouble(100);
    calc.pushInt(StyleConstants.PLATFORM_LENGTH_UNIT_NUMBER);
    array.pushArray(calc);

    mLength1 = new PlatformLength(array.getDynamic(0), StyleConstants.PLATFORM_LENGTH_UNIT_NUMBER);
    mLength2 = new PlatformLength(array.getDynamic(1), StyleConstants.PLATFORM_LENGTH_UNIT_PERCENT);
    mLength3 = new PlatformLength(array.getDynamic(2), StyleConstants.PLATFORM_LENGTH_UNIT_CALC);
    mLength4 = new PlatformLength(array.getDynamic(2), StyleConstants.PLATFORM_LENGTH_UNIT_CALC);
  }

  @Test
  public void getValue() {
    assertEquals(0, mLength1.getValue(100), 0.01);
    assertEquals(50, mLength2.getValue(100), 0.01);
    assertEquals(100, mLength3.getValue(0), 0.01);
  }

  @Test
  public void isZero() {
    assertTrue(mLength1.isZero());
    assertFalse(mLength2.isZero());
    assertFalse(mLength3.isZero());
  }

  @Test
  public void equals() {
    assertNotEquals(mLength1, mLength2);
    assertNotEquals(mLength3, mLength2);
    assertEquals(mLength3, mLength4);
  }
}
