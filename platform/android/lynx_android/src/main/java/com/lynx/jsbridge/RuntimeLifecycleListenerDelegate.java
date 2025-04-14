// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.utils.CallStackUtil;
import java.lang.ref.WeakReference;
import java.lang.reflect.Constructor;

@RestrictTo(RestrictTo.Scope.LIBRARY)
@Keep
public class RuntimeLifecycleListenerDelegate implements RuntimeFullLifecycleListener {
  private static final String TAG = "RuntimeListenerDelegate";
  private static final int PART_TYPE = 0;
  private static final int FULL_TYPE = 1;

  private final WeakReference<LynxContext> mLynxContextWeak;
  private final int mType;
  private RuntimeLifecycleListener mPartListener;
  private RuntimeFullLifecycleListener mFullListener;

  public RuntimeLifecycleListenerDelegate(
      @NonNull WeakReference<LynxContext> lynxContext, @NonNull RuntimeLifecycleListener listener) {
    this.mLynxContextWeak = lynxContext;
    if (listener instanceof RuntimeFullLifecycleListener) {
      this.mFullListener = (RuntimeFullLifecycleListener) listener;
      mType = FULL_TYPE;
    } else {
      this.mPartListener = listener;
      mType = PART_TYPE;
    }
  }

  @CalledByNative
  public void onRuntimeCreate(long vsyncMonitorPtr) {
    if (mFullListener != null) {
      try {
        mFullListener.onRuntimeCreate(vsyncMonitorPtr);
      } catch (Exception e) {
        onError(e);
      }
    }
  }

  @CalledByNative
  public void onRuntimeAttach(long napiEnv) {
    RuntimeLifecycleListener listener = mType == PART_TYPE ? mPartListener : mFullListener;
    if (listener != null) {
      try {
        listener.onRuntimeAttach(napiEnv);
      } catch (Exception e) {
        onError(e);
      }
    }
  }

  @CalledByNative
  public void onRuntimeDetach() {
    RuntimeLifecycleListener listener = mType == PART_TYPE ? mPartListener : mFullListener;
    if (listener != null) {
      try {
        listener.onRuntimeDetach();
      } catch (Exception e) {
        onError(e);
      }
    }
  }

  @CalledByNative
  public void onRuntimeInit(long runtimeId) {
    if (mFullListener != null) {
      try {
        mFullListener.onRuntimeInit(runtimeId);
      } catch (Exception e) {
        onError(e);
      }
    }
  }

  @CalledByNative
  public void onAppEnterBackground() {
    if (mFullListener != null) {
      try {
        mFullListener.onAppEnterBackground();
      } catch (Exception e) {
        onError(e);
      }
    }
  }

  @CalledByNative
  public void onAppEnterForeground() {
    if (mFullListener != null) {
      try {
        mFullListener.onAppEnterForeground();
      } catch (Exception e) {
        onError(e);
      }
    }
  }

  public int getListenerType() {
    return mType;
  }

  private void onError(Exception e) {
    LLog.e(TAG, e.toString());
    LynxContext lynxContext = mLynxContextWeak.get();
    if (lynxContext != null) {
      String stack = CallStackUtil.getStackTraceStringTrimmed(e);
      LynxError error =
          new LynxError(LynxSubErrorCode.E_BTS_LIFECYCLE_LISTENER_ERROR_EXCEPTION, e.getMessage());
      error.setCallStack(stack);
      lynxContext.handleLynxError(error);
    }
  }
}
