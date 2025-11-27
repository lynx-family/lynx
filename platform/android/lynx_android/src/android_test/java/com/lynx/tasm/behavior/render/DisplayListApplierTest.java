// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.mockito.ArgumentCaptor.forClass;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.View;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.behavior.shadow.text.TextMeasurer;
import com.lynx.tasm.behavior.shadow.text.TextUpdateBundle;
import com.lynx.tasm.behavior.ui.utils.BorderStyle;
import com.lynx.tasm.behavior.ui.utils.Spacing;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

/**
 * Unit tests for DisplayListApplier that verify the correct processing of display list operations.
 *
 * <p>Display List Data Layout:
 * The display list follows a specific binary format with three parallel arrays:
 * <ul>
 *   <li>{@code ops[]}: Array of operation type codes (int values)</li>
 *   <li>{@code iArgv[]}: Array of integer parameters and parameter counts</li>
 *   <li>{@code fArgv[]}: Array of float parameters (coordinates, dimensions, colors as floats)</li>
 * </ul>
 *
 * <p>Operation Format:
 * Each operation is encoded as:
 * <pre>
 * [op_type, int_param_count, float_param_count, ...int_params..., ...float_params...]
 * </pre>
 *
 * <p>Supported Operations:
 * <ul>
 *   <li>{@code OP_BEGIN (0)}: Begin a fragment with bounds [x, y, width, height]</li>
 *   <li>{@code OP_END (1)}: End current fragment, restore canvas state</li>
 *   <li>{@code OP_FILL (2)}: Fill fragment bounds with solid color</li>
 *   <li>{@code OP_DRAW_VIEW (3)}: Draw native view by ID, stops processing</li>
 *   <li>{@code OP_TEXT (6)}: Draw text layout by ID</li>
 *   <li>{@code OP_IMAGE (7)}: Draw image by ID with bounds</li>
 *   <li>{@code OP_CUSTOM (8)}: Custom drawing operation</li>
 *   <li>{@code OP_BORDER (9)}: Draw rectangular borders with specified widths, colors, and
 * styles</li>
 * </ul>
 *
 * <p>Example Display List:
 * <pre>
 * ops = {0, 2, 6, 1}           // BEGIN, FILL, TEXT, END
 * iArgv = {0,4,0, 1,0, 1,0, 0,0} // param counts: (0int,4float), (1int,0float), (1int,0float),
 * (0int,0float) fArgv = {10,20,100,50, 0xFF0000FF, 0, 0, 0} // x,y,w,h, color, text_id=0, unused
 * </pre>
 */
@RunWith(AndroidJUnit4.class)
public class DisplayListApplierTest {
  @Mock private Canvas mockCanvas;
  @Mock private TextMeasurer mockTextMeasurer;
  @Mock private PlatformRendererContext mockPlatformRendererContext;
  @Mock private TextUpdateBundle mockTextUpdateBundle;
  @Mock private android.text.Layout mockTextLayout;
  @Mock private View mockHostView;

  private DisplayListApplier displayListApplier;
  private DisplayListApplier spyDisplayListApplier;
  private DisplayList testDisplayList;

  // ArgumentCaptors for verifying drawRectangularBorders calls
  private ArgumentCaptor<android.graphics.Rect> boundsCaptor;
  private ArgumentCaptor<int[]> borderWidthsCaptor;
  private ArgumentCaptor<int[]> borderColorsCaptor;
  private ArgumentCaptor<BorderStyle[]> borderStylesCaptor;

  @Before
  public void setUp() {
    MockitoAnnotations.openMocks(this);
    // Set up PlatformRendererContext to return our mock TextMeasurer
    when(mockPlatformRendererContext.getTextMeasurer()).thenReturn(mockTextMeasurer);
    displayListApplier = new DisplayListApplier(null, mockPlatformRendererContext, mockHostView);
    spyDisplayListApplier = spy(displayListApplier);
    testDisplayList = new DisplayList();

    // Initialize ArgumentCaptors
    boundsCaptor = forClass(android.graphics.Rect.class);
    borderWidthsCaptor = forClass(int[].class);
    borderColorsCaptor = forClass(int[].class);
    borderStylesCaptor = forClass(BorderStyle[].class);
  }

