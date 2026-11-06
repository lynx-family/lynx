// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import static org.junit.Assert.*;

import android.graphics.Typeface;
import android.os.Build;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextPaint;
import android.text.style.MetricAffectingSpan;
import android.text.style.StyleSpan;
import android.util.SparseArray;
import androidx.test.filters.SdkSuppress;
import com.lynx.react.bridge.mapbuffer.ReadableCompactArrayBuffer;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.shadow.MeasureMode;
import com.lynx.tasm.event.LynxDetailEvent;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
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

  // The compact text protocol uses eight-byte slots followed by string data.
  private ReadableCompactArrayBuffer textProps(Object... values) {
    int dynamicSize = 0;
    for (Object value : values) {
      if (value instanceof String) {
        dynamicSize += 4 + ((String) value).getBytes(StandardCharsets.UTF_8).length;
      }
    }
    int dynamicStart = 8 + values.length * 8;
    ByteBuffer buffer = ByteBuffer.allocate(dynamicStart + dynamicSize);
    buffer.order(ByteOrder.nativeOrder());
    int dynamicOffset = 0;
    for (int i = 0; i < values.length; i++) {
      int slot = 8 + i * 8;
      Object value = values[i];
      if (value instanceof String) {
        byte[] bytes = ((String) value).getBytes(StandardCharsets.UTF_8);
        buffer.putInt(slot, dynamicOffset);
        buffer.position(dynamicStart + dynamicOffset);
        buffer.putInt(bytes.length);
        buffer.put(bytes);
        dynamicOffset += 4 + bytes.length;
      } else if (value instanceof Double) {
        buffer.putDouble(slot, (Double) value);
      } else {
        buffer.putInt(slot, (Integer) value);
      }
    }
    return ReadableCompactArrayBuffer.fromByteBufferWithCount(buffer.array(), values.length);
  }

  @Test
  public void fontVariationProtocolDecodesAxesAndClearsOnReplacement() {
    // 25 = font variations, followed by axis count and (tag, double) pairs.
    mMeasurer.dispatchLayoutBefore(
        SIGN, textProps(25, 2, "wght", 500.0, "opsz", 17.25, 3, 30.0, 2, "Font axes"));
    AttributedTextBundle bundle = (AttributedTextBundle) mAttributedTextBundles.get(SIGN);
    assertEquals("'wght' 500,'opsz' 17.25", bundle.getTextAttributes().getFontVariationSettings());

    mMeasurer.dispatchLayoutBefore(SIGN, textProps(25, 0, 3, 30.0, 2, "Font axes"));
    bundle = (AttributedTextBundle) mAttributedTextBundles.get(SIGN);
    assertEquals("", bundle.getTextAttributes().getFontVariationSettings());
  }

  @Test
  @SdkSuppress(minSdkVersion = 26)
  public void normalWeightInlineFontVariationsApplyAndReset() {
    mMeasurer.dispatchLayoutBefore(
        SIGN, textProps(0, 0, 3, 30.0, 25, 1, "wght", 625.5, 1, 4, 3, 30.0, 2, "Axes"));
    AttributedTextBundle bundle = (AttributedTextBundle) mAttributedTextBundles.get(SIGN);
    Spanned span = (Spanned) bundle.getSpan();
    TextPaint paint = new TextPaint();
    paint.setTypeface(Typeface.DEFAULT);
    for (MetricAffectingSpan style : span.getSpans(0, span.length(), MetricAffectingSpan.class)) {
      style.updateMeasureState(paint);
    }
    assertEquals("'wght' 625.5", paint.getFontVariationSettings());

    mMeasurer.dispatchLayoutBefore(SIGN, textProps(0, 0, 3, 30.0, 25, 0, 1, 4, 3, 30.0, 2, "Axes"));
    bundle = (AttributedTextBundle) mAttributedTextBundles.get(SIGN);
    span = (Spanned) bundle.getSpan();
    for (MetricAffectingSpan style : span.getSpans(0, span.length(), MetricAffectingSpan.class)) {
      style.updateMeasureState(paint);
    }
    assertNull(paint.getFontVariationSettings());
  }

  @Test
  @SdkSuppress(minSdkVersion = 26)
  public void inlineFontVariationsPreserveInheritedWeightAndStyle() {
    for (int style : new int[] {Typeface.BOLD, Typeface.ITALIC, Typeface.BOLD_ITALIC}) {
      for (boolean reset : new boolean[] {false, true}) {
        // Omit inline weight/style, as with CSS inheritance disabled.
        Spanned text = inlineFontVariations(reset, false, style);
        assertInlineTypeface(text, style, reset);
      }
    }
  }

  @Test
  @SdkSuppress(minSdkVersion = 26)
  public void boldInlineFontVariationsAndResetPreserveBold() {
    for (boolean reset : new boolean[] {false, true}) {
      Spanned text = inlineFontVariations(reset, true, Typeface.NORMAL);
      if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P) {
        StyleSpan[] styles = text.getSpans(0, text.length(), StyleSpan.class);
        assertEquals(1, styles.length);
        assertEquals(Typeface.BOLD, styles[0].getStyle());
      }
      assertInlineTypeface(text, Typeface.BOLD, reset);
    }
  }

  private Spanned inlineFontVariations(boolean reset, boolean inlineBold, int parentStyle) {
    List<Object> props = new ArrayList<>();
    Collections.addAll(props, 0, 0, 3, 30.0);
    if (inlineBold) {
      Collections.addAll(props, 7, StyleConstants.FONTWEIGHT_BOLD);
    }
    Collections.addAll(props, 25, reset ? 0 : 1);
    if (!reset) {
      Collections.addAll(props, "opsz", 18.0);
    }
    Collections.addAll(props, 1, 4, 3, 30.0, 7,
        (parentStyle & Typeface.BOLD) != 0 ? StyleConstants.FONTWEIGHT_BOLD
                                           : StyleConstants.FONTWEIGHT_NORMAL,
        8,
        (parentStyle & Typeface.ITALIC) != 0 ? StyleConstants.FONTSTYLE_ITALIC
                                             : StyleConstants.FONTSTYLE_NORMAL,
        2, "Axes");
    mMeasurer.dispatchLayoutBefore(SIGN, textProps(props.toArray()));
    return (Spanned) ((AttributedTextBundle) mAttributedTextBundles.get(SIGN)).getSpan();
  }

  private void assertInlineTypeface(Spanned text, int expectedStyle, boolean reset) {
    mMeasurer.measureText(SIGN, 300, MeasureMode.EXACTLY.intValue(), 0,
        MeasureMode.UNDEFINED.intValue(), new float[0]);
    TextUpdateBundle bundle = (TextUpdateBundle) mMeasurer.takeTextLayout(SIGN);
    for (boolean draw : new boolean[] {false, true}) {
      TextPaint paint = new TextPaint(bundle.getTextLayout().getPaint());
      if (reset) {
        paint.setFontVariationSettings("'wght' 900");
      }
      for (MetricAffectingSpan span : text.getSpans(0, text.length(), MetricAffectingSpan.class)) {
        if (draw) {
          span.updateDrawState(paint);
        } else {
          span.updateMeasureState(paint);
        }
      }
      assertEquals(expectedStyle, paint.getTypeface().getStyle());
      if (reset) {
        assertNull(paint.getFontVariationSettings());
      }
    }
  }

  @Test
  @SdkSuppress(minSdkVersion = 26)
  public void inlineFontVariationsDistinguishLayoutCacheKeys() {
    TextRendererKey light = inlineVariationKey(100);
    TextRendererKey heavy = inlineVariationKey(900);
    TextRendererKey reset = inlineVariationKey(0);
    assertFalse(light.equals(heavy));
    assertFalse(light.equals(reset));
    assertEquals(light, inlineVariationKey(100));
    assertEquals(light.hashCode(), inlineVariationKey(100).hashCode());
    assertEquals(reset, inlineVariationKey(0));
  }

  private TextRendererKey inlineVariationKey(double weight) {
    mMeasurer.dispatchLayoutBefore(SIGN,
        weight == 0 ? textProps(0, 0, 3, 30.0, 25, 0, 1, 4, 3, 30.0, 2, "Axes")
                    : textProps(0, 0, 3, 30.0, 25, 1, "wght", weight, 1, 4, 3, 30.0, 2, "Axes"));
    AttributedTextBundle bundle = (AttributedTextBundle) mAttributedTextBundles.get(SIGN);
    return new TextRendererKey(bundle.getSpan(), bundle.getTextAttributes(), MeasureMode.EXACTLY,
        MeasureMode.UNDEFINED, 300, 0, 0, false, true, true);
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
