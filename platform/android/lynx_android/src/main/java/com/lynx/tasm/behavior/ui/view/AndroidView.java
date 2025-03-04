// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.view;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Rect;
import android.os.Build;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import com.lynx.tasm.behavior.ui.IDrawChildHook;
import com.lynx.tasm.behavior.ui.IDrawChildHook.IDrawChildHookBinding;
import com.lynx.tasm.gesture.arena.GestureArenaManager;
import com.lynx.tasm.utils.BlurUtils;
import java.lang.ref.WeakReference;

public class AndroidView extends ViewGroup implements IDrawChildHookBinding {
  private Bitmap mBlurBitmap;
  private Canvas mBlurCanvas;
  private float mBlurRadius = 0;
  private boolean mPreDrawListenerAdded = false;
  private boolean mNeedUsePreDrawListener = false;
  private boolean mIsAttachToWindow = false;
  // for gesture arena
  private WeakReference<GestureArenaManager> mGestureArenaManager;

  public void setBlurSampling(int sampling) {
    this.mBlurSampling = sampling;
  }
  private int mBlurSampling = 1;
  private final ViewTreeObserver.OnPreDrawListener mPreDrawListener = () -> {
    // If a bitmap is drawn to a HW accelerated canvas, android will hold its reference. And
    // update the content of the reference to the screen. We do not invalidate here, just let
    updateBlur();
    return true;
  };

