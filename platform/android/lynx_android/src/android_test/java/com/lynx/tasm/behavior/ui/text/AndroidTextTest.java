// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.text;

import static org.junit.Assert.*;

import android.graphics.Canvas;
import android.graphics.PointF;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.text.Layout;
import android.text.SpannableStringBuilder;
import android.text.StaticLayout;
import android.text.TextPaint;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.shadow.text.TextUpdateBundle;
import com.lynx.tasm.service.ILynxTextService.Page;
import com.lynx.testing.base.TestingUtils;
import java.util.ArrayList;
import java.util.HashSet;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class AndroidTextTest {
  private AndroidText mAndroidText;
  private int mTextHeight = 0;
  private static final class FakeTextServicePage implements Page {
    private static final int TEXT_LENGTH = 30;
    private static final int CHARS_PER_LINE = 10;
    private static final float CHAR_WIDTH = 10.f;
    private static final float LINE_HEIGHT = 20.f;
    private int mSelectionRectsCallCount;

    @Override
    public void drawPageCanvas(Canvas canvas, Drawable.Callback callback) {}

    @Override
    public int getSelectionCharIndex(float touchX, float touchY) {
      int line = Math.max(0, (int) (touchY / LINE_HEIGHT));
      int column = Math.max(0, (int) (touchX / CHAR_WIDTH));
      return Math.min(TEXT_LENGTH, line * CHARS_PER_LINE + column);
    }

    @Override
    public float[] getSelectionRects(int start, int end) {
      mSelectionRectsCallCount++;
      if (start < 0 || end < start || start >= TEXT_LENGTH || start == end) {
        return new float[0];
      }
      end = Math.min(end, TEXT_LENGTH);
      ArrayList<Float> rects = new ArrayList<>();
      int current = start;
      while (current < end) {
        int line = current / CHARS_PER_LINE;
        int lineEnd = Math.min(end, (line + 1) * CHARS_PER_LINE);
        float left = (current - line * CHARS_PER_LINE) * CHAR_WIDTH;
        float top = line * LINE_HEIGHT;
        rects.add(left);
        rects.add(top);
        rects.add((lineEnd - current) * CHAR_WIDTH);
        rects.add(LINE_HEIGHT);
        current = lineEnd;
      }
      float[] result = new float[rects.size()];
      for (int i = 0; i < rects.size(); i++) {
        result[i] = rects.get(i);
      }
      return result;
    }

    @Override
    public int getTextLength() {
      return TEXT_LENGTH;
    }

    @Override
    public String getSelectedText(int start, int end) {
      String text = "abcdefghijklmnopqrstuvwxyz1234";
      return text.substring(start, end);
    }

    @Override
    public void destroy() {}

    int getSelectionRectsCallCount() {
      return mSelectionRectsCallCount;
    }
  }

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
    ArrayList<RectF> rectFArrayList = mAndroidText.setTextSelection(1, 10, 10, 10, false, false);
    assertTrue(rectFArrayList.size() == 1);
    RectF rect = rectFArrayList.get(0);
    assertTrue(rect.left == 0 && rect.top == 0 && rect.right > 0 && rect.bottom > 0);

    rectFArrayList = mAndroidText.setTextSelection(-1, 10, 10, 10, false, false);
    assertTrue(rectFArrayList.size() == 0);

    rectFArrayList = mAndroidText.setTextSelection(2, 10, 100, 50, false, false);
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

    mAndroidText.setTextSelection(1, 10, 10, 10, false, false);
    handles = mAndroidText.getHandlesInfo();
    assertTrue(handles.length == 2);
    assertTrue(handles[1].get(0) > handles[0].get(0));
    assertTrue(handles[0].get(1).equals(handles[1].get(1)));
    assertTrue(handles[0].get(2).equals(handles[1].get(2)));

    mAndroidText.setTextSelection(2, 10, 100, 50, false, false);
    handles = mAndroidText.getHandlesInfo();
    assertTrue(handles.length == 2);
    assertTrue(handles[1].get(1).floatValue() > handles[0].get(1).floatValue());
  }

  @Test
  public void testGetSelectedText() {
    String selectedText = mAndroidText.getSelectedText();
    assertTrue(selectedText.isEmpty());

    mAndroidText.setTextSelection(1, 10, 10, 10, false, false);
    selectedText = mAndroidText.getSelectedText();
    assertTrue(selectedText.length() > 0);
  }

  @Test
  public void testTextServiceGetTextBoundingBoxes() {
    mAndroidText.setTextBundle(new FakeTextServicePage());
    ArrayList<RectF> rectFArrayList = mAndroidText.getTextBoundingBoxes(2, 14);
    assertEquals(2, rectFArrayList.size());

    RectF firstLine = rectFArrayList.get(0);
    assertEquals(20.f, firstLine.left, 0.f);
    assertEquals(0.f, firstLine.top, 0.f);
    assertEquals(100.f, firstLine.right, 0.f);
    assertEquals(20.f, firstLine.bottom, 0.f);

    RectF secondLine = rectFArrayList.get(1);
    assertEquals(0.f, secondLine.left, 0.f);
    assertEquals(20.f, secondLine.top, 0.f);
    assertEquals(40.f, secondLine.right, 0.f);
    assertEquals(40.f, secondLine.bottom, 0.f);
  }

  @Test
  public void testTextServiceSetTextSelection() {
    FakeTextServicePage page = new FakeTextServicePage();
    mAndroidText.setTextBundle(page);
    ArrayList<RectF> rectFArrayList = mAndroidText.setTextSelection(5, 5, 35, 5, false, false);
    assertEquals(1, rectFArrayList.size());
    assertEquals(0.f, rectFArrayList.get(0).left, 0.f);
    assertEquals(30.f, rectFArrayList.get(0).right, 0.f);
    assertEquals(1, page.getSelectionRectsCallCount());

    ArrayList<Float>[] handles = mAndroidText.getHandlesInfo();
    assertEquals(2, handles.length);
    assertTrue(handles[1].get(0) > handles[0].get(0));

    assertEquals("abc", mAndroidText.getSelectedText());

    rectFArrayList = mAndroidText.setTextSelection(5, 5, 5, 5, false, false);
    assertEquals(1, rectFArrayList.size());
    assertEquals(10.f, rectFArrayList.get(0).width(), 0.f);
  }

  @Test
  public void testTextServiceSelectionRectCache() {
    FakeTextServicePage page = new FakeTextServicePage();
    mAndroidText.setTextBundle(page);

    mAndroidText.setTextSelection(5, 5, 35, 5, false, false);
    assertEquals(1, page.getSelectionRectsCallCount());

    mAndroidText.getTextBoundingBoxes(0, 3);
    assertEquals(1, page.getSelectionRectsCallCount());

    mAndroidText.getTextBoundingBoxes(0, 4);
    assertEquals(2, page.getSelectionRectsCallCount());
  }
}
