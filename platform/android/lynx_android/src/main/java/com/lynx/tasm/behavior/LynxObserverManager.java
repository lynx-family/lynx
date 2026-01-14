// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior;

import static java.lang.Math.max;

import android.app.Activity;
import android.graphics.Canvas;
import android.graphics.RectF;
import android.os.Handler;
import android.os.Looper;
import android.util.DisplayMetrics;
import android.view.Choreographer;
import android.view.View;
import android.view.ViewTreeObserver;
import android.view.Window;
import androidx.annotation.Nullable;
import androidx.annotation.UiThread;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxFlattenUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.utils.LynxUIHelper;
import com.lynx.tasm.utils.DisplayMetricsHolder;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;

public abstract class LynxObserverManager {
  private static class LynxViewIntervalRunnable implements Runnable {
    private final WeakReference<LynxObserverManager> mManagerRef;

    public LynxViewIntervalRunnable(LynxObserverManager manager) {
      mManagerRef = new WeakReference<>(manager);
    }

    @Override
    public void run() {
      LynxObserverManager manager = mManagerRef.get();
      if (manager != null) {
        manager.isLynxViewChanged();
      }
    }
  }

  protected WeakReference<UIBody> mRootBodyRef;
  private ViewTreeObserver.OnGlobalLayoutListener mGlobalLayoutListener;
  private ViewTreeObserver.OnScrollChangedListener mScrollChangedListener;
  private ViewTreeObserver.OnDrawListener mDrawListener;
  private ViewTreeObserver mViewTreeObserver;
  // A flag that shows whether the RootView's onDraw func has been executed
  protected boolean mRootViewPainted;
  protected long mInterval;
  protected long mLastCheckTime;
  private Handler mHandler;
  private Runnable mIntervalRunnable;
  private Handler mHandlerForLynxView;
  private Runnable mIntervalRunnableForLynxView;
  private RectF mLynxViewOldRect = null;
  private long mIntervalForLynxView;
  // if observerHandlerInner is delayed, set it to true, and observerHandler will not call Inner.
  protected boolean mDelayedInInner;

  // A point, which first value is the offset of left, second value is the offset of top.
  final private int[] mLocationOnScreen;

  // The switch controlling whether to enable send disexposure events when lynxview is hidden.
  protected boolean mEnableDisexposureWhenLynxHidden;

  // The switch controlling whether to enable exposure check when lynxview is isLayoutRequested.
  // If mEnableExposureWhenLayout == true, execute exposure check when lynxview is
  // isLayoutRequested. Otherwise, do not execute exposure check.
  protected boolean mEnableExposureWhenLayout = false;

  protected boolean mDelayedTaskPosted = false;
  protected int[] mWindowSize;

  final private String TAG;

  public LynxObserverManager(String tag) {
    mRootBodyRef = null;
    mRootViewPainted = false;

    // align to iOS, default value is 50ms.
    mInterval = 50;
    mIntervalForLynxView = 50;
    mLastCheckTime = 0;
    mHandler = null;
    mIntervalRunnable = null;
    mDelayedInInner = false;
    mLocationOnScreen = new int[2];
    mWindowSize = new int[2];
    mEnableDisexposureWhenLynxHidden = true;

    TAG = tag;
  }

  public void observerHandler() {
    if (!mRootViewPainted) {
      LLog.e(TAG, "Lynx observerHandler failed since rootView not draw");
      return;
    }
    if (mDelayedInInner) {
      LLog.w(TAG, "Lynx observerHandler failed since inner function is delayed");
      return;
    }
    Runnable runnable = new Runnable() {
      @Override
      public void run() {
        TraceEvent.beginSection(TraceEventDef.OBSERVER_MANAGER_OBSERVER_HANDLER);
        try {
          // sub class may override this and call postHandlerCallBackDelay function such that
          // mDelayedInInner may be true after exec this function
          observerHandlerInner();
        } catch (Throwable e) {
          LLog.e(TAG, "observerManager.intersectionObserverHandler failed: " + e.toString());
        }
        TraceEvent.endSection(TraceEventDef.OBSERVER_MANAGER_OBSERVER_HANDLER);
      }
    };
    UIThreadUtils.runOnUiThreadImmediately(runnable);
  }

  protected void postHandlerCallBackDelay(Choreographer.FrameCallback callback) {
    mDelayedInInner = true;
    Choreographer.getInstance().postFrameCallbackDelayed(callback, mInterval);
  }

  // called by sub class only when exec observerHandlerInner successfully
  protected void didObserveInner() {
    mDelayedInInner = false;
  }

  protected void observerHandlerInner(){};

