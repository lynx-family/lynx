// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import static org.junit.Assert.*;

import android.graphics.Typeface;
import android.text.Layout;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextPaint;
import android.text.style.CharacterStyle;
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
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
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
}
