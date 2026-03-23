// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtoolwrapper;

import android.text.TextUtils;
import com.lynx.devtoolwrapper.LynxDevtool;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import java.util.ArrayList;
import java.util.List;

public class LynxDevToolPool {
  private static final String TAG = "LynxDevToolPool";
  private List<LynxDevtool> mDevtools;
  private boolean mDebuggable = false;
  private String mUrl;
  private long mNativePtr = 0;
  private static int nextContextId = 0;

  public LynxDevToolPool(String url, boolean debuggable) {
    LLog.i(TAG, "create LynxDevToolPool, url: " + url + ", debuggable: " + debuggable);
    mDevtools = new ArrayList<>();
    mDebuggable = debuggable;
    mUrl = url;
    if (TextUtils.isEmpty(mUrl)) {
      mUrl = "anonymous bundle";
    }
    mNativePtr = nativeCreateDevToolPool();
  }

  public long getNativePtr() {
    return mNativePtr;
  }

  public void destroy() {
    if (!mDevtools.isEmpty()) {
      mDevtools.forEach(LynxDevtool::destroy);
      mDevtools.clear();
    }
    if (mNativePtr != 0) {
      nativeDestroyDevToolPool(mNativePtr);
      mNativePtr = 0;
    }
  }

  @CalledByNative
  public void createDevTool() {
    if (LynxEnv.inst().isLynxDebugEnabled()) {
      mDevtools.add(new LynxDevtool(null, mDebuggable));
    }
  }

  @CalledByNative
  public void popDevTool() {
    if (mDevtools.isEmpty()) {
      return;
    }
    LynxDevtool devtool = mDevtools.get(mDevtools.size() - 1);
    devtool.destroy();
    mDevtools.remove(mDevtools.size() - 1);
  }

  @CalledByNative
  public void onMTSRuntimeCreated(long poolRawPtr) {
    if (mDevtools.isEmpty()) {
      return;
    }
    LynxDevtool devtool = mDevtools.get(mDevtools.size() - 1);
    devtool.onMTSRuntimeCreated(poolRawPtr);
    String url = String.format("%s - context%d", mUrl, nextContextId++);
    devtool.attachToDebugBridge(url);
  }

  private native long nativeCreateDevToolPool();
  private native void nativeDestroyDevToolPool(long poolPtr);
}
