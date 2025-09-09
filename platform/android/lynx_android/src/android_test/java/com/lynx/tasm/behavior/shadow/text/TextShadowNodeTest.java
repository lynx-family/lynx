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
}
