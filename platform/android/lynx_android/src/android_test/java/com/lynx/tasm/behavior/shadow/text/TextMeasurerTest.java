// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import static org.junit.Assert.*;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Typeface;
import android.os.Build;
import android.text.Layout;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.StaticLayout;
import android.text.TextPaint;
import android.text.style.CharacterStyle;
import android.text.style.MetricAffectingSpan;
import android.text.style.StyleSpan;
import android.util.SparseArray;
import androidx.test.filters.SdkSuppress;
import com.lynx.react.bridge.mapbuffer.ReadableCompactArrayBuffer;
import com.lynx.tasm.EmbeddedMode;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.render.PlatformRendererContext;
import com.lynx.tasm.behavior.shadow.MeasureMode;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.image.InlineImageSpan;
import com.lynx.tasm.event.LynxDetailEvent;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.IdentityHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mockito;

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
    paint.setTextSize(30);
    TextPaint expected = new TextPaint(paint);
    expected.setFontVariationSettings("'wght' 625.5");
    for (MetricAffectingSpan style : span.getSpans(0, span.length(), MetricAffectingSpan.class)) {
      style.updateMeasureState(paint);
    }
    assertEquals(expected.measureText("Axes"), paint.measureText("Axes"), 0.01f);

    mMeasurer.dispatchLayoutBefore(SIGN, textProps(0, 0, 3, 30.0, 25, 0, 1, 4, 3, 30.0, 2, "Axes"));
    bundle = (AttributedTextBundle) mAttributedTextBundles.get(SIGN);
    span = (Spanned) bundle.getSpan();
    // Android copies the paragraph paint before applying the replacement spans.
    paint.setTypeface(Typeface.DEFAULT);
    expected.setTypeface(Typeface.DEFAULT);
    for (MetricAffectingSpan style : span.getSpans(0, span.length(), MetricAffectingSpan.class)) {
      style.updateMeasureState(paint);
    }
    assertEquals(expected.measureText("Axes"), paint.measureText("Axes"), 0.01f);
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
                                             : StyleConstants.FONTSTYLE_NORMAL);
    if (reset) {
      Collections.addAll(props, 25, 1, "wght", 900.0);
    }
    Collections.addAll(props, 2, "Axes");
    mMeasurer.dispatchLayoutBefore(SIGN, textProps(props.toArray()));
    return (Spanned) ((AttributedTextBundle) mAttributedTextBundles.get(SIGN)).getSpan();
  }

  private void assertInlineTypeface(Spanned text, int expectedStyle, boolean reset) {
    mMeasurer.measureText(SIGN, 300, MeasureMode.EXACTLY.intValue(), 0,
        MeasureMode.UNDEFINED.intValue(), new float[0]);
    TextUpdateBundle bundle = (TextUpdateBundle) mMeasurer.takeTextLayout(SIGN);
    for (boolean draw : new boolean[] {false, true}) {
      TextPaint paint = new TextPaint(bundle.getTextLayout().getPaint());
      TextPaint expected = new TextPaint(paint);
      for (MetricAffectingSpan span : text.getSpans(0, text.length(), MetricAffectingSpan.class)) {
        if (span instanceof FontVariationSpan) {
          expected.setFontVariationSettings(reset ? "" : "'opsz' 18");
        } else {
          span.updateMeasureState(expected);
        }
        if (draw) {
          span.updateDrawState(paint);
        } else {
          span.updateMeasureState(paint);
        }
      }
      assertEquals(expectedStyle, paint.getTypeface().getStyle());
      assertEquals(expected.measureText("Axes"), paint.measureText("Axes"), 0.01f);
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

  @Test
  @SdkSuppress(minSdkVersion = 26)
  public void repeatedFontVariationUpdatesReuseTypeface() {
    FontVariationSpan span = new FontVariationSpan("'wght' 900");
    TextPaint base = new TextPaint();
    base.setTypeface(Typeface.DEFAULT);
    base.setTextSize(40);
    TextPaint work = new TextPaint(base);
    span.updateMeasureState(work);
    Typeface resolved = work.getTypeface();
    float width = work.measureText("Hamburger");
    for (int i = 0; i < 100; ++i) {
      work.set(base);
      if (i % 2 == 0) {
        span.updateMeasureState(work);
      } else {
        span.updateDrawState(work);
      }
      assertSame(resolved, work.getTypeface());
      assertEquals(width, work.measureText("Hamburger"), 0.01f);
    }
  }

  @Test
  @SdkSuppress(minSdkVersion = 26)
  public void repeatedLayoutDrawingReusesVariationTypeface() {
    Set<Typeface> typefaces = Collections.newSetFromMap(new IdentityHashMap<>());
    SpannableStringBuilder text = new SpannableStringBuilder("Hamburger");
    text.setSpan(
        new FontVariationSpan("'wght' 900"), 0, text.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    text.setSpan(new MetricAffectingSpan() {
      @Override
      public void updateMeasureState(TextPaint paint) {
        typefaces.add(paint.getTypeface());
      }

      @Override
      public void updateDrawState(TextPaint paint) {
        typefaces.add(paint.getTypeface());
      }
    }, 0, text.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    TextPaint paint = new TextPaint();
    paint.setTypeface(Typeface.DEFAULT);
    paint.setTextSize(30);
    StaticLayout layout = StaticLayout.Builder.obtain(text, 0, text.length(), paint, 400).build();
    Bitmap bitmap = Bitmap.createBitmap(400, layout.getHeight(), Bitmap.Config.ARGB_8888);
    Canvas canvas = new Canvas(bitmap);
    for (int i = 0; i < 100; ++i) {
      layout.draw(canvas);
    }
    bitmap.recycle();
    assertEquals(1, typefaces.size());
  }

  @Test
  @SdkSuppress(minSdkVersion = 26)
  public void fontVariationCacheUsesInputTypefaceAndPreservesOtherPaintState() {
    FontVariationSpan span = new FontVariationSpan("'wght' 900");
    Typeface previous = null;
    for (Typeface input : new Typeface[] {
             Typeface.DEFAULT, Typeface.DEFAULT_BOLD, Typeface.create("serif", Typeface.ITALIC)}) {
      TextPaint base = new TextPaint();
      base.setTypeface(input);
      base.setTextSize(30);
      TextPaint expected = new TextPaint(base);
      expected.setFontVariationSettings("'wght' 900");
      TextPaint work = new TextPaint(base);
      span.updateMeasureState(work);
      Typeface resolved = work.getTypeface();
      assertNotSame(previous, resolved);
      assertEquals(expected.measureText("Hamburger"), work.measureText("Hamburger"), 0.01f);
      assertEquals(expected.getTypeface().getStyle(), resolved.getStyle());
      work.set(base);
      work.setTextSize(45);
      work.setColor(0xff123456);
      work.setLetterSpacing(0.2f);
      work.baselineShift = 3;
      span.updateDrawState(work);
      assertSame(resolved, work.getTypeface());
      assertEquals(45, work.getTextSize(), 0);
      assertEquals(0xff123456, work.getColor());
      assertEquals(0.2f, work.getLetterSpacing(), 0);
      assertEquals(3, work.baselineShift);
      previous = resolved;
    }
  }

  @Test
  @SdkSuppress(minSdkVersion = 26)
  public void nestedResetAfterParentVariationCacheHitUsesDefaultAxes() {
    // Child precedes parent in the compact protocol; spans are installed in reverse order.
    mMeasurer.dispatchLayoutBefore(SIGN,
        textProps(0, 1, 3, 30.0, 25, 0, 1, 3, 0, 0, 3, 30.0, 25, 1, "wght", 900.0, 1, 4, 3, 30.0, 2,
            "AAAA"));
    Spanned text = (Spanned) ((AttributedTextBundle) mAttributedTextBundles.get(SIGN)).getSpan();
    TextPaint base = new TextPaint();
    base.setTypeface(Typeface.DEFAULT);
    base.setTextSize(30);
    TextPaint work = new TextPaint(base);
    FontVariationSpan parent = text.getSpans(0, 1, FontVariationSpan.class)[0];
    // Warm only the parent, leaving the descendant reset unresolved.
    parent.updateDrawState(work);
    work.set(base);
    for (FontVariationSpan span : text.getSpans(1, 2, FontVariationSpan.class)) {
      span.updateDrawState(work);
    }
    assertEquals(base.measureText("AAAA"), work.measureText("AAAA"), 0.01f);
    Typeface reset = work.getTypeface();
    for (int i = 0; i < 100; ++i) {
      work.set(base);
      for (FontVariationSpan span : text.getSpans(1, 2, FontVariationSpan.class)) {
        span.updateDrawState(work);
      }
      assertSame(reset, work.getTypeface());
    }
  }

  @Test
  @SdkSuppress(minSdkVersion = 26)
  public void nestedResetSkipsUnsupportedParentAxesAndKeepsSiblingAxes() {
    mMeasurer.dispatchLayoutBefore(SIGN,
        textProps(0, 1, 3, 30.0, 25, 0, 1, 2, 0, 0, 3, 30.0, 25, 1, "xxxx", 1.0, 1, 3, 0, 4, 3,
            30.0, 25, 1, "wght", 100.0, 1, 5, 0, 0, 3, 30.0, 25, 1, "wght", 900.0, 1, 6, 3, 30.0, 2,
            "AAAAAA"));
    Spanned text = (Spanned) ((AttributedTextBundle) mAttributedTextBundles.get(SIGN)).getSpan();
    TextPaint base = new TextPaint();
    base.setTypeface(Typeface.DEFAULT);
    base.setTextSize(30);
    for (int start : new int[] {4, 1, 0, 1, 4}) {
      TextPaint work = new TextPaint(base);
      for (FontVariationSpan span : text.getSpans(start, start + 1, FontVariationSpan.class)) {
        span.updateDrawState(work);
      }
      TextPaint expected = new TextPaint(base);
      if (start != 1) {
        expected.setFontVariationSettings(start == 4 ? "'wght' 100" : "'wght' 900");
      }
      assertEquals(expected.measureText("AAAA"), work.measureText("AAAA"), 0.01f);
    }
  }

  private float[] measure(String text, int maxLines, int overflow, float width) {
    TextAttributes attributes = new TextAttributes();
    attributes.setFontSize(30);
    attributes.setMaxLineCount(maxLines);
    attributes.setTextOverflow(overflow);
    return measure(text, attributes, width);
  }

  private float[] measure(String text, TextAttributes attributes, float width) {
    mAttributedTextBundles.put(
        SIGN, new AttributedTextBundle(new SpannableStringBuilder(text), attributes));
    return mMeasurer.measureText(SIGN, width, MeasureMode.EXACTLY.intValue(), 0,
        MeasureMode.UNDEFINED.intValue(), new float[0]);
  }

  private float[] measurePrepared(float width, float height, MeasureMode heightMode) {
    return mMeasurer.measureText(
        SIGN, width, MeasureMode.EXACTLY.intValue(), height, heightMode.intValue(), new float[0]);
  }

  private TextUpdateBundle takeLayout() {
    return (TextUpdateBundle) mMeasurer.takeTextLayout(SIGN);
  }

  @Test
  @SdkSuppress(minSdkVersion = 28)
  public void truncationUsesComputedWeightAndItalic() {
    int normal = StyleConstants.FONTWEIGHT_NORMAL;
    int bold = StyleConstants.FONTWEIGHT_BOLD;
    assertTruncationFont(bold, false, normal, false, 400);
    assertTruncationFont(bold, false, StyleConstants.FONTWEIGHT_300, false, 300);
    assertTruncationFont(normal, true, normal, false, 400);
    assertTruncationFont(bold, false, bold, true, 700);
    assertTruncationFont(normal, true, bold, true, 700);
    assertTruncationFont(bold, true, null, true, 700);
  }

  private void assertTruncationFont(int parentWeight, boolean parentItalic, Integer tokenWeight,
      boolean tokenItalic, int expectedWeight) {
    String body = "A long paragraph that must be truncated.";
    String token = "More";
    List<Object> props = new ArrayList<>();
    Collections.addAll(props, 0, body.length(), 3, 32.0);
    if (tokenWeight != null) {
      Collections.addAll(props, 7, tokenWeight, 8,
          tokenItalic ? StyleConstants.FONTSTYLE_ITALIC : StyleConstants.FONTSTYLE_NORMAL);
    }
    Collections.addAll(props, tokenWeight == null ? 1 : 114, body.length() + token.length(), 113,
        body.length(), 3, 32.0, 7, parentWeight, 8,
        parentItalic ? StyleConstants.FONTSTYLE_ITALIC : StyleConstants.FONTSTYLE_NORMAL, 9,
        "serif", 99, 1, 2, body + token);
    mMeasurer.dispatchLayoutBefore(SIGN, textProps(props.toArray()));
    measurePrepared(220, 0, MeasureMode.UNDEFINED);
    Layout layout = takeLayout().getTextLayout();
    Spanned displayed = (Spanned) layout.getText();
    assertTrue(displayed.toString().endsWith(token));
    int start = displayed.length() - token.length();
    CustomStyleSpan[] fonts = displayed.getSpans(start, displayed.length(), CustomStyleSpan.class);
    assertEquals(tokenWeight == null ? 0 : 1, fonts.length);
    TextPaint expected = new TextPaint(layout.getPaint());
    expected.setTypeface(Typeface.create(expected.getTypeface(), expectedWeight, tokenItalic));
    for (boolean draw : new boolean[] {false, true}) {
      TextPaint paint = new TextPaint(layout.getPaint());
      for (CustomStyleSpan font : fonts) {
        if (draw) {
          font.updateDrawState(paint);
        } else {
          font.updateMeasureState(paint);
        }
      }
      assertEquals(expectedWeight, paint.getTypeface().getWeight());
      assertEquals(tokenItalic, paint.getTypeface().isItalic());
      assertEquals(expected.measureText(token), paint.measureText(token), 0.01f);
    }
    assertEquals(Layout.getDesiredWidth(token, expected),
        Layout.getDesiredWidth(displayed, start, displayed.length(), layout.getPaint()), 0.01f);
  }

  @Test
  public void inlineTruncationPreservesAuthoredTokenAndUtf16Ranges() {
    String body = "Text with emoji \ud83d\ude00 and enough words to overflow several lines.";
    for (String token : new String[] {"More", "\u2026More"}) {
      mMeasurer.dispatchLayoutBefore(SIGN,
          textProps(0, body.length(), 112, 42, 4, 0xff0000ff, 1, body.length() + token.length(),
              113, body.length(), 3, 24.0, 99, 1, 6, StyleConstants.TEXTOVERFLOW_ELLIPSIS, 2,
              body + token));
      AttributedTextBundle attributed = (AttributedTextBundle) mAttributedTextBundles.get(SIGN);
      assertEquals(body, attributed.getSpan().toString());
      assertEquals(token, attributed.getTruncationSpan().toString());
      float[] result = measurePrepared(220, 0, MeasureMode.UNDEFINED);
      TextUpdateBundle bundle = takeLayout();
      Spanned displayed = (Spanned) bundle.getTextLayout().getText();
      assertTrue(displayed.toString().endsWith(token));
      int cut = (int) result[result.length - 1];
      assertTrue(cut >= 0 && cut < body.length());
      assertEquals(body.substring(0, cut) + token, displayed.toString());
      assertFalse(cut > 0 && Character.isHighSurrogate(body.charAt(cut - 1)));
      EventTargetSpan[] events = displayed.getSpans(cut, displayed.length(), EventTargetSpan.class);
      assertEquals(1, events.length);
      assertEquals(42, events[0].getSign());
      assertEquals(cut, displayed.getSpanStart(events[0]));
      TextPaint paint = new TextPaint();
      for (CharacterStyle style :
          displayed.getSpans(cut, displayed.length(), CharacterStyle.class)) {
        style.updateDrawState(paint);
      }
      assertEquals(0xff0000ff, paint.getColor());
      assertEquals(body.length() - cut, bundle.getLayoutEventEllipsisCount());
      assertEquals(0, bundle.getTextLayout().getEllipsisCount(0));

      int[] geometryRanges = {21, 0, body.length(), 22, cut, body.length(), 42, body.length(),
          body.length() + token.length()};
      float[] measuredRegions = InlineTextGeometry.getRegions(bundle, geometryRanges);
      ArrayList<Float> expected = new ArrayList<>();
      Layout layout = bundle.getTextLayout();
      InlineTextGeometry.appendLayoutRegions(
          expected, 21, layout, 0, cut, bundle.getTextTranslateOffset());
      InlineTextGeometry.appendLayoutRegions(
          expected, 42, layout, cut, displayed.length(), bundle.getTextTranslateOffset());
      assertFalse(expected.isEmpty());
      assertEquals(expected.size(), measuredRegions.length);
      for (int i = 0; i < measuredRegions.length; i++) {
        assertEquals(expected.get(i), measuredRegions[i], 0.f);
      }
    }
  }

  @Test
  public void inlineTruncationKeepsBothHitRegionsForClickableAncestor() throws Exception {
    LynxEnv.inst().initNativeLibraries(System::loadLibrary);
    LynxContext context = TestingUtils.getLynxContext();
    context.setEmbeddedMode(EmbeddedMode.FRAGMENT_LAYER_RENDER);
    PlatformRendererContext renderer =
        new PlatformRendererContext(Mockito.mock(UIBody.UIBodyView.class), context, null);
    try {
      mMeasurer = renderer.getTextMeasurer();
      Method getRegions = PlatformRendererContext.class.getDeclaredMethod(
          "getTextEventTargetRegions", int.class, int[].class);
      getRegions.setAccessible(true);
      String body = "A long clickable body that needs more than one line";
      String token = "More";
      int end = body.length();
      int total = end + token.length();
      int inner = 11, outer = 12, label = 13;
      mMeasurer.dispatchLayoutBefore(SIGN,
          textProps(0, 0, 112, inner, 1, end, 0, 0, 112, outer, 1, end, 0, end, 112, label, 1,
              total, 0, end, 112, outer, 1, total, 113, end, 3, 24.0, 99, 1, 2, body + token));
      for (int width : new int[] {2000, 220, 2000}) {
        float[] measured = measurePrepared(width, 0, MeasureMode.UNDEFINED);
        int cut = (int) measured[measured.length - 1];
        assertEquals(width == 220, cut >= 0);
        int[] ranges = cut < 0 ? new int[] {inner, 0, end, outer, 0, end}
                               : new int[] {inner, 0, cut, outer, 0, cut, label, cut,
                                   cut + token.length(), outer, cut, cut + token.length()};
        float[] regions = (float[]) getRegions.invoke(renderer, SIGN, ranges);
        assertEquals(ranges.length / 3 * 5, regions.length);
        for (int i = 0; i < regions.length; i += 5) {
          assertEquals(ranges[i / 5 * 3], Float.floatToRawIntBits(regions[i]));
        }
        for (int i = 1; i <= 4; i++) {
          assertEquals("ancestor over body", regions[i], regions[5 + i], 0.f);
          if (cut >= 0) {
            assertEquals("ancestor over token", regions[10 + i], regions[15 + i], 0.f);
          }
        }
        if (cut >= 0) {
          assertTrue("token follows visible body", regions[11] > regions[1]);
        }
      }
    } finally {
      renderer.destroy();
    }
  }

  @Test
  public void inlineTruncationHidesTokenWhenBodyFitsAndCanBeRemoved() {
    mMeasurer.dispatchLayoutBefore(SIGN, textProps(113, 5, 3, 24.0, 99, 1, 2, "shortMore"));
    float[] result = measurePrepared(400, 0, MeasureMode.UNDEFINED);
    assertEquals(-1, (int) result[result.length - 1]);
    assertEquals("short", takeLayout().getTextLayout().getText().toString());

    mMeasurer.dispatchLayoutBefore(SIGN, textProps(3, 24.0, 99, 1, 2, "replacement"));
    measurePrepared(400, 0, MeasureMode.UNDEFINED);
    AttributedTextBundle attributed = (AttributedTextBundle) mAttributedTextBundles.get(SIGN);
    assertNull(attributed.getTruncationSpan());
    assertEquals("replacement", takeLayout().getTextLayout().getText().toString());
  }

  @Test
  public void inlineTruncationRelocatesViewsAndRestoresThemAcrossWidths() {
    String body =
        "A sufficiently long body ending in a view " + TextAttributes.INLINE_IMAGE_PLACEHOLDER;
    String token = "More" + TextAttributes.INLINE_IMAGE_PLACEHOLDER;
    int bodyView = body.length() - 1;
    int tokenView = body.length() + token.length() - 1;
    mMeasurer.dispatchLayoutBefore(SIGN,
        textProps(0, bodyView, 102, 11, 1, bodyView + 1, 0, tokenView, 102, 12, 1, tokenView + 1,
            113, body.length(), 3, 24.0, 99, 1, 2, body + token));
    float[] sizes = new float[] {11, 12, 12, 10, 12, 18, 12, 10};
    for (float width : new float[] {220, 2000, 220}) {
      mMeasurer.measureText(
          SIGN, width, MeasureMode.EXACTLY.intValue(), 0, MeasureMode.UNDEFINED.intValue(), sizes);
      float[] aligned = mMeasurer.align(SIGN);
      assertEquals(
          "width=" + width + " text=" + takeLayout().getTextLayout().getText(), 3, aligned.length);
      assertEquals(width < 1000 ? 12 : 11, (int) aligned[0]);
      TextUpdateBundle bundle = takeLayout();
      assertTrue(bundle.getViewTruncatedSet().contains(width < 1000 ? 11 : 12));
      assertFalse(bundle.getViewTruncatedSet().contains((int) aligned[0]));
      assertTrue(Math.abs(aligned[1]) < Float.POSITIVE_INFINITY);
      assertTrue(Math.abs(aligned[2]) < Float.POSITIVE_INFINITY);
    }
  }

  @Test
  public void inlineTruncationHidesTokenWhenOnlyLineHeightOverflows() {
    mMeasurer.dispatchLayoutBefore(SIGN, textProps(113, 2, 3, 24.0, 2, "HiMore"));
    for (int height : new int[] {27, 30}) {
      float[] result = measurePrepared(180, height, MeasureMode.AT_MOST);
      assertEquals("Hi", takeLayout().getTextLayout().getText().toString());
      assertEquals(0, (int) result[6]); // First line's ellipsis count.
      assertEquals(-1, (int) result[result.length - 1]); // No custom token.
    }
  }

  @Test
  public void inlineTruncationHonorsHeightConstraintAndOversizedTokenFallback() {
    String body = "One line of words. Another line of words. A final line of words.";
    mMeasurer.dispatchLayoutBefore(SIGN, textProps(113, body.length(), 3, 24.0, 2, body + "More"));
    measurePrepared(220, 40, MeasureMode.AT_MOST);
    TextUpdateBundle bundle = takeLayout();
    assertTrue(bundle.getTextLayout().getText().toString().endsWith("More"));
    assertEquals(1, bundle.getLayoutEventLineCount());

    String token = "An oversized token that cannot fit in the available width";
    mMeasurer.dispatchLayoutBefore(SIGN,
        textProps(113, body.length(), 3, 24.0, 99, 1, 6, StyleConstants.TEXTOVERFLOW_ELLIPSIS, 2,
            body + token));
    float[] result = measurePrepared(100, 0, MeasureMode.UNDEFINED);
    assertEquals(-1, (int) result[result.length - 1]);
    bundle = takeLayout();
    assertFalse(bundle.getTextLayout().getText().toString().contains(token));
    assertTrue(bundle.getTextLayout().getEllipsisCount(0) > 0);
  }

  @Test
  public void inlineTruncationChoosesLastFittingCaret() {
    LynxContext context = Mockito.spy(TestingUtils.getLynxContext());
    Mockito.doReturn(true).when(context).isTextBoringLayoutEnabled();
    Mockito.doReturn(true).when(context).isTextRefactorEnabled();
    mMeasurer = new TextMeasurer(context);
    String body = "WWWWWWWWWWWW", token = "More";
    mMeasurer.dispatchLayoutBefore(SIGN,
        textProps(113, body.length(), 3, 24.0, 5, StyleConstants.WHITESPACE_NOWRAP, 99, 1, 6,
            StyleConstants.TEXTOVERFLOW_ELLIPSIS, 2, body + token));
    TextPaint paint = new TextPaint();
    paint.setTextSize(24);
    float boundary = paint.measureText("WW") + (float) Math.ceil(paint.measureText(token));
    for (int delta : new int[] {-2, 2}) {
      measurePrepared(boundary + delta, 0, MeasureMode.UNDEFINED);
      assertEquals(
          (delta < 0 ? "W" : "WW") + token, takeLayout().getTextLayout().getText().toString());
    }
  }

  @Test
  public void inlineTruncationPreservesImageSpanAcrossWidths() {
    String body = "A sufficiently long body with several words to wrap.";
    SpannableStringBuilder text =
        new SpannableStringBuilder(body + "More" + TextAttributes.INLINE_IMAGE_PLACEHOLDER);
    InlineImageSpan image = new InlineImageSpan(16, 16, new int[] {0, 0, 0, 0});
    text.setSpan(image, text.length() - 1, text.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    TextAttributes attributes = new TextAttributes();
    attributes.setFontSize(24);
    attributes.setMaxLineCount(1);
    attributes.setHasImageSpan(true);
    AttributedTextBundle attributed = new AttributedTextBundle(text, attributes, body.length());
    mAttributedTextBundles.put(SIGN, attributed);
    for (float width : new float[] {220, 2000, 220}) {
      measurePrepared(width, 0, MeasureMode.UNDEFINED);
      Spanned displayed = (Spanned) takeLayout().getTextLayout().getText();
      assertEquals(width < 1000, displayed.getSpanStart(image) >= 0);
    }
  }

  @Test
  public void inlineTruncationSupportsMultilineRtlAndEmptyBody() {
    String words = "\u05e9\u05dc\u05d5\u05dd \u05e2\u05d5\u05dc\u05dd";
    String body = words + " " + words + " " + words + " " + words + " " + words + " " + words;
    String token = " \u05e2\u05d5\u05d3";
    mMeasurer.dispatchLayoutBefore(
        SIGN, textProps(113, body.length(), 3, 24.0, 99, 2, 2, body + token));
    float[] result = measurePrepared(180, 0, MeasureMode.UNDEFINED);
    int cut = (int) result[result.length - 1];
    assertTrue(cut >= 0 && cut < body.length());
    assertEquals(body.substring(0, cut) + token, takeLayout().getTextLayout().getText().toString());
    assertEquals(2, takeLayout().getLayoutEventLineCount());

    mMeasurer.dispatchLayoutBefore(SIGN, textProps(113, 0, 3, 24.0, 99, 1, 2, token));
    result = measurePrepared(180, 0, MeasureMode.UNDEFINED);
    assertEquals(-1, (int) result[result.length - 1]);
    assertEquals("", takeLayout().getTextLayout().getText().toString());
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
  public void inlineGeometryExcludesThePlatformEllipsis() {
    String text = "prefix first second third fourth fifth suffix";
    measure(text, 1, StyleConstants.TEXTOVERFLOW_ELLIPSIS, 100);
    TextUpdateBundle bundle = (TextUpdateBundle) mMeasurer.takeTextLayout(SIGN);
    Layout layout = bundle.getTextLayout();
    assertTrue(layout.getEllipsisCount(0) > 0);
    assertInlineGeometryKeepsOnlyVisibleText(bundle, text.length(), layout.getEllipsisStart(0));
  }

  @Test
  public void inlineGeometryKeepsTheVisiblePrefixAfterRtlRelayout() {
    String text = "prefix first second third fourth fifth suffix";
    TextAttributes attributes = new TextAttributes();
    attributes.setFontSize(30);
    attributes.setMaxLineCount(1);
    attributes.setTextOverflow(StyleConstants.TEXTOVERFLOW_ELLIPSIS);
    attributes.mDirection = StyleConstants.DIRECTION_RTL;
    measure(text, attributes, 150);

    TextUpdateBundle bundle = (TextUpdateBundle) mMeasurer.takeTextLayout(SIGN);
    Layout layout = bundle.getTextLayout();
    // The renderer rewrites the text with a literal ellipsis.
    assertEquals(0, layout.getEllipsisCount(0));
    String rendered = layout.getText().toString();
    assertTrue(rendered.length() < text.length());
    int ellipsis = rendered.indexOf('\u2026');
    assertTrue(ellipsis > 0);
    int visibleEnd = rendered.charAt(ellipsis - 1) == '\u200f' ? ellipsis - 1 : ellipsis;
    assertInlineGeometryKeepsOnlyVisibleText(bundle, text.length(), visibleEnd);
  }

  @Test
  public void inlineGeometryExcludesTheMaxTextLengthEllipsis() {
    String text = "prefix first second third";
    TextAttributes attributes = new TextAttributes();
    attributes.setFontSize(30);
    attributes.mMaxTextLength = 11;
    measure(text, attributes, 600);
    TextUpdateBundle bundle = (TextUpdateBundle) mMeasurer.takeTextLayout(SIGN);
    assertEquals("prefix firs\u2026", bundle.getTextLayout().getText().toString());
    assertInlineGeometryKeepsOnlyVisibleText(bundle, text.length(), 11);
  }

  private void assertInlineGeometryKeepsOnlyVisibleText(
      TextUpdateBundle bundle, int sourceLength, int visibleEnd) {
    float[] visible = InlineTextGeometry.getRegions(bundle, new int[] {SIGN, 1, visibleEnd});
    assertTrue(visible.length > 0);
    assertArrayEquals(
        visible, InlineTextGeometry.getRegions(bundle, new int[] {SIGN, 1, sourceLength}), 0.01f);
    assertEquals(0,
        InlineTextGeometry.getRegions(bundle, new int[] {SIGN, visibleEnd, visibleEnd + 1}).length);
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
