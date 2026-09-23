// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import static org.junit.Assert.*;

import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextPaint;
import android.util.LruCache;
import com.lynx.tasm.behavior.shadow.MeasureMode;
import org.junit.Test;

public class CustomStyleSpanTest {
  private final TextPaint mTextPaint = new TextPaint();

  @Test
  public void testConstructorInitialization() {
    CustomStyleSpan span = new CustomStyleSpan(2, 7, "sans-serif", null, null, false);

    assertEquals("Style should be initialized", 2, span.getStyle());
  }

  @Test
  public void testEquals() {
    CustomStyleSpan span1 = new CustomStyleSpan(1, 0, "Arial", null, null, false);
    CustomStyleSpan span2 = new CustomStyleSpan(1, 0, "Helvetica", null, null, false);
    CustomStyleSpan span3 = new CustomStyleSpan(2, 7, "Arial", null, null, false);

    assertTrue("Spans with same style/weight should be equal", span1.equals(span2));
    assertFalse("Different style should not be equal", span1.equals(span3));
    assertFalse("Null comparison should return false", span1.equals(null));
    assertFalse("Different class should return false", span1.equals(new Object()));
  }

  @Test
  public void testHashCodeConsistency() {
    CustomStyleSpan span1 = new CustomStyleSpan(1, 0, "Arial", null, null, false);
    CustomStyleSpan span2 = new CustomStyleSpan(1, 0, "Helvetica", null, null, false);

    assertEquals(
        "Hash codes should match for same style/weight", span1.hashCode(), span2.hashCode());
  }

  @Test
  public void testLayoutCacheDistinguishesInlineFontVariations() {
    // cspell:ignore slnt
    LruCache<TextRendererKey, String> cache = new LruCache<>(4);
    TextRendererKey regular = createInlineTextKey("'wght' 400, 'slnt' 0");
    cache.put(regular, "regular layout");

    // The text, parent attributes and ordinary style/weight are identical.
    assertNull(cache.get(createInlineTextKey("'wght' 900, 'slnt' 0")));
    assertNull(cache.get(createInlineTextKey("'wght' 400, 'slnt' -6")));
    assertEquals("regular layout", cache.get(createInlineTextKey("'wght' 400, 'slnt' 0")));
  }

  @Test
  public void testLayoutCacheRetainsBothInlineVariationLayouts() {
    LruCache<TextRendererKey, String> cache = new LruCache<>(4);
    cache.put(createInlineTextKey("'wght' 400"), "regular layout");
    cache.put(createInlineTextKey("'wght' 900"), "heavy layout");

    assertEquals(2, cache.size());
    assertEquals("regular layout", cache.get(createInlineTextKey("'wght' 400")));
    assertEquals("heavy layout", cache.get(createInlineTextKey("'wght' 900")));
  }

  @Test
  public void testLayoutCacheDistinguishesResetInlineVariations() {
    TextRendererKey unset = createInlineTextKey(null);
    TextRendererKey variable = createInlineTextKey("'wght' 400");
    assertFalse(unset.equals(variable));
    assertFalse(variable.equals(unset));
    assertEquals(unset, createInlineTextKey(null));
    assertEquals(unset.hashCode(), createInlineTextKey(null).hashCode());
  }

  @Test
  public void testEqualVariationStringsShareLayoutCacheKey() {
    TextRendererKey first = createInlineTextKey("'wght' 900");
    TextRendererKey second = createInlineTextKey(new String("'wght' 900"));
    assertEquals(first, second);
    assertEquals(first.hashCode(), second.hashCode());
  }

  private TextRendererKey createInlineTextKey(String variations) {
    return createInlineTextKey(variations, null);
  }

  @Test
  public void testLayoutCacheDistinguishesInlineFontFeatures() {
    LruCache<TextRendererKey, String> cache = new LruCache<>(4);
    cache.put(createInlineTextKey("'wght' 400", "'kern' 1"), "kerning layout");

    assertNull(cache.get(createInlineTextKey("'wght' 400", "'kern' 0")));
    assertEquals("kerning layout", cache.get(createInlineTextKey("'wght' 400", "'kern' 1")));

    cache.put(createInlineTextKey("'wght' 400", "'kern' 0"), "no kerning layout");
    assertEquals(2, cache.size());
    assertEquals("kerning layout", cache.get(createInlineTextKey("'wght' 400", "'kern' 1")));
    assertEquals("no kerning layout", cache.get(createInlineTextKey("'wght' 400", "'kern' 0")));
  }

  @Test
  public void testLayoutCacheDistinguishesResetInlineFontFeatures() {
    TextRendererKey unset = createInlineTextKey(null, null);
    TextRendererKey enabled = createInlineTextKey(null, "'kern' 1");
    assertFalse(unset.equals(enabled));
    assertFalse(enabled.equals(unset));
    assertEquals(unset, createInlineTextKey(null, null));
    assertEquals(unset.hashCode(), createInlineTextKey(null, null).hashCode());
  }

  @Test
  public void testEqualFeatureStringsShareLayoutCacheKey() {
    TextRendererKey first = createInlineTextKey(null, "'kern' 1");
    TextRendererKey second = createInlineTextKey(null, new String("'kern' 1"));
    assertEquals(first, second);
    assertEquals(first.hashCode(), second.hashCode());
  }

  private TextRendererKey createInlineTextKey(String variations, String features) {
    SpannableStringBuilder text = new SpannableStringBuilder("Sample text");
    text.setSpan(new CustomStyleSpan(0, 0, "sans-serif", variations, features, false), 0,
        text.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    return new TextRendererKey(text, new TextAttributes(), MeasureMode.EXACTLY,
        MeasureMode.UNDEFINED, 300, 0, 0, false, false, false);
  }

  @Test
  public void testUpdateMethodsCallApply() {
    CustomStyleSpan span = new CustomStyleSpan(2, 0, "monospace", "'wdth' 50", null, false);

    // Test measure state update
    span.updateMeasureState(mTextPaint);
    verifyTextPaintUpdate(2, 400);

    // Test draw state update
    span.updateDrawState(mTextPaint);
    verifyTextPaintUpdate(2, 400);
  }

  private void verifyTextPaintUpdate(int expectedStyle, int expectedWeight) {
    assertEquals(mTextPaint.getTypeface().getStyle(), expectedStyle);
    assertEquals(mTextPaint.getTypeface().getWeight(), expectedWeight);
  }

  @Test
  public void testNullFontFamilyHandling() {
    CustomStyleSpan span = new CustomStyleSpan(0, 0, null, null, null, false);
    span.updateDrawState(mTextPaint);

    verifyTextPaintUpdate(0, 400);
  }

  @Test
  public void testGetStyleMethod() {
    CustomStyleSpan span = new CustomStyleSpan(3, 500, "Roboto", null, null, false);
    assertEquals("getStyle() should return initialized value", 3, span.getStyle());
  }
}
