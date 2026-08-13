// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.render;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;

import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import org.junit.Test;
import org.mockito.ArgumentCaptor;

public class NativePaintingContextInputEventTest {
  @Test
  public void testPointerMetadataUsesSevenValueStride() throws Exception {
    NativePaintingContext paintingContext = createPaintingContext();
    MotionEvent event = MotionEvent.obtain(0, 1, MotionEvent.ACTION_DOWN, 12, 34, 0);

    assertTrue(paintingContext.dispatchPlatformMotionEvent(event, 17));

    ArgumentCaptor<int[]> integerData = ArgumentCaptor.forClass(int[].class);
    ArgumentCaptor<float[]> floatData = ArgumentCaptor.forClass(float[].class);
    verify(paintingContext)
        .nativeDispatchPlatformInputEvent(anyLong(), integerData.capture(), floatData.capture());
    assertArrayEquals(new int[] {0, MotionEvent.ACTION_DOWN, event.getSource(), 1, 17},
        integerData.getValue());
    assertArrayEquals(new float[] {0, 12, 34, 0, 1, 0, 1}, floatData.getValue(), 0);
    event.recycle();
  }

  @Test
  public void testKeyEventIncludesUtf16KeyPayload() throws Exception {
    NativePaintingContext paintingContext = createPaintingContext();
    KeyEvent event = new KeyEvent(0, 1, KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_A, 1,
        KeyEvent.META_ALT_ON | KeyEvent.META_SHIFT_ON, -1, 0, 0);

    assertTrue(paintingContext.dispatchPlatformKeyEvent(event, "😀", 17));

    ArgumentCaptor<int[]> integerData = ArgumentCaptor.forClass(int[].class);
    ArgumentCaptor<float[]> floatData = ArgumentCaptor.forClass(float[].class);
    verify(paintingContext)
        .nativeDispatchPlatformInputEvent(anyLong(), integerData.capture(), floatData.capture());
    assertArrayEquals(new int[] {1, 0, KeyEvent.KEYCODE_A, 1, 17, 2, 0xD83D, 0xDE00},
        integerData.getValue());
    assertArrayEquals(new float[] {1, 0, 1, 0}, floatData.getValue(), 0);
  }

  @Test
  public void testStylusHoverHasNoPressedButtons() throws Exception {
    NativePaintingContext paintingContext = createPaintingContext();
    MotionEvent.PointerProperties properties = new MotionEvent.PointerProperties();
    properties.id = 0;
    properties.toolType = MotionEvent.TOOL_TYPE_STYLUS;
    MotionEvent.PointerCoords coordinates = new MotionEvent.PointerCoords();
    coordinates.x = 12;
    coordinates.y = 34;
    MotionEvent event = MotionEvent.obtain(0, 1, MotionEvent.ACTION_HOVER_MOVE, 1,
        new MotionEvent.PointerProperties[] {properties},
        new MotionEvent.PointerCoords[] {coordinates}, 0, 0, 1, 1, 0, 0,
        InputDevice.SOURCE_STYLUS, 0);

    assertTrue(paintingContext.dispatchPlatformMotionEvent(event, 17));

    ArgumentCaptor<float[]> floatData = ArgumentCaptor.forClass(float[].class);
    verify(paintingContext)
        .nativeDispatchPlatformInputEvent(anyLong(), any(int[].class), floatData.capture());
    assertArrayEquals(new float[] {0, 12, 34, 2, 1, -1, 0}, floatData.getValue(), 0);
    event.recycle();
  }

  @Test
  public void testWheelDispatchesBeginUpdateEnd() throws Exception {
    NativePaintingContext paintingContext = createPaintingContext();
    List<Integer> actions = new ArrayList<>();
    doAnswer(invocation -> {
      actions.add(((int[]) invocation.getArguments()[1])[1]);
      return false;
    })
        .when(paintingContext)
        .nativeDispatchPlatformInputEvent(anyLong(), any(int[].class), any(float[].class));
    MotionEvent.PointerProperties properties = new MotionEvent.PointerProperties();
    properties.id = 0;
    properties.toolType = MotionEvent.TOOL_TYPE_MOUSE;
    MotionEvent.PointerCoords coordinates = new MotionEvent.PointerCoords();
    coordinates.x = 12;
    coordinates.y = 34;
    coordinates.setAxisValue(MotionEvent.AXIS_VSCROLL, 1);
    MotionEvent event = MotionEvent.obtain(0, 1, MotionEvent.ACTION_SCROLL, 1,
        new MotionEvent.PointerProperties[] {properties},
        new MotionEvent.PointerCoords[] {coordinates}, 0, 0, 1, 1, 0, 0,
        InputDevice.SOURCE_MOUSE, 0);

    assertFalse(paintingContext.dispatchPlatformMotionEvent(event, 17));

    assertEquals(Arrays.asList(0, 1, 2), actions);
    event.recycle();
  }

  private NativePaintingContext createPaintingContext() throws Exception {
    NativePaintingContext paintingContext =
        mock(NativePaintingContext.class, CALLS_REAL_METHODS);
    Field nativePtr = NativePaintingContext.class.getDeclaredField("mNativePtr");
    nativePtr.setAccessible(true);
    nativePtr.setLong(paintingContext, 1);
    Field primaryPointerId =
        NativePaintingContext.class.getDeclaredField("mPrimaryPointerId");
    primaryPointerId.setAccessible(true);
    primaryPointerId.setInt(paintingContext, -1);
    Field context = NativePaintingContext.class.getDeclaredField("mContext");
    context.setAccessible(true);
    context.set(paintingContext, TestingUtils.getLynxContext());
    doReturn(true)
        .when(paintingContext)
        .nativeDispatchPlatformInputEvent(anyLong(), any(int[].class), any(float[].class));
    return paintingContext;
  }
}
