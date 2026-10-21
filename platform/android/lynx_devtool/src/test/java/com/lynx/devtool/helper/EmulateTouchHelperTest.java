// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.helper;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockConstruction;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.view.MotionEvent;
import com.lynx.devtoolwrapper.IDevToolDelegate;
import com.lynx.devtoolwrapper.ScreenshotMode;
import com.lynx.tasm.LynxView;
import java.lang.ref.WeakReference;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.MockedConstruction;
import org.mockito.MockedStatic;

public class EmulateTouchHelperTest {
  private MockedConstruction<Handler> mHandlers;
  private MockedStatic<Looper> mLoopers;
  private MockedStatic<SystemClock> mClock;
  private MockedStatic<MotionEvent> mMotionEvents;
  private LynxView mView;
  private IDevToolDelegate mDelegate;
  private EmulateTouchHelper mHelper;

  @Before
  public void setUp() {
    mHandlers = mockConstruction(Handler.class);
    mLoopers = mockStatic(Looper.class);
    mClock = mockStatic(SystemClock.class);
    mMotionEvents = mockStatic(MotionEvent.class);
    mView = mock(LynxView.class);
    mDelegate = mock(IDevToolDelegate.class);
    doAnswer(invocation -> {
      int[] location = invocation.getArgument(0);
      location[0] = 40;
      location[1] = 70;
      return null;
    })
        .when(mView)
        .getLocationOnScreen(any(int[].class));
    mHelper = new EmulateTouchHelper(new WeakReference<>(mView));
  }

  @After
  public void tearDown() {
    mMotionEvents.close();
    mClock.close();
    mLoopers.close();
    mHandlers.close();
  }

  @Test
  public void nativeFullscreenUsesRequestDespiteStaleRendererMode() {
    when(mDelegate.getActualScreenshotMode()).thenReturn(ScreenshotMode.SCREEN_SHOT_MODE_LYNX_VIEW);
    mHelper.emulateTouch("mousePressed", 140, 270, 0, 0, "left", mDelegate,
        ScreenshotMode.SCREEN_SHOT_MODE_FULL_SCREEN);

    verifyDownCoordinates(100, 200);
    verify(mDelegate, never()).getActualScreenshotMode();
    verify(mDelegate, never()).takeScreenshot(any(), any());
  }

  @Test
  public void nativeLynxviewUsesRequestWithoutScreencast() {
    when(mDelegate.getActualScreenshotMode())
        .thenReturn(ScreenshotMode.SCREEN_SHOT_MODE_FULL_SCREEN);
    mHelper.emulateTouch("mousePressed", 140, 270, 0, 0, "left", mDelegate,
        ScreenshotMode.SCREEN_SHOT_MODE_LYNX_VIEW);

    verifyDownCoordinates(140, 270);
    verify(mView, never()).getLocationOnScreen(any(int[].class));
    verify(mDelegate, never()).getActualScreenshotMode();
    verify(mDelegate, never()).takeScreenshot(any(), any());
  }

  @Test
  public void clayRetainsRendererMode() {
    when(mDelegate.isClayRenderer()).thenReturn(true);
    when(mDelegate.getActualScreenshotMode()).thenReturn(ScreenshotMode.SCREEN_SHOT_MODE_LYNX_VIEW);
    mHelper.emulateTouch("mousePressed", 140, 270, 0, 0, "left", mDelegate,
        ScreenshotMode.SCREEN_SHOT_MODE_FULL_SCREEN);

    verifyDownCoordinates(140, 270);
    verify(mView, never()).getLocationOnScreen(any(int[].class));
    verify(mDelegate).getActualScreenshotMode();
  }

  private void verifyDownCoordinates(float x, float y) {
    mMotionEvents.verify(()
                             -> MotionEvent.obtain(anyLong(), anyLong(),
                                 eq(MotionEvent.ACTION_DOWN), eq(x), eq(y), eq(0)));
  }
}
