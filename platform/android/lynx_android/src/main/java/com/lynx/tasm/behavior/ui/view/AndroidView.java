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
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.render.IRendererHost;
import com.lynx.tasm.behavior.render.PlatformRendererContext;
import com.lynx.tasm.behavior.render.Renderer;
import com.lynx.tasm.behavior.ui.IDrawChildHook;
import com.lynx.tasm.behavior.ui.IDrawChildHook.IDrawChildHookBinding;
import com.lynx.tasm.behavior.ui.MeaningfulPaintingArea.IMeaningfulPaintingAreaInvalidateHook;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.gesture.arena.GestureArenaManager;
import com.lynx.tasm.utils.BlurUtils;
import java.lang.ref.WeakReference;

public class AndroidView extends ViewGroup
    implements IDrawChildHookBinding, IMeaningfulPaintingAreaInvalidateHook, IRendererHost {
  private Bitmap mBlurBitmap;
  private Canvas mBlurCanvas;
  private float mBlurRadius = 0;
  private boolean mPreDrawListenerAdded = false;
  private boolean mNeedUsePreDrawListener = false;
  private boolean mIsAttachToWindow = false;
  private boolean mEnableBlurAutoUpdate = true;
  // for gesture arena
  private WeakReference<GestureArenaManager> mGestureArenaManager;

  // whether to intercept gestures to current, parents and children's gesture
  private Boolean mInterceptGesture = null;

  private static final int MAX_BITMAP_SIZE = 100 * 1024 * 1024; // 100 MB
  public void setBlurSampling(int sampling) {
    this.mBlurSampling = sampling;
  }

  public void setBlurAutoUpdate(boolean enabled) {
    mEnableBlurAutoUpdate = enabled;
    if (enabled && mNeedUsePreDrawListener && mIsAttachToWindow && !mPreDrawListenerAdded) {
      getViewTreeObserver().addOnPreDrawListener(mPreDrawListener);
      mPreDrawListenerAdded = true;
    }
    invalidate();
  }

  private int mBlurSampling = 1;
  private final ViewTreeObserver.OnPreDrawListener mPreDrawListener =
      new ViewTreeObserver.OnPreDrawListener() {
        @Override
        public boolean onPreDraw() {
          TraceEvent.beginSection(TraceEventDef.VIEW_BLUR);
          // If a bitmap is drawn to a HW accelerated canvas, android will hold its reference. And
          // update the content of the reference to the screen. We do not invalidate here, just let
          updateBlur();
          TraceEvent.endSection(TraceEventDef.VIEW_BLUR);
          if (!mEnableBlurAutoUpdate) {
            getViewTreeObserver().removeOnPreDrawListener(mPreDrawListener);
            mPreDrawListenerAdded = false;
          }
          return true;
        }
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

    if (w * h * 4 >= MAX_BITMAP_SIZE) {
      LLog.e("AndroidView",
          "Skip blur bitmap creation: w=" + w + " h=" + h
              + " exceeds limit maxPixels=" + MAX_BITMAP_SIZE);
      mBlurBitmap = null;
      // Avoid subsequent operations on null bitmap in this pass
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
  private Renderer mRenderer;
  private boolean mConsumeHoverEvent = false;
  private boolean nativeInteractionEnabled = false;
  private boolean mPanInterceptSelf = false;
  private boolean mPanInterceptAncestors = false;
  private boolean mHasSetPanInterceptAncestors = false;
  private boolean mPanInterceptDescendants = false;

  public AndroidView(Context context) {
    super(context);
  }

  @Override
  public Renderer createRenderer(PlatformRendererContext platformRendererContext, int sign) {
    return new Renderer(platformRendererContext, sign);
  }

  @Override
  public void setRenderer(Renderer renderer) {
    mRenderer = renderer;
  }

  @Override
  public Renderer getRenderer() {
    return mRenderer;
  }

  @Override
  public View getView() {
    return this;
  }

  @Override
  protected void onLayout(boolean changed, int l, int t, int r, int b) {
    if (mRenderer != null) {
      if (mRenderer.getUIHost() != null) {
        mRenderer.getUIHost().measure();
      }
      mRenderer.onLayout(changed, l, t, r, b);
    }

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

  /**
   * @breif Dynamically intercepting native gestures
   * @param interceptGesture true: intercept native gesture, false: not intercept native gesture
   * @return void
   */
  public void interceptGesture(boolean interceptGesture) {
    mInterceptGesture = interceptGesture;
  }

  private boolean isInterceptGestureNotNull() {
    return mInterceptGesture != null;
  }

  private boolean isNeedInterceptGesture() {
    return isInterceptGestureNotNull() && mInterceptGesture;
  }

  @Override
  public boolean dispatchTouchEvent(MotionEvent event) {
    if (mPanInterceptAncestors) {
      getParent().requestDisallowInterceptTouchEvent(true);
    } else {
      if (mHasSetPanInterceptAncestors) {
        getParent().requestDisallowInterceptTouchEvent(false);
      }
    }
    return super.dispatchTouchEvent(event);
  }

  @Override
  public boolean onInterceptTouchEvent(MotionEvent ev) {
    if (mPanInterceptDescendants) {
      return true;
    }
    if (isNeedInterceptGesture()) {
      return mInterceptGesture;
    }
    return super.onInterceptTouchEvent(ev);
  }

  @Override
  public boolean onTouchEvent(MotionEvent event) {
    if (mPanInterceptAncestors) {
      return true;
    }
    if (mPanInterceptSelf) {
      return false;
    }
    if (this.nativeInteractionEnabled) {
      return true;
    }
    if (isInterceptGestureNotNull()) {
      if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
        getParent().requestDisallowInterceptTouchEvent(true);
        return true;
      } else if (event.getActionMasked() == MotionEvent.ACTION_MOVE) {
        getParent().requestDisallowInterceptTouchEvent(mInterceptGesture);
        boolean res = mInterceptGesture;
        if (!mInterceptGesture) {
          res = super.onTouchEvent(event);
        }
        return res;
      } else if (event.getActionMasked() == MotionEvent.ACTION_UP
          || event.getActionMasked() == MotionEvent.ACTION_CANCEL) {
        mInterceptGesture = null;
        return true;
      }
    }

    return super.onTouchEvent(event);
  }

  public void setNativeInteractionEnabled(boolean enabled) {
    this.nativeInteractionEnabled = enabled;
  }

  public void setPanInterceptSelf(boolean panInterceptSelf) {
    mPanInterceptSelf = panInterceptSelf;
  }

  public void setPanInterceptAncestors(boolean panInterceptAncestors) {
    mPanInterceptAncestors = panInterceptAncestors;
    if (mPanInterceptAncestors) {
      mHasSetPanInterceptAncestors = true;
    }
  }

  public void setPanInterceptDescendants(boolean panInterceptDescendants) {
    mPanInterceptDescendants = panInterceptDescendants;
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

    if (mRenderer != null) {
      mRenderer.afterDispatchDraw(canvas);
    }
  }

  @Override
  protected void onDraw(Canvas canvas) {
    if (mRenderer != null) {
      mRenderer.onDraw(canvas);
      return;
    }

    super.onDraw(canvas);
  }

  @Override
  protected boolean drawChild(Canvas canvas, View child, long drawingTime) {
    Rect bound = null;
    if (mDrawChildHook != null) {
      bound = mDrawChildHook.beforeDrawChild(canvas, child, drawingTime);
    }
    if (mRenderer != null) {
      mRenderer.beforeDrawChild(canvas, child);
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
    if (mRenderer != null) {
      mRenderer.afterDrawChild(canvas, child);
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

  @Override
  public void invalidateMeaningfulPaintingArea() {
    if (getContext() instanceof LynxContext) {
      UIBody.UIBodyView view = ((LynxContext) getContext()).getUIBodyView();
      if (view != null) {
        view.invalidateMeaningfulPaintingArea();
      }
    }
  }
}
