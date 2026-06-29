// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.text;

import static org.junit.Assert.*;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;

import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.PointF;
import android.graphics.drawable.Drawable;
import android.text.Layout;
import android.text.SpannableStringBuilder;
import android.text.StaticLayout;
import android.text.TextPaint;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.shadow.text.TextUpdateBundle;
import com.lynx.testing.base.TestingUtils;
import java.util.HashSet;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class FlattenUITextTest {
  private LynxContext mContext;

  @Before
  public void setUp() throws Exception {
    mContext = TestingUtils.getLynxContext();
  }

  @After
  public void tearDown() throws Exception {}

  @Test
  public void getDrawPositionLeft() {
    FlattenUIText text = spy(new FlattenUIText(mContext));

    assertEquals(text.getDrawOffsetLeft(), 0);
  }

  @Test
  public void getDrawPositionTop() {
    FlattenUIText text = spy(new FlattenUIText(mContext));

    assertEquals(text.getDrawOffsetTop(), 0);
  }

  @Test
  public void invalidateDrawable() {
    FlattenUIText text = spy(new FlattenUIText(mContext));
    FlattenUIText.DrawableCallback callback = spy(new FlattenUIText.DrawableCallback(text));

    callback.invalidateDrawable(new Drawable() {
      @Override
      public void draw(Canvas canvas) {}

      @Override
      public void setAlpha(int i) {}

      @Override
      public void setColorFilter(ColorFilter colorFilter) {}

      @Override
      public int getOpacity() {
        return 0;
      }
    });
  }

  @Test
  public void displayNoneSkipsDraw() {
    FlattenUIText text = new FlattenUIText(mContext);
    Layout layoutSpy = spy(buildTextLayout("test", 200));
    TextUpdateBundle bundle = new TextUpdateBundle(layoutSpy, false, new HashSet<>(), false);
    bundle.setTextTranslateOffset(new PointF());
    text.setTextBundle(bundle);

    text.setDisplayNone(true);
    text.onDraw(new Canvas());
    verify(layoutSpy, never()).draw(any(Canvas.class));

    text.setDisplayNone(false);
    text.onDraw(new Canvas());
    verify(layoutSpy).draw(any(Canvas.class));
  }

  private Layout buildTextLayout(String text, int width) {
    SpannableStringBuilder span = new SpannableStringBuilder(text);
    TextPaint textPaint = new TextPaint();
    textPaint.setTextSize(30);
    return new StaticLayout(span, textPaint, width, Layout.Alignment.ALIGN_NORMAL, 1, 0, false);
  }
}
