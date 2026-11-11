// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentCaptor.forClass;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyFloat;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Application;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RadialGradient;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Shader;
import android.util.DisplayMetrics;
import android.view.View;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.behavior.shadow.text.TextMeasurer;
import com.lynx.tasm.behavior.shadow.text.TextUpdateBundle;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.image.LynxImageManager;
import com.lynx.tasm.behavior.ui.utils.BorderStyle;
import com.lynx.tasm.behavior.ui.utils.Spacing;
import com.lynx.tasm.utils.DisplayMetricsHolder;
import com.lynx.tasm.utils.PixelUtils;
import java.lang.reflect.Field;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

/** Verifies that {@link DisplayListApplier} consumes typed display-list buffers correctly. */
@RunWith(AndroidJUnit4.class)
public class DisplayListApplierTest {
  private static final int VIEW_TYPE = PlatformRendererContext.PlatformRendererType.kView;

  @Mock private Canvas mockCanvas;
  @Mock private TextMeasurer mockTextMeasurer;
  @Mock private PlatformRendererContext mockPlatformRendererContext;
  @Mock private TextUpdateBundle mockTextUpdateBundle;
  @Mock private android.text.Layout mockTextLayout;
  @Mock private IRendererHost mockRendererHost;
  @Mock private View mockHostView;
  @Mock private LynxImageManager mockImageManager;

  private DisplayListApplier displayListApplier;
  private DisplayListApplier spyDisplayListApplier;
  private NativeDisplayListBuilder testDisplayList;
  private final ArrayList<NativeDisplayListBuilder> nativeDisplayLists = new ArrayList<>();

  // ArgumentCaptors for verifying drawRectangularBorders calls
  private ArgumentCaptor<android.graphics.Rect> boundsCaptor;
  private ArgumentCaptor<int[]> borderWidthsCaptor;
  private ArgumentCaptor<int[]> borderColorsCaptor;
  private ArgumentCaptor<BorderStyle[]> borderStylesCaptor;
  private ArgumentCaptor<RoundedRectangle> outBoxCaptor;
  private ArgumentCaptor<RoundedRectangle> innerBoxCaptor;

  @Before
  public void setUp() {
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    LynxEnv.inst().init((Application) context, null, null, null, null);
    NativeDisplayListBuilder.ensureRegistered();

    MockitoAnnotations.openMocks(this);
    // Set up PlatformRendererContext to return our mock TextMeasurer
    when(mockPlatformRendererContext.getTextMeasurer()).thenReturn(mockTextMeasurer);
    when(mockRendererHost.getView()).thenReturn(mockHostView);
    displayListApplier =
        new DisplayListApplier(null, null, mockPlatformRendererContext, mockRendererHost);
    spyDisplayListApplier = spy(displayListApplier);
    testDisplayList = createDisplayList();

    // Initialize ArgumentCaptors
    boundsCaptor = forClass(android.graphics.Rect.class);
    borderWidthsCaptor = forClass(int[].class);
    borderColorsCaptor = forClass(int[].class);
    borderStylesCaptor = forClass(BorderStyle[].class);
    outBoxCaptor = forClass(RoundedRectangle.class);
    innerBoxCaptor = forClass(RoundedRectangle.class);
  }

  @After
  public void tearDown() {
    for (NativeDisplayListBuilder displayList : nativeDisplayLists) {
      displayList.close();
    }
    nativeDisplayLists.clear();
  }

  private NativeDisplayListBuilder createDisplayList() {
    NativeDisplayListBuilder displayList = new NativeDisplayListBuilder();
    nativeDisplayLists.add(displayList);
    return displayList;
  }

  private void setDisplayList(DisplayListApplier applier, NativeDisplayListBuilder displayList) {
    applier.setBuffer(displayList == null ? null : displayList.toItemsBuffer(),
        displayList == null ? null : displayList.toDataBuffer());
  }

  private void setDisplayList(NativeDisplayListBuilder displayList) {
    setDisplayList(displayListApplier, displayList);
  }

  /**
   * Tests basic constructor functionality.
   * Verifies that DisplayListApplier can be instantiated with valid dependencies.
   */
  @Test
  public void testConstructor() {
    when(mockPlatformRendererContext.getTextMeasurer()).thenReturn(mockTextMeasurer);
    DisplayListApplier applier = new DisplayListApplier(testDisplayList.toItemsBuffer(),
        testDisplayList.toDataBuffer(), mockPlatformRendererContext, mockRendererHost);
    assertNotNull(applier);
  }

  /** Verifies that reset() clears internal processing state. */
  @Test
  public void testReset() {
    testDisplayList.begin(0, VIEW_TYPE, 10f, 20f, 100f, 50f).end();

    setDisplayList(displayListApplier, testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    // Reset and verify it doesn't crash
    displayListApplier.reset();
    displayListApplier.drawTillNextView(mockCanvas);
  }

  /**
   * Tests handling of null display list.
   * Verifies that the applier handles null display list gracefully without crashing.
   */
  @Test
  public void testNullDisplayList() {
    setDisplayList(displayListApplier, null);
    displayListApplier.drawTillNextView(mockCanvas);
    // Should not crash
    verify(mockCanvas, never()).save();
  }

  /** Verifies that the applier handles an empty typed buffer. */
  @Test
  public void testEmptyDisplayList() {
    setDisplayList(displayListApplier, testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas, never()).save();
  }

  /** Verifies OP_BEGIN canvas state and translation handling. */
  @Test
  public void testOpBegin() {
    testDisplayList.begin(0, VIEW_TYPE, 10f, 20f, 100f, 50f);

    setDisplayList(displayListApplier, testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas).save();
    verify(mockCanvas).translate(0f, 0f);
  }

  @Test
  public void testOverlayOpBeginNormalizesRootOffset() {
    Renderer renderer = new Renderer(mockPlatformRendererContext, 1);
    LynxBaseUI overlayUI = mock(LynxBaseUI.class);
    when(overlayUI.isOverlay()).thenReturn(true);
    renderer.setUIHost(overlayUI);
    when(mockRendererHost.getRenderer()).thenReturn(renderer);

    DisplayListApplier overlayApplier =
        new DisplayListApplier(null, null, mockPlatformRendererContext, mockRendererHost);
    testDisplayList.begin(0, VIEW_TYPE, 10f, 20f, 100f, 50f);

    setDisplayList(overlayApplier, testDisplayList);
    overlayApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas).save();
    verify(mockCanvas).translate(0f, 0f);
  }

  /** Verifies OP_END restores the canvas state. */
  @Test
  public void testOpEnd() {
    testDisplayList.begin(0, VIEW_TYPE, 10f, 20f, 100f, 50f).end();

    setDisplayList(displayListApplier, testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas).save();
    verify(mockCanvas).restore();
  }

  /** Verifies OP_FILL uses its typed color and clip-box fields. */
  @Test
  public void testOpFill() {
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 100f, 100f)
        .recordBox(0f, 0f, 100f, 100f)
        .fill(0xFF0000FF, 0)
        .end();

