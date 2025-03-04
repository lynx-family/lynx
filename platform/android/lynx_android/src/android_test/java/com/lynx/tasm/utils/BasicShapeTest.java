// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.utils;

import static org.junit.Assert.*;

import android.util.DisplayMetrics;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.ui.shapes.BasicShape;
import org.junit.Test;
public class BasicShapeTest {
  @Test
  public void getPath() {
    // Unknown should be null
    BasicShape shape = new BasicShape(StyleConstants.BASIC_SHAPE_TYPE_UNKNOWN);
    assertNull(shape.getPath(100, 100));

    shape = new BasicShape("a b c d e", new DisplayMetrics().scaledDensity);
    assertNull(shape.getPath(100, 100));

    shape = new BasicShape(
        "M 0 200 L 0,75 A 5,5 0,0,1 150,75 L 200 200 z", new DisplayMetrics().scaledDensity);
    assertNotNull(shape.getPath(100, 100));

    // params array null check
    shape = new BasicShape(StyleConstants.BASIC_SHAPE_TYPE_CIRCLE);
    assertNotNull(shape.getPath(100, 100));
    assertTrue(shape.getPath(100, 100).isEmpty());

    // valid params
    shape = new BasicShape(StyleConstants.BASIC_SHAPE_TYPE_CIRCLE);
    shape.params = new BasicShape.Length[3];
    shape.params[0] = new BasicShape.Length(100, StyleConstants.PLATFORM_LENGTH_UNIT_PERCENT);
    shape.params[1] = new BasicShape.Length(100, StyleConstants.PLATFORM_LENGTH_UNIT_NUMBER);
    shape.params[2] = new BasicShape.Length(100, StyleConstants.PLATFORM_LENGTH_UNIT_NUMBER);
    assertNotNull(shape.getPath(100, 100));
    assertFalse(shape.getPath(100, 100).isEmpty());

    // params value null check, should not crash, and should be empty.
    shape = new BasicShape(StyleConstants.BASIC_SHAPE_TYPE_CIRCLE);
    shape.params = new BasicShape.Length[3];
    shape.params[0] = null;
    shape.params[1] = null;
    shape.params[2] = null;
    assertNotNull(shape.getPath(100, 100));
    assertTrue(shape.getPath(100, 100).isEmpty());
  }
  @Test
  public void getSuperEllipsePath() {
    BasicShape shape = new BasicShape(StyleConstants.BASIC_SHAPE_TYPE_SUPER_ELLIPSE);
    assertNotNull(shape.getPath(100, 100));
    assertTrue(shape.getPath(100, 100).isEmpty());

    // value null test, should not crash and should be empty.
    shape = new BasicShape(StyleConstants.BASIC_SHAPE_TYPE_SUPER_ELLIPSE);
    shape.params = new BasicShape.Length[4];
    shape.exponents = new double[2];
    assertTrue(shape.getPath(100, 100).isEmpty());

    // valid values.
    shape = new BasicShape(StyleConstants.BASIC_SHAPE_TYPE_SUPER_ELLIPSE);
    shape.params = new BasicShape.Length[4];
    shape.params[0] = new BasicShape.Length(100, StyleConstants.PLATFORM_LENGTH_UNIT_PERCENT);
    shape.params[1] = new BasicShape.Length(100, StyleConstants.PLATFORM_LENGTH_UNIT_NUMBER);
    shape.params[2] = new BasicShape.Length(100, StyleConstants.PLATFORM_LENGTH_UNIT_NUMBER);
    shape.params[3] = new BasicShape.Length(100, StyleConstants.PLATFORM_LENGTH_UNIT_NUMBER);
    shape.exponents = new double[2];
    shape.exponents[0] = 2;
    shape.exponents[1] = 2;
    assertNotNull(shape.getPath(100, 100));
    assertFalse(shape.getPath(100, 100).isEmpty());
  }
  @Test
  public void getEllipsePath() {
    BasicShape shape = new BasicShape(StyleConstants.BASIC_SHAPE_TYPE_ELLIPSE);
    assertNotNull(shape.getPath(100, 100));
    assertTrue(shape.getPath(100, 100).isEmpty());

    // value null test, should not crash and should be empty.
    shape = new BasicShape(StyleConstants.BASIC_SHAPE_TYPE_ELLIPSE);
    shape.params = new BasicShape.Length[4];
    assertTrue(shape.getPath(100, 100).isEmpty());

    // valid values.
    shape = new BasicShape(StyleConstants.BASIC_SHAPE_TYPE_ELLIPSE);
    shape.params = new BasicShape.Length[4];
    shape.params[0] = new BasicShape.Length(100, StyleConstants.PLATFORM_LENGTH_UNIT_PERCENT);
    shape.params[1] = new BasicShape.Length(100, StyleConstants.PLATFORM_LENGTH_UNIT_NUMBER);
    shape.params[2] = new BasicShape.Length(100, StyleConstants.PLATFORM_LENGTH_UNIT_NUMBER);
    shape.params[3] = new BasicShape.Length(100, StyleConstants.PLATFORM_LENGTH_UNIT_NUMBER);
    assertNotNull(shape.getPath(100, 100));
    assertFalse(shape.getPath(100, 100).isEmpty());
  }
  @Test
  public void getInsetPath() {
    BasicShape shape = new BasicShape(StyleConstants.BASIC_SHAPE_TYPE_INSET);
    assertNotNull(shape.getPath(100, 100));
    assertTrue(shape.getPath(100, 100).isEmpty());

    // value null test, should not crash and should be empty.
    shape = new BasicShape(StyleConstants.BASIC_SHAPE_TYPE_INSET);
    shape.params = new BasicShape.Length[4];
    assertTrue(shape.getPath(100, 100).isEmpty());

    // valid values.
    JavaOnlyArray array = new JavaOnlyArray();
    array.pushInt(StyleConstants.BASIC_SHAPE_TYPE_INSET);
    // Rect
    for (int i = 0; i < 4; i++) {
      array.pushDouble(30);
      array.pushInt(1);
    }
    shape = BasicShape.CreateFromReadableArray(array, new DisplayMetrics().scaledDensity);
    assertNotNull(shape);
    assertNotNull(shape.getPath(100, 100));
    assertFalse(shape.getPath(100, 100).isEmpty());
    // rounded corner
    for (int i = 0; i < 8; i++) {
      array.pushDouble(30);
      array.pushInt(1);
    }
    shape = BasicShape.CreateFromReadableArray(array, new DisplayMetrics().scaledDensity);
    assertNotNull(shape);
    assertNotNull(shape.getPath(100, 100));
    assertFalse(shape.getPath(100, 100).isEmpty());
    // super ellipse corner
    array.clear();
    array.pushInt(StyleConstants.BASIC_SHAPE_TYPE_INSET);
    for (int i = 0; i < 4; i++) {
      // four inset
      array.pushDouble(30);
      array.pushInt(1);
    }
    // two exponent
    array.pushDouble(3.0);
    array.pushDouble(3.0);
    // eight border radius
    for (int i = 0; i < 8; i++) {
      array.pushDouble(30);
      array.pushInt(1);
    }
    shape = BasicShape.CreateFromReadableArray(array, new DisplayMetrics().scaledDensity);
    assertNotNull(shape);
    assertNotNull(shape.getPath(100, 100));
    assertFalse(shape.getPath(100, 100).isEmpty());
  }