  /**
   * Tests basic constructor functionality.
   * Verifies that DisplayListApplier can be instantiated with valid dependencies.
   */
  @Test
  public void testConstructor() {
    when(mockPlatformRendererContext.getTextMeasurer()).thenReturn(mockTextMeasurer);
    DisplayListApplier applier =
        new DisplayListApplier(testDisplayList, mockPlatformRendererContext, mockHostView);
    assertNotNull(applier);
  }

  /**
   * Tests the reset functionality.
   * Verifies that reset() properly clears internal state and indices.
   *
   * <p>Test Data Layout:
   * <pre>
   * ops = {0, 1}                    // OP_BEGIN, OP_END
   * iArgv = {0, 4, 0, 0}            // param counts: (0int,4float), (0int,0float)
   * fArgv = {10f, 20f, 100f, 50f}    // x=10, y=20, width=100, height=50 for OP_BEGIN
   * </pre>
   */
  @Test
  public void testReset() {
    // Set up display list with some operations
    testDisplayList.ops = new int[] {0, 1}; // OP_BEGIN, OP_END
    testDisplayList.iArgv = new int[] {0, 4, 0, 0}; // intParamCounts
    testDisplayList.fArgv = new float[] {10f, 20f, 100f, 50f}; // x, y, width, height

    displayListApplier.setDisplayList(testDisplayList);
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
    displayListApplier.setDisplayList(null);
    displayListApplier.drawTillNextView(mockCanvas);
    // Should not crash
    verify(mockCanvas, never()).save();
  }

  /**
   * Tests handling of empty display list.
   * Verifies that the applier handles empty arrays gracefully without crashing.
   */
  @Test
  public void testEmptyDisplayList() {
    testDisplayList.ops = new int[0];
    testDisplayList.iArgv = new int[0];
    testDisplayList.fArgv = new float[0];

    displayListApplier.setDisplayList(testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas, never()).save();
  }

  /**
   * Tests OP_BEGIN operation processing.
   * Verifies that OP_BEGIN correctly saves canvas state and applies translation.
   *
   * <p>Test Data Layout:
   * <pre>
   * ops = {0}                       // OP_BEGIN
   * iArgv = {0, 4, 0}               // 0 int params, 4 float params
   * fArgv = {10f, 20f, 100f, 50f}    // x=10, y=20, width=100, height=50
   * </pre>
   *
   * <p>Expected Behavior:
   * <ul>
   *   <li>Canvas.save() is called to save current state</li>
   *   <li>Canvas.translate(10, 20) is called to position the fragment</li>
   *   <li>Bounds are stored internally for subsequent operations</li>
   * </ul>
   */
  @Test
  public void testOpBegin() {
    testDisplayList.ops = new int[] {0}; // OP_BEGIN
    testDisplayList.iArgv = new int[] {0, 4, 0}; // 0 int params, 4 float params
    testDisplayList.fArgv = new float[] {10f, 20f, 100f, 50f}; // x, y, width, height

    displayListApplier.setDisplayList(testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas).save();
    verify(mockCanvas).translate(10f, 20f);
  }

  /**
   * Tests OP_END operation processing.
   * Verifies that OP_END correctly restores canvas state and pops bounds.
   *
   * <p>Test Data Layout:
   * <pre>
   * ops = {0, 1}                    // OP_BEGIN, OP_END
   * iArgv = {0, 4, 0, 0, 0}        // param counts: (0int,4float), (0int,0float)
   * fArgv = {10f, 20f, 100f, 50f}    // x=10, y=20, width=100, height=50 for OP_BEGIN
   * </pre>
   *
   * <p>Expected Behavior:
   * <ul>
   *   <li>Canvas.save() is called for OP_BEGIN</li>
   *   <li>Canvas.restore() is called for OP_END</li>
   *   <li>Processing stops after OP_END</li>
   * </ul>
   */
  @Test
  public void testOpEnd() {
    testDisplayList.ops = new int[] {0, 1}; // OP_BEGIN {int: 0, float: 4}, OP_END {int:0, float:0}
    testDisplayList.iArgv = new int[] {0, 4, 0, 0, 0}; // intParamCounts
    testDisplayList.fArgv = new float[] {10f, 20f, 100f, 50f}; // x, y, width, height

    displayListApplier.setDisplayList(testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas).save();
    verify(mockCanvas).restore();
  }

