// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import static org.mockito.Mockito.mock;

import android.graphics.Typeface;
import android.text.Layout;
import com.lynx.react.bridge.DynamicFromMap;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.behavior.LayoutNodeManager;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.shadow.LayoutNode;
import com.lynx.tasm.behavior.shadow.MeasureMode;
import com.lynx.tasm.behavior.shadow.MeasureUtils;
import com.lynx.testing.base.TestingUtils;
import java.util.Map;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

public class TextShadowNodeTest {
  private TextShadowNode textShadowNode = null;

  @Before
  public void setUp() {
    textShadowNode = new TextShadowNode();
    LayoutNodeManager layoutNodeManager = mock(LayoutNodeManager.class);
    textShadowNode.setLayoutNodeManager(layoutNodeManager);
    textShadowNode.setContext(TestingUtils.getLynxContext());
  }

  @After
  public void tearDown() {
    textShadowNode = null;
  }

  @Test
  public void testMeasure() {
    RawTextShadowNode rawTextShadowNode = new RawTextShadowNode();
    JavaOnlyMap map = new JavaOnlyMap();
    map.put("text", "This is a test text.This is a test text.This is a test text.");
    rawTextShadowNode.setText(new DynamicFromMap(map, "text"));
    textShadowNode.addChildAt(rawTextShadowNode, 0);

    RawTextShadowNode rawTextShadowNodeInTruncation = new RawTextShadowNode();
    JavaOnlyMap mapInTruncation = new JavaOnlyMap();
    mapInTruncation.put("text", "truncation");
    rawTextShadowNodeInTruncation.setText(new DynamicFromMap(mapInTruncation, "text"));
    InlineTruncationShadowNode inlineTruncationShadowNode = new InlineTruncationShadowNode();
    inlineTruncationShadowNode.setContext(TestingUtils.getLynxContext());
    inlineTruncationShadowNode.addChildAt(rawTextShadowNodeInTruncation, 0);
    textShadowNode.addChildAt(inlineTruncationShadowNode, 1);

    textShadowNode.setTextMaxLine("1");
    textShadowNode.onLayoutBefore();
    LayoutNode layoutNode = mock(LayoutNode.class);

    textShadowNode.measure(
        layoutNode, 300.f, MeasureMode.EXACTLY, MeasureUtils.UNDEFINED, MeasureMode.UNDEFINED);
    Layout layout = textShadowNode.getTextRenderer().getTextLayout();
    Assert.assertTrue("Text layout string should be end with 'truncation'",
        layout.getLineCount() == 1 && String.valueOf(layout.getText()).endsWith("truncation"));

    textShadowNode.measure(
        layoutNode, 300.f, MeasureMode.EXACTLY, MeasureUtils.UNDEFINED, MeasureMode.UNDEFINED);
    Assert.assertEquals("Layout result of truncated text should not be same",
        textShadowNode.getTextRenderer().getTextLayout(), layout);

    textShadowNode.setTextMaxLine("2");
    textShadowNode.onLayoutBefore();
    textShadowNode.measure(
        layoutNode, 300.f, MeasureMode.EXACTLY, MeasureUtils.UNDEFINED, MeasureMode.UNDEFINED);
    Assert.assertTrue("Text layout string should be end width 'truncation'",
        textShadowNode.getTextRenderer().getTextLayout().getLineCount() == 2
            && String.valueOf(textShadowNode.getTextRenderer().getTextLayout().getText())
                   .endsWith("truncation"));
  }

  @Test
  public void testUpdateFontWeight() {
    textShadowNode.setFontStyle(StyleConstants.FONTSTYLE_ITALIC);
    Assert.assertEquals(Typeface.ITALIC, textShadowNode.getTextAttributes().getFontStyle());

    textShadowNode.setFontWeight(StyleConstants.FONTWEIGHT_700);
    Assert.assertEquals(Typeface.ITALIC, textShadowNode.getTextAttributes().getFontStyle());
    Assert.assertEquals(
        StyleConstants.FONTWEIGHT_700, textShadowNode.getTextAttributes().getFontWeight());
    Assert.assertEquals(
        Typeface.BOLD_ITALIC, textShadowNode.getTextAttributes().getTypefaceStyle());
  }

  @Test
  public void testHandleHeightOverflowByLineCount() {
    RawTextShadowNode rawTextShadowNode = new RawTextShadowNode();
    JavaOnlyMap map = new JavaOnlyMap();
    map.put("text", "This is a test text.This is a test text.This is a test text.");
    rawTextShadowNode.setText(new DynamicFromMap(map, "text"));
    textShadowNode.addChildAt(rawTextShadowNode, 0);

    textShadowNode.setLineHeight(30.1f);
    textShadowNode.setTextMaxLine("2");
    textShadowNode.setTextOverflow(StyleConstants.TEXTOVERFLOW_ELLIPSIS);
    textShadowNode.onLayoutBefore();
    LayoutNode layoutNode = mock(LayoutNode.class);
    textShadowNode.measure(layoutNode, 300.f, MeasureMode.EXACTLY, 60.2f, MeasureMode.EXACTLY);
    Layout layout = textShadowNode.getTextRenderer().getTextLayout();
    Assert.assertEquals(layout.getLineCount(), 2);
  }

