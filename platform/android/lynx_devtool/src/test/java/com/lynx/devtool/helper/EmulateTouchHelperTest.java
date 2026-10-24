// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.helper;

import static org.mockito.ArgumentMatchers.*;
import static org.mockito.Mockito.*;

import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.view.MotionEvent;
import com.lynx.devtoolwrapper.IDevToolDelegate;
import com.lynx.tasm.LynxView;
import java.lang.ref.WeakReference;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.MockedConstruction;
import org.mockito.MockedStatic;

public class EmulateTouchHelperTest {
  private MockedStatic<Looper> mLoopers;
  private MockedStatic<SystemClock> mClock;
  private MockedStatic<MotionEvent> mEvents;
  private MockedConstruction<Handler> mHandlers;
  private LynxView mView;
  private IDevToolDelegate mDelegate;
  private EmulateTouchHelper mHelper;

  @Before
  public void setUp() {
    mLoopers = mockStatic(Looper.class);
    mClock = mockStatic(SystemClock.class);
    mEvents = mockStatic(MotionEvent.class);
    mHandlers = mockConstruction(Handler.class);
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
    mHandlers.close();
    mEvents.close();
    mClock.close();
    mLoopers.close();
  }

  @Test
  public void fullscreenUsesAcceptedRequestWithoutTakingScreenshot() {
    mHelper.emulateTouch("mousePressed", 140, 270, 0, 0, "left", mDelegate, "fullscreen");
    mEvents.verify(()
                       -> MotionEvent.obtain(anyLong(), anyLong(), eq(MotionEvent.ACTION_DOWN),
                           eq(100.0f), eq(200.0f), eq(0)));
    verifyNoInteractions(mDelegate);
  }

  @Test
  public void lynxviewUsesLocalCoordinatesForEveryRenderer() {
    mHelper.emulateTouch("mousePressed", 140, 270, 0, 0, "left", mDelegate, "lynxview");
    mEvents.verify(()
                       -> MotionEvent.obtain(anyLong(), anyLong(), eq(MotionEvent.ACTION_DOWN),
                           eq(140.0f), eq(270.0f), eq(0)));
    verify(mView, never()).getLocationOnScreen(any(int[].class));
    verifyNoInteractions(mDelegate);
  }
}
