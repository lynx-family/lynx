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
import com.lynx.tasm.base.CleanupReference;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.utils.CallStackUtil;
import java.lang.ref.WeakReference;
import java.lang.reflect.Constructor;

@RestrictTo(RestrictTo.Scope.LIBRARY)
@Keep
public class RuntimeLifecycleListenerDelegate implements RuntimeLifecycleListener {
  private static final String TAG = "RuntimeListenerDelegate";

  private final WeakReference<LynxContext> mLynxContextWeak;
  private RuntimeLifecycleListener mListener;

  public RuntimeLifecycleListenerDelegate(
      @NonNull WeakReference<LynxContext> lynxContext, @NonNull RuntimeLifecycleListener listener) {
    this.mLynxContextWeak = lynxContext;
    this.mListener = listener;
  }

  public interface Factory {
    RuntimeLifecycleListenerDelegate create();
  }

  // Owns a native extension delegate, which gives each runtime a separate
  // listener from the factory, observing the page's own Env. Return
  // getNativePtr() from LynxExtensionModule.getExtensionDelegatePtr() and call
  // destroy() with the module. The native delegate is released on the UI
  // thread after destroy() or when the handle is collected.
  public static final class ExtensionDelegate {
    private final CleanupReference mCleanup;
    private long mNativePtr;

    private ExtensionDelegate(Factory factory) {
      final long ptr = nativeCreateExtensionDelegate(factory);
      mNativePtr = ptr;
      // Capture only the pointer so that the handle can be collected.
      mCleanup = new CleanupReference(this, () -> nativeDestroyExtensionDelegate(ptr), true);
    }

    // Returns 0 after destroy().
    public synchronized long getNativePtr() {
      return mNativePtr;
    }

    public synchronized void destroy() {
      mNativePtr = 0;
      mCleanup.cleanupNow();
    }
  }

  public static ExtensionDelegate createExtensionDelegate(Factory factory) {
    return new ExtensionDelegate(factory);
  }

  private static native long nativeCreateExtensionDelegate(Object factory);

  private static native void nativeDestroyExtensionDelegate(long delegate);

  @CalledByNative
  private static RuntimeLifecycleListenerDelegate createForExtension(Object factory) {
    return ((Factory) factory).create();
  }

  @CalledByNative
  public void onRuntimeAttach(long napiEnv, String runtimeType) {
    if (mListener != null) {
      try {
        mListener.onRuntimeAttach(napiEnv, runtimeType);
      } catch (Exception e) {
        onError(e);
      }
    }
  }

  @CalledByNative
  public void onRuntimeDetach() {
    if (mListener != null) {
      try {
        mListener.onRuntimeDetach();
      } catch (Exception e) {
        onError(e);
      }
    }
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
