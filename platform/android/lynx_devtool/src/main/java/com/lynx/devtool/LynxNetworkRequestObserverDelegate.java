// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool;

import com.lynx.devtoolwrapper.LynxNetworkRequestObserver;
import com.lynx.react.bridge.JavaOnlyMap;

final class LynxNetworkRequestObserverDelegate implements LynxNetworkRequestObserver {
  // This is a borrowed LynxDevToolNG handle. The owner and this delegate share
  // mDevToolLock so the handle cannot be deleted while a Network JNI call is
  // in progress.
  private long mLynxDevToolNGPtr;
  private final Object mDevToolLock;

  LynxNetworkRequestObserverDelegate(long lynxDevToolNGPtr, Object devToolLock) {
    mLynxDevToolNGPtr = lynxDevToolNGPtr;
    mDevToolLock = devToolLock;
  }

  void invalidate() {
    synchronized (mDevToolLock) {
      mLynxDevToolNGPtr = 0;
    }
  }

  @Override
  public boolean isEnabled() {
    synchronized (mDevToolLock) {
      return mLynxDevToolNGPtr != 0 && nativeIsEnabled(mLynxDevToolNGPtr);
    }
  }

  private native boolean nativeIsEnabled(long lynxDevToolNGPtr);

  @Override
  public String requestWillBeSent(String url, String method, JavaOnlyMap headers, byte[] body) {
    synchronized (mDevToolLock) {
      return mLynxDevToolNGPtr != 0
          ? nativeRequestWillBeSent(mLynxDevToolNGPtr, url, method, headers, body)
          : "";
    }
  }

  private native String nativeRequestWillBeSent(
      long lynxDevToolNGPtr, String url, String method, JavaOnlyMap headers, byte[] body);

  @Override
  public void responseReceived(
      String requestId, String url, int status, String statusText, JavaOnlyMap headers) {
    synchronized (mDevToolLock) {
      if (mLynxDevToolNGPtr != 0) {
        nativeResponseReceived(mLynxDevToolNGPtr, requestId, url, status, statusText, headers);
      }
    }
  }

  private native void nativeResponseReceived(long lynxDevToolNGPtr, String requestId, String url,
      int status, String statusText, JavaOnlyMap headers);

  @Override
  public void dataReceived(String requestId, byte[] data) {
    synchronized (mDevToolLock) {
      if (mLynxDevToolNGPtr != 0) {
        nativeDataReceived(mLynxDevToolNGPtr, requestId, data);
      }
    }
  }

  private native void nativeDataReceived(long lynxDevToolNGPtr, String requestId, byte[] data);

  @Override
  public void loadingFinished(String requestId) {
    synchronized (mDevToolLock) {
      if (mLynxDevToolNGPtr != 0) {
        nativeLoadingFinished(mLynxDevToolNGPtr, requestId);
      }
    }
  }

  private native void nativeLoadingFinished(long lynxDevToolNGPtr, String requestId);

  @Override
  public void loadingFailed(String requestId, String errorText, boolean canceled) {
    synchronized (mDevToolLock) {
      if (mLynxDevToolNGPtr != 0) {
        nativeLoadingFailed(mLynxDevToolNGPtr, requestId, errorText, canceled);
      }
    }
  }

  private native void nativeLoadingFailed(
      long lynxDevToolNGPtr, String requestId, String errorText, boolean canceled);
}
