// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.image;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import androidx.annotation.NonNull;

public class ImageContent {
  private Bitmap mBitmap;

  private ReleasableImage mReleasableBitmap;

  private Drawable mDrawable;

  private boolean mIsBitmap;

  private float mAlpha = 1;

  private int mLeft;

  private int mTop;

  private int mBottom;

  private int mRight;

  private Paint mPaint;

  private Rect src;

  private Rect dst;

  public ImageContent(@NonNull ReleasableImage bitmapRef) {
    mReleasableBitmap = bitmapRef;

    if (bitmapRef.getBitmap() != null) {
      mBitmap = mReleasableBitmap.getBitmap();
      mIsBitmap = true;
      mPaint = new Paint();
      mPaint.setAntiAlias(true);
      src = new Rect(0, 0, mBitmap.getWidth(), mBitmap.getHeight());
    } else if (bitmapRef.getDrawable() != null) {
      this.mDrawable = bitmapRef.getDrawable();
    }
  }

  public ImageContent(@NonNull Bitmap bitmap) {
    this.mBitmap = bitmap;
    mIsBitmap = true;
    mPaint = new Paint();
    mPaint.setAntiAlias(true);
    src = new Rect(0, 0, mBitmap.getWidth(), mBitmap.getHeight());
  }

  public ImageContent(@NonNull Drawable drawable) {
    this.mDrawable = drawable;
  }

  public void releaseImageResource() {
    if (mReleasableBitmap != null) {
      mReleasableBitmap.release();
    }
  }

  public int getIntrinsicWidth() {
    if (mIsBitmap) {
      return mBitmap.getWidth();
    } else {
      return mDrawable.getIntrinsicWidth();
    }
  }

  public int getIntrinsicHeight() {
    if (mIsBitmap) {
      return mBitmap.getHeight();
    } else {
      return mDrawable.getIntrinsicHeight();
    }
  }

  public void setBounds(@androidx.annotation.NonNull Rect bounds) {
    this.setBounds(bounds.left, bounds.top, bounds.right, bounds.bottom);
  }

  public void setBounds(int left, int top, int right, int bottom) {
    if (mIsBitmap) {
      if (dst == null) {
        dst = new Rect();
      }
      if (mLeft != left || mRight != right || mTop != top || mBottom != bottom) {
        dst.set(left, top, right, bottom);
      }
      mLeft = left;
      mTop = top;
      mRight = right;
      mBottom = bottom;
    } else {
      mDrawable.setBounds(left, top, right, bottom);
    }
  }

  public void draw(Canvas canvas) {
    if (mIsBitmap) {
      canvas.drawBitmap(mBitmap, src, dst, mPaint);
    } else {
      mDrawable.draw(canvas);
    }
  }

  public final Bitmap getBitmap() {
    if (mIsBitmap) {
      return mBitmap;
    } else if (mDrawable instanceof BitmapDrawable) {
      return ((BitmapDrawable) mDrawable).getBitmap();
    } else {
      return null;
    }
  }

  public Drawable getDrawable() {
    return mDrawable;
  }

  public void setAlpha(int alpha) {
    if (mIsBitmap) {
      if (mAlpha != alpha) {
        mAlpha = alpha;
        mPaint.setAlpha(alpha);
      }
    } else {
      mDrawable.setAlpha(alpha);
    }
  }

  public int getOpacity() {
    if (mIsBitmap) {
      return (mBitmap == null || mBitmap.hasAlpha() || mPaint.getAlpha() < 255)
          ? PixelFormat.TRANSLUCENT
          : PixelFormat.OPAQUE;
    } else {
      return mDrawable.getOpacity();
    }
  }

  public void setColorFilter(ColorFilter colorFilter) {
    if (mIsBitmap) {
      mPaint.setColorFilter(colorFilter);
    } else {
      mDrawable.setColorFilter(colorFilter);
    }
  }

  public final void setCallback(Drawable.Callback cb) {
    if (!mIsBitmap) {
      mDrawable.setCallback(cb);
    }
  }

  public void setFilterBitmap(boolean filter) {
    if (mIsBitmap) {
      mPaint.setFilterBitmap(filter);
    } else {
      mDrawable.setFilterBitmap(filter);
    }
  }
}