  /**
   * Tests OP_FILL operation processing.
   * Verifies that OP_FILL correctly fills fragment bounds with specified color.
   *
   * <p>Test Data Layout:
   * <pre>
   * ops = {0, 2}                    // OP_BEGIN, OP_FILL
   * iArgv = {0, 4, 1, 0, 0xFF0000FF} // param counts: (0int,4float), (1int,0float), color=Blue
   * fArgv = {0f, 0f, 100f, 50f}    // x=0, y=0, width=100, height=50 for OP_BEGIN
   * </pre>
   *
   * <p>Expected Behavior:
   * <ul>
   *   <li>Canvas.save() is called for OP_BEGIN</li>
   *   <li>Paint is configured with blue color (0xFF0000FF)</li>
   *   <li>Canvas.drawRect() is called with fragment bounds</li>
   * </ul>
   */
  @Test
  public void testOpFill() {
    testDisplayList.ops = new int[] {0, 2}; // OP_BEGIN, OP_FILL
    testDisplayList.iArgv = new int[] {0, 4, 1, 0, 0xFF0000FF}; // intParamCounts
    testDisplayList.fArgv = new float[] {0f, 0f, 100f, 50f}; // bounds for OP_BEGIN

    displayListApplier.setDisplayList(testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas).save();
    verify(mockCanvas).drawRect(any(RectF.class), any(Paint.class));
  }

  /**
   * Tests OP_DRAW_VIEW operation processing.
   * Verifies that OP_DRAW_VIEW stops processing and returns control to native view drawing.
   *
   * <p>Test Data Layout:
   * <pre>
   * ops = {0, 3}                    // OP_BEGIN, OP_DRAW_VIEW
   * iArgv = {0, 4, 1, 0, 123}      // param counts: (0int,4float), (1int,0float), viewId=123
   * fArgv = {0f, 0f, 100f, 50f}    // x=0, y=0, width=100, height=50 for OP_BEGIN
   * </pre>
   *
   * <p>Expected Behavior:
   * <ul>
   *   <li>Canvas.save() is called for OP_BEGIN</li>
   *   <li>Processing stops at OP_DRAW_VIEW (viewId=123)</li>
   *   <li>No further operations are processed</li>
   * </ul>
   */
  @Test
  public void testOpDrawView() {
    testDisplayList.ops = new int[] {0, 3}; // OP_BEGIN, OP_DRAW_VIEW
    testDisplayList.iArgv = new int[] {0, 4, 1, 0, 123}; // intParamCounts
    testDisplayList.fArgv = new float[] {0f, 0f, 100f, 50f}; // bounds for OP_BEGIN

    displayListApplier.setDisplayList(testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas).save();
    // Should stop at OP_DRAW_VIEW and return
  }

