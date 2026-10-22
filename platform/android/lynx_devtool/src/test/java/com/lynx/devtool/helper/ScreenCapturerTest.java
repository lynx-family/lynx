// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.helper;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.view.Choreographer;
import android.view.View;
import com.lynx.devtool.framecapture.FrameTraceUtil;
import com.lynx.devtoolwrapper.IDevToolDelegate;
import com.lynx.devtoolwrapper.ScreenshotBitmapHandler;
import org.junit.Test;
import org.mockito.MockedStatic;

public class ScreenCapturerTest {
  @Test
  public void capturesAndStopsStayWithinTheirTarget() {
    try (MockedStatic<Choreographer> choreographers = mockStatic(Choreographer.class);
         MockedStatic<FrameTraceUtil> frameTrace = mockStatic(FrameTraceUtil.class)) {
      choreographers.when(Choreographer::getInstance).thenReturn(mock(Choreographer.class));
      ScreenCapturer first = spy(new ScreenCapturer());
      ScreenCapturer second = spy(new ScreenCapturer());
      doNothing().when(first).triggerNextCapture();
      doNothing().when(second).triggerNextCapture();
      View firstView = mock(View.class);
      View secondView = mock(View.class);
      IDevToolDelegate firstDelegate = mock(IDevToolDelegate.class);
      IDevToolDelegate secondDelegate = mock(IDevToolDelegate.class);
      ScreenshotBitmapHandler handler = mock(ScreenshotBitmapHandler.class);
      first.attachView(firstView);
      first.setDevToolDelegate(firstDelegate);
      second.attachView(secondView);
      second.setDevToolDelegate(secondDelegate);
      first.startCapture(400, 800, 80, "fullscreen", "jpeg", null);
      second.startCapture(400, 800, 80, "lynxview", "jpeg", null);

      first.onScreenshotActionStart();
      first.screenshot(firstView, handler);
      second.onScreenshotActionStart();
      second.screenshot(secondView, handler);
      verify(firstDelegate).takeScreenshot(handler, "fullscreen");
      verify(secondDelegate).takeScreenshot(handler, "lynxview");

      first.stopCapture(firstView);
      assertFalse(first.isEnabled());
      assertTrue(second.isEnabled());
      second.onScreenshotActionStart();
      second.screenshot(secondView, handler);
      verify(secondDelegate, times(2)).takeScreenshot(handler, "lynxview");
      second.stopCapture(secondView);
    }
  }
}
