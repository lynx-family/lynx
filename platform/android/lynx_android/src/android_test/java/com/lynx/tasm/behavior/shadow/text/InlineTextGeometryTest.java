// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import static org.junit.Assert.*;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.graphics.PointF;
import android.text.Layout;
import android.text.StaticLayout;
import android.text.TextPaint;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.shadow.TextLayout;
import org.junit.Test;

public class InlineTextGeometryTest {
  private TextUpdateBundle bundle(String text, int width) {
    TextPaint paint = new TextPaint();
    paint.setTextSize(20);
    StaticLayout layout = StaticLayout.Builder.obtain(text, 0, text.length(), paint, width)
                              .setAlignment(Layout.Alignment.ALIGN_NORMAL)
                              .setIncludePad(false)
                              .build();
    TextUpdateBundle bundle = new TextUpdateBundle(layout, false, null, false);
    bundle.setTextTranslateOffset(new PointF(3, 5));
    return bundle;
  }

  @Test
  public void wrappedRangesRetainSeparateLinesAndTextOffset() {
    String text = "prefix multiple wrapped words suffix";
    TextUpdateBundle bundle = bundle(text, 100);
    Layout layout = bundle.getTextLayout();
    float[] regions = InlineTextGeometry.getRegions(bundle, new int[] {42, 7, 29});
    assertTrue(regions.length > 5);
    assertEquals(0, regions.length % 5);
    for (int i = 0; i < regions.length; i += 5) {
      assertEquals(42, Float.floatToRawIntBits(regions[i]));
      assertTrue(regions[i + 3] > 0);
      assertTrue(regions[i + 4] > 0);
      assertTrue(regions[i + 1] >= layout.getLineLeft(i / 5) + 3);
      assertTrue(regions[i + 2] >= 5);
      if (i > 0)
        assertTrue(regions[i + 2] > regions[i - 3]);
    }
    float[] wide = InlineTextGeometry.getRegions(bundle(text, 1000), new int[] {42, 7, 29});
    assertEquals(5, wide.length);
    assertEquals(5, wide[2], 0.01f);
    assertEquals(bundle(text, 1000).getTextLayout().getPrimaryHorizontal(7) + 3, wide[1], 0.01f);
  }

  @Test
  public void mixedDirectionsPreserveGapsBetweenRunsOnTheSameLine() {
    TextUpdateBundle bundle = bundle("abc אבג xyz", 1000);
    float[] regions = InlineTextGeometry.getRegions(bundle, new int[] {42, 0, 5});
    assertEquals(10, regions.length);
    assertEquals(5, regions[2], 0.01f);
    assertEquals(5, regions[7], 0.01f);
    assertTrue(regions[1] + regions[3] < regions[6]);
    assertEquals(bundle.getTextLayout().getPrimaryHorizontal(5) + 3, regions[6], 0.01f);
  }

  @Test
  public void selectionEndingAtSoftWrapDoesNotIncludeTrailingBlankArea() {
    TextUpdateBundle bundle = bundle("word word word", 70);
    Layout layout = bundle.getTextLayout();
    assertTrue(layout.getLineCount() > 1);
    float[] regions =
        InlineTextGeometry.getRegions(bundle, new int[] {42, 0, layout.getLineEnd(0)});
    assertEquals(5, regions.length);
    assertEquals(layout.getLineRight(0) + 3, regions[1] + regions[3], 0.01f);
  }

  @Test
  public void onlyRequestedRangesAreReturnedInOrder() {
    TextUpdateBundle bundle = bundle("first second", 1000);
    float[] regions =
        InlineTextGeometry.getRegions(bundle, new int[] {Integer.MAX_VALUE, 6, 12, 10, 0, 5});
    assertEquals(10, regions.length);
    assertEquals(Integer.MAX_VALUE, Float.floatToRawIntBits(regions[0]));
    assertEquals(10, Float.floatToRawIntBits(regions[5]));
    assertTrue(regions[1] > regions[6] + regions[8]);
  }

  @Test
  public void unavailableAndEmptyRangesHaveNoGeometry() {
    assertEquals(0, InlineTextGeometry.getRegions(null, new int[] {42, 0, 1}).length);
    assertEquals(0, InlineTextGeometry.getRegions(new Object(), new int[] {42, 0, 1}).length);
    TextUpdateBundle bundle = bundle("text", 100);
    assertEquals(0, InlineTextGeometry.getRegions(bundle, new int[0]).length);
    assertEquals(0, InlineTextGeometry.getRegions(bundle, new int[] {42, 2, 2}).length);
    assertEquals(0, InlineTextGeometry.getRegions(bundle, new int[] {42, 0, 99}).length);
  }

  @Test
  public void newlineDoesNotCreateAFullWidthLinkRegion() {
    TextUpdateBundle bundle = bundle("prefix link\nnext", 1000);
    float[] regions = InlineTextGeometry.getRegions(bundle, new int[] {42, 7, 12});
    assertEquals(5, regions.length);
    Layout layout = bundle.getTextLayout();
    assertEquals(layout.getPrimaryHorizontal(7) + 3, regions[1], 0.01f);
    assertEquals(
        layout.getPrimaryHorizontal(11) - layout.getPrimaryHorizontal(7), regions[3], 0.01f);
  }

  @Test
  public void legacyOwnerGeometryTracksMeasuredAndReleasedText() {
    LynxUIOwner owner = mock(LynxUIOwner.class);
    TextLayout layout = new TextLayout(owner);
    TextUpdateBundle measured = bundle("prefix linked words suffix", 100);
    int[] ranges = new int[] {42, 7, 19};
    when(owner.takeTextLayout(11)).thenReturn(measured);
    assertArrayEquals(InlineTextGeometry.getRegions(measured, ranges),
        layout.getInlineTextRegions(11, ranges), 0.f);
    verify(owner).takeTextLayout(11);

    when(owner.takeTextLayout(11)).thenReturn(null);
    assertEquals(0, layout.getInlineTextRegions(11, ranges).length);
  }
}