    setDisplayList(displayListApplier, testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas).save();
    verify(mockCanvas).drawRect(any(RectF.class), any(Paint.class));
    verify(mockCanvas).restore();
  }

  /** Verifies OP_DRAW_VIEW stops processing and returns control to native view drawing. */
  @Test
  public void testOpDrawView() {
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 100f, 50f).drawView(123, 15f, 26f);

    setDisplayList(displayListApplier, testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas).save();
    // Should stop at OP_DRAW_VIEW and return
  }

  /** Verifies OP_TEXT retrieves and draws the requested text layout. */
  @Test
  public void testOpText() {
    when(mockTextMeasurer.takeTextLayout(anyInt())).thenReturn(mockTextUpdateBundle);
    when(mockTextUpdateBundle.getTextLayout()).thenReturn(mockTextLayout);

    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 100f, 50f).text(456, -1);

    setDisplayList(displayListApplier, testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockTextMeasurer).takeTextLayout(456);
    verify(mockTextLayout).draw(mockCanvas);
  }

  /** Verifies OP_TEXT handles a missing text layout. */
  @Test
  public void testOpTextWithNullLayout() {
    when(mockTextMeasurer.takeTextLayout(anyInt())).thenReturn(null);

    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 100f, 50f).text(456, -1);

    setDisplayList(displayListApplier, testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockTextMeasurer).takeTextLayout(456);
    verify(mockTextLayout, never()).draw(any(Canvas.class));
  }

  /** Verifies OP_IMAGE consumes typed image identifiers and box indices. */
  @Test
  public void testOpImage() {
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 100f, 50f).image(789, -1);
    when(mockPlatformRendererContext.getImage(789)).thenReturn(mockImageManager);

    setDisplayList(displayListApplier, testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas).save();
    verify(mockPlatformRendererContext).getImage(789);
    verify(mockImageManager).setRendererHost(mockRendererHost);
    verify(mockImageManager).onDraw(mockCanvas);
  }

  @Test
  public void testClippingReusesRecordedPathAndReplacementDropsIt() {
    final int size = DisplayListApplier.DISPLAY_LIST_ITEM_SIZE;
    ByteBuffer ops = ByteBuffer.allocateDirect(3 * size).order(ByteOrder.nativeOrder());
    ops.putInt(0, DisplayListApplier.OP_RECORD_PATH);
    ops.putInt(8, 56); // Move(2,3), Line(12,20).
    ops.putInt(size, DisplayListApplier.OP_CLIP_PATH);
    ops.putInt(2 * size, DisplayListApplier.OP_CLIP_PATH); // Both reference path zero.
    ByteBuffer commands = ByteBuffer.allocateDirect(56).order(ByteOrder.nativeOrder());
    commands.putInt(0, 0).putFloat(4, 2).putFloat(8, 3);
    commands.putInt(28, 1).putFloat(32, 12).putFloat(36, 20);
    displayListApplier.setBuffer(ops, commands);
    displayListApplier.drawTillNextView(mockCanvas);
    ArgumentCaptor<android.graphics.Path> paths = forClass(android.graphics.Path.class);
    verify(mockCanvas, times(2)).clipPath(paths.capture());
    org.junit.Assert.assertSame(paths.getAllValues().get(0), paths.getAllValues().get(1));
    RectF bounds = new RectF();
    paths.getValue().computeBounds(bounds, true);
    assertEquals(new RectF(2, 3, 12, 20), bounds);

    // Replaying the same buffer must use the same decoded Path.
    displayListApplier.reset();
    displayListApplier.drawTillNextView(mockCanvas);
    ArgumentCaptor<android.graphics.Path> replayed = forClass(android.graphics.Path.class);
    verify(mockCanvas, times(4)).clipPath(replayed.capture());
    org.junit.Assert.assertSame(paths.getValue(), replayed.getValue());

    // Path 0 in a new list must never resolve to geometry in the old list.
    ByteBuffer replacement = ByteBuffer.allocateDirect(size).order(ByteOrder.nativeOrder());
    replacement.putInt(0, DisplayListApplier.OP_CLIP_PATH);
    displayListApplier.setBuffer(replacement, null);
    displayListApplier.drawTillNextView(mockCanvas);
    verify(mockCanvas, never()).clipRect(0, 0, 0, 0);
    verify(mockCanvas, times(4)).clipPath(any(android.graphics.Path.class));
  }

  private Bitmap renderRecordedClip(int kind, ByteBuffer data, int fill) {
    int stride = DisplayListApplier.DISPLAY_LIST_ITEM_SIZE;
    ByteBuffer ops = ByteBuffer.allocateDirect(2 * stride).order(ByteOrder.nativeOrder());
    ops.putInt(0, DisplayListApplier.OP_RECORD_PATH)
        .putInt(8, data.capacity())
        .putInt(12, kind)
        .putInt(16, fill);
    ops.putInt(stride, DisplayListApplier.OP_CLIP_PATH);
    displayListApplier.setBuffer(ops, data);
    Bitmap bitmap = Bitmap.createBitmap(32, 32, Bitmap.Config.ARGB_8888);
    Canvas canvas = new Canvas(bitmap);
    displayListApplier.drawTillNextView(canvas);
    canvas.drawColor(0xff00ff00);
    return bitmap;
  }

  private ByteBuffer floats(float... values) {
    ByteBuffer data = ByteBuffer.allocateDirect(values.length * 4).order(ByteOrder.nativeOrder());
    data.asFloatBuffer().put(values);
    return data;
  }

  @Test
  public void testBasicShapeClipsPreserveTheirGeometry() {
    int green = 0xff00ff00;
    Bitmap circle = renderRecordedClip(1, floats(16, 16, 12), 0);
    assertEquals(green, circle.getPixel(16, 5));
    assertEquals(0, circle.getPixel(6, 6));
    Bitmap ellipse = renderRecordedClip(2, floats(16, 16, 12, 5), 0);
    assertEquals(green, ellipse.getPixel(6, 16));
    assertEquals(0, ellipse.getPixel(16, 6));
    Bitmap rect = renderRecordedClip(3, floats(4, 4, 24, 24, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2), 0);
    assertEquals(green, rect.getPixel(4, 4));
    assertEquals(0, rect.getPixel(28, 20));
    Bitmap rounded = renderRecordedClip(3, floats(4, 4, 24, 24, 12, 0, 0, 0, 12, 0, 0, 0, 2, 2), 0);
    assertEquals(0, rounded.getPixel(4, 4));
    assertEquals(green, rounded.getPixel(27, 4));
    Bitmap superCorner =
        renderRecordedClip(3, floats(4, 4, 24, 24, 12, 0, 0, 0, 12, 0, 0, 0, 6, 6), 0);
    assertEquals(green, superCorner.getPixel(6, 6));
    assertEquals(green, superCorner.getPixel(27, 4));
    Bitmap superEllipse = renderRecordedClip(5, floats(16, 16, 12, 12, 6, 6), 0);
    assertEquals(green, superEllipse.getPixel(6, 6));
    assertEquals(0, superEllipse.getPixel(4, 4));
  }

  @Test
  public void testNativeRectRecordMatchesAndroidRoundRect() {
    Bitmap actual = renderRecordedClip(3, floats(3, 5, 24, 20, 11, 2, 7, 4, 8, 9, 3, 6, 2, 2), 0);
    Bitmap expected = Bitmap.createBitmap(32, 32, Bitmap.Config.ARGB_8888);
    Canvas canvas = new Canvas(expected);
    Path path = new Path();
    path.addRoundRect(
        new RectF(3, 5, 27, 25), new float[] {11, 8, 2, 9, 7, 3, 4, 6}, Path.Direction.CW);
    canvas.clipPath(path);
    canvas.drawColor(0xff00ff00);
    assertTrue(expected.sameAs(actual));
  }

  @Test
  public void testPathFillRuleAndEmptyClip() {
    // Two clockwise rectangles. The inner one is a hole under even-odd fill.
    ByteBuffer data = ByteBuffer.allocateDirect(10 * 28).order(ByteOrder.nativeOrder());
    int i = 0;
    for (int[] command : new int[][] {{0, 2, 2}, {1, 30, 2}, {1, 30, 30}, {1, 2, 30}, {3, 0, 0},
             {0, 10, 10}, {1, 22, 10}, {1, 22, 22}, {1, 10, 22}, {3, 0, 0}}) {
      data.putInt(i, command[0]).putFloat(i + 4, command[1]).putFloat(i + 8, command[2]);
      i += 28;
    }
    Bitmap clip = renderRecordedClip(0, data, 1);
    assertEquals(0xff00ff00, clip.getPixel(26, 16));
    assertEquals(0, clip.getPixel(16, 16));
    assertEquals(0, clip.getPixel(0, 0));
    Bitmap zeroRadius = renderRecordedClip(1, floats(16, 16, 0), 0);
    assertEquals(0, zeroRadius.getPixel(16, 16));
  }

  @Test
  public void testCssClipAndOverflowClipOnTheChildCanvas() {
    final int size = DisplayListApplier.DISPLAY_LIST_ITEM_SIZE;
    ByteBuffer ops = ByteBuffer.allocateDirect(8 * size).order(ByteOrder.nativeOrder());
    ops.putInt(0, DisplayListApplier.OP_BEGIN);
    ops.putInt(size, DisplayListApplier.OP_RECORD_PATH).putInt(size + 8, 4 * 28);
    ops.putInt(2 * size, DisplayListApplier.OP_CLIP_PATH);
    ops.putInt(3 * size, DisplayListApplier.OP_RECORD_BOX);
    ops.putFloat(3 * size + DisplayListApplier.RECORD_BOX_W_OFFSET, 32);
    ops.putFloat(3 * size + DisplayListApplier.RECORD_BOX_H_OFFSET, 32);
    ops.putInt(4 * size, DisplayListApplier.OP_FILL);
    ops.putInt(4 * size + DisplayListApplier.FILL_COLOR_OFFSET, 0xffff0000);
    ops.putInt(5 * size, DisplayListApplier.OP_CLIP_RECT);
    ops.putFloat(5 * size + DisplayListApplier.CLIP_RECT_X_OFFSET, 8);
    ops.putFloat(5 * size + DisplayListApplier.CLIP_RECT_Y_OFFSET, 8);
    ops.putFloat(5 * size + DisplayListApplier.CLIP_RECT_W_OFFSET, 24);
    ops.putFloat(5 * size + DisplayListApplier.CLIP_RECT_H_OFFSET, 24);
    ops.putInt(6 * size, DisplayListApplier.OP_DRAW_VIEW);
    ops.putInt(7 * size, DisplayListApplier.OP_END);
    // M 0 0 L 32 0 L 0 32 Z: the lower-right corner is outside the CSS clip.
    ByteBuffer commands = ByteBuffer.allocateDirect(4 * 28).order(ByteOrder.nativeOrder());
    commands.putInt(28, 1).putFloat(32, 32);
    commands.putInt(56, 1).putFloat(64, 32);
    commands.putInt(84, 3);
    displayListApplier.setBuffer(ops, commands);
    Bitmap bitmap = Bitmap.createBitmap(32, 32, Bitmap.Config.ARGB_8888);
    Canvas canvas = new Canvas(bitmap);
    displayListApplier.drawTillNextView(canvas);
    View child = new View(InstrumentationRegistry.getInstrumentation().getTargetContext());
    child.setBackgroundColor(0xff00ff00);
    child.layout(0, 0, 32, 32);
    child.draw(canvas);
    displayListApplier.drawTillNextView(canvas);
    assertEquals(0xffff0000, bitmap.getPixel(4, 4)); // Background only.
    assertEquals(0xff00ff00, bitmap.getPixel(10, 10)); // Child inside both clips.
    assertEquals(0, bitmap.getPixel(26, 26)); // Outside the triangle, inside overflow.
    assertEquals(1, canvas.getSaveCount());
  }

  /** Verifies that replacing the typed buffers resets and processes the new display list. */
  @Test
  public void testSetDisplayList() {
    NativeDisplayListBuilder newDisplayList = createDisplayList()
                                                  .begin(0, VIEW_TYPE, 0f, 0f, 32f, 32f)
                                                  .recordBox(0f, 0f, 32f, 32f)
                                                  .fill(0xFF00FF00, 0)
                                                  .end();

    setDisplayList(displayListApplier, newDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    // Should process the new display list
    verify(mockCanvas).drawRect(any(RectF.class), any(Paint.class));
  }

  /** Verifies that a truncated typed item is rejected without reading beyond the buffer. */
  @Test
  public void testInvalidItemBufferSize() {
    ByteBuffer truncatedItem =
        ByteBuffer.allocateDirect(DisplayListApplier.DISPLAY_LIST_ITEM_SIZE - Integer.BYTES);
    displayListApplier.setBuffer(truncatedItem, null);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas, never()).save();
  }

  /** Verifies a complete sequence of typed display-list items. */
  @Test
  public void testMultipleOperations() {
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 100f, 50f)
        .recordBox(0f, 0f, 100f, 50f)
        .fill(0xFF0000FF, 0)
        .text(999, -1)
        .end();

    when(mockTextMeasurer.takeTextLayout(anyInt())).thenReturn(mockTextUpdateBundle);
    when(mockTextUpdateBundle.getTextLayout()).thenReturn(mockTextLayout);

    setDisplayList(displayListApplier, testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas).save();
    verify(mockCanvas).translate(0f, 0f);
    verify(mockCanvas).drawRect(any(RectF.class), any(Paint.class)); // OP_FILL creates rect
    verify(mockTextMeasurer).takeTextLayout(999);
    verify(mockCanvas).restore();
  }

  /** Verifies OP_BORDER with uniform colors and styles. */
  @Test
  public void testOpBorderUniform() {
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 100f, 50f)
        .recordBox(0f, 0f, 100f, 50f)
        .recordBox(5f, 5f, 90f, 40f)
        .border(0, 1, new int[] {0xFF0000FF, 0xFF0000FF, 0xFF0000FF, 0xFF0000FF},
            new int[] {0, 0, 0, 0})
        .end();

    setDisplayList(spyDisplayListApplier, testDisplayList);
    spyDisplayListApplier.drawTillNextView(mockCanvas);

    verify(spyDisplayListApplier)
        .recordRoundedRectangle(eq(new RoundedRectangle(new RectF(0, 0, 100, 50), null)));
    verify(spyDisplayListApplier)
        .recordRoundedRectangle(eq(new RoundedRectangle(new RectF(5, 5, 95, 45), null)));

    // Verify drawRectangularBorders is called with correct parameters
    verify(spyDisplayListApplier)
        .drawRectangularBorders(eq(mockCanvas), any(Paint.class), outBoxCaptor.capture(),
            innerBoxCaptor.capture(), borderColorsCaptor.capture(), borderStylesCaptor.capture());
    assertEquals(new RoundedRectangle(new RectF(0, 0, 100, 50), null), outBoxCaptor.getValue());
    assertEquals(new RoundedRectangle(new RectF(5, 5, 95, 45), null), innerBoxCaptor.getValue());

    // Verify border colors
    int[] capturedColors = borderColorsCaptor.getValue();
    assertEquals(0xFF0000FF, capturedColors[Spacing.LEFT]);
    assertEquals(0xFF0000FF, capturedColors[Spacing.TOP]);
    assertEquals(0xFF0000FF, capturedColors[Spacing.RIGHT]);
    assertEquals(0xFF0000FF, capturedColors[Spacing.BOTTOM]);

    // Verify border styles
    BorderStyle[] capturedStyles = borderStylesCaptor.getValue();
    assertEquals(BorderStyle.SOLID, capturedStyles[Spacing.LEFT]);
    assertEquals(BorderStyle.SOLID, capturedStyles[Spacing.TOP]);
    assertEquals(BorderStyle.SOLID, capturedStyles[Spacing.RIGHT]);
    assertEquals(BorderStyle.SOLID, capturedStyles[Spacing.BOTTOM]);
  }

  @Test
  public void testOpBorderComplex() {
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 100f, 100f)
        .recordBox(0f, 0f, 100f, 100f)
        .recordBox(5f, 5f, 90f, 90f)
        .border(0, 1, new int[] {0xFFFF00FF, 0xFF00FFFF, 0xFF0000FF, 0xFF000000},
            new int[] {0, 1, 2, 3})
        .end();

    setDisplayList(spyDisplayListApplier, testDisplayList);
    spyDisplayListApplier.drawTillNextView(mockCanvas);

    verify(spyDisplayListApplier)
        .recordRoundedRectangle(eq(new RoundedRectangle(new RectF(0, 0, 100, 100), null)));
    verify(spyDisplayListApplier)
        .recordRoundedRectangle(eq(new RoundedRectangle(new RectF(5, 5, 95, 95), null)));

    // Verify drawRectangularBorders is called with correct parameters
    verify(spyDisplayListApplier)
        .drawRectangularBorders(eq(mockCanvas), any(Paint.class), outBoxCaptor.capture(),
            innerBoxCaptor.capture(), borderColorsCaptor.capture(), borderStylesCaptor.capture());
    assertEquals(new RoundedRectangle(new RectF(0, 0, 100, 100), null), outBoxCaptor.getValue());
    assertEquals(new RoundedRectangle(new RectF(5, 5, 95, 95), null), innerBoxCaptor.getValue());

    // Verify border colors
    int[] capturedColors = borderColorsCaptor.getValue();
    assertEquals(0xFFFF00FF, capturedColors[Spacing.TOP]);
    assertEquals(0xFF00FFFF, capturedColors[Spacing.RIGHT]);
    assertEquals(0xFF0000FF, capturedColors[Spacing.BOTTOM]);
    assertEquals(0xFF000000, capturedColors[Spacing.LEFT]);

    // Verify border styles
    BorderStyle[] capturedStyles = borderStylesCaptor.getValue();
    assertEquals(BorderStyle.SOLID, capturedStyles[Spacing.TOP]);
    assertEquals(BorderStyle.DASHED, capturedStyles[Spacing.RIGHT]);
    assertEquals(BorderStyle.DOTTED, capturedStyles[Spacing.BOTTOM]);
    assertEquals(BorderStyle.DOUBLE, capturedStyles[Spacing.LEFT]);
  }

  @Test
  public void testOpBorderDashedOrDotted() {
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 100f, 50f)
        .recordBox(0f, 0f, 100f, 50f)
        .recordBox(5f, 5f, 90f, 40f)
        .border(0, 1, new int[] {0xFF0000FF, 0xFF0000FF, 0xFF0000FF, 0xFF0000FF},
            new int[] {1, 1, 1, 1})
        .end();

    setDisplayList(spyDisplayListApplier, testDisplayList);
    spyDisplayListApplier.drawTillNextView(mockCanvas);

    verify(spyDisplayListApplier)
        .recordRoundedRectangle(eq(new RoundedRectangle(new RectF(0, 0, 100, 50), null)));
    verify(spyDisplayListApplier)
        .recordRoundedRectangle(eq(new RoundedRectangle(new RectF(5, 5, 95, 45), null)));

    // Verify drawRectangularBorders is called with correct parameters
    verify(spyDisplayListApplier)
        .drawRectangularBorders(eq(mockCanvas), any(Paint.class), outBoxCaptor.capture(),
            innerBoxCaptor.capture(), borderColorsCaptor.capture(), borderStylesCaptor.capture());
    assertEquals(new RoundedRectangle(new RectF(0, 0, 100, 50), null), outBoxCaptor.getValue());
    assertEquals(new RoundedRectangle(new RectF(5, 5, 95, 45), null), innerBoxCaptor.getValue());

    // Verify border colors
    int[] capturedColors = borderColorsCaptor.getValue();
    assertEquals(0xFF0000FF, capturedColors[Spacing.TOP]);

    // Verify border styles
    BorderStyle[] capturedStyles = borderStylesCaptor.getValue();
    assertEquals(BorderStyle.DASHED, capturedStyles[Spacing.TOP]);
    assertEquals(BorderStyle.DASHED, capturedStyles[Spacing.RIGHT]);
    assertEquals(BorderStyle.DASHED, capturedStyles[Spacing.BOTTOM]);
    assertEquals(BorderStyle.DASHED, capturedStyles[Spacing.LEFT]);
  }

  @Test
  public void testOpLinearGradient() {
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 100f, 100f)
        .recordBox(0f, 0f, 100f, 100f)
        .recordBox(10f, 10f, 80f, 80f)
        .linearGradient(new int[] {0xFFFF0000, 0xFF0000FF}, new float[] {0f, 1f}, 0, 1, 1, 1, 45f)
        .end();

    final AtomicReference<Shader> capturedShader = new AtomicReference<>();
    doAnswer(new Answer<Void>() {
      @Override
      public Void answer(InvocationOnMock invocation) throws Throwable {
        Paint paint = invocation.getArgument(4);
        capturedShader.set(paint.getShader());
        return null;
      }
    })
        .when(mockCanvas)
        .drawRect(anyFloat(), anyFloat(), anyFloat(), anyFloat(), any(Paint.class));

    setDisplayList(displayListApplier, testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    // The gradient implementation uses nested save/restore calls:
    // - 1 save at OP_BEGIN, 1 restore at OP_END
    // - 1 save for clip, 1 restore after drawing (in drawLinearGradient)
    // - 1 save for single tile drawing in no-repeat mode, 1 restore
    verify(mockCanvas, times(3)).save();
    verify(mockCanvas, times(3)).restore();
    verify(mockCanvas).drawRect(anyFloat(), anyFloat(), anyFloat(), anyFloat(), any(Paint.class));
    assertNotNull(capturedShader.get());
  }

  @Test
  public void testOpLinearGradientIsIndependentOfTileOrigin() {
    // A padding or content origin moves the tile without changing its gradient.
    for (float angle : new float[] {60f, 150f, 240f, 330f}) {
      for (int repeat : new int[] {0, 1}) {
        Bitmap reference = drawLinearGradientTile(angle, 0, 0, repeat);
        Bitmap translated = drawLinearGradientTile(angle, 11, 17, repeat);
        try {
          for (int y = 0; y < 50; y++) {
            for (int x = 0; x < 70; x++) {
              assertEquals(
                  "Gradient at angle " + angle + ", repeat " + repeat + " and pixel " + x + "," + y,
                  reference.getPixel(x, y), translated.getPixel(x + 11, y + 17));
            }
          }
        } finally {
          reference.recycle();
          translated.recycle();
        }
      }
    }
  }

  private Bitmap drawLinearGradientTile(float angle, int left, int top, int repeat) {
    NativeDisplayListBuilder displayList = createDisplayList();
    displayList.begin(0, VIEW_TYPE, 0f, 0f, 100f, 100f)
        .recordBox(left, top, 35f, 35f)
        .recordBox(left, top, 70f, 50f)
        .linearGradient(new int[] {0xFFFF0000, 0xFFFFFF00, 0xFFFF0000}, new float[] {0f, 0.5f, 1f},
            0, 1, repeat, repeat, angle)
        .end();
    Bitmap bitmap = Bitmap.createBitmap(100, 100, Bitmap.Config.ARGB_8888);
    setDisplayList(displayList);
    displayListApplier.drawTillNextView(new Canvas(bitmap));
    return bitmap;
  }

  @Test
  public void testRootLinearGradientKeepsExtendedAndNegativeOriginTiles() {
    when(mockRendererHost.getRendererHostWidth()).thenReturn(100);
    when(mockRendererHost.getRendererHostHeight()).thenReturn(100);
    final float[][] tiles = {{-40, 0, 180, 100}, {0, -50, 100, 200}, {-20, -10, 100, 100}};
    final float[][] endpoints = {{-40, 50, 140, 50}, {50, -50, 50, 150}, {80, 90, -20, -10}};
    final float[] angles = {90, 180, 315};
    for (int i = 0; i < tiles.length; i++) {
      float[] tile = tiles[i];
      float[] e = endpoints[i];
      NativeDisplayListBuilder list = createDisplayList();
      list.begin(0, VIEW_TYPE, 0, 0, 100, 100)
          .recordBox(tile[0], tile[1], tile[2], tile[3])
          .recordBox(-10, 10, 80, 50)
          .linearGradient(
              new int[] {0x80FF0000, 0x800000FF}, new float[] {.2f, .8f}, 0, 1, 1, 1, angles[i])
          .end();
      setDisplayList(list);
      Bitmap actual = Bitmap.createBitmap(100, 100, Bitmap.Config.ARGB_8888);
      Bitmap expected = Bitmap.createBitmap(100, 100, Bitmap.Config.ARGB_8888);
      try {
        displayListApplier.drawTillNextView(new Canvas(actual));
        Paint reference = new Paint(Paint.ANTI_ALIAS_FLAG);
        reference.setShader(new LinearGradient(e[0], e[1], e[2], e[3],
            new int[] {0x80FF0000, 0x800000FF}, new float[] {.2f, .8f}, Shader.TileMode.CLAMP));
        new Canvas(expected).drawRect(-10, 10, 70, 60, reference);
        assertBitmapColorsNear(expected, actual);
      } finally {
        actual.recycle();
        expected.recycle();
      }
    }
  }

  @Test
  public void testRootLinearGradientPreservesRepeatedTilePhase() {
    NativeDisplayListBuilder list = createDisplayList();
    list.begin(0, VIEW_TYPE, 0, 0, 100, 100)
        .recordBox(-17, -13, 35, 35)
        .recordBox(0, 0, 100, 100)
        .linearGradient(new int[] {0xFFFF0000, 0xFF0000FF}, new float[] {0, 1}, 0, 1, 0, 0, 150)
        .end();
    Bitmap expected = Bitmap.createBitmap(100, 100, Bitmap.Config.ARGB_8888);
    Bitmap actual = Bitmap.createBitmap(100, 100, Bitmap.Config.ARGB_8888);
    try {
      setDisplayList(list);
      displayListApplier.drawTillNextView(new Canvas(expected));
      when(mockRendererHost.getRendererHostWidth()).thenReturn(100);
      when(mockRendererHost.getRendererHostHeight()).thenReturn(100);
      setDisplayList(list);
      displayListApplier.drawTillNextView(new Canvas(actual));
      assertBitmapColorsNear(expected, actual);
    } finally {
      expected.recycle();
      actual.recycle();
    }
  }

  private void assertBitmapColorsNear(Bitmap expected, Bitmap actual) {
    for (int y = 0; y < 100; y++) {
      for (int x = 0; x < 100; x++) {
        for (int shift : new int[] {0, 8, 16, 24}) {
          int a = (expected.getPixel(x, y) >>> shift) & 255;
          int b = (actual.getPixel(x, y) >>> shift) & 255;
          assertEquals("pixel " + x + "," + y + " channel " + shift, a, b, 1);
        }
      }
    }
  }

  @Test
  public void testOpRadialGradient() {
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 100f, 100f)
        .recordBox(0f, 0f, 100f, 100f)
        .recordBox(10f, 10f, 80f, 80f)
        .radialGradient(new int[] {0xFFFF0000, 0xFF0000FF}, new float[] {0f, 1f}, 0, 1, 1, 1, 50f,
            40f, 60f, 30f)
        .end();

    final AtomicReference<Shader> capturedShader = new AtomicReference<>();
    doAnswer(new Answer<Void>() {
      @Override
      public Void answer(InvocationOnMock invocation) throws Throwable {
        Paint paint = invocation.getArgument(4);
        capturedShader.set(paint.getShader());
        return null;
      }
    })
        .when(mockCanvas)
        .drawRect(anyFloat(), anyFloat(), anyFloat(), anyFloat(), any(Paint.class));

    setDisplayList(displayListApplier, testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas, times(3)).save();
    verify(mockCanvas, times(3)).restore();
    verify(mockCanvas).drawRect(anyFloat(), anyFloat(), anyFloat(), anyFloat(), any(Paint.class));
    assertNotNull(capturedShader.get());
    assertEquals(RadialGradient.class, capturedShader.get().getClass());
  }

  @Test
  public void testOpRadialGradientWithZeroRadius() {
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 100f, 100f)
        .recordBox(0f, 0f, 100f, 100f)
        .recordBox(0f, 0f, 100f, 100f)
        .radialGradient(
            new int[] {0xFFFF0000, 0xFF0000FF}, new float[] {0f, 1f}, 0, 1, 1, 1, 0f, 50f, 0f, 50f)
        .end();

    final AtomicReference<Shader> capturedShader = new AtomicReference<>();
    doAnswer(invocation -> {
      Paint paint = invocation.getArgument(4);
      capturedShader.set(paint.getShader());
      return null;
    })
        .when(mockCanvas)
        .drawRect(anyFloat(), anyFloat(), anyFloat(), anyFloat(), any(Paint.class));

    setDisplayList(displayListApplier, testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas).drawRect(anyFloat(), anyFloat(), anyFloat(), anyFloat(), any(Paint.class));
    assertNotNull(capturedShader.get());
    assertEquals(RadialGradient.class, capturedShader.get().getClass());
  }

  @Test
  public void testOpRadialGradientClampsSubpixelRepeatStep() {
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 1f, 1f)
        .recordBox(0f, 0f, 0.4f, 0.4f)
        .recordBox(0f, 0f, 1f, 1f)
        .radialGradient(new int[] {0xFFFF0000, 0xFF0000FF}, new float[] {0f, 1f}, 0, 1, 0, 0, 0.2f,
            0.2f, 0.2f, 0.2f)
        .end();

    setDisplayList(displayListApplier, testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas).drawRect(eq(0.f), eq(0.f), eq(1.f), eq(1.f), any(Paint.class));
  }

  @Test
  public void testOpRadialGradientDrawsSubpixelNoRepeatTile() {
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 100f, 100f)
        .recordBox(0f, 0f, 0.4f, 100f)
        .recordBox(0f, 0f, 100f, 100f)
        .radialGradient(new int[] {0xFFFF0000, 0xFF0000FF}, new float[] {0f, 1f}, 0, 1, 1, 1, 0.2f,
            50f, 1f, 50f)
        .end();

    setDisplayList(displayListApplier, testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas).drawRect(eq(0.f), eq(0.f), eq(0.4f), eq(100.f), any(Paint.class));
  }

  @Test
  public void testOpBackgroundImageRepeatX() {
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 100f, 40f)
        .recordBox(0f, 0f, 40f, 20f)
        .recordBox(0f, 0f, 90f, 40f)
        .backgroundImage(321, 0, 1, 0, 1)
        .end();
    when(mockPlatformRendererContext.getImage(321)).thenReturn(mockImageManager);

    setDisplayList(displayListApplier, testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas, times(2)).save();
    verify(mockCanvas).clipRect(eq(new RectF(0f, 0f, 90f, 40f)));
    verify(mockImageManager).setRendererHost(mockRendererHost);
    verify(mockImageManager).updateInnerClipPathForBorderRadius(null);
    verify(mockImageManager).updateDrawableBounds(eq(new Rect(0, 0, 40, 20)));
    verify(mockImageManager).updateDrawableBounds(eq(new Rect(40, 0, 80, 20)));
    verify(mockImageManager).updateDrawableBounds(eq(new Rect(80, 0, 120, 20)));
    verify(mockImageManager, times(3)).onDraw(mockCanvas);
    verify(mockCanvas, times(2)).restore();
  }

  @Test
  public void testBackgroundImageFitPreservesAspectRatioAndPercentPosition() {
    when(mockPlatformRendererContext.getImage(321)).thenReturn(mockImageManager);
    int[][] cases = {{200, 100, DisplayListApplier.BACKGROUND_SIZE_COVER},
        {200, 100, DisplayListApplier.BACKGROUND_SIZE_CONTAIN},
        {40, 80, DisplayListApplier.BACKGROUND_SIZE_COVER},
        {40, 80, DisplayListApplier.BACKGROUND_SIZE_CONTAIN}};
    for (int[] testCase : cases) {
      when(mockImageManager.getImageWidth()).thenReturn(testCase[0]);
      when(mockImageManager.getImageHeight()).thenReturn(testCase[1]);
      NativeDisplayListBuilder list = createDisplayList();
      list.begin(0, VIEW_TYPE, 0f, 0f, 240f, 240f)
          .recordBox(10f, 20f, 100f, 100f)
          .recordBox(0f, 0f, 240f, 240f)
          .backgroundImage(321, 0, 1, 1, 1, testCase[2], 0.5f, 0.5f)
          .end();

      setDisplayList(list);
      displayListApplier.drawTillNextView(mockCanvas);
    }

    verify(mockImageManager, times(4)).updateDrawableBounds(boundsCaptor.capture());
    assertEquals(Arrays.asList(new Rect(-40, 20, 160, 120), new Rect(10, 45, 110, 95),
                     new Rect(10, -30, 110, 170), new Rect(35, 20, 85, 120)),
        boundsCaptor.getAllValues());
    verify(mockImageManager, times(4)).onDraw(mockCanvas);
  }

  @Test
  public void testBackgroundImageFitUsesUnclippedOriginAtRendererRoot() {
    when(mockRendererHost.getRendererHostWidth()).thenReturn(100);
    when(mockRendererHost.getRendererHostHeight()).thenReturn(80);
    when(mockPlatformRendererContext.getImage(321)).thenReturn(mockImageManager);
    when(mockImageManager.getImageWidth()).thenReturn(80);
    when(mockImageManager.getImageHeight()).thenReturn(40);
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 100f, 80f)
        .recordBox(10f, 0f, 200f, 80f)
        .recordBox(0f, 0f, 100f, 80f)
        .backgroundImage(321, 0, 1, 1, 1, DisplayListApplier.BACKGROUND_SIZE_COVER, 0.5f, 0.5f)
        .backgroundImage(321, 0, 1, 1, 1, DisplayListApplier.BACKGROUND_SIZE_CONTAIN, 0.5f, 0.5f)
        .end();

    setDisplayList(testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas, times(2)).clipRect(eq(new RectF(0f, 0f, 100f, 80f)));
    verify(mockImageManager, times(2)).updateDrawableBounds(boundsCaptor.capture());
    assertEquals(Arrays.asList(new Rect(10, -10, 210, 90), new Rect(30, 0, 190, 80)),
        boundsCaptor.getAllValues());
    assertEquals(new RectF(10f, 0f, 210f, 80f), getBoxArray(displayListApplier).get(0).getRectF());
  }

  @Test
  public void testBackgroundImageContainRepeatUsesResolvedTileAndPosition() {
    when(mockPlatformRendererContext.getImage(321)).thenReturn(mockImageManager);
    when(mockImageManager.getImageWidth()).thenReturn(200);
    when(mockImageManager.getImageHeight()).thenReturn(100);
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 100f, 100f)
        .recordBox(0f, 0f, 100f, 100f)
        .recordBox(0f, 0f, 100f, 100f)
        .backgroundImage(321, 0, 1, 1, 0, DisplayListApplier.BACKGROUND_SIZE_CONTAIN, 0.5f, 0.5f)
        .end();

    setDisplayList(testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockImageManager, times(3)).updateDrawableBounds(boundsCaptor.capture());
    assertEquals(Arrays.asList(new Rect(0, -25, 100, 25), new Rect(0, 25, 100, 75),
                     new Rect(0, 75, 100, 125)),
        boundsCaptor.getAllValues());
  }

  @Test
  public void testBackgroundImageFitRepaintUsesCurrentIntrinsicSizeWithoutMutatingBoxes() {
    for (int sizeMode : new int[] {DisplayListApplier.BACKGROUND_SIZE_COVER,
             DisplayListApplier.BACKGROUND_SIZE_CONTAIN}) {
      LynxImageManager imageManager = mock(LynxImageManager.class);
      when(mockPlatformRendererContext.getImage(321)).thenReturn(imageManager);
      NativeDisplayListBuilder list = createDisplayList();
      list.begin(0, VIEW_TYPE, 0f, 0f, 120f, 120f)
          .recordBox(0f, 7f, 100f, 100f)
          .recordBox(0f, 0f, 120f, 120f)
          .backgroundImage(321, 0, 1, 1, 1, sizeMode, 0.5f, 0.5f)
          .end();
      ArrayList<Rect> drawnBounds = new ArrayList<>();
      doAnswer(invocation -> {
        drawnBounds.add(new Rect((Rect) invocation.getArgument(0)));
        return null;
      })
          .when(imageManager)
          .updateDrawableBounds(any(Rect.class));

      setDisplayList(list);
      displayListApplier.drawTillNextView(mockCanvas);
      verify(imageManager).setRendererHost(mockRendererHost);
      verify(imageManager, never()).onDraw(any(Canvas.class));

      when(imageManager.getImageWidth()).thenReturn(40);
      when(imageManager.getImageHeight()).thenReturn(-1);
      displayListApplier.reset();
      displayListApplier.drawTillNextView(mockCanvas);
      verify(imageManager, times(2)).setRendererHost(mockRendererHost);
      verify(imageManager, never()).updateDrawableBounds(any(Rect.class));

      when(imageManager.getImageHeight()).thenReturn(20);
      for (int i = 0; i < 2; ++i) {
        displayListApplier.reset();
        displayListApplier.drawTillNextView(mockCanvas);
        assertEquals(
            new RectF(0f, 7f, 100f, 107f), getBoxArray(displayListApplier).get(0).getRectF());
      }
      when(imageManager.getImageWidth()).thenReturn(20);
      displayListApplier.reset();
      displayListApplier.drawTillNextView(mockCanvas);
      assertEquals(
          new RectF(0f, 7f, 100f, 107f), getBoxArray(displayListApplier).get(0).getRectF());

      Rect firstTile = sizeMode == DisplayListApplier.BACKGROUND_SIZE_COVER
          ? new Rect(-50, 7, 150, 107)
          : new Rect(0, 32, 100, 82);
      assertEquals(Arrays.asList(firstTile, firstTile, new Rect(0, 7, 100, 107)), drawnBounds);
      verify(imageManager, times(3)).onDraw(mockCanvas);
    }
  }

  @Test
  public void testBackgroundImageAutoAutoUsesIntrinsicSizeAndDensity() throws Exception {
    // PixelUtils reads the global screen metrics; replace only that test input and restore it.
    Field metricsField = DisplayMetricsHolder.class.getDeclaredField("sScreenDisplayMetrics");
    metricsField.setAccessible(true);
    Object previousMetrics = metricsField.get(null);
    DisplayMetrics metrics = new DisplayMetrics();
    metrics.density = 2f;
    try {
      metricsField.set(null, metrics);
      assertEquals(2f, PixelUtils.dipToPx(1f), 0f);
      when(mockPlatformRendererContext.getImage(321)).thenReturn(mockImageManager);
      when(mockImageManager.getImageWidth()).thenReturn(30);
      when(mockImageManager.getImageHeight()).thenReturn(20);
      testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 240f, 160f)
          .recordBox(10f, 12f, 200f, 120f)
          .recordBox(0f, 0f, 240f, 160f)
          .backgroundImage(
              321, 0, 1, 1, 1, DisplayListApplier.BACKGROUND_SIZE_AUTO_BOTH, 0.5f, 0.75f)
          .end();

      setDisplayList(testDisplayList);
      displayListApplier.drawTillNextView(mockCanvas);

      verify(mockImageManager).setRendererHost(mockRendererHost);
      verify(mockImageManager).updateDrawableBounds(boundsCaptor.capture());
      assertEquals(new Rect(80, 72, 140, 112), boundsCaptor.getValue());
      verify(mockImageManager).onDraw(mockCanvas);
      assertEquals(
          new RectF(10f, 12f, 210f, 132f), getBoxArray(displayListApplier).get(0).getRectF());
    } finally {
      metricsField.set(null, previousMetrics);
    }
  }

  @Test
  public void testBackgroundImageAutoWidthPreservesRatioAndPercentOrAbsolutePosition() {
    when(mockPlatformRendererContext.getImage(321)).thenReturn(mockImageManager);
    when(mockImageManager.getImageWidth()).thenReturn(80);
    when(mockImageManager.getImageHeight()).thenReturn(40);
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 240f, 160f)
        .recordBox(10f, 7f, 200f, 30f)
        .recordBox(0f, 0f, 240f, 160f)
        .backgroundImage(321, 0, 1, 1, 1, DisplayListApplier.BACKGROUND_SIZE_AUTO_WIDTH, 0.5f, 0f)
        .backgroundImage(321, 0, 1, 1, 1, DisplayListApplier.BACKGROUND_SIZE_AUTO_WIDTH, 0f, 0f)
        .end();

    setDisplayList(testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockImageManager, times(2)).updateDrawableBounds(boundsCaptor.capture());
    assertEquals(Arrays.asList(new Rect(80, 7, 140, 37), new Rect(10, 7, 70, 37)),
        boundsCaptor.getAllValues());
    verify(mockImageManager, times(2)).onDraw(mockCanvas);
  }

  @Test
  public void testBackgroundImageAutoHeightPreservesRatioAndPercentOrAbsolutePosition() {
    when(mockPlatformRendererContext.getImage(321)).thenReturn(mockImageManager);
    when(mockImageManager.getImageWidth()).thenReturn(80);
    when(mockImageManager.getImageHeight()).thenReturn(40);
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 240f, 160f)
        .recordBox(9f, 12f, 60f, 120f)
        .recordBox(0f, 0f, 240f, 160f)
        .backgroundImage(321, 0, 1, 1, 1, DisplayListApplier.BACKGROUND_SIZE_AUTO_HEIGHT, 0f, 1f)
        .backgroundImage(321, 0, 1, 1, 1, DisplayListApplier.BACKGROUND_SIZE_AUTO_HEIGHT, 0f, 0f)
        .end();

    setDisplayList(testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockImageManager, times(2)).updateDrawableBounds(boundsCaptor.capture());
    assertEquals(Arrays.asList(new Rect(9, 102, 69, 132), new Rect(9, 12, 69, 42)),
        boundsCaptor.getAllValues());
    verify(mockImageManager, times(2)).onDraw(mockCanvas);
  }

  @Test
  public void testBackgroundImageAutoWidthRepeatUsesResolvedTileSizeAndPosition() {
    when(mockPlatformRendererContext.getImage(321)).thenReturn(mockImageManager);
    when(mockImageManager.getImageWidth()).thenReturn(80);
    when(mockImageManager.getImageHeight()).thenReturn(40);
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 90f, 45f)
        .recordBox(0f, 0f, 90f, 20f)
        .recordBox(0f, 0f, 90f, 45f)
        .backgroundImage(321, 0, 1, 0, 0, DisplayListApplier.BACKGROUND_SIZE_AUTO_WIDTH, 0.5f, 0f)
        .end();

    setDisplayList(testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas).clipRect(eq(new RectF(0f, 0f, 90f, 45f)));
    verify(mockImageManager, times(9)).updateDrawableBounds(boundsCaptor.capture());
    assertEquals(Arrays.asList(new Rect(-15, 0, 25, 20), new Rect(-15, 20, 25, 40),
                     new Rect(-15, 40, 25, 60), new Rect(25, 0, 65, 20), new Rect(25, 20, 65, 40),
                     new Rect(25, 40, 65, 60), new Rect(65, 0, 105, 20), new Rect(65, 20, 105, 40),
                     new Rect(65, 40, 105, 60)),
        boundsCaptor.getAllValues());
    verify(mockImageManager, times(9)).onDraw(mockCanvas);
  }

  @Test
  public void testBackgroundImageAutoHeightRepeatLargerThanClipBox() {
    when(mockPlatformRendererContext.getImage(321)).thenReturn(mockImageManager);
    when(mockImageManager.getImageWidth()).thenReturn(40);
    when(mockImageManager.getImageHeight()).thenReturn(100);
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 90f, 60f)
        .recordBox(0f, 0f, 40f, 60f)
        .recordBox(0f, 0f, 90f, 60f)
        .backgroundImage(321, 0, 1, 0, 0, DisplayListApplier.BACKGROUND_SIZE_AUTO_HEIGHT, 0f, 0.5f)
        .end();

    setDisplayList(testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas).clipRect(eq(new RectF(0f, 0f, 90f, 60f)));
    verify(mockImageManager, times(3)).updateDrawableBounds(boundsCaptor.capture());
    assertEquals(Arrays.asList(new Rect(0, -20, 40, 80), new Rect(40, -20, 80, 80),
                     new Rect(80, -20, 120, 80)),
        boundsCaptor.getAllValues());
    verify(mockImageManager, times(3)).onDraw(mockCanvas);
  }

  @Test
  public void testBackgroundImageAutoUsesUnclippedTilingSizeAtRendererRoot() {
    when(mockRendererHost.getRendererHostWidth()).thenReturn(100);
    when(mockRendererHost.getRendererHostHeight()).thenReturn(80);
    when(mockPlatformRendererContext.getImage(321)).thenReturn(mockImageManager);
    when(mockImageManager.getImageWidth()).thenReturn(80);
    when(mockImageManager.getImageHeight()).thenReturn(40);
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 100f, 80f)
        .recordBox(10f, 0f, 200f, 80f)
        .recordBox(0f, 0f, 100f, 80f)
        .backgroundImage(321, 0, 1, 1, 1, DisplayListApplier.BACKGROUND_SIZE_AUTO_HEIGHT, 0f, 0.5f)
        .end();

    setDisplayList(testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas).clipRect(eq(new RectF(0f, 0f, 100f, 80f)));
    verify(mockImageManager).updateDrawableBounds(eq(new Rect(10, -10, 210, 90)));
    verify(mockImageManager).onDraw(mockCanvas);
  }

  @Test
  public void testBackgroundImageAutoRepaintUsesCurrentIntrinsicSizeWithoutMutatingBoxes() {
    when(mockPlatformRendererContext.getImage(321)).thenReturn(mockImageManager);
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 120f, 80f)
        .recordBox(0f, 7f, 120f, 20f)
        .recordBox(0f, 0f, 120f, 80f)
        .backgroundImage(321, 0, 1, 1, 1, DisplayListApplier.BACKGROUND_SIZE_AUTO_WIDTH, 0.5f, 0f)
        .end();
    ArrayList<Rect> drawnBounds = new ArrayList<>();
    doAnswer(invocation -> {
      drawnBounds.add(new Rect((Rect) invocation.getArgument(0)));
      return null;
    })
        .when(mockImageManager)
        .updateDrawableBounds(any(Rect.class));

    // No decoded dimensions yet: registering the host still enables load invalidation.
    setDisplayList(testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);
    verify(mockImageManager).setRendererHost(mockRendererHost);
    verify(mockImageManager, never()).updateDrawableBounds(any(Rect.class));
    verify(mockImageManager, never()).onDraw(any(Canvas.class));
    assertBackgroundImageFallbackBoxesUnchanged();

    // A partially available or invalid intrinsic size must not produce a tile either.
    when(mockImageManager.getImageWidth()).thenReturn(40);
    when(mockImageManager.getImageHeight()).thenReturn(-1);
    displayListApplier.reset();
    displayListApplier.drawTillNextView(mockCanvas);
    verify(mockImageManager, times(2)).setRendererHost(mockRendererHost);
    verify(mockImageManager, never()).updateDrawableBounds(any(Rect.class));
    verify(mockImageManager, never()).onDraw(any(Canvas.class));
    assertBackgroundImageFallbackBoxesUnchanged();

    when(mockImageManager.getImageHeight()).thenReturn(20);
    for (int i = 0; i < 2; ++i) {
      displayListApplier.reset();
      displayListApplier.drawTillNextView(mockCanvas);
      assertBackgroundImageFallbackBoxesUnchanged();
    }
    // Reuse the same native list and applier even when the resource dimensions change again.
    when(mockImageManager.getImageWidth()).thenReturn(80);
    displayListApplier.reset();
    displayListApplier.drawTillNextView(mockCanvas);
    assertBackgroundImageFallbackBoxesUnchanged();

    assertEquals(
        Arrays.asList(new Rect(40, 7, 80, 27), new Rect(40, 7, 80, 27), new Rect(20, 7, 100, 27)),
        drawnBounds);
    verify(mockImageManager, times(3)).onDraw(mockCanvas);
  }

  private void assertBackgroundImageFallbackBoxesUnchanged() {
    ArrayList<RoundedRectangle> boxes = getBoxArray(displayListApplier);
    assertEquals(2, boxes.size());
    assertEquals(new RectF(0f, 7f, 120f, 27f), boxes.get(0).getRectF());
    assertEquals(new RectF(0f, 0f, 120f, 80f), boxes.get(1).getRectF());
  }

  @Test
  public void testBackgroundImageExplicitSizeDoesNotRequireIntrinsicDimensions() {
    when(mockPlatformRendererContext.getImage(321)).thenReturn(mockImageManager);
    when(mockImageManager.getImageWidth()).thenReturn(0);
    when(mockImageManager.getImageHeight()).thenReturn(0);
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 120f, 80f)
        .recordBox(9f, 7f, 60f, 30f)
        .recordBox(0f, 0f, 120f, 80f)
        .backgroundImage(321, 0, 1, 1, 1)
        .end();

    setDisplayList(testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockImageManager).setRendererHost(mockRendererHost);
    verify(mockImageManager).updateDrawableBounds(boundsCaptor.capture());
    assertEquals(new Rect(9, 7, 69, 37), boundsCaptor.getValue());
    verify(mockImageManager).onDraw(mockCanvas);
  }

  @Test
  public void testOpClipRectPlain() {
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 100f, 50f).clipRect(10f, 12f, 80f, 30f).end();

    setDisplayList(displayListApplier, testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas).save();
    verify(mockCanvas).clipRect(any(RectF.class));
    verify(mockCanvas).restore();
  }

  private ArrayList<RoundedRectangle> getBoxArray(DisplayListApplier applier) {
    ArrayList<RoundedRectangle> list = null;
    try {
      Field field = DisplayListApplier.class.getDeclaredField("mRoundedRectangleArray");
      field.setAccessible(true);
      list = (ArrayList<RoundedRectangle>) field.get(applier);
    } catch (NoSuchFieldException | IllegalAccessException e) {
      fail(e.toString());
    }
    return list;
  }

  @Test
  public void testOpRecordBox0() {
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 100f, 50f).recordBox(10f, 12f, 80f, 30f).end();

    ArrayList<RoundedRectangle> list = getBoxArray(displayListApplier);
    assertEquals(0, list.size());

    setDisplayList(displayListApplier, testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    assertEquals(1, list.size());
    assertEquals(new RoundedRectangle(new RectF(10.0f, 12.0f, 90.0f, 42.0f), null), list.get(0));

    displayListApplier.reset();
    assertEquals(0, list.size());
  }

  @Test
  public void testOpRecordBox1() {
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 100f, 50f)
        .recordBox(10f, 12f, 80f, 30f, 2f, 2f, 2f, 2f, 2f, 2f, 2f, 2f)
        .end();

    ArrayList<RoundedRectangle> list = getBoxArray(displayListApplier);
    assertEquals(0, list.size());

    setDisplayList(displayListApplier, testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    float[] border = new float[8];
    border[0] = 2.0f;
    border[1] = 2.0f;
    border[2] = 2.0f;
    border[3] = 2.0f;
    border[4] = 2.0f;
    border[5] = 2.0f;
    border[6] = 2.0f;
    border[7] = 2.0f;

    assertEquals(1, list.size());
    assertEquals(new RoundedRectangle(new RectF(10.0f, 12.0f, 90.0f, 42.0f), border), list.get(0));

    displayListApplier.reset();
    assertEquals(0, list.size());
  }

  @Test
  public void testOpClipRectRounded() {
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 100f, 50f)
        .clipRect(10f, 12f, 80f, 30f, 5f, 6f, 7f, 8f, 9f, 10f, 11f, 12f)
        .end();

    setDisplayList(displayListApplier, testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas).save();
    verify(mockCanvas).clipPath(any(android.graphics.Path.class));
    verify(mockCanvas).restore();
  }

  @Test
  public void testOpacityBoundsIncludeOutsetBlurAndResetWithDisplayList() {
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 100f, 50f)
        .recordBox(0f, 0f, 100f, 50f)
        .recordBox(-20f, -10f, 100f, 50f)
        .boxShadow(1, 0, 0xFF000000, 5f, 0)
        .recordBox(-100f, -100f, 300f, 300f)
        .boxShadow(2, 0, 0xFF000000, 5f, 1)
        .clipRect(0f, 0f, 100f, 50f)
        .begin(1, VIEW_TYPE, 0f, 0f, 100f, 50f)
        .boxShadow(2, 0, 0xFF000000, 5f, 0)
        .end()
        .end();
    setDisplayList(testDisplayList);
    RectF bounds = new RectF(0f, 0f, 100f, 50f);
    displayListApplier.includeRootShadowBounds(bounds);
    assertEquals(new RectF(-35f, -25f, 100f, 55f), bounds);

    NativeDisplayListBuilder withoutShadow = createDisplayList();
    withoutShadow.begin(0, VIEW_TYPE, 0f, 0f, 100f, 50f).end();
    setDisplayList(withoutShadow);
    bounds.set(0f, 0f, 100f, 50f);
    displayListApplier.includeRootShadowBounds(bounds);
    assertEquals(new RectF(0f, 0f, 100f, 50f), bounds);
  }

  @Test
  public void testCollapsedInsetShadowFillsPaddingBox() {
    for (float[] hole : new float[][] {{0f, 20f}, {60f, 0f}, {0f, 0f}}) {
      for (float blur : new float[] {0f, 10f}) {
        for (float radius : new float[] {0f, 8f}) {
          NativeDisplayListBuilder list = createDisplayList();
          list.begin(0, VIEW_TYPE, 0f, 0f, 80f, 40f)
              .recordBox(10f, 10f, 60f, 20f, radius, radius, radius, radius, radius, radius, radius,
                  radius)
              .recordBox(25f, 15f, hole[0], hole[1])
              .boxShadow(1, 0, 0x80FF00FF, blur, 1)
              .end();
          Bitmap bitmap = Bitmap.createBitmap(80, 40, Bitmap.Config.ARGB_8888);
          try {
            setDisplayList(list);
            displayListApplier.drawTillNextView(new Canvas(bitmap));
            assertEquals(0x80FF00FF, bitmap.getPixel(40, 20));
            assertEquals(0x80FF00FF, bitmap.getPixel(40, 11));
            assertEquals(0, bitmap.getPixel(40, 9));
            assertEquals(0, bitmap.getPixel(9, 20));
            if (radius > 0f) {
              assertEquals(0, bitmap.getPixel(10, 10));
            }
          } finally {
            bitmap.recycle();
          }
        }
      }
    }
  }

  /** Verifies OP_BOX_SHADOW reads the typed payload and draws the shadow. */
  @Test
  public void testOpBoxShadow() {
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 100f, 100f)
        .recordBox(5f, 5f, 110f, 110f)
        .recordBox(0f, 0f, 100f, 100f)
        .boxShadow(0, 1, 0xFF000000, 5f, 0)
        .end();

    setDisplayList(displayListApplier, testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas, times(2)).save();
    verify(mockCanvas).drawPath(any(android.graphics.Path.class), any(Paint.class));
    verify(mockCanvas).restoreToCount(anyInt());
    verify(mockCanvas).restore();
  }
}