  public void onAttachedToWindow() {
    ViewTreeObserver observer = getRootViewTreeObserver();
    if (observer == null) {
      LLog.e(TAG, "LynxObserverManager add listeners failed since observer is null");
      return;
    }
    if (mGlobalLayoutListener == null) {
      mGlobalLayoutListener = new ViewTreeObserver.OnGlobalLayoutListener() {
        @Override
        public void onGlobalLayout() {
          requestCheckUI();
        }
      };
    }
    if (mScrollChangedListener == null) {
      mScrollChangedListener = new ViewTreeObserver.OnScrollChangedListener() {
        @Override
        public void onScrollChanged() {
          requestCheckUI();
        }
      };
    }
    if (mDrawListener == null) {
      mDrawListener = new ViewTreeObserver.OnDrawListener() {
        @Override
        public void onDraw() {
          requestCheckUI();
        }
      };
    }
    if (observer != mViewTreeObserver) {
      mViewTreeObserver = observer;
      mViewTreeObserver.addOnGlobalLayoutListener(mGlobalLayoutListener);
      mViewTreeObserver.addOnScrollChangedListener(mScrollChangedListener);
      mViewTreeObserver.addOnDrawListener(mDrawListener);
    }
  }

  public void addToObserverTree() {
    LLog.i(TAG, "addToObserverTree");
    LynxContext context = null;
    UIBody body = mRootBodyRef.get();
    if (body != null) {
      context = body.getLynxContext();
    }
    if (context != null) {
      int rate = context.getObserverFrameRate();
      if (rate > 0) {
        // To avoid observerFrameRate being too large to affect performance,
        // the max observerFrameRate is set to 60. (1000ms / 60 = 16ms)
        mInterval = max(16, 1000 / rate);
      }
      mEnableDisexposureWhenLynxHidden = context.getEnableDisexposureWhenLynxHidden();
      mEnableExposureWhenLayout = context.getEnableExposureWhenLayout();
    }

    onAttachedToWindow();

    if (mHandler == null) {
      mHandler = new Handler(Looper.getMainLooper());
    }

    if (mIntervalRunnable == null) {
      mIntervalRunnable = new Runnable() {
        @Override
        public void run() {
          observerHandler();
        }
      };
    }
    mHandler.post(mIntervalRunnable);

    if (mHandlerForLynxView == null) {
      mHandlerForLynxView = new Handler(Looper.getMainLooper());
    }
    if (mIntervalRunnableForLynxView == null) {
      mIntervalRunnableForLynxView = new LynxViewIntervalRunnable(this);
    }
    mHandlerForLynxView.postDelayed(mIntervalRunnableForLynxView, mIntervalForLynxView);
  }

  public void isLynxViewChanged() {
    if (mIntervalForLynxView != 0 && mHandlerForLynxView != null
        && mIntervalRunnableForLynxView != null) {
      mHandlerForLynxView.postDelayed(mIntervalRunnableForLynxView, mIntervalForLynxView);
    }

    if (mRootBodyRef.get() == null || !mRootViewPainted) {
      return;
    }

    RectF lynxViewRect = getBoundsOnScreenOfLynxBaseUI(mRootBodyRef.get());
    if (!lynxViewRect.equals(mLynxViewOldRect) && mHandler != null && mIntervalRunnable != null) {
      mHandler.post(mIntervalRunnable);
    }
    mLynxViewOldRect = lynxViewRect;
  }

  private int getFrameRate(ReadableMap options, String name) {
    int rate = -1;
    if (options.hasKey(name)) {
      rate = options.getInt(name);
    }

    return rate;
  }

  public void setObserverFrameRate(ReadableMap options) {
    if (options == null) {
      return;
    }

    int rate = getFrameRate(options, "forExposureCheck");
    if (rate > 0) {
      mInterval = max(16, 1000 / rate);
    }

    rate = getFrameRate(options, "forPageRect");
    if (rate >= 0) {
      mIntervalForLynxView = rate != 0 ? max(16, 1000 / rate) : 0;
      if (mIntervalForLynxView != 0 && mHandlerForLynxView != null
          && mIntervalRunnableForLynxView != null) {
        mHandlerForLynxView.postDelayed(mIntervalRunnableForLynxView, mIntervalForLynxView);
      }
    }
  }

  public void onRootViewDraw(Canvas canvas) {
    Runnable runnable = new Runnable() {
      @Override
      public void run() {
        if (!mRootViewPainted) {
          mRootViewPainted = true;
          Choreographer.getInstance().postFrameCallback(new Choreographer.FrameCallback() {
            @Override
            public void doFrame(long frameTimeNanos) {
              LLog.i(TAG, "onRootViewDraw ObserverHandler");
              observerHandler();
            }
          });
        }
      }
    };
    UIThreadUtils.runOnUiThreadImmediately(runnable);
  }

  @Nullable
  protected UIBody.UIBodyView getRootView() {
    UIBody rootUI = mRootBodyRef.get();
    if (rootUI == null) {
      LLog.e(TAG, "LynxObserver getRootView failed since rootUI is null");
      return null;
    }
    return rootUI.getBodyView();
  }

