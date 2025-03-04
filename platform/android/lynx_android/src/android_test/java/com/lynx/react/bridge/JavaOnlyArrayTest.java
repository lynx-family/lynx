// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.react.bridge;

import static org.junit.Assert.*;

import org.junit.Before;
import org.junit.Test;

public class JavaOnlyArrayTest {
  @Before
  public void setUp() throws Exception {}

  @Test
  public void shallowCopy() {
    JavaOnlyArray array = new JavaOnlyArray();
    array.pushNull();
    array.pushMap(new JavaOnlyMap());
    array.pushArray(new JavaOnlyArray());
    array.pushBoolean(true);
    byte[] byteArray = {0x12, 0x34, 0x56, 0x78};
    array.pushByteArray(byteArray);
    array.pushDouble(0.01);
    array.pushInt(6);
    array.pushLong(666);
    array.pushString("test");
    JavaOnlyArray shallowCopyArray = JavaOnlyArray.shallowCopy(array);
    assertTrue(shallowCopyArray != array);
    assertTrue(shallowCopyArray.getMap(1) == array.getMap(1));
    assertTrue(shallowCopyArray.getArray(2) == array.getArray(2));
    assertTrue(shallowCopyArray.getBoolean(3) == true);
    assertTrue(shallowCopyArray.getByteArray(4)[0] == 0x12);
    assertTrue(shallowCopyArray.getDouble(5) == array.getDouble(5));
    assertTrue(shallowCopyArray.getString(8) == array.getString(8));
  }
}
