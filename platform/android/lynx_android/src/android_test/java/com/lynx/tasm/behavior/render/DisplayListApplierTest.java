// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
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
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Shader;
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
import java.lang.reflect.Field;
import java.nio.ByteBuffer;
import java.util.ArrayList;
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
    testDisplayList.begin(0, VIEW_TYPE, 0f, 0f, 100f, 50f);

    setDisplayList(displayListApplier, testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas).save();
    verify(mockCanvas).translate(0f, 0f);
  }

  @Test
  public void testOverlayOpBeginKeepsHorizontalOffsetOnly() {
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
    verify(mockCanvas).translate(10f, 0f);
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

    setDisplayList(displayListApplier, testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas).save();
    // Image drawing would be implemented with actual image data lookup
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
    verify(mockImageManager).setView(mockHostView);
    verify(mockImageManager).updateInnerClipPathForBorderRadius(null);
    verify(mockImageManager).updateDrawableBounds(eq(new Rect(0, 0, 40, 20)));
    verify(mockImageManager).updateDrawableBounds(eq(new Rect(40, 0, 80, 20)));
    verify(mockImageManager).updateDrawableBounds(eq(new Rect(80, 0, 120, 20)));
    verify(mockImageManager, times(3)).onDraw(mockCanvas);
    verify(mockCanvas, times(2)).restore();
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
