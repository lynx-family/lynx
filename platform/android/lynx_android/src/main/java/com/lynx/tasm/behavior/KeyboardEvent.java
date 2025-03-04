// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import android.app.Activity;
import android.graphics.Rect;
import android.graphics.drawable.GradientDrawable;
import android.os.Build;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.OrientationEventListener;
import android.view.Surface;
import android.view.View;
import android.view.ViewTreeObserver;
import androidx.appcompat.widget.LinearLayoutCompat;
import androidx.recyclerview.widget.OrientationHelper;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.core.LynxThreadPool;
import com.lynx.tasm.event.LynxKeyboardEvent;
import com.lynx.tasm.utils.ContextUtils;
import com.lynx.tasm.utils.LynxConstants;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;
import java.util.Map;
import java.util.WeakHashMap;

/**
 * Each LynxContext or LynxView would own a KeyboardEvent once it enable that.
 */
public class KeyboardEvent {
  private LynxContext mLynxContext;
  private ViewTreeObserver.OnGlobalLayoutListener mListener = null;
  private float mDpi;
  private boolean isStartedInUIThread = false;
  private KeyboardMonitor mKeyboardMonitor;
  private int mViewHeight = 0;
  @Deprecated private int mOldViewHeight = 0;
  private Rect mDisplayFrame;
  private static final double KEYBOARD_LOWER_THRESHOLD = 0.4;
  private static final double KEYBOARD_HIGHER_THRESHOLD = 0.9;
  private int keyboardHeightForLast = 0;
  private int keyboardTopFromLynxView = 0;
  private WeakHashMap<Object, ViewTreeObserver.OnGlobalLayoutListener> mOnGlobalLayoutListenerList =
      new WeakHashMap<>();

  public KeyboardEvent(LynxContext lynxContext) {
    LLog.d(LynxConstants.TAG, "KeyboardEvent initialized.");

    mLynxContext = lynxContext;
    mDpi = lynxContext.getScreenMetrics().density;
    mDisplayFrame = new Rect();
  }

