// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import static org.junit.Assert.*;

import android.text.SpannableStringBuilder;
import android.util.SparseArray;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.shadow.MeasureMode;
import com.lynx.tasm.event.LynxDetailEvent;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import java.util.List;
import java.util.Map;
import org.junit.Before;
import org.junit.Test;

public class TextMeasurerTest {
  private static final int SIGN = 1;
  private TextMeasurer mMeasurer;
  private SparseArray<Object> mAttributedTextBundles;

  @Before
  @SuppressWarnings("unchecked")
  public void setUp() throws Exception {
    mMeasurer = new TextMeasurer(TestingUtils.getLynxContext());
    Field field = TextMeasurer.class.getDeclaredField("mAttributedTextBundles");
    field.setAccessible(true);
    mAttributedTextBundles = (SparseArray<Object>) field.get(mMeasurer);
  }

  private float[] measure(String text, int maxLines, int overflow, float width) {
    TextAttributes attributes = new TextAttributes();
    attributes.setFontSize(30);
    attributes.setMaxLineCount(maxLines);
    attributes.setTextOverflow(overflow);
    mAttributedTextBundles.put(
        SIGN, new AttributedTextBundle(new SpannableStringBuilder(text), attributes));
    return mMeasurer.measureText(SIGN, width, MeasureMode.EXACTLY.intValue(), 0,
        MeasureMode.UNDEFINED.intValue(), new float[0]);
  }

  @SuppressWarnings("unchecked")
  private void assertMatchesLegacyEvent(float[] result) {
    TextUpdateBundle bundle = (TextUpdateBundle) mMeasurer.takeTextLayout(SIGN);
    LynxDetailEvent event = TextHelper.getTextLayoutEvent(SIGN, bundle.getTextLayout(),
        bundle.getLayoutEventTextOverflow(), bundle.getLayoutEventLineCount(),
        bundle.getLayoutEventEllipsisCount(), bundle.getLayoutEventSpannableStringLength(),
        bundle.getLayoutEventTextLayoutWidth(), true);
    Map<String, Object> details = event.eventParams();
    int lineCount = (int) result[3];
    assertEquals(details.get("lineCount"), lineCount);
    assertEquals(6 + lineCount * 3, result.length);
    List<Map<String, Integer>> lines = (List<Map<String, Integer>>) details.get("lines");
    for (int i = 0; i < lineCount; ++i) {
      assertEquals(lines.get(i).get("start").intValue(), (int) result[4 + i * 3]);
      assertEquals(lines.get(i).get("end").intValue(), (int) result[5 + i * 3]);
      assertEquals(lines.get(i).get("ellipsisCount").intValue(), (int) result[6 + i * 3]);
    }
    Map<String, Float> size = (Map<String, Float>) details.get("size");
    assertEquals(size.get("width"), result[4 + lineCount * 3], 0.f);
    assertEquals(size.get("height"), result[5 + lineCount * 3], 0.f);
    assertFalse(bundle.hasDispatchedLayoutEvent());
  }

  @Test
  public void eventDimensionsPreserveTextSizeAndDipUnits() {
    float[] result = measure("short", 10, StyleConstants.TEXTOVERFLOW_CLIP, 600);
    assertMatchesLegacyEvent(result);
    assertEquals(600.f, result[0], 0.f);
    assertTrue(result[result.length - 2] < result[0] / 3.f);
  }

  @Test
  public void utf16AndTrailingNewlineMatchLegacyEvent() {
    assertMatchesLegacyEvent(
        measure("ab\ud83d\ude00\ncd\n", 10, StyleConstants.TEXTOVERFLOW_CLIP, 200));
  }

  @Test
  public void clippedLastLineMatchesLegacyEvent() {
    float[] result =
        measure("first line\nsecond line\nthird line", 1, StyleConstants.TEXTOVERFLOW_CLIP, 200);
    assertMatchesLegacyEvent(result);
    assertTrue(result[6] > 0);
  }

  @Test
  public void ellipsizedLastLineMatchesLegacyEvent() {
    float[] result = measure("long text that must be truncated at the first line", 1,
        StyleConstants.TEXTOVERFLOW_ELLIPSIS, 80);
    assertMatchesLegacyEvent(result);
    assertTrue(result[6] > 0);
  }

  @Test
  public void remeasurementReplacesMetadataAndInstallationBundle() {
    float[] first = measure("first line\nsecond line", 10, StyleConstants.TEXTOVERFLOW_CLIP, 200);
    Object firstBundle = mMeasurer.takeTextLayout(SIGN);
    float[] latest = measure("new", 10, StyleConstants.TEXTOVERFLOW_CLIP, 200);
    assertMatchesLegacyEvent(latest);
    assertNotSame(firstBundle, mMeasurer.takeTextLayout(SIGN));
    assertTrue(first[3] > latest[3]);
  }

  @Test
  public void missingTextDoesNotReturnPendingEventMetadata() {
    mMeasurer.releaseLayoutObject(SIGN);
    float[] result = mMeasurer.measureText(SIGN, 200, MeasureMode.EXACTLY.intValue(), 0,
        MeasureMode.UNDEFINED.intValue(), new float[0]);
    assertEquals(3, result.length);
    assertNull(mMeasurer.takeTextLayout(SIGN));
  }
}