  @Test
  public void createShape() {
    JavaOnlyArray array = new JavaOnlyArray();
    array.pushInt(StyleConstants.BASIC_SHAPE_TYPE_UNKNOWN);
    BasicShape shape =
        BasicShape.CreateFromReadableArray(array, new DisplayMetrics().scaledDensity);
    assertNull(shape);
    array.clear();

    array.pushInt(StyleConstants.BASIC_SHAPE_TYPE_CIRCLE);
    shape = BasicShape.CreateFromReadableArray(array, new DisplayMetrics().scaledDensity);
    assertNull(shape);
    array.pushDouble(30);
    array.pushInt(1);
    array.pushDouble(30);
    array.pushInt(1);
    array.pushDouble(30);
    array.pushInt(1);
    shape = BasicShape.CreateFromReadableArray(array, new DisplayMetrics().scaledDensity);
    assertNotNull(shape);
    assertNotNull(shape.getPath(100, 100));

    array.clear();
    array.pushInt(StyleConstants.BASIC_SHAPE_TYPE_ELLIPSE);
    array.pushDouble(30);
    array.pushInt(1);
    array.pushDouble(30);
    array.pushInt(1);
    array.pushDouble(30);
    array.pushInt(1);
    shape = BasicShape.CreateFromReadableArray(array, new DisplayMetrics().scaledDensity);
    assertNull(shape);
    array.pushDouble(30);
    array.pushInt(1);
    shape = BasicShape.CreateFromReadableArray(array, new DisplayMetrics().scaledDensity);
    assertNotNull(shape);
    assertNotNull(shape.getPath(100, 100));
    assertFalse(shape.getPath(100, 100).isEmpty());

    array.clear();
    array.pushInt(StyleConstants.BASIC_SHAPE_TYPE_SUPER_ELLIPSE);
    array.pushDouble(30);
    array.pushInt(1);
    array.pushDouble(30);
    array.pushInt(1);
    array.pushDouble(30);
    array.pushInt(1);
    shape = BasicShape.CreateFromReadableArray(array, new DisplayMetrics().scaledDensity);
    assertNull(shape);
    array.pushDouble(30);
    array.pushInt(1);
    shape = BasicShape.CreateFromReadableArray(array, new DisplayMetrics().scaledDensity);
    assertNull(shape);
    array.pushDouble(30);
    array.pushInt(1);
    shape = BasicShape.CreateFromReadableArray(array, new DisplayMetrics().scaledDensity);
    assertNotNull(shape);
    assertNotNull(shape.getPath(100, 100));
    assertFalse(shape.getPath(100, 100).isEmpty());

    array.clear();
    array.pushInt(StyleConstants.BASIC_SHAPE_TYPE_INSET);
    array.pushDouble(30);
    array.pushInt(1);
    array.pushDouble(30);
    array.pushInt(1);
    array.pushDouble(30);
    array.pushInt(1);
    shape = BasicShape.CreateFromReadableArray(array, new DisplayMetrics().scaledDensity);
    assertNull(shape);
    array.pushDouble(30);
    array.pushInt(1);
    shape = BasicShape.CreateFromReadableArray(array, new DisplayMetrics().scaledDensity);
    assertNotNull(shape);
    assertEquals(4, shape.params.length);
    array.pushDouble(30);
    array.pushInt(1);
    array.pushDouble(30);
    array.pushInt(1);
    array.pushDouble(30);
    array.pushInt(1);
    array.pushDouble(30);
    array.pushInt(1);
    array.pushDouble(30);
    array.pushInt(1);
    array.pushDouble(30);
    array.pushInt(1);
    array.pushDouble(30);
    array.pushInt(1);
    array.pushDouble(30);
    array.pushInt(1);
    shape = BasicShape.CreateFromReadableArray(array, new DisplayMetrics().scaledDensity);
    assertNotNull(shape);
  }
}
