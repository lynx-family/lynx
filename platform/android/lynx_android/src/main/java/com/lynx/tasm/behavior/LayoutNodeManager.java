// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

public class LayoutNodeManager {
  public static float UNDEFINED_MAX_SIZE = (float) 0x7FFFFFF;
  public static float UNDEFINED_MIN_SIZE = 0f;
  private long mNativePtr = 0;

  public void attachNativePtr(long mNativePtr) {
    this.mNativePtr = mNativePtr;
  }

  public void setMeasureFunc(int id, Object shadowNode) {
    nativeSetMeasureFunc(mNativePtr, id, shadowNode);
  }

  public void markDirty(int id) {
    nativeMarkDirty(mNativePtr, id);
  }
  public boolean isDirty(int id) {
    return nativeIsDirty(mNativePtr, id);
  }

  // inline view methods
  public long measureNativeNode(
      int id, float width, int widthMode, float height, int heightMode, boolean finalMeasure) {
    return nativeMeasureNativeNode(
        mNativePtr, id, width, widthMode, height, heightMode, finalMeasure);
  }

  public int[] measureNativeNodeReturnWithBaseline(
      int id, float width, int widthMode, float height, int heightMode, boolean finalMeasure) {
    return nativeMeasureNativeNodeReturnWithBaseline(
        mNativePtr, id, width, widthMode, height, heightMode, finalMeasure);
  }

  public void alignNativeNode(int id, float offset_top, float offset_left) {
    nativeAlignNativeNode(mNativePtr, id, offset_top, offset_left);
  }

  // Style relevant methods
  public int getFlexDirection(int id) {
    return nativeGetFlexDirection(mNativePtr, id);
  }
  public float getWidth(int id) {
    return nativeGetWidth(mNativePtr, id);
  }

  public float getHeight(int id) {
    return nativeGetHeight(mNativePtr, id);
  }

  public float getMinWidth(int id) {
    return nativeGetMinWidth(mNativePtr, id);
  }

  public float getMaxWidth(int id) {
    return nativeGetMaxWidth(mNativePtr, id);
  }

  public float getMinHeight(int id) {
    return nativeGetMinHeight(mNativePtr, id);
  }

  // if max-height is not set, return DEFAULT_MAX_HEIGHT
  public float getMaxHeight(int id) {
    return nativeGetMinHeight(mNativePtr, id);
  }

  public int[] getPadding(int id) {
    return nativeGetPadding(mNativePtr, id);
  }

  public int[] getMargin(int id) {
    return nativeGetMargin(mNativePtr, id);
  }

  private native void nativeSetMeasureFunc(long nativePtr, int id, Object shadowNode);
  private native void nativeMarkDirty(long nativePtr, int id);
  private native boolean nativeIsDirty(long nativePtr, int id);

  // inline view methods
  private native long nativeMeasureNativeNode(long nativePtr, int id, float width, int widthMode,
      float height, int heightMode, boolean finalMeasure);
  private native int[] nativeMeasureNativeNodeReturnWithBaseline(long nativePtr, int id,
      float width, int widthMode, float height, int heightMode, boolean finalMeasure);
  private native void nativeAlignNativeNode(
      long nativePtr, int id, float offset_top, float offset_left);

  // Style relevant methods
  private native int nativeGetFlexDirection(long nativePtr, int id);
  private native float nativeGetWidth(long nativePtr, int id);
  private native float nativeGetHeight(long nativePtr, int id);

  private native float nativeGetMinWidth(long nativePtr, int id);
  private native float nativeGetMaxWidth(long nativePtr, int id);
  private native float nativeGetMinHeight(long nativePtr, int id);

  private native float nativeGetMaxHeight(long nativePtr, int id);
  private native int[] nativeGetPadding(long nativePtr, int id);
  private native int[] nativeGetMargin(long nativePtr, int id);
}
