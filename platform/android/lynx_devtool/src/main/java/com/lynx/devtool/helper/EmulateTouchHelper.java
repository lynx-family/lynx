// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.helper;

import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.view.MotionEvent;
import com.lynx.devtoolwrapper.IDevToolDelegate;
import com.lynx.devtoolwrapper.ScreenshotMode;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.event.LynxTouchEvent;
import com.lynx.tasm.utils.LynxConstants;
import java.lang.ref.WeakReference;

public class EmulateTouchHelper {
  private static final String TAG = "EmulateTouchHelper";

  private WeakReference<LynxView> mLynxView;
  private MotionEvent mEvent;
  private boolean mMouseWheelFlag;
  private float mLastX;
  private float mLastY;
  private Handler mMouseWheelHandler;
  private int mDeltaScale;

  public EmulateTouchHelper(WeakReference<LynxView> lynxView) {
    mLynxView = lynxView;
    mEvent = null;
    mMouseWheelFlag = false;
    mMouseWheelHandler = new Handler(Looper.getMainLooper());
    mDeltaScale = 10;
  }
  public void attach(LynxView lynxView) {
    mLynxView = new WeakReference<>(lynxView);
  }

  public void emulateTouch(String type, int x, int y, float deltaX, float deltaY, String button,
      IDevToolDelegate devToolDelegate) {
    if (type == null) {
      LLog.e(TAG, "emulateTouch: type is null");
      return;
    }
    LynxView view = mLynxView.get();
    if (view == null) {
      LLog.e(TAG, "emulateTouch: view is null");
      return;
    }

    if (devToolDelegate == null) {
      LLog.e(TAG, "emulateTouch: devToolDelegate is null");
      return;
    }

    if (devToolDelegate.getActualScreenshotMode().equals(
            ScreenshotMode.SCREEN_SHOT_MODE_FULL_SCREEN)) {
      int[] outLocation = new int[2];
      view.getLocationOnScreen(outLocation);
      x -= outLocation[0];
      y -= outLocation[1];
    }
    if (type.equals("mousePressed")) {
      mEvent = MotionEvent.obtain(
          SystemClock.uptimeMillis(), SystemClock.uptimeMillis(), MotionEvent.ACTION_DOWN, x, y, 0);
      view.dispatchTouchEvent(mEvent);
    } else if (type.equals("mouseMoved")) {
      mEvent = MotionEvent.obtain(
          SystemClock.uptimeMillis(), SystemClock.uptimeMillis(), MotionEvent.ACTION_MOVE, x, y, 0);
      view.dispatchTouchEvent(mEvent);
    } else if (type.equals("mouseReleased")) {
      mEvent = MotionEvent.obtain(
          SystemClock.uptimeMillis(), SystemClock.uptimeMillis(), MotionEvent.ACTION_UP, x, y, 0);
      view.dispatchTouchEvent(mEvent);
      mEvent.recycle();
    } else if (type.equals("mouseWheel")) {
      mMouseWheelHandler.removeCallbacksAndMessages(null);
      if (!mMouseWheelFlag) {
        mMouseWheelFlag = true;
        mLastX = x;
        mLastY = y;
        mEvent = MotionEvent.obtain(SystemClock.uptimeMillis(), SystemClock.uptimeMillis(),
            MotionEvent.ACTION_DOWN, x, y, 0);
        view.dispatchTouchEvent(mEvent);
      }
      mLastX += deltaX / mDeltaScale;
      mLastY += deltaY / mDeltaScale;
      mEvent = MotionEvent.obtain(SystemClock.uptimeMillis(), SystemClock.uptimeMillis(),
          MotionEvent.ACTION_MOVE, mLastX, mLastY, 0);
      view.dispatchTouchEvent(mEvent);

      Runnable runnable = new Runnable() {
        @Override
        public void run() {
          stopMouseWheel();
        }
      };
      mMouseWheelHandler.postDelayed(runnable, 100);
    } else if (type.equals(LynxTouchEvent.EVENT_TOUCH_START)
        || type.equals(LynxTouchEvent.EVENT_TOUCH_MOVE)
        || type.equals(LynxTouchEvent.EVENT_TOUCH_END)
        || type.equals(LynxTouchEvent.EVENT_TOUCH_CANCEL) || type.equals(LynxTouchEvent.EVENT_TAP)
        || type.equals(LynxTouchEvent.EVENT_LONG_PRESS)
        || type.equals(LynxTouchEvent.EVENT_CLICK)) {
      if (view.getLynxContext() != null && view.getLynxContext().getEventEmitter() != null) {
        view.getLynxContext().getEventEmitter().sendTouchEvent(
            new LynxTouchEvent(Integer.parseInt(button), type, x, y));
      }
    }
  }

  private void stopMouseWheel() {
    LynxView lynxView = mLynxView.get();
    if (lynxView == null) {
      LLog.e(TAG, "stopMouseWheel: lynxView is null");
      return;
    }

    if (mMouseWheelFlag && mEvent.getAction() == MotionEvent.ACTION_MOVE) {
      mMouseWheelFlag = false;
      mEvent = MotionEvent.obtain(SystemClock.uptimeMillis(), SystemClock.uptimeMillis(),
          MotionEvent.ACTION_UP, mLastX, mLastY, 0);
      lynxView.dispatchTouchEvent(mEvent);
      mEvent.recycle();
    }
  }
}
