// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.text;

import static org.junit.Assert.*;
import static org.mockito.Mockito.spy;

import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.drawable.Drawable;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.testing.base.TestingUtils;
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
}