  @Test
  public void testCalculateLastLineGlyphWidthWithZeroWidthSpace() throws Exception {
    RawTextShadowNode rawTextShadowNode = new RawTextShadowNode();
    JavaOnlyMap map = new JavaOnlyMap();
    String text = "A\u200BB";
    map.put("text", text);
    rawTextShadowNode.setText(new DynamicFromMap(map, "text"));
    textShadowNode.addChildAt(rawTextShadowNode, 0);

    textShadowNode.setTextMaxLine("1");
    textShadowNode.onLayoutBefore();
    LayoutNode layoutNode = mock(LayoutNode.class);
    textShadowNode.measure(
        layoutNode, 300.f, MeasureMode.EXACTLY, MeasureUtils.UNDEFINED, MeasureMode.UNDEFINED);
    Layout layout = textShadowNode.getTextRenderer().getTextLayout();

    int lastLineIndex = layout.getLineCount() - 1;
    int lastLineStartCharIndex = layout.getLineStart(lastLineIndex);
    int lastLineEndCharIndex = layout.getLineEnd(lastLineIndex);

    java.lang.reflect.Method method = TextShadowNode.class.getDeclaredMethod(
        "calculateLastLineGlyphWidth", int.class, int.class, int.class, Layout.class);
    method.setAccessible(true);
    @SuppressWarnings("unchecked")
    Map<Integer, Float> lastLineGlyphWidthMap = (Map<Integer, Float>) method.invoke(
        textShadowNode, lastLineIndex, lastLineStartCharIndex, lastLineEndCharIndex, layout);

    int zeroWidthCharIndex = text.indexOf('\u200B');
    Assert.assertTrue(zeroWidthCharIndex >= 0);

    Assert.assertTrue(lastLineGlyphWidthMap.containsKey(zeroWidthCharIndex));
    Assert.assertEquals(0.f, lastLineGlyphWidthMap.get(zeroWidthCharIndex), 0.0f);

    // Neighbouring visible characters should still have positive width.
    int indexOfA = text.indexOf('A');
    int indexOfB = text.indexOf('B');
    Assert.assertTrue(lastLineGlyphWidthMap.get(indexOfA) > 0.f);
    Assert.assertTrue(lastLineGlyphWidthMap.get(indexOfB) > 0.f);
  }

  @Test
  public void testCalculateLastLineGlyphWidthWithZeroWidthJoinerAtEnd() throws Exception {
    RawTextShadowNode rawTextShadowNode = new RawTextShadowNode();
    JavaOnlyMap map = new JavaOnlyMap();
    String text = "ABC\u200D";
    map.put("text", text);
    rawTextShadowNode.setText(new DynamicFromMap(map, "text"));
    textShadowNode.addChildAt(rawTextShadowNode, 0);

    textShadowNode.setTextMaxLine("1");
    textShadowNode.onLayoutBefore();
    LayoutNode layoutNode = mock(LayoutNode.class);
    textShadowNode.measure(
        layoutNode, 300.f, MeasureMode.EXACTLY, MeasureUtils.UNDEFINED, MeasureMode.UNDEFINED);
    Layout layout = textShadowNode.getTextRenderer().getTextLayout();

    int lastLineIndex = layout.getLineCount() - 1;
    int lastLineStartCharIndex = layout.getLineStart(lastLineIndex);
    int lastLineEndCharIndex = layout.getLineEnd(lastLineIndex);

    java.lang.reflect.Method method = TextShadowNode.class.getDeclaredMethod(
        "calculateLastLineGlyphWidth", int.class, int.class, int.class, Layout.class);
    method.setAccessible(true);
    @SuppressWarnings("unchecked")
    Map<Integer, Float> lastLineGlyphWidthMap = (Map<Integer, Float>) method.invoke(
        textShadowNode, lastLineIndex, lastLineStartCharIndex, lastLineEndCharIndex, layout);

    int zeroWidthCharIndex = text.indexOf('\u200D');
    Assert.assertTrue(zeroWidthCharIndex >= 0);

    Assert.assertTrue(lastLineGlyphWidthMap.containsKey(zeroWidthCharIndex));
    Assert.assertEquals(0.f, lastLineGlyphWidthMap.get(zeroWidthCharIndex), 0.0f);

    // Other characters in the line should still have positive width.
    for (int i = 0; i < text.length() - 1; i++) {
      Assert.assertTrue(lastLineGlyphWidthMap.get(i) > 0.f);
    }

    // Sum of glyph widths should be consistent with line width.
    float sumOfGlyphWidth = 0.f;
    for (Map.Entry<Integer, Float> entry : lastLineGlyphWidthMap.entrySet()) {
      sumOfGlyphWidth += entry.getValue();
    }
    float lineWidth = layout.getLineRight(lastLineIndex) - layout.getLineLeft(lastLineIndex);
    Assert.assertTrue(Math.abs(sumOfGlyphWidth - lineWidth) <= 1.0f);
  }
}
