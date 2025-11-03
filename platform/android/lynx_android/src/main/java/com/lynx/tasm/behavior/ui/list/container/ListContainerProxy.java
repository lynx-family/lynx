// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.list.container;

public class ListContainerProxy {
  private UIListContainer mUIList;
  private long mNativeEnginePtr;
  private long mNativeContainerPtr;

  private ListContainerProxy() {}

  public ListContainerProxy(UIListContainer uiList, long nativeEnginePtr) {
    mUIList = uiList;
    mNativeEnginePtr = nativeEnginePtr;
    mNativeContainerPtr = nativeCreate(mNativeEnginePtr);
  }

  public void scrollByListContainer(int sign, float x, float y, float originalX, float originalY) {
    if (mNativeContainerPtr != 0) {
      nativeScrollByListContainer(mNativeContainerPtr, sign, x, y, originalX, originalY);
    }
  }

  public void scrollToPosition(int sign, int position, float offset, int align, boolean smooth) {
    if (mNativeContainerPtr != 0) {
      nativeScrollToPosition(mNativeContainerPtr, sign, position, offset, align, smooth);
    }
  }

  public void scrollStopped(int sign) {
    if (mNativeContainerPtr != 0) {
      nativeScrollStopped(mNativeContainerPtr, sign);
    }
  }

  public void destroy() {
    if (mNativeContainerPtr != 0) {
      nativeDestroy(mNativeContainerPtr);
      mNativeContainerPtr = 0;
    }
  }

  // ------------------JNI Method start -----------------------

  // create 'list_container_proxy' C++ object
  private native long nativeCreate(long enginePtr);

  private native void nativeScrollByListContainer(
      long ptr, int sign, float x, float y, float originalX, float originalY);

  private native void nativeScrollToPosition(
      long ptr, int sign, int position, float offset, int align, boolean smooth);

  private native void nativeScrollStopped(long ptr, int sign);

  // release 'list_container_proxy' C++ object
  private native void nativeDestroy(long containerPtr);

  // ------------------JNI Method end -----------------------
}