  public synchronized void start() {
    if (isStartedInUIThread) {
      LLog.d(LynxConstants.TAG, "KeyboardEvent already started");
      return;
    }

    /*
     ** Need to post to UI thread to avoid multi-thread issue
     * since Template Renderer may run in async mode and setSoftInputMode
     * should be run in main thread
     */
    if (!UIThreadUtils.isOnUiThread()) {
      UIThreadUtils.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          startInMain();
        }
      });
    } else {
      startInMain();
    }
  }

  public boolean isStart() {
    return isStartedInUIThread;
  }

  /*
   * @return {null} by default
   */
  public KeyboardMonitor getKeyboardMonitor() {
    return mKeyboardMonitor;
  }

  public void addOnGlobalLayoutListener(
      Object key, ViewTreeObserver.OnGlobalLayoutListener listener) {
    mOnGlobalLayoutListenerList.put(key, listener);
    start();
  }

  public void removeOnGlobalLayoutListener(
      Object key, ViewTreeObserver.OnGlobalLayoutListener listener) {
    try {
      if (listener != null && mKeyboardMonitor != null) {
        mOnGlobalLayoutListenerList.remove(key);
      }
    } catch (Exception e) {
    }
  }

  public ViewTreeObserver.OnGlobalLayoutListener getListener(Object key) {
    return mOnGlobalLayoutListenerList.get(key);
  }

  public void detectKeyboardChangeAndSendEvent() {
    try {
      Activity activity = ContextUtils.getActivity(mLynxContext);
      if (activity == null) {
        LLog.e(LynxConstants.TAG, "KeyboardEvent's context must be Activity.");
        return;
      }
      LynxContext lynxContext = mLynxContext;
      final View decorView = activity.getWindow().getDecorView();
      final boolean useRelativeKeyboardHeightApi = lynxContext.useRelativeKeyboardHeightApi();
      if (lynxContext.getUIBody() == null) {
        return;
      }
      final WeakReference<UIBody.UIBodyView> bodyView =
          new WeakReference<>(lynxContext.getUIBody().getBodyView());

      mKeyboardMonitor.getDecorView().getWindowVisibleDisplayFrame(mDisplayFrame);
      // get height of visible screen part
      int keyboardTop = mKeyboardMonitor.getDecorView().getHeight() + mDisplayFrame.top;
      int displayHeight = mKeyboardMonitor.getDecorView().getHeight();
      // get total screen height
      if (mOldViewHeight == 0) {
        // this old implementation is incorrect and takes status bar height into account,
        // so we have to keep backward compatible...
        mOldViewHeight = decorView.getHeight();
      }
      mViewHeight = mKeyboardMonitor.getDefaultMonitorBottom() - mDisplayFrame.top;
      int rotation = activity.getWindow().getWindowManager().getDefaultDisplay().getRotation();
      int height = mOldViewHeight;
      int viewHeight = mViewHeight;
      // get the height of keyboard
      double visibleRatio = (double) displayHeight / viewHeight;
      if ((rotation == Surface.ROTATION_0 || rotation == Surface.ROTATION_180)
          && visibleRatio < KEYBOARD_LOWER_THRESHOLD) {
        // cover bad cases in Android 11
        // the mKeyboardMonitor.decorView may own a very small height value,
        // we should ignore this LayoutRequest.
        UIThreadUtils.runOnUiThread(new Runnable() {
          @Override
          public void run() {
            mKeyboardMonitor.getDecorView().requestLayout();
          }
        });
        return;
      }
      // TODO(zhangkaijie.9): use getRootWindowInsets().isVisible(WindowInsetsCompat.Type.ime())
      boolean visible = visibleRatio < KEYBOARD_HIGHER_THRESHOLD;
      int keyboardHeight = 0;
      int keyboardHeightCompat = 0;
      UIBody.UIBodyView bodyInst = bodyView.get();
      if (visible) {
        keyboardHeight = (int) ((height - displayHeight) / mDpi);
      }

      if (useRelativeKeyboardHeightApi && bodyInst != null) {
        int[] bodyLocation = new int[2];
        bodyInst.getLocationOnScreen(bodyLocation);
        int bodyY = bodyLocation[1];
        int bodyHeight = bodyInst.getHeight();
        keyboardHeightCompat = (int) ((bodyY + bodyHeight - keyboardTop) / mDpi);
      } else {
        if (visible) {
          keyboardHeightCompat = (int) ((viewHeight - displayHeight) / mDpi);
        }
      }

      LLog.d(LynxConstants.TAG,
          "KeyboardEvent visible = " + visible + ", height = " + keyboardHeight
              + ", compatHeight = " + keyboardHeightCompat);

      if ((keyboardHeight != keyboardHeightForLast)
          || (useRelativeKeyboardHeightApi && (keyboardHeightCompat != keyboardTopFromLynxView))) {
        sendKeyboardEvent(visible, keyboardHeight, keyboardHeightCompat);
        keyboardHeightForLast = keyboardHeight;
        keyboardTopFromLynxView = keyboardHeightCompat;
      }

      dispatchOnGlobalLayout();
    } catch (Exception e) {
      LLog.e(LynxConstants.TAG, e.getMessage());
    }
  }

  private void startInMain() {
    LLog.d(LynxConstants.TAG, "KeyboardEvent starting");
    Activity activity = ContextUtils.getActivity(mLynxContext);
    if (activity == null) {
      LLog.e(LynxConstants.TAG, "KeyboardEvent's context must be Activity");
      return;
    }
    if (mKeyboardMonitor == null) {
      mKeyboardMonitor = new KeyboardMonitor(activity);
    }

    mListener = new ViewTreeObserver.OnGlobalLayoutListener() {
      @Override
      public void onGlobalLayout() {
        LLog.d(LynxConstants.TAG, "onGlobalLayout invoked.");
        detectKeyboardChangeAndSendEvent();
      }
    };

    mKeyboardMonitor.addOnGlobalLayoutListener(mListener);
    mKeyboardMonitor.start();
    isStartedInUIThread = true;
  }

  public synchronized void stop() {
    if (!isStartedInUIThread) {
      return;
    }
    /*
     ** Need to post to UI thread to avoid multi-thread issue
     * since Template Renderer may run in async mode and setSoftInputMode
     * should be run in main thread
     */
    if (!UIThreadUtils.isOnUiThread()) {
      UIThreadUtils.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          stopInMain();
        }
      });
    } else {
      stopInMain();
    }
  }

  private void stopInMain() {
    LLog.d(LynxConstants.TAG, "KeyboardEvent stopping");
    try {
      if (mListener != null && mKeyboardMonitor != null) {
        mKeyboardMonitor.removeOnGlobalLayoutListener(mListener);
        mKeyboardMonitor.stop();
      }
    } catch (Exception e) {
      LLog.w(LynxConstants.TAG, "stop KeyboardEvent failed for " + e.toString());
    }
    isStartedInUIThread = false;
  }

  private void sendKeyboardEvent(boolean isVisible, int height, int heightCompat) {
    // for keeping backward compatible, we send 2 kinds of height here.
    // height is the older implementation, base view height below the status bar
    // heightCompat is the newer one, base view height below the status bar and above the virtual
    // buttons
    if (mLynxContext.getEventEmitter() != null) {
      JavaOnlyArray args = new JavaOnlyArray();
      args.pushString(isVisible ? "on" : "off");
      args.pushInt(heightCompat);
      args.pushInt(heightCompat);
      mLynxContext.sendGlobalEvent(LynxKeyboardEvent.KEYBOARD_STATUS_CHANGED, args);
    }
  }

  public Rect getDisplayFrame() {
    return mDisplayFrame;
  }

  public int getEventViewHeight() {
    return mViewHeight;
  }

  private void dispatchOnGlobalLayout() {
    for (Map.Entry<Object, ViewTreeObserver.OnGlobalLayoutListener> entry :
        mOnGlobalLayoutListenerList.entrySet()) {
      Object key = entry.getKey();
      if (key != null) {
        ViewTreeObserver.OnGlobalLayoutListener listener = entry.getValue();
        listener.onGlobalLayout();
      }
    }
  }
}
