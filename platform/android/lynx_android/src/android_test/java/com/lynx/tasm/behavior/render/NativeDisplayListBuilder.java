// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import android.graphics.Path;
import java.nio.ByteBuffer;

/**
 * Test-only JNI wrapper around the production C++ {@code DisplayListBuilder}.
 *
 * <p>Valid display-list buffers used by Android behavior tests must come from this builder so the
 * tests do not duplicate the native {@code DisplayListItem} ABI.
 */
final class NativeDisplayListBuilder implements AutoCloseable {
  static final int PATH_CLIP = 0;
  static final int PATH_OWNER_CLIP = 1;
  private static boolean sRegistered;

  private long mNativePtr;
  private PlatformRendererContext mContext;
  private boolean mBuilt;
  private boolean mTransferred;
  private boolean mRendererState;

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
    return backgroundImage(imageId, tilingIndex, clipIndex, repeatX, repeatY,
        DisplayListApplier.BACKGROUND_SIZE_RESOLVED, 0f, 0f);
  }

  NativeDisplayListBuilder backgroundImage(int imageId, int tilingIndex, int clipIndex, int repeatX,
      int repeatY, int sizeMode, float positionX, float positionY) {
    ensureMutable();
    nativeBackgroundImage(mNativePtr, imageId, tilingIndex, clipIndex, repeatX, repeatY, sizeMode,
        positionX, positionY);
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
    return linearGradient(colors, stops, tilingIndex, clipIndex, repeatX, repeatY, angle, -1);
  }

  NativeDisplayListBuilder linearGradient(int[] colors, float[] stops, int tilingIndex,
      int clipIndex, int repeatX, int repeatY, float angle, int innerClipIndex) {
    ensureMutable();
    nativeLinearGradient(
        mNativePtr, colors, stops, tilingIndex, clipIndex, repeatX, repeatY, angle, innerClipIndex);
    return this;
  }

  NativeDisplayListBuilder radialGradient(int[] colors, float[] stops, int tilingIndex,
      int clipIndex, int repeatX, int repeatY, float centerX, float centerY, float radiusX,
      float radiusY) {
    ensureMutable();
    nativeRadialGradient(mNativePtr, colors, stops, tilingIndex, clipIndex, repeatX, repeatY,
        centerX, centerY, radiusX, radiusY);
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

  NativeDisplayListBuilder rectPath(
      int operation, float width, float height, int rule, float x, float y, float... radii) {
    ensureMutable();
    validateRadii(radii);
    nativeRecordRectPath(mNativePtr, operation, width, height, radii, rule, x, y);
    return this;
  }

  NativeDisplayListBuilder sourceOnlyRectPath(
      int operation, float width, float height, int rule, float x, float y) {
    ensureMutable();
    nativeRecordSourceOnlyRectPath(mNativePtr, operation, width, height, rule, x, y);
    return this;
  }

  NativeDisplayListBuilder svgPath(
      int operation, String data, float scale, int rule, float x, float y) {
    ensureMutable();
    nativeRecordSvgPath(mNativePtr, operation, data, scale, rule, x, y);
    return this;
  }

  NativeDisplayListBuilder commandPath(int operation, int rule, float x, float y) {
    ensureMutable();
    nativeRecordCommandPath(mNativePtr, operation, rule, x, y);
    return this;
  }

  NativeDisplayListBuilder reusePath(
      NativeDisplayListBuilder source, int index, int operation, int rule, float x, float y) {
    ensureMutable();
    source.ensureBuilt();
    nativeReusePath(mNativePtr, source.mNativePtr, index, operation, rule, x, y);
    return this;
  }

  NativeDisplayListBuilder clearOwnerClip() {
    ensureMutable();
    nativeClearOwnerClip(mNativePtr);
    return this;
  }

  long getPathKey(int index) {
    ensureBuilt();
    return nativeGetPathKey(mNativePtr, index);
  }

  byte[] getPathBytes(int index) {
    ensureBuilt();
    return nativeGetPath(mNativePtr, index);
  }

  Path getPathObject(int index, int rule) {
    ensureBuilt();
    long key = getPathKey(index);
    return mContext.getOrBuildPath(key, rule, new PlatformRendererContext.PathBytesLoader() {
      @Override
      byte[] load() {
        return getPathBytes(index);
      }
    });
  }

  long createRendererContext(PlatformRendererContext context, int id) {
    ensureOpen();
    mContext = context;
    long result = nativeCreateRendererContext(mNativePtr, context, id);
    mRendererState = true;
    mBuilt = true;
    return result;
  }

  NativeDisplayListBuilder usePathContext(NativeDisplayListBuilder context) {
    ensureMutable();
    context.ensureOpen();
    mContext = context.mContext;
    nativeUsePathContext(mNativePtr, context.mNativePtr);
    return this;
  }

  void installIntoRenderer(NativeDisplayListBuilder rendererState) {
    ensureBuilt();
    rendererState.ensureOpen();
    nativeInstallIntoRenderer(mNativePtr, rendererState.mNativePtr);
    mTransferred = true;
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
    if (mTransferred && !mRendererState) {
      throw new IllegalStateException("The display list has been transferred to its renderer.");
    }
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
  private static native void nativeBackgroundImage(long nativePtr, int imageId, int tilingIndex,
      int clipIndex, int repeatX, int repeatY, int sizeMode, float positionX, float positionY);
  private static native void nativeBorder(
      long nativePtr, int outIndex, int innerIndex, int[] colors, int[] styles);
  private static native void nativeClipRect(
      long nativePtr, float x, float y, float width, float height, float[] radii);
  private static native void nativeRecordBox(
      long nativePtr, float x, float y, float width, float height, float[] radii);
  private static native void nativeLinearGradient(long nativePtr, int[] colors, float[] stops,
      int tilingIndex, int clipIndex, int repeatX, int repeatY, float angle, int innerClipIndex);
  private static native void nativeRadialGradient(long nativePtr, int[] colors, float[] stops,
      int tilingIndex, int clipIndex, int repeatX, int repeatY, float centerX, float centerY,
      float radiusX, float radiusY);
  private static native void nativeBoxShadow(long nativePtr, int shadowBoxIndex, int clipBoxIndex,
      int color, float blurRadius, int clipMode);
  private static native void nativeBuild(long nativePtr);
  private static native ByteBuffer nativeGetItemsBuffer(long nativePtr);
  private static native ByteBuffer nativeGetDataBuffer(long nativePtr);
  private static native void nativeRecordRectPath(long nativePtr, int operation, float width,
      float height, float[] radii, int rule, float x, float y);
  private static native void nativeRecordSourceOnlyRectPath(
      long nativePtr, int operation, float width, float height, int rule, float x, float y);
  private static native void nativeRecordSvgPath(
      long nativePtr, int operation, String data, float scale, int rule, float x, float y);
  private static native void nativeRecordCommandPath(
      long nativePtr, int operation, int rule, float x, float y);
  private static native void nativeReusePath(
      long nativePtr, long sourcePtr, int index, int operation, int rule, float x, float y);
  private static native void nativeClearOwnerClip(long nativePtr);
  private static native long nativeGetPathKey(long nativePtr, int index);
  private static native byte[] nativeGetPath(long nativePtr, int index);
  private static native long nativeCreateRendererContext(
      long nativePtr, PlatformRendererContext context, int id);
  private static native void nativeUsePathContext(long nativePtr, long contextPtr);
  private static native void nativeInstallIntoRenderer(long nativePtr, long rendererPtr);
}
