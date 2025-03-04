// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import com.lynx.tasm.behavior.shadow.text.TextHelper;
import org.junit.Test;

public class DoubleFormatTest {
  @Test
  public void emailValidator_CorrectEmailSimple_ReturnsTrue() {
    assertTrue(checkFormat(0d));
    assertTrue(checkFormat(0.0d));
    assertTrue(checkFormat(123d));
    assertTrue(checkFormat(1.00000000d));
    assertTrue(checkFormat(0.00000001d));
    assertTrue(checkFormat(2.45001d));
    assertTrue(checkFormat(-12432423d));
    assertTrue(checkFormat(-100d));
    assertTrue(checkFormat(-1d));
    assertTrue(checkFormat(-1.2d));
    assertTrue(checkFormat(-0.0002d));
    assertTrue(checkFormat(-100.0002d));

    assertTrue(checkFormat(127328462764827684268436126348912764398217346d));
    assertTrue(checkFormat(11273284627687276789263947848912764398217346d));
    assertTrue(checkFormat(-111273284627687276789263947848912764398217346d));

    assertFalse(checkFormat(-0d));
  }

  private boolean checkFormat(double num) {
    String v1 = TextHelper.formatDoubleToString(num);
    String v2 = TextHelper.formatDoubleToStringManually(num);
    return v1.equals(v2);
  }
}
