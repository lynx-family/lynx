// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import com.lynx.react.bridge.JavaOnlyMap;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class StylesDiffMapTest {
  @Before
  public void setUp() throws Exception {}

  @After
  public void tearDown() throws Exception {}

  @Test
  public void testIsEmpty() {
    StylesDiffMap map0 = new StylesDiffMap(null);
    assertTrue(map0.isEmpty());

    StylesDiffMap map1 = new StylesDiffMap(new JavaOnlyMap());
    assertTrue(map1.isEmpty());

    JavaOnlyMap onlyMap = new JavaOnlyMap();
    onlyMap.put("test", 1);
    StylesDiffMap map2 = new StylesDiffMap(onlyMap);
    assertFalse(map2.isEmpty());
  }
}
