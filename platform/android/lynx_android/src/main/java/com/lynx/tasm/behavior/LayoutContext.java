// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import android.util.DisplayMetrics;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.mapbuffer.ReadableMapBuffer;
import com.lynx.tasm.base.CalledByNative;

public abstract class LayoutContext {
  private long mNativePtr = 0;
  private boolean mDestroyed;
  protected long mNativeLayoutContextPtr = 0;

  /**
   * @return true when the ShadowNode is virtual, otherwise false
   */
  @CalledByNative
  public abstract int createNode(int signature, String tagName, ReadableMap props,
      ReadableMapBuffer initialStyles, ReadableArray eventListeners, boolean allowInline);

  @CalledByNative
  public abstract void removeNode(int parentSignature, int childSignature, int index);

  @CalledByNative
  public abstract void insertNode(int parentSignature, int childSignature, int index);

  @CalledByNative
  public abstract void moveNode(
      int parentSignature, int childSignature, int fromIndex, int toIndex);

  @CalledByNative public abstract void destroyNodes(int[] signatures);

  @CalledByNative public abstract void dispatchOnLayoutBefore(int rootSignature);

  @CalledByNative
  public abstract void dispatchOnLayout(int sign, int left, int top, int width, int height);

  @CalledByNative
  public abstract void updateProps(
      int signature, ReadableMap props, ReadableMapBuffer styles, ReadableArray eventListeners);

  @CalledByNative public abstract void setFontFaces(ReadableMap props);

  @CalledByNative public abstract Object getExtraBundle(int signature);

  @CalledByNative
  protected void attachNativePtr(long ptr) {
    mNativePtr = ptr;
  }

  @CalledByNative public abstract void attachLayoutNodeManager(long nativeLayoutNodeManagerPtr);

  @CalledByNative
  protected void detachNativePtr() {
    mNativePtr = 0;
  }

  @CalledByNative protected abstract void scheduleLayout();

  public abstract DisplayMetrics getScreenMetrics();

  public void triggerLayout() {
    if (mNativePtr != 0) {
      nativeTriggerLayout(mNativePtr);
    }
  }

  protected void createNativeLayoutContext(Object layoutContext) {
    mNativeLayoutContextPtr = nativeCreateLayoutContext(layoutContext);
  }

  public long getNativeLayoutContextPtr() {
    return mNativeLayoutContextPtr;
  }

  private native void nativeTriggerLayout(long ptr);
  private native long nativeCreateLayoutContext(Object layoutContext);

  public void destroy() {
    mDestroyed = true;
  }

  protected boolean isDestroyed() {
    return mDestroyed;
  }
}
