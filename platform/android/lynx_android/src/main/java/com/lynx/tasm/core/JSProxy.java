// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.core;

import androidx.annotation.GuardedBy;
import androidx.annotation.NonNull;
import com.lynx.jsbridge.JSModule;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.LynxBackgroundRuntime;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.behavior.LynxContext;
import java.lang.Runnable;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.concurrent.locks.ReadWriteLock;
import java.util.concurrent.locks.ReentrantReadWriteLock;

public class JSProxy {
  // parameter maximum accumulation amount
  private static final int MAX_ARGS_COUNT = 200;
  // ensure thread safe
  @GuardedBy("mLock") private long mNativePtr;
  // set from native in nativeCreate
  private long mRuntimeId;
  private final WeakReference<LynxContext> mContext;
  private final String mJSGroupThreadName;
  private final ReadWriteLock mLock = new ReentrantReadWriteLock();

  @GuardedBy("mLock") private long mArgsId = 0;
  @GuardedBy("mLock") private HashMap<Long, Object> mArgsMap = new HashMap<Long, Object>();
  private boolean hasReport = false;

  public JSProxy(long nativeCreator, WeakReference<LynxContext> context, String jsGroupThreadName) {
    mContext = context;
    mJSGroupThreadName = jsGroupThreadName;
    mNativePtr = nativeCreate(nativeCreator, jsGroupThreadName);
  }

  public JSProxy(LynxBackgroundRuntime runtime, String jsGroupThreadName) {
    mContext = new WeakReference<LynxContext>(null);
    mJSGroupThreadName = jsGroupThreadName;
    mNativePtr = nativeCreateWithRuntimeActor(runtime.getNativePtr(), jsGroupThreadName);
  }

  public JSModule getJSModule(String module) {
    return new JSModule(module, this);
  }

  public long getRuntimeId() {
    return mRuntimeId;
  }

  public void destroy() {
    mLock.writeLock().lock();
    if (mNativePtr != 0) {
      nativeDestroy(mNativePtr, mJSGroupThreadName);
      mNativePtr = 0;
    }
    mArgsMap.clear();
    mLock.writeLock().unlock();
  }

  public void callFunction(String module, String method, JavaOnlyArray arguments) {
    mLock.writeLock().lock();
    if (mNativePtr != 0) {
      nativeCallJSFunction(mNativePtr, module, method, putArgs(arguments));
    }
    final int currentSize = mArgsMap.size();
    mLock.writeLock().unlock();
    checkArgsCount(currentSize, "callFunction:" + module + "." + method);
  }

  public void callIntersectionObserver(int observerId, int callbackId, JavaOnlyMap args) {
    mLock.writeLock().lock();
    if (mNativePtr != 0) {
      nativeCallIntersectionObserver(mNativePtr, observerId, callbackId, putArgs(args));
    }
    final int currentSize = mArgsMap.size();
    mLock.writeLock().unlock();
    checkArgsCount(currentSize, "callIntersectionObserver");
  }

  public void callJSApiCallbackWithValue(int callbackId, JavaOnlyMap args) {
    mLock.writeLock().lock();
    if (mNativePtr != 0) {
      nativeCallJSApiCallbackWithValue(mNativePtr, callbackId, putArgs(args));
    }
    final int currentSize = mArgsMap.size();
    mLock.writeLock().unlock();
    checkArgsCount(currentSize, "callJSApiCallbackWithValue");
  }

  public void evaluateScript(String url, byte[] data, int callbackId) {
    mLock.readLock().lock();
    if (mNativePtr != 0) {
      nativeEvaluateScript(mNativePtr, url, data, callbackId);
    }
    mLock.readLock().unlock();
  }

  public void rejectDynamicComponentLoad(String url, int callbackId, int errCode, String errMsg) {
    mLock.readLock().lock();
    if (mNativePtr != 0) {
      nativeRejectDynamicComponentLoad(mNativePtr, url, callbackId, errCode, errMsg);
    }
    mLock.readLock().unlock();
  }

  public void runOnJSThread(@NonNull Runnable runnable) {
    mLock.readLock().lock();
    if (mNativePtr != 0) {
      nativeRunOnJSThread(mNativePtr, runnable);
    }
    mLock.readLock().unlock();
  }

  @GuardedBy("mLock")
  private long putArgs(Object args) {
    mArgsMap.put(++mArgsId, args);
    return mArgsId;
  }

  private void checkArgsCount(final int currentSize, String functionName) {
    if (!hasReport && currentSize > MAX_ARGS_COUNT && null != mContext) {
      hasReport = true;
      LynxContext context = mContext.get();
      if (null != context) {
        LynxError error =
            new LynxError(LynxSubErrorCode.E_BTS_PLATFORM_CALL_JS_FUNCTION_TOO_FREQUENCY,
                "Calling [" + functionName + "] too frequently.This may cause OOM issues.");
        context.handleLynxError(error);
      }
    }
  }

  // Only Called by js thread.
  @CalledByNative
  private Object getArgs(long id) {
    mLock.writeLock().lock();
    try {
      return mArgsMap.remove(id);
    } finally {
      mLock.writeLock().unlock();
    }
  }

  private native long nativeCreate(long nativeCreator, String jsGroupThreadName);

  private native long nativeCreateWithRuntimeActor(long nativeCreator, String jsGroupThreadName);

  private native void nativeDestroy(long nativePtr, String jsGroupThreadName);

  private native void nativeCallJSFunction(
      long nativePtr, String module, String method, long argsId);

  private native void nativeCallIntersectionObserver(
      long nativePtr, int observerID, int callbackID, long argsId);

  private native void nativeCallJSApiCallbackWithValue(long nativePtr, int callbackID, long argsId);

  private static native void nativeEvaluateScript(
      long nativePtr, String url, byte[] data, int callbackId);

  private static native void nativeRejectDynamicComponentLoad(
      long nativePtr, String url, int callbackId, int errCode, String errMsg);

  private static native void nativeRunOnJSThread(long nativePtr, Runnable runnable);
  @CalledByNative
  private void setRuntimeId(long runtimeId) {
    mRuntimeId = runtimeId;
  }
}
