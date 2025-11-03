// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.image;

import android.os.Handler;
import android.os.Looper;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.PropsConstants;
import com.lynx.tasm.behavior.shadow.LayoutNode;
import com.lynx.tasm.behavior.shadow.MeasureFunc;
import com.lynx.tasm.behavior.shadow.MeasureMode;
import com.lynx.tasm.behavior.shadow.MeasureOutput;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.group.BitmapSize;
import com.lynx.tasm.group.ILynxViewRuntimeCacheManager;

public class AutoSizeImage extends ShadowNode implements MeasureFunc {
  private boolean mAutoSize = false;
  private boolean mBlockAutoSizeMarkDirty = false;
  private int mCachedBitmapWidth = 0;
  private int mCachedBitmapHeight = 0;
  private int mBitmapWidth;
  private int mBitmapHeight;
  private boolean mExactly = true;
  private Handler mLayoutHandler;
  private Runnable mPendingRunnable;

  private String source;

  private final Object mLock = new Object();
  public AutoSizeImage() {
    setMeasureFunc(this);
  }

  @LynxProp(name = PropsConstants.SRC)
  public void setSrc(String source) {
    this.source = source;
    // reset bitmap cache hit flag.
    this.mCachedBitmapHeight = 0;
    this.mCachedBitmapWidth = 0;
  }

  private BitmapSize getCacheBitmapSize(String source) {
    if (mContext != null) {
      ILynxViewRuntimeCacheManager cacheManager = mContext.getRuntimeCacheManager();
      if (cacheManager != null) {
        return cacheManager.getBitmapSizeCache(source);
      }
    }
    return null;
  }

  @Override
  public long measure(
      LayoutNode node, float width, MeasureMode widthMode, float height, MeasureMode heightMode) {
    String traceEvent = null;
    if (TraceEvent.isTracingStarted()) {
      traceEvent = "AutoSizeImage Measure: " + width + ":" + widthMode.intValue() + " - " + height
          + ":" + heightMode.intValue() + " " + source;
      TraceEvent.beginSection(traceEvent);
    }
    synchronized (mLock) {
      if (mLayoutHandler == null) {
        // layout thread
        mLayoutHandler = new Handler(Looper.myLooper());
        if (mPendingRunnable != null) {
          mLayoutHandler.post(mPendingRunnable);
          mPendingRunnable = null;
        }
      }
    }
    mExactly = widthMode == MeasureMode.EXACTLY && heightMode == MeasureMode.EXACTLY;
    if (mExactly) {
      if (TraceEvent.isTracingStarted()) {
        TraceEvent.endSection(traceEvent);
      }
      this.mBlockAutoSizeMarkDirty = true;
      return MeasureOutput.make(width, height);
    }

    int bitmapW;
    int bitmapH;
    BitmapSize cachedSize = getCacheBitmapSize(source);
    if (cachedSize != null) {
      String cacheHitEvent = null;
      if (TraceEvent.isTracingStarted()) {
        cacheHitEvent = "CacheHit";
        TraceEvent.beginSection(cacheHitEvent);
      }
      mCachedBitmapWidth = cachedSize.getWidth();
      mCachedBitmapHeight = cachedSize.getHeight();
      bitmapW = mCachedBitmapWidth;
      bitmapH = mCachedBitmapHeight;
      if (TraceEvent.isTracingStarted()) {
        TraceEvent.endSection(cacheHitEvent);
      }
    } else {
      bitmapW = mBitmapWidth;
      bitmapH = mBitmapHeight;
    }

    mExactly = (width == 0 && widthMode != MeasureMode.UNDEFINED)
        || (height == 0 && heightMode != MeasureMode.UNDEFINED);
    if (!mAutoSize || bitmapW <= 0 || bitmapH <= 0 || mExactly) {
      if (TraceEvent.isTracingStarted()) {
        TraceEvent.endSection(traceEvent);
      }
      return MeasureOutput.make(widthMode == MeasureMode.EXACTLY ? width : 0,
          heightMode == MeasureMode.EXACTLY ? height : 0);
    }

    if (widthMode == MeasureMode.EXACTLY) {
      // Height is determined by width
      float expectHeight = bitmapH / (float) bitmapW * width;
      if (heightMode == MeasureMode.AT_MOST) {
        if (height > expectHeight) {
          height = expectHeight;
        }
      } else if (heightMode == MeasureMode.UNDEFINED) {
        height = expectHeight;
      } else {
        // will never reach this point
      }
    } else {
      int maxSize = 65535;
      if (widthMode == MeasureMode.UNDEFINED) {
        width = maxSize;
      }
      if (heightMode == MeasureMode.UNDEFINED) {
        height = maxSize;
      }

      if (heightMode == MeasureMode.EXACTLY) {
        // width is determined by height
        float expectWidth = bitmapW / (float) bitmapH * height;
        if (width > expectWidth) {
          width = expectWidth;
        }
      } else {
        // If the bitmap is smaller than the image view, the image view size is the bitmap size
        if (bitmapW <= width && bitmapH <= height) {
          width = bitmapW;
          height = bitmapH;
        } else {
          // Otherwise, the short side of the image view shall prevail
          float aspectRatio = bitmapH / (float) bitmapW;
          if (height / (float) width < aspectRatio) {
            width = height / aspectRatio;
          } else {
            height = aspectRatio * width;
          }
        }
      }
    }
    if (TraceEvent.isTracingStarted()) {
      TraceEvent.endSection(traceEvent);
    }
    return MeasureOutput.make(width, height);
  }

  public void justSizeIfNeeded(final boolean autoSize, final int bitmapW, final int bitmapH,
      final int width, final int height) {
    synchronized (mLock) {
      if (mLayoutHandler == null) {
        mPendingRunnable = new Runnable() {
          @Override
          public void run() {
            justSize(autoSize, bitmapW, bitmapH, width, height);
          }
        };
      } else {
        mLayoutHandler.post(new Runnable() {
          @Override
          public void run() {
            justSize(autoSize, bitmapW, bitmapH, width, height);
          }
        });
      }
    }
  }

  private void justSize(final boolean autoSize, final int bitmapW, final int bitmapH,
      final int width, final int height) {
    boolean lastState = mAutoSize;
    mAutoSize = autoSize;
    mBitmapWidth = bitmapW;
    mBitmapHeight = bitmapH;
    if ((mBitmapWidth == mCachedBitmapWidth && mBitmapHeight == mCachedBitmapHeight)) {
      // do not markDirty & requestLayout if bitmap size cache hit.
      return;
    }
    if (mContext != null && mContext.isEmbeddedModeOn() && mBlockAutoSizeMarkDirty) {
      // if measure with exactly and in embeddedMode, we should block markDirty.
      return;
    }
    if (lastState != autoSize) {
      markDirty();
      return;
    }
    if (mExactly) {
      return;
    }
    if (autoSize) {
      // If the bitmap aspect ratio is the same as the <image> aspect ratio,
      // there is no need to layout.
      // If less than 0.05, it is considered equal (precision problem)
      if (bitmapW > 0 && bitmapH > 0
          && (width == 0 || height == 0
              || Math.abs(width / (float) height - bitmapW / (float) bitmapH) > 0.05)) {
        markDirty();
      }
    }
  }
}
