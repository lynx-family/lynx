// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import org.junit.Before;
import org.junit.Test;

public class LynxGroupTest {
  @Before
  public void setUp() {}

  @Test
  public void testCreate() {
    LynxGroup.LynxGroupBuilder builder = new LynxGroup.LynxGroupBuilder();
    builder.setID("test_id");
    builder.setStringConfig("string_key", "string_value");
    builder.setBoolConfig("bool_key", true);
    builder.setEnableV8(true);

    LynxGroup group = builder.build();

    boolean boolValue = group.getBoolConfig("bool_key");
    String stringValue = group.getStringConfig("string_key");
    assertTrue(boolValue);
    assertTrue(group.enableV8());
    assertEquals(stringValue, "string_value");
    assertEquals(group.getID(), "test_id");
  }
}