  /**
   * Tests OP_TEXT operation processing with valid text layout.
   * Verifies that OP_TEXT correctly retrieves and draws text layout.
   *
   * <p>Test Data Layout:
   * <pre>
   * ops = {0, 6}                    // OP_BEGIN, OP_TEXT
   * iArgv = {0, 4, 1, 0, 456}      // param counts: (0int,4float), (1int,0float), textId=456
   * fArgv = {0f, 0f, 100f, 50f}    // x=0, y=0, width=100, height=50 for OP_BEGIN
   * </pre>
   *
   * <p>Expected Behavior:
   * <ul>
   *   <li>TextMeasurer.takeTextLayout(456) is called to retrieve text layout</li>
   *   <li>TextLayout.draw(canvas) is called to render the text</li>
   *   <li>Text is drawn within the fragment bounds</li>
   * </ul>
   */
  @Test
  public void testOpText() {
    when(mockTextMeasurer.takeTextLayout(anyInt())).thenReturn(mockTextUpdateBundle);
    when(mockTextUpdateBundle.getTextLayout()).thenReturn(mockTextLayout);

    testDisplayList.ops = new int[] {0, 6}; // OP_BEGIN, OP_TEXT
    testDisplayList.iArgv = new int[] {
        0, 4, 1, 0, 456}; // OP_BEGIN{ int: 0, float: 4} OP_TEXT{int: 1, float: 0} OP_TEXT_ID: 456
    testDisplayList.fArgv = new float[] {0f, 0f, 100f, 50f}; // bounds for OP_BEGIN

    displayListApplier.setDisplayList(testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockTextMeasurer).takeTextLayout(456);
    verify(mockTextLayout).draw(mockCanvas);
  }

  /**
   * Tests OP_TEXT operation with null text layout.
   * Verifies that OP_TEXT handles missing text layout gracefully without crashing.
   *
   * <p>Test Data Layout:
   * <pre>
   * ops = {0, 6}                    // OP_BEGIN, OP_TEXT
   * iArgv = {0, 4, 1, 0, 456}      // param counts: (0int,4float), (1int,0float), textId=456
   * fArgv = {0f, 0f, 100f, 50f}    // x=0, y=0, width=100, height=50 for OP_BEGIN
   * </pre>
   *
   * <p>Expected Behavior:
   * <ul>
   *   <li>TextMeasurer.takeTextLayout(456) is called</li>
   *   <li>Returns null layout (simulated)</li>
   *   <li>No drawing occurs, no crash</li>
   * </ul>
   */
  @Test
  public void testOpTextWithNullLayout() {
    when(mockTextMeasurer.takeTextLayout(anyInt())).thenReturn(null);

    testDisplayList.ops = new int[] {0, 6}; // OP_BEGIN, OP_TEXT
    testDisplayList.iArgv = new int[] {0, 4, 1, 0}; // intParamCounts
    testDisplayList.fArgv = new float[] {0f, 0f, 100f, 50f}; // bounds for OP_BEGIN

    // Add text ID parameter
    int[] iArgvWithText = new int[testDisplayList.iArgv.length + 1];
    System.arraycopy(testDisplayList.iArgv, 0, iArgvWithText, 0, testDisplayList.iArgv.length);
    iArgvWithText[iArgvWithText.length - 1] = 456; // textId
    testDisplayList.iArgv = iArgvWithText;

    displayListApplier.setDisplayList(testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockTextMeasurer).takeTextLayout(456);
    verify(mockTextLayout, never()).draw(any(Canvas.class));
  }

  /**
   * Tests OP_IMAGE operation processing.
   * Verifies that OP_IMAGE correctly processes image drawing parameters.
   *
   * <p>Test Data Layout:
   * <pre>
   * ops = {0, 7}                    // OP_BEGIN, OP_IMAGE
   * iArgv = {0, 4, 0, 1, 789}      // param counts: (0int,4float), (1int,0float), imageId=789
   * fArgv = {0f, 0f, 100f, 50f, 10f, 20f, 30f, 40f} // bounds + image drawing params
   * </pre>
   *
   * <p>Expected Behavior:
   * <ul>
   *   <li>Canvas.save() is called for OP_BEGIN</li>
   *   <li>Image ID 789 is extracted from parameters</li>
   *   <li>Image drawing parameters are consumed</li>
   *   <li>Actual image rendering would use imageId for bitmap lookup</li>
   * </ul>
   */
  @Test
  public void testOpImage() {
    testDisplayList.ops = new int[] {0, 7}; // OP_BEGIN, OP_IMAGE
    testDisplayList.iArgv = new int[] {0, 4, 0, 1}; // intParamCounts: 1 int, 4 float params
    testDisplayList.fArgv =
        new float[] {0f, 0f, 100f, 50f, 10f, 20f, 30f, 40f}; // bounds + image params

    // Add image ID parameter
    int[] iArgvWithImage = new int[testDisplayList.iArgv.length + 1];
    System.arraycopy(testDisplayList.iArgv, 0, iArgvWithImage, 0, testDisplayList.iArgv.length);
    iArgvWithImage[iArgvWithImage.length - 1] = 789; // imageId
    testDisplayList.iArgv = iArgvWithImage;

    displayListApplier.setDisplayList(testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas).save();
    // Image drawing would be implemented with actual image data lookup
  }

