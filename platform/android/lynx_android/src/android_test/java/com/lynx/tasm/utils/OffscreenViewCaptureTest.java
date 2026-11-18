// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.utils;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.view.View;
import android.widget.FrameLayout;
import androidx.test.annotation.UiThreadTest;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.SdkSuppress;
import com.lynx.tasm.LynxEnv;
import org.junit.After;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class OffscreenViewCaptureTest {
  private static final int SIZE = 40;

  private final Context mContext = ApplicationProvider.getApplicationContext();

  @After
  public void tearDown() {
    LynxEnv.inst().enableOffscreenCapture(false);
  }

  @Test
  public void switchIsOffByDefaultAndToggles() {
    assertFalse(LynxEnv.inst().isOffscreenCaptureEnabled());
    LynxEnv.inst().enableOffscreenCapture(true);
    assertTrue(LynxEnv.inst().isOffscreenCaptureEnabled());
  }

  @Test
  @UiThreadTest
  public void disabledCaptureLeavesCanvasUntouched() {
    Bitmap bitmap = Bitmap.createBitmap(SIZE, SIZE, Bitmap.Config.ARGB_8888);
    View view = layout(colorView(Color.RED), SIZE, SIZE);

    assertFalse(OffscreenViewCapture.drawIfEnabled(view, new Canvas(bitmap)));
    assertEquals(Color.TRANSPARENT, bitmap.getPixel(SIZE / 2, SIZE / 2));
  }

  @Test
  @UiThreadTest
  public void viewOutsideHardwareWindowFallsBack() {
    LynxEnv.inst().enableOffscreenCapture(true);
    Bitmap bitmap = Bitmap.createBitmap(SIZE, SIZE, Bitmap.Config.ARGB_8888);
    // A View that is not attached to a hardware accelerated window cannot use display lists.
    View view = layout(colorView(Color.RED), SIZE, SIZE);

    assertFalse(OffscreenViewCapture.drawIfEnabled(view, new Canvas(bitmap)));
    assertEquals(Color.TRANSPARENT, bitmap.getPixel(SIZE / 2, SIZE / 2));
  }

  @Test
  @UiThreadTest
  @SdkSuppress(minSdkVersion = 29)
  public void rendersTheScrolledViewportOfTheView() {
    FrameLayout container = new FrameLayout(mContext);
    container.addView(colorView(Color.RED), new FrameLayout.LayoutParams(SIZE, SIZE));
    FrameLayout.LayoutParams lower = new FrameLayout.LayoutParams(SIZE, SIZE);
    lower.topMargin = SIZE;
    container.addView(colorView(Color.BLUE), lower);
    layout(container, SIZE, SIZE);
    container.scrollTo(0, SIZE);
    Bitmap bitmap = Bitmap.createBitmap(SIZE, SIZE, Bitmap.Config.ARGB_8888);

    assertTrue(OffscreenViewCapture.draw(container, new Canvas(bitmap)));
    assertEquals(Color.BLUE, bitmap.getPixel(SIZE / 2, SIZE / 2));
  }

  @Test
  @UiThreadTest
  @SdkSuppress(minSdkVersion = 29)
  public void tilesKeepEveryPixelAcrossTileBoundaries() {
    final int width = 50;
    final int height = 70;
    View view = layout(new CellView(mContext), width, height);
    Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);

    // Tile sizes that do not divide the View or the cells, so tile edges cut through cells.
    assertTrue(OffscreenViewCapture.draw(view, new Canvas(bitmap), 16, 12));
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        assertEquals("pixel (" + x + ", " + y + ")", CellView.colorAt(x, y), bitmap.getPixel(x, y));
      }
    }
  }

  @Test
  @UiThreadTest
  @SdkSuppress(minSdkVersion = 29)
  public void tilesKeepTheScrolledViewport() {
    FrameLayout container = new FrameLayout(mContext);
    container.addView(colorView(Color.RED), new FrameLayout.LayoutParams(SIZE, SIZE));
    FrameLayout.LayoutParams lower = new FrameLayout.LayoutParams(SIZE, SIZE);
    lower.topMargin = SIZE;
    container.addView(colorView(Color.BLUE), lower);
    layout(container, SIZE, SIZE);
    container.scrollTo(0, SIZE);
    Bitmap bitmap = Bitmap.createBitmap(SIZE, SIZE, Bitmap.Config.ARGB_8888);

    assertTrue(OffscreenViewCapture.draw(container, new Canvas(bitmap), SIZE, SIZE / 3));
    assertEquals(Color.BLUE, bitmap.getPixel(SIZE / 2, 0));
    assertEquals(Color.BLUE, bitmap.getPixel(SIZE / 2, SIZE / 2));
    assertEquals(Color.BLUE, bitmap.getPixel(SIZE / 2, SIZE - 1));
  }

  @Test
  @UiThreadTest
  @SdkSuppress(minSdkVersion = 29)
  public void rendersHardwareBitmapContent() {
    Bitmap source = Bitmap.createBitmap(SIZE, SIZE, Bitmap.Config.ARGB_8888);
    source.eraseColor(Color.GREEN);
    final Bitmap hardware = source.copy(Bitmap.Config.HARDWARE, false);
    // Drawing this View into a software canvas throws, because the canvas cannot read a
    // HARDWARE bitmap.
    View view = layout(new View(mContext) {
      @Override
      protected void onDraw(Canvas canvas) {
        canvas.drawBitmap(hardware, 0, 0, null);
      }
    }, SIZE, SIZE);
    Bitmap bitmap = Bitmap.createBitmap(SIZE, SIZE, Bitmap.Config.ARGB_8888);

    assertTrue(OffscreenViewCapture.draw(view, new Canvas(bitmap)));
    assertEquals(Color.GREEN, bitmap.getPixel(SIZE / 2, SIZE / 2));
  }

  /** Fills 5 x 5 pixel cells with colors that differ between neighboring cells. */
  private static final class CellView extends View {
    private static final int CELL = 5;
    private final Paint mPaint = new Paint();

    CellView(Context context) {
      super(context);
    }

    static int colorAt(int x, int y) {
      return Color.rgb((x / CELL) * 23 % 256, (y / CELL) * 37 % 256, 128);
    }

    @Override
    protected void onDraw(Canvas canvas) {
      for (int y = 0; y < getHeight(); y += CELL) {
        for (int x = 0; x < getWidth(); x += CELL) {
          mPaint.setColor(colorAt(x, y));
          canvas.drawRect(x, y, x + CELL, y + CELL, mPaint);
        }
      }
    }
  }

  private View colorView(int color) {
    View view = new View(mContext);
    view.setBackgroundColor(color);
    return view;
  }

  private static <T extends View> T layout(T view, int width, int height) {
    view.measure(View.MeasureSpec.makeMeasureSpec(width, View.MeasureSpec.EXACTLY),
        View.MeasureSpec.makeMeasureSpec(height, View.MeasureSpec.EXACTLY));
    view.layout(0, 0, width, height);
    return view;
  }
}
