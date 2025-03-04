// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.accessibility;

import android.os.Build;
import android.view.accessibility.AccessibilityManager;
import androidx.annotation.RequiresApi;
import com.lynx.tasm.base.LLog;
import java.lang.ref.WeakReference;

public class LynxAccessibilityStateHelper {
  private static final String TAG = "LynxAccessibilityStateHelper";

  /** Listener for the system accessibility state and touch exploration state. */
  public interface OnStateListener {
    /**
     * Called back on change in the accessibility state.
     *
     * @param enable Whether accessibility is enabled.
     */
    void onAccessibilityEnable(final boolean enable);

    /**
     * Called when the touch exploration enabled state changes.
     *
     * @param enable Whether touch exploration is enabled.
     */
    void onTouchExplorationEnable(final boolean enable);
  }

  /** System accessibility manager */
  private AccessibilityManager mAccessibilityManager = null;

  private LynxAccessibilityStateChangeListener mLynxAccessibilityStateChangeListener = null;

  @RequiresApi(api = Build.VERSION_CODES.KITKAT)
  private LynxTouchExplorationStateChangeListener mLynxTouchExplorationStateChangeListener = null;

  public LynxAccessibilityStateHelper(
      AccessibilityManager accessibilityManager, OnStateListener onStateListener) {
    if (accessibilityManager != null && onStateListener != null) {
      mAccessibilityManager = accessibilityManager;
      final boolean isEnabled = mAccessibilityManager.isEnabled();
      final boolean isTouchExplorationEnabled = mAccessibilityManager.isTouchExplorationEnabled();
      // Note: here we should invoke callbacks of OnStateListener, because
      // onAccessibilityStateChanged() and onTouchExplorationStateChanged() will not be invoked when
      // registering delegate.
      onStateListener.onAccessibilityEnable(isEnabled);
      onStateListener.onTouchExplorationEnable(isTouchExplorationEnabled);
      LLog.i(TAG,
          "Construct LynxAccessibilityStateHelper with mAccessibilityEnable = " + isEnabled
              + ", mTouchExplorationEnable = " + isTouchExplorationEnabled);
      mLynxAccessibilityStateChangeListener =
          new LynxAccessibilityStateChangeListener(onStateListener);
      mAccessibilityManager.addAccessibilityStateChangeListener(
          mLynxAccessibilityStateChangeListener);
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
        // TouchExplorationStateChangeListener is added in android 4.4
        mLynxTouchExplorationStateChangeListener =
            new LynxTouchExplorationStateChangeListener(onStateListener);
        mAccessibilityManager.addTouchExplorationStateChangeListener(
            mLynxTouchExplorationStateChangeListener);
      }
    }
  }

  public void removeAllListeners() {
    if (mAccessibilityManager != null) {
      if (mLynxAccessibilityStateChangeListener != null) {
        mAccessibilityManager.removeAccessibilityStateChangeListener(
            mLynxAccessibilityStateChangeListener);
      }
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT
          && mLynxTouchExplorationStateChangeListener != null) {
        // TouchExplorationStateChangeListener is added in android 4.4
        mAccessibilityManager.removeTouchExplorationStateChangeListener(
            mLynxTouchExplorationStateChangeListener);
      }
    }
  }

  /**
   * Listener for the accessibility state.
   */
  private static class LynxAccessibilityStateChangeListener
      implements AccessibilityManager.AccessibilityStateChangeListener {
    private WeakReference<OnStateListener> mStateListenerRef;

    public LynxAccessibilityStateChangeListener(OnStateListener stateListener) {
      mStateListenerRef = new WeakReference<>(stateListener);
    }

    @Override
    public void onAccessibilityStateChanged(boolean enabled) {
      LLog.i(TAG, "onAccessibilityStateChanged: " + enabled);
      if (mStateListenerRef != null && mStateListenerRef.get() != null) {
        mStateListenerRef.get().onAccessibilityEnable(enabled);
      }
    }
  }

  /**
   * Listener for the system touch exploration state. Note: listener is added in Android 4.4
   */
  @RequiresApi(api = Build.VERSION_CODES.KITKAT)
  private static class LynxTouchExplorationStateChangeListener
      implements AccessibilityManager.TouchExplorationStateChangeListener {
    private WeakReference<OnStateListener> mStateListenerRef;

    public LynxTouchExplorationStateChangeListener(OnStateListener stateListener) {
      mStateListenerRef = new WeakReference<>(stateListener);
    }

    @Override
    public void onTouchExplorationStateChanged(boolean enabled) {
      LLog.i(TAG, "onTouchExplorationStateChanged: " + enabled);
      if (mStateListenerRef != null && mStateListenerRef.get() != null) {
        mStateListenerRef.get().onTouchExplorationEnable(enabled);
      }
    }
  }
}
