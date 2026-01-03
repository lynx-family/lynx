// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtoolwrapper;

import androidx.annotation.RestrictTo;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;

/**
 * Manages the state transitions for the DevTool.
 * Ensures valid transitions between states like UNAVAILABLE, ATTACHED, INITIALIZED, etc.
 */
@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
public class DevToolLifecycle {
  private static final String TAG = "DevToolLifecycle";
  private volatile DevToolState mState = DevToolState.UNAVAILABLE;

  private static volatile DevToolLifecycle sInstance;

  private DevToolLifecycle() {}

  public static DevToolLifecycle getInstance() {
    if (sInstance == null) {
      synchronized (DevToolLifecycle.class) {
        if (sInstance == null) {
          sInstance = new DevToolLifecycle();
        }
      }
    }
    return sInstance;
  }

  public DevToolState getState() {
    return mState;
  }

  public boolean isAttached() {
    return mState.ordinal() >= DevToolState.ATTACHED.ordinal();
  }

  public boolean isInitialized() {
    return mState.ordinal() >= DevToolState.INITIALIZED.ordinal();
  }

  public boolean isEnabled() {
    return mState.ordinal() >= DevToolState.ENABLED.ordinal();
  }

  public boolean isConnected() {
    return mState.ordinal() >= DevToolState.CONNECTED.ordinal();
  }

  /**
   * Called when the DevTool component is detected (e.g., via reflection).
   * Transition: UNAVAILABLE -> ATTACHED
   */
  public synchronized void onAttached() {
    if (mState == DevToolState.UNAVAILABLE) {
      LLog.i(TAG, "DevTool attached. Transitioning to ATTACHED.");
      mState = DevToolState.ATTACHED;
      syncStateToNative();
    } else {
      LLog.w(TAG, "onAttached() called but state is " + mState);
    }
  }

  /**
   * Called when DevTool is enabled (by policy or user switch).
   * Transition: ATTACHED -> ENABLED
   */
  public synchronized void onEnabled() {
    if (mState == DevToolState.ATTACHED) {
      LLog.i(TAG, "DevTool enabled. Transitioning to ENABLED.");
      mState = DevToolState.ENABLED;
      syncStateToNative();
    } else if (mState == DevToolState.UNAVAILABLE) {
      LLog.w(TAG, "Cannot enable DevTool because it is " + mState);
    } else {
      // Already enabled or further along
      LLog.d(TAG, "onEnabled() called but state is " + mState);
    }
  }

  /**
   * Called when DevTool is disabled (by policy or user switch).
   * Transition: ENABLED/INITIALIZED/CONNECTED -> ATTACHED
   */
  public synchronized void onDisabled() {
    if (mState == DevToolState.ENABLED || mState == DevToolState.INITIALIZED
        || mState == DevToolState.CONNECTED) {
      LLog.i(TAG, "DevTool disabled. Transitioning to ATTACHED.");
      mState = DevToolState.ATTACHED;
      syncStateToNative();
    }
  }

  /**
   * Called when the DevTool env is initialized, i.e. native library is loaded.
   * Transition: ENABLED -> INITIALIZED
   */
  public synchronized void onInitialized() {
    if (mState == DevToolState.ENABLED) {
      LLog.i(TAG, "DevTool env initialized. Transitioning to INITIALIZED.");
      mState = DevToolState.INITIALIZED;
      syncStateToNative();
    }
  }

  /**
   * Called when a DevTool client connects.
   * Transition: INITIALIZED -> CONNECTED
   */
  public synchronized void onConnected() {
    if (mState == DevToolState.INITIALIZED) {
      LLog.i(TAG, "DevTool client connected. Transitioning to CONNECTED.");
      mState = DevToolState.CONNECTED;
      syncStateToNative();
    }
  }

  /**
   * Called when a DevTool client disconnects.
   * Transition: CONNECTED -> INITIALIZED
   */
  public synchronized void onDisconnected() {
    if (mState == DevToolState.CONNECTED) {
      LLog.i(TAG, "DevTool client disconnected. Transitioning to INITIALIZED.");
      mState = DevToolState.INITIALIZED;
      syncStateToNative();
    }
  }

  private synchronized void syncStateToNative() {
    // This method should be called each time the state changes.
    try {
      nativeSyncStateToNative(mState.ordinal());
    } catch (UnsatisfiedLinkError e) {
      LLog.w(TAG,
          "UnsatisfiedLinkError! "
              + "Trying to sync state [" + mState + "] before native library is loaded.");
    }
  }

  private native void nativeSyncStateToNative(int value);
  @CalledByNative
  private synchronized static void syncStateFromNative(int value) {
    DevToolLifecycle instance = getInstance();
    DevToolState state = instance.mState;
    try {
      // Warning: this line is fragile. It works only when definitions are identical.
      state = DevToolState.values()[value];
    } catch (ArrayIndexOutOfBoundsException e) {
      LLog.e(TAG,
          "Invalid value during state sync. "
              + "The definition of states in Android platform and native are not identical");
      return;
    }

    if (instance.mState != state) {
      LLog.i(TAG, "SyncStateFromNative: Transitioning from " + instance.mState + " to " + state);
      instance.mState = state;
      // Do NOT sync to native again to avoid circular call.
    }
  }

  /**
   * Stay default (package-private) accessibility for testing purpose.
   */
  void reset() {
    mState = DevToolState.UNAVAILABLE;
    // Since this method is for testing only, we don't call syncStateToNative();
  }
}