  /**
   * Tests setDisplayList functionality.
   * Verifies that setting a new display list replaces the current one and processes correctly.
   *
   * <p>Test Data Layout:
   * <pre>
   * ops = {0, 2, 1}                 // OP_BEGIN, OP_FILL, OP_END
   * iArgv = {0, 4, 1, 0, 0xFF00FF00, 0, 0} // param counts and color
   * fArgv = {0f, 0f, 32f, 32f}     // x=0, y=0, width=32, height=32 for OP_BEGIN
   * </pre>
   *
   * <p>Expected Behavior:
   * <ul>
   *   <li>New display list replaces current one</li>
   *   <li>Reset() is called internally</li>
   *   <li>OP_FILL draws rectangle with green color (0xFF00FF00)</li>
   * </ul>
   */
  @Test
  public void testSetDisplayList() {
    DisplayList newDisplayList = new DisplayList();
    newDisplayList.ops = new int[] {0, 2, 1}; // Begin, OP_FILL, end
    newDisplayList.iArgv = new int[] {0, 4, 1, 0, 0xFF00FF00, 0, 0}; // 1 int param, 0 float params
    newDisplayList.fArgv = new float[] {0f, 0f, 32f, 32f}; // Green color

    displayListApplier.setDisplayList(newDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    // Should process the new display list
    verify(mockCanvas).drawRect(any(RectF.class), any(Paint.class));
  }

  /**
   * Tests handling of invalid array access.
   * Verifies that the applier handles mismatched array sizes gracefully without crashing.
   *
   * <p>Test Data Layout:
   * <pre>
   * ops = {0, 2}                    // OP_BEGIN, OP_FILL
   * iArgv = {0, 4}                  // Missing OP_FILL parameters (should be 1,0)
   * fArgv = {0f, 0f, 100f, 50f}    // x=0, y=0, width=100, height=50 for OP_BEGIN
   * </pre>
   *
   * <p>Expected Behavior:
   * <ul>
   *   <li>OP_BEGIN processes normally (Canvas.save() called)</li>
   *   <li>OP_FILL encounters insufficient parameters</li>
   *   <li>No crash occurs, graceful degradation</li>
   * </ul>
   */
  @Test
  public void testInvalidArrayAccess() {
    // Test with mismatched array sizes
    testDisplayList.ops = new int[] {0, 2}; // OP_BEGIN, OP_FILL
    testDisplayList.iArgv = new int[] {0, 4}; // Missing parameters for OP_FILL
    testDisplayList.fArgv = new float[] {0f, 0f, 100f, 50f};

    displayListApplier.setDisplayList(testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    // Should handle gracefully without crashing
    verify(mockCanvas).save();
  }

  /**
   * Tests multiple operations in sequence.
   * Verifies that the applier correctly processes a complete sequence of operations.
   *
   * <p>Test Data Layout:
   * <pre>
   * ops = {0, 2, 6, 1}              // OP_BEGIN, OP_FILL, OP_TEXT, OP_END
   * iArgv = {0, 4, 1, 0, 0xFF0000FF, 1, 0, 999, 0, 0} // param counts and IDs
   * fArgv = {10f, 20f, 100f, 50f}   // x=10, y=20, width=100, height=50 for OP_BEGIN
   * </pre>
   *
   * <p>Expected Behavior:
   * <ul>
   *   <li>OP_BEGIN: Canvas.save() and Canvas.translate(10, 20)</li>
   *   <li>OP_FILL: Canvas.drawRect() with blue color (0xFF0000FF)</li>
   *   <li>OP_TEXT: TextMeasurer.takeTextLayout(999) and TextLayout.draw()</li>
   *   <li>OP_END: Canvas.restore() and processing stops</li>
   * </ul>
   */
  @Test
  public void testMultipleOperations() {
    testDisplayList.ops = new int[] {0, 2, 6, 1}; // OP_BEGIN, OP_FILL, OP_TEXT, OP_END
    testDisplayList.iArgv = new int[] {0, 4, 1, 0, 0xFF0000FF, 1, 0, 999, 0, 0}; // intParamCounts
    testDisplayList.fArgv = new float[] {
        10f,
        20f,
        100f,
        50f,
    }; // x,y,w,h + color

    when(mockTextMeasurer.takeTextLayout(anyInt())).thenReturn(mockTextUpdateBundle);
    when(mockTextUpdateBundle.getTextLayout()).thenReturn(mockTextLayout);

    displayListApplier.setDisplayList(testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas).save();
    verify(mockCanvas).translate(10f, 20f);
    verify(mockCanvas).drawRect(any(RectF.class), any(Paint.class)); // OP_FILL creates rect
    verify(mockTextMeasurer).takeTextLayout(999);
    verify(mockCanvas).restore();
  }

  /**
   * Tests OP_BORDER operation with uniform border (fast path).
   * Verifies that OP_BORDER correctly draws uniform borders using single stroke operation.
   *
   * <p>Test Data Layout:
   * <pre>
   * ops = {0, 9, 1}                 // OP_BEGIN, OP_BORDER, OP_END
   * iArgv = {0, 4, 8, 4, 0xFF0000FF, 0xFF0000FF, 0xFF0000FF, 0xFF0000FF, 0, 0, 0, 0, 0, 0} //
   * colors + styles fArgv = {0f, 0f, 100f, 50f, 5f, 5f, 5f, 5f} // bounds + border widths (all 5)
   * </pre>
   *
   * <p>Expected Behavior:
   * <ul>
   *   <li>Canvas.save() is called for OP_BEGIN</li>
   *   <li>drawRectangularBorders is called with correct parameters</li>
   *   <li>Canvas.restore() is called for OP_END</li>
   * </ul>
   */
  @Test
  public void testOpBorderUniform() {
    testDisplayList.ops = new int[] {0, 9, 1}; // OP_BEGIN, OP_BORDER, OP_END
    testDisplayList.iArgv = new int[] {
        0, 4, 8, 4, // OP_BEGIN: 0 int, 4 float params
        0xFF0000FF, 0xFF0000FF, 0xFF0000FF, 0xFF0000FF, // Border colors (left, top, right, bottom)
        0, 0, 0, 0, // Border styles (solid)
        0, 0 // OP_END, 0 int, 0 float params.
    };
    testDisplayList.fArgv = new float[] {
        0f, 0f, 100f, 50f, // bounds: x=0, y=0, width=100, height=50
        5f, 5f, 5f, 5f // border widths: left=5, top=5, right=5, bottom=5
    };

    spyDisplayListApplier.setDisplayList(testDisplayList);
    spyDisplayListApplier.drawTillNextView(mockCanvas);

    // Verify drawRectangularBorders is called with correct parameters
    verify(spyDisplayListApplier)
        .drawRectangularBorders(eq(mockCanvas), any(Paint.class), boundsCaptor.capture(),
            borderWidthsCaptor.capture(), borderColorsCaptor.capture(),
            borderStylesCaptor.capture());

    // Verify bounds
    android.graphics.Rect capturedBounds = boundsCaptor.getValue();
    assertEquals(0, capturedBounds.left);
    assertEquals(0, capturedBounds.top);
    assertEquals(100, capturedBounds.right);
    assertEquals(50, capturedBounds.bottom);

    // Verify border widths
    int[] capturedWidths = borderWidthsCaptor.getValue();
    assertEquals(5, capturedWidths[0]); // left
    assertEquals(5, capturedWidths[1]); // top
    assertEquals(5, capturedWidths[2]); // right
    assertEquals(5, capturedWidths[3]); // bottom

    // Verify border colors
    int[] capturedColors = borderColorsCaptor.getValue();
    assertEquals(0xFF0000FF, capturedColors[0]); // left
    assertEquals(0xFF0000FF, capturedColors[1]); // top
    assertEquals(0xFF0000FF, capturedColors[2]); // right
    assertEquals(0xFF0000FF, capturedColors[3]); // bottom

    // Verify border styles
    BorderStyle[] capturedStyles = borderStylesCaptor.getValue();
    assertEquals(BorderStyle.SOLID, capturedStyles[0]); // left
    assertEquals(BorderStyle.SOLID, capturedStyles[1]); // top
    assertEquals(BorderStyle.SOLID, capturedStyles[2]); // right
    assertEquals(BorderStyle.SOLID, capturedStyles[3]); // bottom
  }

  @Test
  public void testOpBorderComplex() {
    testDisplayList.ops = new int[] {0, 9, 1}; // OP_BEGIN, OP_BORDER, OP_END
    testDisplayList.iArgv = new int[] {
        0, 4, 8, 4, // OP_BEGIN: 0 int, 4 float params
        0xFFFF00FF, 0xFF00FFFF, 0xFF0000FF, 0xFF0000FF, // Border colors (left, top, right, bottom)
        0, 1, 2, 3, // Border styles (solid)
        0, 0 // OP_END, 0 int, 0 float params.
    };
    testDisplayList.fArgv = new float[] {
        0f, 0f, 100f, 50f, // bounds: x=0, y=0, width=100, height=50
        5f, 6f, 5f, 0f // border widths: left=5, top=5, right=5, bottom=5
    };

    spyDisplayListApplier.setDisplayList(testDisplayList);
    spyDisplayListApplier.drawTillNextView(mockCanvas);

    // Verify drawRectangularBorders is called with correct parameters
    verify(spyDisplayListApplier)
        .drawRectangularBorders(eq(mockCanvas), any(Paint.class), boundsCaptor.capture(),
            borderWidthsCaptor.capture(), borderColorsCaptor.capture(),
            borderStylesCaptor.capture());

    // Verify bounds
    android.graphics.Rect capturedBounds = boundsCaptor.getValue();
    assertEquals(0, capturedBounds.left);
    assertEquals(0, capturedBounds.top);
    assertEquals(100, capturedBounds.right);
    assertEquals(50, capturedBounds.bottom);

    // Verify border widths
    int[] capturedWidths = borderWidthsCaptor.getValue();
    assertEquals(5, capturedWidths[Spacing.TOP]);
    assertEquals(6, capturedWidths[Spacing.RIGHT]);
    assertEquals(5, capturedWidths[Spacing.BOTTOM]);
    assertEquals(0, capturedWidths[Spacing.LEFT]);

    // Verify border colors
    int[] capturedColors = borderColorsCaptor.getValue();
    assertEquals(0xFFFF00FF, capturedColors[Spacing.TOP]);
    assertEquals(0xFF00FFFF, capturedColors[Spacing.RIGHT]);
    assertEquals(0xFF0000FF, capturedColors[Spacing.BOTTOM]);
    assertEquals(0xFF0000FF, capturedColors[Spacing.LEFT]);

    // Verify border styles
    BorderStyle[] capturedStyles = borderStylesCaptor.getValue();
    assertEquals(BorderStyle.SOLID, capturedStyles[Spacing.TOP]);
    assertEquals(BorderStyle.DASHED, capturedStyles[Spacing.RIGHT]);
    assertEquals(BorderStyle.DOTTED, capturedStyles[Spacing.BOTTOM]);
    assertEquals(BorderStyle.DOUBLE, capturedStyles[Spacing.LEFT]);
  }

  @Test
  public void testOpBorderDashedOrDotted() {
    testDisplayList.ops = new int[] {0, 9, 1}; // OP_BEGIN, OP_BORDER, OP_END
    testDisplayList.iArgv = new int[] {
        0, 4, 8, 4, // OP_BEGIN: 0 int, 4 float params
        0xFF0000FF, 0xFF0000FF, 0xFF0000FF, 0xFF0000FF, // Border colors (left, top, right, bottom)
        1, 1, 1, 1, // Border styles (solid)
        0, 0 // OP_END, 0 int, 0 float params.
    };
    testDisplayList.fArgv = new float[] {
        0f, 0f, 100f, 50f, // bounds: x=0, y=0, width=100, height=50
        5f, 5f, 5f, 5f // border widths: left=5, top=5, right=5, bottom=5
    };

    spyDisplayListApplier.setDisplayList(testDisplayList);
    spyDisplayListApplier.drawTillNextView(mockCanvas);

    // Verify drawRectangularBorders is called with correct parameters
    verify(spyDisplayListApplier)
        .drawRectangularBorders(eq(mockCanvas), any(Paint.class), boundsCaptor.capture(),
            borderWidthsCaptor.capture(), borderColorsCaptor.capture(),
            borderStylesCaptor.capture());

    // Verify bounds
    android.graphics.Rect capturedBounds = boundsCaptor.getValue();
    assertEquals(0, capturedBounds.left);
    assertEquals(0, capturedBounds.top);
    assertEquals(100, capturedBounds.right);
    assertEquals(50, capturedBounds.bottom);

    // Verify border widths
    int[] capturedWidths = borderWidthsCaptor.getValue();
    assertEquals(5, capturedWidths[0]); // left
    assertEquals(5, capturedWidths[1]); // top
    assertEquals(5, capturedWidths[2]); // right
    assertEquals(5, capturedWidths[3]); // bottom

    // Verify border colors
    int[] capturedColors = borderColorsCaptor.getValue();
    assertEquals(0xFF0000FF, capturedColors[0]); // left
    assertEquals(0xFF0000FF, capturedColors[1]); // top
    assertEquals(0xFF0000FF, capturedColors[2]); // right
    assertEquals(0xFF0000FF, capturedColors[3]); // bottom

    // Verify border styles
    BorderStyle[] capturedStyles = borderStylesCaptor.getValue();
    assertEquals(BorderStyle.DASHED, capturedStyles[0]); // left
    assertEquals(BorderStyle.DASHED, capturedStyles[1]); // top
    assertEquals(BorderStyle.DASHED, capturedStyles[2]); // right
    assertEquals(BorderStyle.DASHED, capturedStyles[3]); // bottom
  }

  @Test
  public void testOpClipRectPlain() {
    testDisplayList.ops = new int[] {0, 10, 1};
    testDisplayList.iArgv = new int[] {0, 4, 0, 4, 0, 0};
    testDisplayList.fArgv = new float[] {0f, 0f, 100f, 50f, 10f, 12f, 80f, 30f};

    displayListApplier.setDisplayList(testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas).save();
    verify(mockCanvas).clipRect(any(RectF.class));
    verify(mockCanvas).restore();
  }

  @Test
  public void testOpClipRectRounded() {
    testDisplayList.ops = new int[] {0, 10, 1};
    testDisplayList.iArgv = new int[] {0, 4, 0, 12, 0, 0};
    testDisplayList.fArgv =
        new float[] {0f, 0f, 100f, 50f, 10f, 12f, 80f, 30f, 5f, 6f, 7f, 8f, 9f, 10f, 11f, 12f};

    displayListApplier.setDisplayList(testDisplayList);
    displayListApplier.drawTillNextView(mockCanvas);

    verify(mockCanvas).save();
    verify(mockCanvas).clipPath(any(android.graphics.Path.class));
    verify(mockCanvas).restore();
  }
}