  private void updateBlur() {
    int w = getWidth();
    int h = getHeight();

    /*
     * If size is changed to 0, {@link #draw(Canvas)} will remove the blur effect in the same draw
     * pass. Else if size is 0 at beginning, nothing needs to do. This will prevent
     * IllegalArgumentException from creating Bitmap with size 0.
     */

    if (w <= 0 || h <= 0) {
      mBlurBitmap = null;
      return;
    }

    if (mBlurBitmap == null || mBlurBitmap.getWidth() != w || mBlurBitmap.getHeight() != h) {
      mBlurBitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
      mBlurCanvas = new Canvas(mBlurBitmap);
    }
    mBlurBitmap.eraseColor(Color.TRANSPARENT);
    mBlurCanvas.save();
    // Draw contents on the bitmap.
    super.draw(mBlurCanvas);
    mBlurCanvas.restore();
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
      mBlurBitmap = BlurUtils.blur(getContext(), mBlurBitmap, w, h, mBlurRadius, mBlurSampling);
    }
    // update the reference for the bitmap.
    mBlurCanvas.setBitmap(mBlurBitmap);
  }
  private String mImpressionId;
  protected IDrawChildHook mDrawChildHook;
  private boolean mConsumeHoverEvent = false;
  private boolean nativeInteractionEnabled = false;

  public AndroidView(Context context) {
    super(context);
  }

  @Override
  protected void onLayout(boolean changed, int l, int t, int r, int b) {
    // no-op since UIGroup handles actually laying out children.
    if (!getRootView().isLayoutRequested() && mDrawChildHook != null) {
      // it means this onLayout is not from rootview's performTraversals, it may be triggered from
      // ReccyleView we should performLayoutChildrenUI just like LynxView!
      mDrawChildHook.performLayoutChildrenUI();
    }
  }

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    setMeasuredDimension(
        MeasureSpec.getSize(widthMeasureSpec), MeasureSpec.getSize(heightMeasureSpec));

    if (!getRootView().isLayoutRequested() && mDrawChildHook != null) {
      // it means this onMeasure is not from rootview's performTraversals, it may be triggered from
      // Swiper's ViewPager we should performMeasureChildrenUI just like LynxView!
      mDrawChildHook.performMeasureChildrenUI();
    }
  }

  @Override
  protected void onAttachedToWindow() {
    super.onAttachedToWindow();
    mIsAttachToWindow = true;
    if (mNeedUsePreDrawListener && !mPreDrawListenerAdded) {
      getViewTreeObserver().addOnPreDrawListener(mPreDrawListener);
      mPreDrawListenerAdded = true;
    }
  }

  @Override
  protected void onDetachedFromWindow() {
    super.onDetachedFromWindow();
    mIsAttachToWindow = false;
    if (mPreDrawListenerAdded) {
      getViewTreeObserver().removeOnPreDrawListener(mPreDrawListener);
      mPreDrawListenerAdded = false;
    }
  }

  @Override
  public boolean onTouchEvent(MotionEvent event) {
    if (this.nativeInteractionEnabled) {
      return true;
    }
    return super.onTouchEvent(event);
  }

  public void setNativeInteractionEnabled(boolean enabled) {
    this.nativeInteractionEnabled = enabled;
  }

  public void setImpressionId(String id) {
    mImpressionId = id;
  }

  public String getImpressionId() {
    return mImpressionId;
  }

  public IDrawChildHook getDrawChildHook() {
    return mDrawChildHook;
  }

  @Override
  public void bindDrawChildHook(IDrawChildHook hook) {
    mDrawChildHook = hook;
  }

  @Override
  public void draw(Canvas canvas) {
    if (mDrawChildHook != null) {
      mDrawChildHook.beforeDraw(canvas);
    }
    if (mBlurRadius != 0 && mBlurBitmap != null) {
      canvas.drawBitmap(mBlurBitmap, 0, 0, null);
    } else {
      super.draw(canvas);
    }
    if (mDrawChildHook != null) {
      mDrawChildHook.afterDraw(canvas);
    }
  }

  /**
   * Sets the gesture manager using a WeakReference.
   * @param manager The GestureArenaManager to set.
   */
  public void setGestureManager(GestureArenaManager manager) {
    mGestureArenaManager = new WeakReference<>(manager);
  }

  /**
   * Computes scroll and handles gestures using the GestureArenaManager if available.
   */
  @Override
  public void computeScroll() {
    super.computeScroll(); // Compute scroll as usual
    if (mGestureArenaManager != null) {
      GestureArenaManager manager = mGestureArenaManager.get();
      if (manager != null) {
        manager.computeScroll(); // Compute gesture-related scroll using GestureArenaManager if
                                 // available
      }
    }
  }

  @Override
  protected void dispatchDraw(final Canvas canvas) {
    if (mDrawChildHook != null) {
      mDrawChildHook.beforeDispatchDraw(canvas);
    }

    super.dispatchDraw(canvas);

    if (mDrawChildHook != null) {
      mDrawChildHook.afterDispatchDraw(canvas);
    }
  }

  @Override
  protected boolean drawChild(Canvas canvas, View child, long drawingTime) {
    Rect bound = null;
    if (mDrawChildHook != null) {
      bound = mDrawChildHook.beforeDrawChild(canvas, child, drawingTime);
    }
    boolean ret;
    if (bound != null) {
      canvas.save();
      canvas.clipRect(bound);
      ret = super.drawChild(canvas, child, drawingTime);
      canvas.restore();
    } else {
      ret = super.drawChild(canvas, child, drawingTime);
    }

    if (mDrawChildHook != null) {
      mDrawChildHook.afterDrawChild(canvas, child, drawingTime);
    }

    return ret;
  }

  @Override
  public void setChildrenDrawingOrderEnabled(boolean enabled) {
    super.setChildrenDrawingOrderEnabled(enabled);
  }

  @Override
  protected int getChildDrawingOrder(int childCount, int index) {
    if (mDrawChildHook != null) {
      return mDrawChildHook.getChildDrawingOrder(childCount, index);
    }
    return super.getChildDrawingOrder(childCount, index);
  }

  @Override
  public boolean hasOverlappingRendering() {
    if (mDrawChildHook != null) {
      return mDrawChildHook.hasOverlappingRendering();
    }
    return super.hasOverlappingRendering();
  }

  @Override
  public boolean onHoverEvent(MotionEvent event) {
    return super.onHoverEvent(event) || mConsumeHoverEvent;
  }

  public void setConsumeHoverEvent(boolean consumeHoverEvent) {
    mConsumeHoverEvent = consumeHoverEvent;
  }

  public void setBlur(float radius) {
    if (radius < 0) {
      radius = 0;
    }
    mBlurRadius = radius;
    if (BlurUtils.createEffect(this, radius)) {
      return;
    }
    if (radius == 0) {
      removeBlur();
      return;
    }
    mNeedUsePreDrawListener = true;
    if (mIsAttachToWindow) {
      if (!mPreDrawListenerAdded) {
        getViewTreeObserver().addOnPreDrawListener(mPreDrawListener);
        setBackgroundColor(Color.TRANSPARENT);
        mPreDrawListenerAdded = true;
      }
    }
  }

  public void removeBlur() {
    mBlurRadius = 0;
    if (BlurUtils.removeEffect(this)) {
      return;
    }
    mNeedUsePreDrawListener = false;
    // If api level less than 31, should remove onPreDrawListener.
    if (mPreDrawListenerAdded) {
      if (mIsAttachToWindow) {
        getViewTreeObserver().removeOnPreDrawListener(mPreDrawListener);
        mPreDrawListenerAdded = false;
      }
      mBlurBitmap = null;
      mBlurCanvas = null;
    }
  }
}
