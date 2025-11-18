// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.background;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.mockito.Mockito.mock;

import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.Shader;
import com.lynx.react.bridge.JavaOnlyArray;
import org.junit.Test;

public class BackgroundConicGradientLayerTest {
  @Test
  public void testConstructorWithValidArray() {
    // Create test data: [angle, center, colors, stops]
    JavaOnlyArray array = new JavaOnlyArray();

    // angle
    array.pushDouble(45.0);

    // center: [x_value, x_type, y_value, y_type]
    JavaOnlyArray center = new JavaOnlyArray();
    center.pushDouble(50); // x value
    center.pushInt(0); // x type (0 = number)
    center.pushDouble(50); // y value
    center.pushInt(0); // y type (0 = number)
    array.pushArray(center);

    // colors
    JavaOnlyArray colors = new JavaOnlyArray();
    colors.pushLong(0xFFFF0000L); // red
    colors.pushLong(0xFF00FF00L); // green
    colors.pushLong(0xFF0000FFL); // blue
    array.pushArray(colors);

    // stops
    JavaOnlyArray stops = new JavaOnlyArray();
    stops.pushDouble(0);
    stops.pushDouble(50);
    stops.pushDouble(100);
    array.pushArray(stops);

    BackgroundConicGradientLayer layer = new BackgroundConicGradientLayer(array);

    assertNotNull(layer);
  }

  @Test
  public void testConstructorWithNullArray() {
    BackgroundConicGradientLayer layer = new BackgroundConicGradientLayer(null);
    assertNotNull(layer);
  }

  @Test
  public void testConstructorWithInvalidArraySize() {
    JavaOnlyArray array = new JavaOnlyArray();
    array.pushDouble(45.0); // Only angle, missing other required elements

    BackgroundConicGradientLayer layer = new BackgroundConicGradientLayer(array);
    assertNotNull(layer);
  }

  @Test
  public void testSetBoundsWithValidData() {
    // Create test data
    JavaOnlyArray array = new JavaOnlyArray();
    array.pushDouble(45.0); // angle

    // center
    JavaOnlyArray center = new JavaOnlyArray();
    center.pushDouble(50); // x value
    center.pushInt(0); // x type
    center.pushDouble(50); // y value
    center.pushInt(0); // y type
    array.pushArray(center);

    // colors
    JavaOnlyArray colors = new JavaOnlyArray();
    colors.pushLong(0xFFFF0000L); // red
    colors.pushLong(0xFF00FF00L); // green
    array.pushArray(colors);

    // stops
    JavaOnlyArray stops = new JavaOnlyArray();
    stops.pushDouble(0);
    stops.pushDouble(100);
    array.pushArray(stops);

    BackgroundConicGradientLayer layer = new BackgroundConicGradientLayer(array);

    // Set bounds
    Rect bounds = new Rect(0, 0, 100, 100);
    layer.setBounds(bounds);

    // Verify bounds were set
    assertEquals(100, layer.getImageWidth());
    assertEquals(100, layer.getImageHeight());
  }

  @Test
  public void testSetBoundsWithInvalidColors() {
    // Create test data with only one color (invalid for gradient)
    JavaOnlyArray array = new JavaOnlyArray();
    array.pushDouble(45.0); // angle

    // center
    JavaOnlyArray center = new JavaOnlyArray();
    center.pushDouble(50); // x value
    center.pushInt(0); // x type
    center.pushDouble(50); // y value
    center.pushInt(0); // y type
    array.pushArray(center);

    // colors (only one color)
    JavaOnlyArray colors = new JavaOnlyArray();
    colors.pushLong(0xFFFF0000L); // red only
    array.pushArray(colors);

    // stops
    JavaOnlyArray stops = new JavaOnlyArray();
    stops.pushDouble(0);
    array.pushArray(stops);

    BackgroundConicGradientLayer layer = new BackgroundConicGradientLayer(array);

    // Set bounds
    Rect bounds = new Rect(0, 0, 100, 100);
    layer.setBounds(bounds);

    assertEquals(100, layer.getImageWidth());
    assertEquals(100, layer.getImageHeight());
    Shader shader = layer.getShader();
    assertNull(shader);
  }

  @Test
  public void testGetShader() {
    // Create test data
    JavaOnlyArray array = new JavaOnlyArray();
    array.pushDouble(45.0); // angle

    // center
    JavaOnlyArray center = new JavaOnlyArray();
    center.pushDouble(50); // x value
    center.pushInt(0); // x type
    center.pushDouble(50); // y value
    center.pushInt(0); // y type
    array.pushArray(center);

    // colors
    JavaOnlyArray colors = new JavaOnlyArray();
    colors.pushLong(0xFFFF0000L); // red
    colors.pushLong(0xFF00FF00L); // green
    array.pushArray(colors);

    // stops
    JavaOnlyArray stops = new JavaOnlyArray();
    stops.pushDouble(0);
    stops.pushDouble(100);
    array.pushArray(stops);

    BackgroundConicGradientLayer layer = new BackgroundConicGradientLayer(array);

    // Set bounds to create shader
    Rect bounds = new Rect(0, 0, 100, 100);
    layer.setBounds(bounds);

    // Get shader
    Shader shader = layer.getShader();
    assertNotNull(shader);
  }

  @Test
  public void testDraw() {
    // Create test data
    JavaOnlyArray array = new JavaOnlyArray();
    array.pushDouble(45.0); // angle

    // center
    JavaOnlyArray center = new JavaOnlyArray();
    center.pushDouble(50); // x value
    center.pushInt(0); // x type
    center.pushDouble(50); // y value
    center.pushInt(0); // y type
    array.pushArray(center);

    // colors
    JavaOnlyArray colors = new JavaOnlyArray();
    colors.pushLong(0xFFFF0000L); // red
    colors.pushLong(0xFF00FF00L); // green
    array.pushArray(colors);

    // stops
    JavaOnlyArray stops = new JavaOnlyArray();
    stops.pushDouble(0);
    stops.pushDouble(100);
    array.pushArray(stops);

    BackgroundConicGradientLayer layer = new BackgroundConicGradientLayer(array);

    Rect bounds = new Rect(0, 0, 100, 100);
    layer.setBounds(bounds);

    Canvas canvas = mock(Canvas.class);
    layer.draw(canvas);
  }
}
