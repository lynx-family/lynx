// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.helper;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

import android.app.Instrumentation;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.WindowManager;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.devtoolwrapper.ScreenshotMode;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import org.junit.After;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class ScreencastSnapshotTest {
  private final Instrumentation mInstrumentation = InstrumentationRegistry.getInstrumentation();
  private final ScreencastSnapshot.Buffers mBuffers = new ScreencastSnapshot.Buffers();
  private Bitmap mOutput;
  private int mSourceWidth;
  private int mSourceHeight;

  @After
  public void releaseBitmaps() {
    if (mOutput != null) {
      mOutput.recycle();
    }
    mBuffers.release();
  }

  @Test
  public void scalesViewAndReusesMatchingOutputBitmap() throws InterruptedException {
    Bitmap reusable = Bitmap.createBitmap(30, 15, Bitmap.Config.ARGB_8888);
    capture(ScreenshotMode.SCREEN_SHOT_MODE_LYNX_VIEW, 60, 15, reusable,
        () -> makeDrawableView(80, 40));
    assertSame(reusable, mOutput);
    assertEquals(80, mSourceWidth);
    assertEquals(40, mSourceHeight);
    assertEquals(Color.GREEN, mOutput.getPixel(15, 7));
  }

  @Test
  public void replacesWrongSizedOutputWithoutUpscaling() throws InterruptedException {
    Bitmap reusable = Bitmap.createBitmap(2, 2, Bitmap.Config.ARGB_8888);
    capture(ScreenshotMode.SCREEN_SHOT_MODE_LYNX_VIEW, 160, 160, reusable,
        () -> makeDrawableView(80, 40));
    assertTrue(reusable.isRecycled());
    assertEquals(80, mOutput.getWidth());
    assertEquals(40, mOutput.getHeight());
  }

  @Test
  public void capturesRootViewAtDisplaySizeInFullscreenMode() throws InterruptedException {
    DisplayMetrics metrics = new DisplayMetrics();
    capture(ScreenshotMode.SCREEN_SHOT_MODE_FULL_SCREEN, 64, 64, null, () -> {
      Context context = mInstrumentation.getTargetContext();
      WindowManager manager = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
      manager.getDefaultDisplay().getRealMetrics(metrics);
      return makeDrawableView(metrics.widthPixels, metrics.heightPixels);
    });
    assertEquals(metrics.widthPixels, mSourceWidth);
    assertEquals(metrics.heightPixels, mSourceHeight);
    assertEquals(64, Math.max(mOutput.getWidth(), mOutput.getHeight()));
    assertEquals(Color.GREEN, mOutput.getPixel(0, 0));
  }

  @Test
  public void capturesWindowRegionAtItsOwnResolution() throws InterruptedException {
    capture(
        ScreenshotMode.SCREEN_SHOT_MODE_FULL_SCREEN, 48, 48, null, new Rect(40, 45, 80, 75), () -> {
          View root = new View(mInstrumentation.getTargetContext()) {
            @Override
            public boolean isAttachedToWindow() {
              return true;
            }

            @Override
            public void getLocationOnScreen(int[] location) {
              location[0] = 10;
              location[1] = 20;
            }

            @Override
            public void draw(Canvas canvas) {
              canvas.drawColor(Color.RED);
              Paint paint = new Paint();
              paint.setColor(Color.GREEN);
              canvas.drawRect(30, 25, 55, 55, paint);
            }
          };
          root.layout(0, 0, 108, 240);
          return root;
        });
    assertEquals(40, mSourceWidth);
    assertEquals(30, mSourceHeight);
    assertEquals(40, mOutput.getWidth());
    assertEquals(30, mOutput.getHeight());
    assertEquals(Color.GREEN, mOutput.getPixel(0, 0));
    assertEquals(Color.GREEN, mOutput.getPixel(24, 29));
    assertEquals(Color.RED, mOutput.getPixel(25, 29));
  }

  @Test
  public void rejectsUnattachedViewAndRecyclesOutputBuffer() throws InterruptedException {
    Bitmap reusable = Bitmap.createBitmap(8, 8, Bitmap.Config.ARGB_8888);
    capture(ScreenshotMode.SCREEN_SHOT_MODE_LYNX_VIEW, 8, 8, reusable,
        () -> new View(mInstrumentation.getTargetContext()));
    assertTrue(reusable.isRecycled());
    assertNull(mOutput);
    assertEquals(0, mSourceWidth);
    assertEquals(0, mSourceHeight);
  }

  @Test
  public void recyclesOutputWhenDrawingFails() throws InterruptedException {
    Bitmap reusable = Bitmap.createBitmap(8, 8, Bitmap.Config.ARGB_8888);
    capture(ScreenshotMode.SCREEN_SHOT_MODE_LYNX_VIEW, 8, 8, reusable, () -> {
      View view = new View(mInstrumentation.getTargetContext()) {
        @Override
        public boolean isAttachedToWindow() {
          return true;
        }

        @Override
        public void draw(Canvas canvas) {
          throw new IllegalStateException("Test draw failure");
        }
      };
      view.layout(0, 0, 8, 8);
      return view;
    });
    assertTrue(reusable.isRecycled());
    assertNull(mOutput);
  }

  @Test
  public void layerBufferIsReusedResizedAndReleased() {
    Bitmap original = mBuffers.obtain(8, 8);
    assertSame(original, mBuffers.obtain(8, 8));
    Bitmap resized = mBuffers.obtain(8, 4);
    assertNotSame(original, resized);
    assertTrue(original.isRecycled());
    assertFalse(resized.isRecycled());
    mBuffers.release();
    assertTrue(resized.isRecycled());
  }

  private interface ViewFactory {
    View create();
  }

  private void capture(String mode, int maxWidth, int maxHeight, Bitmap reusable,
      ViewFactory viewFactory) throws InterruptedException {
    capture(mode, maxWidth, maxHeight, reusable, null, viewFactory);
  }

  private void capture(String mode, int maxWidth, int maxHeight, Bitmap reusable, Rect crop,
      ViewFactory viewFactory) throws InterruptedException {
    CountDownLatch completion = new CountDownLatch(1);
    mInstrumentation.runOnMainSync(
        ()
            -> ScreencastSnapshot.capture(viewFactory.create(), mode, maxWidth, maxHeight, reusable,
                mBuffers, crop, (bitmap, sourceWidth, sourceHeight) -> {
                  mOutput = bitmap;
                  mSourceWidth = sourceWidth;
                  mSourceHeight = sourceHeight;
                  completion.countDown();
                }));
    assertTrue("Timed out waiting for capture", completion.await(5, TimeUnit.SECONDS));
  }

  private View makeDrawableView(int width, int height) {
    // Emulate a laid-out view without a window surface so this exercises View.draw().
    View view = new View(mInstrumentation.getTargetContext()) {
      @Override
      public boolean isAttachedToWindow() {
        return true;
      }

      @Override
      public void draw(Canvas canvas) {
        canvas.drawColor(Color.GREEN);
      }
    };
    view.layout(0, 0, width, height);
    return view;
  }
}
