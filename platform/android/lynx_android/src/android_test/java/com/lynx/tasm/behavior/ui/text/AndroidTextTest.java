// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.text;

import static org.junit.Assert.*;

import android.graphics.PointF;
import android.graphics.RectF;
import android.text.Layout;
import android.text.SpannableStringBuilder;
import android.text.StaticLayout;
import android.text.TextPaint;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.shadow.text.TextUpdateBundle;
import com.lynx.testing.base.TestingUtils;
import java.util.ArrayList;
import java.util.HashSet;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class AndroidTextTest {
  private AndroidText mAndroidText;
  private int mTextHeight = 0;
  @Before
  public void setUp() throws Exception {
    LynxContext lynxContext = TestingUtils.getLynxContext();
    mAndroidText = new AndroidText(lynxContext);

    Layout layout = buildTextLayout(
        "This is a test text.This is a test text.This is a test text.This is a test text.", 200);
    mTextHeight = layout.getHeight();
    mAndroidText.setRight(200);
    mAndroidText.setBottom(mTextHeight);
    TextUpdateBundle textUpdateBundle = new TextUpdateBundle(layout, false, new HashSet<>(), false);
    textUpdateBundle.setTextTranslateOffset(new PointF());
    mAndroidText.setTextBundle(textUpdateBundle);
  }

  private Layout buildTextLayout(String text, int width) {
    SpannableStringBuilder span = new SpannableStringBuilder(text);
    TextPaint textPaint = new TextPaint();
    textPaint.setTextSize(30);
    return new StaticLayout(span, textPaint, width, Layout.Alignment.ALIGN_NORMAL, 1, 0, false);
  }

  @After
  public void tearDown() throws Exception {
    mAndroidText = null;
  }

  @Test
  public void testGetTextBoundingBoxes() {
    ArrayList<RectF> rectFArrayList = mAndroidText.getTextBoundingBoxes(0, 1);
    assertTrue(rectFArrayList.size() == 1);
    RectF rect = rectFArrayList.get(0);
    assertTrue(rect.left == 0 && rect.top == 0 && rect.right > 0 && rect.bottom > 0);

    rectFArrayList = mAndroidText.getTextBoundingBoxes(-1, 1);
    assertTrue(rectFArrayList.size() == 0);

    rectFArrayList = mAndroidText.getTextBoundingBoxes(0, 100);
    assertTrue(rectFArrayList.size() == 0);

    rectFArrayList = mAndroidText.getTextBoundingBoxes(10, 10);
    assertTrue(rectFArrayList.size() == 1);
    rect = rectFArrayList.get(0);
    assertTrue(rect.left == 0 && rect.right == 0 && rect.top == 0 && rect.bottom > 0);

    rectFArrayList = mAndroidText.getTextBoundingBoxes(2, 40);
    assertTrue(rectFArrayList.size() > 0);
    rect = rectFArrayList.get(0);
    assertTrue(rect.left > 0 && rect.right > rect.left);
  }

  @Test
  public void testSetTextSelection() {
    ArrayList<RectF> rectFArrayList = mAndroidText.setTextSelection(1, 10, 10, 10);
    assertTrue(rectFArrayList.size() == 1);
    RectF rect = rectFArrayList.get(0);
    assertTrue(rect.left == 0 && rect.top == 0 && rect.right > 0 && rect.bottom > 0);

    rectFArrayList = mAndroidText.setTextSelection(-1, 10, 10, 10);
    assertTrue(rectFArrayList.size() == 0);

    rectFArrayList = mAndroidText.setTextSelection(2, 10, 100, 50);
    assertTrue(rectFArrayList.size() > 0);
    rect = rectFArrayList.get(0);
    assertTrue(rect.left >= 0 && rect.right > 0 && rect.top == 0 && rect.bottom > 0);
    rect = rectFArrayList.get(1);
    assertTrue(rect.left == 0 && rect.right > 0 && rect.top > 0 && rect.bottom > rect.top);
  }

  @Test
  public void testGetHandlesInfo() {
    ArrayList<Float>[] handles = mAndroidText.getHandlesInfo();
    assertTrue(handles.length == 0);

    mAndroidText.setTextSelection(1, 10, 10, 10);
    handles = mAndroidText.getHandlesInfo();
    assertTrue(handles.length == 2);
    assertTrue(handles[1].get(0) > handles[0].get(0));
    assertTrue(handles[0].get(1).equals(handles[1].get(1)));
    assertTrue(handles[0].get(2).equals(handles[1].get(2)));

    mAndroidText.setTextSelection(2, 10, 100, 50);
    handles = mAndroidText.getHandlesInfo();
    assertTrue(handles.length == 2);
    assertTrue(handles[1].get(1).floatValue() > handles[0].get(1).floatValue());
  }

  @Test
  public void testGetSelectedText() {
    String selectedText = mAndroidText.getSelectedText();
    assertTrue(selectedText.isEmpty());

    mAndroidText.setTextSelection(1, 10, 10, 10);
    selectedText = mAndroidText.getSelectedText();
    assertTrue(selectedText.length() > 0);
  }
}