  @Nullable
  @UiThread
  private ViewTreeObserver getRootViewTreeObserver() {
    UIBody.UIBodyView rootView = getRootView();
    if (rootView == null) {
      LLog.e(TAG, "LynxObserver getViewTreeObserver failed since rootView is null");
      return null;
    }
    return rootView.getViewTreeObserver();
  }

  public void requestCheckUI() {
    if (mHandler == null) {
      return;
    }

    if (System.currentTimeMillis() - mLastCheckTime > mInterval) {
      mLastCheckTime = System.currentTimeMillis();
      mHandler.post(mIntervalRunnable);
      mDelayedTaskPosted = false;
    } else {
      if (mDelayedTaskPosted) {
        return;
      }
      mHandler.postDelayed(mIntervalRunnable, mInterval);
      mDelayedTaskPosted = true;
    }
  }

  public void destroy() {
    LLog.i(TAG, "destroy");
    if (mHandler != null) {
      mHandler.removeCallbacksAndMessages(null);
      mHandler = null;
    }
    if (mHandlerForLynxView != null) {
      mHandlerForLynxView.removeCallbacksAndMessages(null);
      mHandlerForLynxView = null;
    }
    if (mViewTreeObserver == null || !mViewTreeObserver.isAlive()) {
      mViewTreeObserver = getRootViewTreeObserver();
    }
    if (mViewTreeObserver == null) {
      LLog.e(TAG, "LynxObserverManager remove listeners failed since observer is null");
      return;
    }
    mViewTreeObserver.removeOnGlobalLayoutListener(mGlobalLayoutListener);
    mViewTreeObserver.removeOnScrollChangedListener(mScrollChangedListener);
    mViewTreeObserver.removeOnDrawListener(mDrawListener);
    mViewTreeObserver = null;
    mGlobalLayoutListener = null;
    mScrollChangedListener = null;
    mDrawListener = null;
  }

  private void getLeftAndTopOfBoundsInScreen(View view, RectF bounds) {
    if (view == null) {
      LLog.e(TAG, "LynxObserverManager getLeftAndTopOfBoundsInScreen failed since view is null");
      return;
    }
    view.getLocationOnScreen(mLocationOnScreen);
    bounds.offset(mLocationOnScreen[0], mLocationOnScreen[1]);
  }

  protected RectF getBoundsOnScreenOfLynxBaseUI(LynxBaseUI ui) {
    RectF outBounds = new RectF();
    if (ui == null) {
      LLog.e(TAG, "LynxObserverManager getBoundsOnScreenOfLynxBaseUI failed since ui is null");
      return outBounds;
    }

    if (LynxEnv.inst().enableTransformForPositionCalculation()) {
      RectF res = LynxUIHelper.convertRectFromUIToScreen(
          ui, new RectF(0, 0, ui.getWidth(), ui.getHeight()));
      outBounds.set(res.left, res.top, res.right, res.bottom);
      return outBounds;
    }

    if (ui instanceof LynxUI) {
      // get bounds from LynxUI's mView
      LynxUI realLynxUI = (LynxUI) ui;
      getLeftAndTopOfBoundsInScreen(realLynxUI.getView(), outBounds);
      outBounds.set(outBounds.left, outBounds.top, outBounds.left + ui.getWidth(),
          outBounds.top + ui.getHeight());
    } else if (ui instanceof LynxFlattenUI) {
      // get bounds from parent of LynxFlattenUI, then offset
      LynxBaseUI realLynxUI = ui.getParentBaseUI();
      while (realLynxUI != null && !(realLynxUI instanceof LynxUI)) {
        realLynxUI = realLynxUI.getParentBaseUI();
      }
      if (realLynxUI != null) {
        View parent = ((LynxUI) realLynxUI).getView();
        getLeftAndTopOfBoundsInScreen(parent, outBounds);
        outBounds.offset(-parent.getScrollX(), -parent.getScrollY());
        outBounds.offset(ui.getLeft(), ui.getTop());
        outBounds.set(outBounds.left, outBounds.top, outBounds.left + ui.getWidth(),
            outBounds.top + ui.getHeight());
      }
    }
    return outBounds;
  }

  /**
   * @brief Update window size for Exposure.
   * @detail This function should be called when window size changed.
   * @param context LynxContext.
   */
  public void updateWindowSize(LynxContext context) {
    DisplayMetrics metrics = DisplayMetricsHolder.getRealScreenDisplayMetrics(context);
    mWindowSize[0] = metrics.widthPixels;
    mWindowSize[1] = metrics.heightPixels;
  }

  protected RectF getWindowRect(LynxContext context) {
    if (context != null) {
      Activity activity = context.getActivity();
      int[] position = new int[2];
      if (activity != null) {
        Window window = activity.getWindow();
        if (window != null) {
          window.getDecorView().getLocationOnScreen(position);
        }
      }
      return new RectF(
          position[0], position[1], position[0] + mWindowSize[0], position[1] + mWindowSize[1]);
    } else {
      LLog.e(TAG, "getWindowRect func failed since context is null");
    }
    return new RectF();
  }
}
