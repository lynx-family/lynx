// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.react.bridge;

import static org.junit.Assert.*;
import static org.junit.Assert.assertTrue;

import org.junit.Before;
import org.junit.Test;

public class JavaOnlyMapTest {
  @Before
  public void setUp() throws Exception {}

  @Test
  public void shallowCopy() {
    JavaOnlyMap map = new JavaOnlyMap();
    map.putNull("null");
    map.putMap("map", new JavaOnlyMap());
    map.putArray("array", new JavaOnlyArray());
    map.putBoolean("boolean", true);
    byte[] byteArray = {0x12, 0x34, 0x56, 0x78};
    map.putByteArray("byteArray", byteArray);
    map.putDouble("double", 0.01);
    map.putInt("int", 6);
    map.putLong("long", 666);
    map.putPiperData("piperData", PiperData.fromString("{}"));
    map.putString("string", "test");
    JavaOnlyMap shallowCopyMap = JavaOnlyMap.shallowCopy(map);
    assertTrue(shallowCopyMap != map);
    assertTrue(shallowCopyMap.getBoolean("boolean") == true);
    assertTrue(shallowCopyMap.getByteArray("byteArray")[0] == 0x12);
    assertTrue(shallowCopyMap.getMap("map") == map.getMap("map"));
    assertTrue(shallowCopyMap.getArray("array") == map.getArray("array"));
    assertTrue(shallowCopyMap.getDouble("double") == map.getDouble("double"));
    assertTrue(shallowCopyMap.getString("string") == map.getString("string"));
  }
}
