// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.image;

import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import com.lynx.tasm.behavior.shadow.LayoutNode;
import com.lynx.tasm.behavior.shadow.MeasureFunc;
import com.lynx.tasm.behavior.shadow.MeasureMode;
import com.lynx.tasm.behavior.shadow.MeasureOutput;
import com.lynx.tasm.behavior.shadow.ShadowNode;

public class AutoSizeImage extends ShadowNode implements MeasureFunc {
  private boolean mAutoSize = false;
  private int mBitmapWidth;
  private int mBitmapHeight;
  private boolean mExactly = true;
  private Handler mLayoutHandler;
  private Runnable mPendingRunnable;
  private final Object mLock = new Object();
  public AutoSizeImage() {
    setMeasureFunc(this);
  }

  @Override
  public long measure(
      LayoutNode node, float width, MeasureMode widthMode, float height, MeasureMode heightMode) {
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
      return MeasureOutput.make(width, height);
    }

    int bitmapW = mBitmapWidth;
    int bitmapH = mBitmapHeight;
    mExactly = (width == 0 && widthMode != MeasureMode.UNDEFINED)
        || (height == 0 && heightMode != MeasureMode.UNDEFINED);
    if (!mAutoSize || bitmapW <= 0 || bitmapH <= 0 || mExactly) {
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
