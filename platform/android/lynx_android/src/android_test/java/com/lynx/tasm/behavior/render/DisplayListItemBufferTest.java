// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.app.Application;
import android.content.Context;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.LynxEnv;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class DisplayListItemBufferTest {
  private static final float FLOAT_DELTA = 0.0001f;

  @Before
  public void setUp() {
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    LynxEnv.inst().init((Application) context, null, null, null, null);
    assertTrue(registerJNI());
  }

  @Test
  public void testCppGeneratedDisplayListMatchesAndroidLayout() {
    ByteBuffer items =
        PlatformRendererContext.makeReadOnlyDisplayListBuffer(nativeCreateDisplayListItemsBuffer());
    ByteBuffer data =
        PlatformRendererContext.makeReadOnlyDisplayListBuffer(nativeCreateDisplayListDataBuffer());

    assertNotNull(items);
    assertNotNull(data);
    assertTrue(items.isDirect());
    assertTrue(items.isReadOnly());
    assertTrue(data.isDirect());
    assertTrue(data.isReadOnly());
    items.order(ByteOrder.nativeOrder());
    data.order(ByteOrder.nativeOrder());

    assertEquals(DisplayListApplier.DISPLAY_LIST_ITEM_SIZE, nativeGetDisplayListItemSize());
    assertEquals(12, nativeGetDisplayListItemCount());
    assertEquals(12 * DisplayListApplier.DISPLAY_LIST_ITEM_SIZE, items.capacity());

    assertItemType(items, 0, DisplayListApplier.OP_BEGIN);
    assertEquals(101, getInt(items, 0, DisplayListApplier.BEGIN_ID_OFFSET));
    assertEquals(1, getInt(items, 0, DisplayListApplier.BEGIN_TYPE_OFFSET));
    assertFloatEquals(1.25f, getFloat(items, 0, DisplayListApplier.BEGIN_X_OFFSET));
    assertFloatEquals(2.5f, getFloat(items, 0, DisplayListApplier.BEGIN_Y_OFFSET));
    assertFloatEquals(300.75f, getFloat(items, 0, DisplayListApplier.BEGIN_W_OFFSET));
    assertFloatEquals(400.5f, getFloat(items, 0, DisplayListApplier.BEGIN_H_OFFSET));

    assertItemType(items, 1, DisplayListApplier.OP_FILL);
    assertEquals(0xFFA1B2C3, getInt(items, 1, DisplayListApplier.FILL_COLOR_OFFSET));
    assertEquals(7, getInt(items, 1, DisplayListApplier.FILL_CLIP_INDEX_OFFSET));

    assertItemType(items, 2, DisplayListApplier.OP_DRAW_VIEW);
    assertEquals(202, getInt(items, 2, DisplayListApplier.DRAW_VIEW_ID_OFFSET));
    assertFloatEquals(3.25f, getFloat(items, 2, DisplayListApplier.DRAW_VIEW_OFFSET_X_OFFSET));
    assertFloatEquals(-4.5f, getFloat(items, 2, DisplayListApplier.DRAW_VIEW_OFFSET_Y_OFFSET));

    assertItemType(items, 3, DisplayListApplier.OP_IMAGE);
    assertEquals(303, getInt(items, 3, DisplayListApplier.IMAGE_ID_OFFSET));
    assertEquals(8, getInt(items, 3, DisplayListApplier.IMAGE_BOX_INDEX_OFFSET));

    assertItemType(items, 4, DisplayListApplier.OP_TEXT);
    assertEquals(404, getInt(items, 4, DisplayListApplier.TEXT_ID_OFFSET));
    assertEquals(9, getInt(items, 4, DisplayListApplier.TEXT_BOX_INDEX_OFFSET));

    assertItemType(items, 5, DisplayListApplier.OP_BACKGROUND_IMAGE);
    assertEquals(505, getInt(items, 5, DisplayListApplier.BACKGROUND_IMAGE_ID_OFFSET));
    assertEquals(10, getInt(items, 5, DisplayListApplier.BACKGROUND_IMAGE_TILING_INDEX_OFFSET));
    assertEquals(11, getInt(items, 5, DisplayListApplier.BACKGROUND_IMAGE_CLIP_INDEX_OFFSET));
    assertEquals(1, getInt(items, 5, DisplayListApplier.BACKGROUND_IMAGE_REPEAT_X_OFFSET));
    assertEquals(2, getInt(items, 5, DisplayListApplier.BACKGROUND_IMAGE_REPEAT_Y_OFFSET));

    assertItemType(items, 6, DisplayListApplier.OP_BORDER);
    assertEquals(12, getInt(items, 6, DisplayListApplier.BORDER_OUT_INDEX_OFFSET));
    assertEquals(13, getInt(items, 6, DisplayListApplier.BORDER_INNER_INDEX_OFFSET));
    assertIntArray(items, 6, DisplayListApplier.BORDER_COLORS_OFFSET,
        new int[] {0xFF010203, 0xFF111213, 0xFF212223, 0xFF313233});
    assertIntArray(items, 6, DisplayListApplier.BORDER_STYLES_OFFSET, new int[] {1, 2, 3, 4});

    assertItemType(items, 7, DisplayListApplier.OP_CLIP_RECT);
    assertRect(items, 7, DisplayListApplier.CLIP_RECT_X_OFFSET,
        DisplayListApplier.CLIP_RECT_Y_OFFSET, DisplayListApplier.CLIP_RECT_W_OFFSET,
        DisplayListApplier.CLIP_RECT_H_OFFSET, new float[] {21.25f, 22.5f, 23.75f, 24.5f});
    assertFloatArray(items, 7, DisplayListApplier.CLIP_RECT_RADII_OFFSET,
        new float[] {1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f});
    assertEquals(1, getInt(items, 7, DisplayListApplier.CLIP_RECT_HAS_RADII_OFFSET));

    assertItemType(items, 8, DisplayListApplier.OP_RECORD_BOX);
    assertRect(items, 8, DisplayListApplier.RECORD_BOX_X_OFFSET,
        DisplayListApplier.RECORD_BOX_Y_OFFSET, DisplayListApplier.RECORD_BOX_W_OFFSET,
        DisplayListApplier.RECORD_BOX_H_OFFSET, new float[] {31.25f, 32.5f, 33.75f, 34.5f});
    assertFloatArray(items, 8, DisplayListApplier.RECORD_BOX_RADII_OFFSET,
        new float[] {9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f});
    assertEquals(1, getInt(items, 8, DisplayListApplier.RECORD_BOX_HAS_RADII_OFFSET));

    assertItemType(items, 9, DisplayListApplier.OP_LINEAR_GRADIENT);
    int colorOffset = getInt(items, 9, DisplayListApplier.GRADIENT_COLOR_COUNT_OFFSET_OFFSET);
    int stopOffset = getInt(items, 9, DisplayListApplier.GRADIENT_STOP_COUNT_OFFSET_OFFSET);
    assertEquals(2, getInt(items, 9, DisplayListApplier.GRADIENT_COLOR_COUNT_OFFSET));
    assertEquals(2, getInt(items, 9, DisplayListApplier.GRADIENT_STOP_COUNT_OFFSET));
    assertEquals(14, getInt(items, 9, DisplayListApplier.GRADIENT_TILING_INDEX_OFFSET));
    assertEquals(15, getInt(items, 9, DisplayListApplier.GRADIENT_CLIP_INDEX_OFFSET));
    assertEquals(1, getInt(items, 9, DisplayListApplier.GRADIENT_REPEAT_X_OFFSET));
    assertEquals(0, getInt(items, 9, DisplayListApplier.GRADIENT_REPEAT_Y_OFFSET));
    assertFloatEquals(123.5f, getFloat(items, 9, DisplayListApplier.GRADIENT_ANGLE_OFFSET));
    assertEquals(0xFF414243, data.getInt(colorOffset));
    assertEquals(0xFF515253, data.getInt(colorOffset + Integer.BYTES));
    assertFloatEquals(0.25f, data.getFloat(stopOffset));
    assertFloatEquals(0.75f, data.getFloat(stopOffset + Float.BYTES));

    assertItemType(items, 10, DisplayListApplier.OP_BOX_SHADOW);
    assertEquals(16, getInt(items, 10, DisplayListApplier.BOX_SHADOW_SHADOW_BOX_INDEX_OFFSET));
    assertEquals(17, getInt(items, 10, DisplayListApplier.BOX_SHADOW_CLIP_BOX_INDEX_OFFSET));
    assertEquals(0xCC616263, getInt(items, 10, DisplayListApplier.BOX_SHADOW_COLOR_OFFSET));
    assertFloatEquals(18.5f, getFloat(items, 10, DisplayListApplier.BOX_SHADOW_BLUR_RADIUS_OFFSET));
    assertEquals(1, getInt(items, 10, DisplayListApplier.BOX_SHADOW_CLIP_MODE_OFFSET));

    assertItemType(items, 11, DisplayListApplier.OP_END);
  }

  private static void assertItemType(ByteBuffer buffer, int itemIndex, int expectedType) {
    assertEquals(expectedType, getInt(buffer, itemIndex, DisplayListApplier.TYPE_OFFSET));
  }

  private static void assertRect(ByteBuffer buffer, int itemIndex, int xOffset, int yOffset,
      int widthOffset, int heightOffset, float[] expected) {
    assertFloatEquals(expected[0], getFloat(buffer, itemIndex, xOffset));
    assertFloatEquals(expected[1], getFloat(buffer, itemIndex, yOffset));
    assertFloatEquals(expected[2], getFloat(buffer, itemIndex, widthOffset));
    assertFloatEquals(expected[3], getFloat(buffer, itemIndex, heightOffset));
  }

  private static void assertIntArray(
      ByteBuffer buffer, int itemIndex, int fieldOffset, int[] expected) {
    for (int i = 0; i < expected.length; ++i) {
      assertEquals(expected[i], getInt(buffer, itemIndex, fieldOffset + i * Integer.BYTES));
    }
  }

  private static void assertFloatArray(
      ByteBuffer buffer, int itemIndex, int fieldOffset, float[] expected) {
    for (int i = 0; i < expected.length; ++i) {
      assertFloatEquals(expected[i], getFloat(buffer, itemIndex, fieldOffset + i * Float.BYTES));
    }
  }

  private static int getInt(ByteBuffer buffer, int itemIndex, int fieldOffset) {
    return buffer.getInt(itemIndex * DisplayListApplier.DISPLAY_LIST_ITEM_SIZE + fieldOffset);
  }

  private static float getFloat(ByteBuffer buffer, int itemIndex, int fieldOffset) {
    return buffer.getFloat(itemIndex * DisplayListApplier.DISPLAY_LIST_ITEM_SIZE + fieldOffset);
  }

  private static void assertFloatEquals(float expected, float actual) {
    assertEquals(expected, actual, FLOAT_DELTA);
  }

  private static native boolean registerJNI();
  private native ByteBuffer nativeCreateDisplayListItemsBuffer();
  private native ByteBuffer nativeCreateDisplayListDataBuffer();
  private native int nativeGetDisplayListItemSize();
  private native int nativeGetDisplayListItemCount();
}
