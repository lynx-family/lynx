// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.helper;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.anyFloat;
import static org.mockito.Mockito.anyString;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Instrumentation;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.util.Base64;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.devtool.DevToolPlatformAndroidDelegate;
import com.lynx.devtoolwrapper.IDevToolDelegate;
import com.lynx.devtoolwrapper.ScreenshotMode;
import com.lynx.tasm.LynxView;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class ScreencastSessionTest {
  private final Instrumentation mInstrumentation = InstrumentationRegistry.getInstrumentation();
  private final DevToolPlatformAndroidDelegate mPlatform =
      mock(DevToolPlatformAndroidDelegate.class);
  private final IDevToolDelegate mDelegate = mock(IDevToolDelegate.class);
  private final BlockingQueue<String> mFrames = new LinkedBlockingQueue<>();
  private LynxView mView;
  private ScreenCastHelper mHelper;
  private int mColor = Color.RED;

  @Before
  public void setUp() {
    when(mDelegate.getActualScreenshotMode())
        .thenReturn(ScreenshotMode.SCREEN_SHOT_MODE_FULL_SCREEN);
    doAnswer(invocation -> {
      mFrames.add((String) invocation.getArguments()[0]);
      return null;
    })
        .when(mPlatform)
        .sendScreenCast(
            anyString(), any(ScreenCapturer.ScreenMetadata.class), anyFloat(), anyFloat());
    mInstrumentation.runOnMainSync(() -> {
      mView = new LynxView(mInstrumentation.getTargetContext()) {
        @Override
        public boolean isAttachedToWindow() {
          return true;
        }

        @Override
        public void draw(Canvas canvas) {
          canvas.drawColor(mColor);
        }
      };
      mView.layout(0, 0, 64, 32);
      mHelper = new ScreenCastHelper(mPlatform, mView);
    });
  }

  @After
  public void tearDown() {
    if (mHelper != null) {
      mInstrumentation.runOnMainSync(mHelper::stopCasting);
    }
  }

  private void startCasting() {
    mHelper.startCasting(90, 32, 16, ScreenshotMode.SCREEN_SHOT_MODE_LYNX_VIEW, "jpeg", mDelegate);
  }

  @Test
  public void boundedJpegWaitsForAckBeforeSendingChangedFrame() throws InterruptedException {
    mInstrumentation.runOnMainSync(this::startCasting);
    assertFrame(Color.RED);
    mInstrumentation.runOnMainSync(() -> mColor = Color.BLUE);
    // Allow a full legacy capture interval to elapse while the first frame is unacknowledged.
    assertNull(mFrames.poll(1500, TimeUnit.MILLISECONDS));
    mInstrumentation.runOnMainSync(mHelper::onAckReceived);
    assertFrame(Color.BLUE);
  }

  @Test
  public void resumeSendsFreshFrameAndStopClearsCaptureMode() throws InterruptedException {
    mInstrumentation.runOnMainSync(this::startCasting);
    assertFrame(Color.RED);
    mInstrumentation.runOnMainSync(mHelper::pauseCasting);
    verify(mPlatform).dispatchScreencastVisibilityChanged(false);
    mInstrumentation.runOnMainSync(mHelper::continueCasting);
    // Resuming resets both the pending ACK and the previous image, even for identical pixels.
    assertFrame(Color.RED);
    assertEquals(ScreenshotMode.SCREEN_SHOT_MODE_LYNX_VIEW, mHelper.getCaptureMode(mDelegate));
    mInstrumentation.runOnMainSync(mHelper::stopCasting);
    assertEquals(ScreenshotMode.SCREEN_SHOT_MODE_FULL_SCREEN, mHelper.getCaptureMode(mDelegate));
  }

  private void assertFrame(int color) throws InterruptedException {
    String data = mFrames.poll(10, TimeUnit.SECONDS);
    assertNotNull("Timed out waiting for a JPEG frame", data);
    byte[] encoded = Base64.decode(data, Base64.NO_WRAP);
    Bitmap bitmap = BitmapFactory.decodeByteArray(encoded, 0, encoded.length);
    assertNotNull(bitmap);
    try {
      assertEquals(32, bitmap.getWidth());
      assertEquals(16, bitmap.getHeight());
      int pixel = bitmap.getPixel(16, 8);
      assertTrue(Math.abs(Color.red(pixel) - Color.red(color)) < 8);
      assertTrue(Math.abs(Color.blue(pixel) - Color.blue(color)) < 8);
    } finally {
      bitmap.recycle();
    }
  }
}
