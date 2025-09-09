// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableType;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class PropBundleTest {
  PropBundle mBundle;

  @Before
  public void setUp() throws Exception {
    mBundle = new PropBundle();
  }

  @After
  public void tearDown() throws Exception {
    mBundle = null;
  }

  @Test
  public void testPutNull() {
    mBundle.putNull(null);

    assertTrue(mBundle.contains(null));
    assertTrue(mBundle.getProps().hasKey(null));
    assertTrue(mBundle.getProps().isNull(null));
    assertEquals(ReadableType.Null, mBundle.getProps().getType(null));

    mBundle.putNull("null");

    assertTrue(mBundle.contains("null"));
    assertTrue(mBundle.getProps().hasKey("null"));
    assertTrue(mBundle.getProps().isNull("null"));
    assertEquals(ReadableType.Null, mBundle.getProps().getType("null"));
  }

  @Test
  public void testPutInt() {
    mBundle.putInt(null, 1);

    assertTrue(mBundle.contains(null));
    assertEquals(ReadableType.Int, mBundle.getProps().getType(null));
    assertEquals(1, mBundle.getProps().getInt(null));

    mBundle.putInt("test", 2);

    assertTrue(mBundle.contains("test"));
    assertTrue(mBundle.getProps().hasKey("test"));
    assertEquals(ReadableType.Int, mBundle.getProps().getType("test"));
    assertEquals(2, mBundle.getProps().getInt("test"));
  }

  @Test
  public void testPutLong() {
    mBundle.putLong(null, 1L);

    assertTrue(mBundle.contains(null));
    assertEquals(ReadableType.Long, mBundle.getProps().getType(null));
    assertEquals(1, mBundle.getProps().getLong(null));

    mBundle.putLong("test", 2L);

    assertTrue(mBundle.contains("test"));
    assertTrue(mBundle.getProps().hasKey("test"));
    assertEquals(ReadableType.Long, mBundle.getProps().getType("test"));
    assertEquals(2, mBundle.getProps().getLong("test"));
  }

  @Test
  public void testPutString() {
    mBundle.putString(null, "null");

    assertTrue(mBundle.contains(null));
    assertTrue(mBundle.getProps().hasKey(null));
    assertEquals(ReadableType.String, mBundle.getProps().getType(null));
    assertEquals("null", mBundle.getProps().getString(null));

    mBundle.putString("test", "value");

    assertTrue(mBundle.contains("test"));
    assertTrue(mBundle.getProps().hasKey("test"));
    assertEquals(ReadableType.String, mBundle.getProps().getType("test"));
    assertEquals("value", mBundle.getProps().getString("test"));
  }

  @Test
  public void testPutDouble() {
    mBundle.putDouble(null, 1);

    assertTrue(mBundle.contains(null));
    assertTrue(mBundle.getProps().hasKey(null));
    assertEquals(ReadableType.Number, mBundle.getProps().getType(null));
    assertEquals(1.0, mBundle.getProps().getDouble(null), 0.0001);

    mBundle.putDouble("test", 1);

    assertTrue(mBundle.contains("test"));
    assertTrue(mBundle.getProps().hasKey("test"));
    assertEquals(ReadableType.Number, mBundle.getProps().getType("test"));
    assertEquals(1.0, mBundle.getProps().getDouble("test"), 0.0001);
  }

  @Test
  public void testPutBool() {
    mBundle.putBool(null, true);

    assertTrue(mBundle.contains(null));
    assertTrue(mBundle.getProps().hasKey(null));
    assertEquals(ReadableType.Boolean, mBundle.getProps().getType(null));
    assertTrue(mBundle.getProps().getBoolean(null));

    mBundle.putBool("test", true);

    assertTrue(mBundle.contains("test"));
    assertTrue(mBundle.getProps().hasKey("test"));
    assertEquals(ReadableType.Boolean, mBundle.getProps().getType("test"));
    assertTrue(mBundle.getProps().getBoolean("test"));
  }

  @Test
  public void testPutMap() {
    JavaOnlyMap map0 = new JavaOnlyMap();
    mBundle.putMap(null, map0);

    assertTrue(mBundle.contains(null));
    assertTrue(mBundle.getProps().hasKey(null));
    assertEquals(ReadableType.Map, mBundle.getProps().getType(null));
    assertEquals(map0, mBundle.getProps().getMap(null));

    JavaOnlyMap map1 = new JavaOnlyMap();
    mBundle.putMap("test", map1);

    assertTrue(mBundle.contains("test"));
    assertTrue(mBundle.getProps().hasKey("test"));
    assertEquals(ReadableType.Map, mBundle.getProps().getType("test"));
    assertEquals(map1, mBundle.getProps().getMap("test"));
  }

  @Test
  public void testPutArray() {
    JavaOnlyArray array0 = new JavaOnlyArray();
    mBundle.putArray(null, array0);

    assertTrue(mBundle.contains(null));
    assertTrue(mBundle.getProps().hasKey(null));
    assertEquals(ReadableType.Array, mBundle.getProps().getType(null));
    assertEquals(array0, mBundle.getProps().getArray(null));

    JavaOnlyArray array1 = new JavaOnlyArray();
    mBundle.putArray("test", array1);

    assertTrue(mBundle.contains("test"));
    assertTrue(mBundle.getProps().hasKey("test"));
    assertEquals(ReadableType.Array, mBundle.getProps().getType("test"));
    assertEquals(array1, mBundle.getProps().getArray("test"));
  }

  @Test
  public void testPutEventHandler() {
    JavaOnlyMap map = new JavaOnlyMap();
    mBundle.putEventHandler(map);

    assertEquals(1, mBundle.getEventHandlers().size());
  }

  @Test
  public void testPutGesture() {
    JavaOnlyMap map = new JavaOnlyMap();
    mBundle.putGesture(map);

    assertEquals(1, mBundle.getGestures().size());
  }

  @Test
  public void testShallowCopy() {
    mBundle.putString("string", "string");
    mBundle.putInt("int", 1);
    mBundle.putDouble("double", 1.0);
    mBundle.putNull("null");

    JavaOnlyMap map = new JavaOnlyMap();
    mBundle.putEventHandler(map);
    mBundle.putGesture(map);

    PropBundle bundle = mBundle.shallowCopy();
    assertEquals(bundle.getProps(), mBundle.getProps());
    assertEquals(bundle.getEventHandlers(), mBundle.getEventHandlers());
    assertEquals(bundle.getGestures(), mBundle.getGestures());

    bundle.putMap("map", map);
    bundle.putEventHandler(map);
    bundle.putGesture(map);
    assertNotEquals(bundle.getProps(), mBundle.getProps());
    assertNotEquals(bundle.getEventHandlers(), mBundle.getEventHandlers());
    assertNotEquals(bundle.getGestures(), mBundle.getGestures());
  }
}
