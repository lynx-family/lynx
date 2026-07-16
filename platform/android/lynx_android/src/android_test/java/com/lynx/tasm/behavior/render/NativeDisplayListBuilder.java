// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import java.nio.ByteBuffer;

/**
 * Test-only JNI wrapper around the production C++ {@code DisplayListBuilder}.
 *
 * <p>Valid display-list buffers used by Android behavior tests must come from this builder so the
 * tests do not duplicate the native {@code DisplayListItem} ABI.
 */
final class NativeDisplayListBuilder implements AutoCloseable {
  private static boolean sRegistered;

  private long mNativePtr;
  private boolean mBuilt;

  static synchronized void ensureRegistered() {
    if (!sRegistered) {
      if (!registerJNI()) {
        throw new IllegalStateException("Failed to register NativeDisplayListBuilder JNI.");
      }
      sRegistered = true;
    }
  }

  NativeDisplayListBuilder() {
    if (!sRegistered) {
      throw new IllegalStateException("Call ensureRegistered() before creating a display list.");
    }
    mNativePtr = nativeCreate();
    if (mNativePtr == 0) {
      throw new IllegalStateException("Failed to create native DisplayListBuilder.");
    }
  }

  NativeDisplayListBuilder begin(int id, int type, float x, float y, float width, float height) {
    ensureMutable();
    nativeBegin(mNativePtr, id, type, x, y, width, height);
    return this;
  }

  NativeDisplayListBuilder end() {
    ensureMutable();
    nativeEnd(mNativePtr);
    return this;
  }

  NativeDisplayListBuilder fill(int color, int clipIndex) {
    ensureMutable();
    nativeFill(mNativePtr, color, clipIndex);
    return this;
  }

  NativeDisplayListBuilder drawView(int viewId, float offsetX, float offsetY) {
    ensureMutable();
    nativeDrawView(mNativePtr, viewId, offsetX, offsetY);
    return this;
  }

  NativeDisplayListBuilder text(int textId, int boxIndex) {
    ensureMutable();
    nativeText(mNativePtr, textId, boxIndex);
    return this;
  }

  NativeDisplayListBuilder image(int imageId, int boxIndex) {
    ensureMutable();
    nativeImage(mNativePtr, imageId, boxIndex);
    return this;
  }

  NativeDisplayListBuilder backgroundImage(
      int imageId, int tilingIndex, int clipIndex, int repeatX, int repeatY) {
    ensureMutable();
    nativeBackgroundImage(mNativePtr, imageId, tilingIndex, clipIndex, repeatX, repeatY);
    return this;
  }

  NativeDisplayListBuilder border(int outIndex, int innerIndex, int[] colors, int[] styles) {
    ensureMutable();
    if (colors.length != 4 || styles.length != 4) {
      throw new IllegalArgumentException("Border colors and styles must contain four values.");
    }
    nativeBorder(mNativePtr, outIndex, innerIndex, colors, styles);
    return this;
  }

  NativeDisplayListBuilder clipRect(float x, float y, float width, float height, float... radii) {
    ensureMutable();
    validateRadii(radii);
    nativeClipRect(mNativePtr, x, y, width, height, radii);
    return this;
  }

  NativeDisplayListBuilder recordBox(float x, float y, float width, float height, float... radii) {
    ensureMutable();
    validateRadii(radii);
    nativeRecordBox(mNativePtr, x, y, width, height, radii);
    return this;
  }

  NativeDisplayListBuilder linearGradient(int[] colors, float[] stops, int tilingIndex,
      int clipIndex, int repeatX, int repeatY, float angle) {
    ensureMutable();
    nativeLinearGradient(
        mNativePtr, colors, stops, tilingIndex, clipIndex, repeatX, repeatY, angle);
    return this;
  }

  NativeDisplayListBuilder boxShadow(
      int shadowBoxIndex, int clipBoxIndex, int color, float blurRadius, int clipMode) {
    ensureMutable();
    nativeBoxShadow(mNativePtr, shadowBoxIndex, clipBoxIndex, color, blurRadius, clipMode);
    return this;
  }

  ByteBuffer toItemsBuffer() {
    ensureBuilt();
    return PlatformRendererContext.makeReadOnlyDisplayListBuffer(nativeGetItemsBuffer(mNativePtr));
  }

  ByteBuffer toDataBuffer() {
    ensureBuilt();
    return PlatformRendererContext.makeReadOnlyDisplayListBuffer(nativeGetDataBuffer(mNativePtr));
  }

  @Override
  public void close() {
    if (mNativePtr != 0) {
      nativeDestroy(mNativePtr);
      mNativePtr = 0;
    }
  }

  private void ensureMutable() {
    ensureOpen();
    if (mBuilt) {
      throw new IllegalStateException("The native display list has already been built.");
    }
  }

  private void ensureBuilt() {
    ensureOpen();
    if (!mBuilt) {
      nativeBuild(mNativePtr);
      mBuilt = true;
    }
  }

  private void ensureOpen() {
    if (mNativePtr == 0) {
      throw new IllegalStateException("The native display list has been closed.");
    }
  }

  private static void validateRadii(float[] radii) {
    if (radii.length != 0 && radii.length != 8) {
      throw new IllegalArgumentException("Rounded rectangles must contain zero or eight radii.");
    }
  }

  private static native boolean registerJNI();
  private static native long nativeCreate();
  private static native void nativeDestroy(long nativePtr);
  private static native void nativeBegin(
      long nativePtr, int id, int type, float x, float y, float width, float height);
  private static native void nativeEnd(long nativePtr);
  private static native void nativeFill(long nativePtr, int color, int clipIndex);
  private static native void nativeDrawView(
      long nativePtr, int viewId, float offsetX, float offsetY);
  private static native void nativeText(long nativePtr, int textId, int boxIndex);
  private static native void nativeImage(long nativePtr, int imageId, int boxIndex);
  private static native void nativeBackgroundImage(
      long nativePtr, int imageId, int tilingIndex, int clipIndex, int repeatX, int repeatY);
  private static native void nativeBorder(
      long nativePtr, int outIndex, int innerIndex, int[] colors, int[] styles);
  private static native void nativeClipRect(
      long nativePtr, float x, float y, float width, float height, float[] radii);
  private static native void nativeRecordBox(
      long nativePtr, float x, float y, float width, float height, float[] radii);
  private static native void nativeLinearGradient(long nativePtr, int[] colors, float[] stops,
      int tilingIndex, int clipIndex, int repeatX, int repeatY, float angle);
  private static native void nativeBoxShadow(long nativePtr, int shadowBoxIndex, int clipBoxIndex,
      int color, float blurRadius, int clipMode);
  private static native void nativeBuild(long nativePtr);
  private static native ByteBuffer nativeGetItemsBuffer(long nativePtr);
  private static native ByteBuffer nativeGetDataBuffer(long nativePtr);
}
