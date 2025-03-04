// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui;

import static com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper.ACCESSIBILITY_ELEMENT_TRUE;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.widget.FrameLayout;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import com.lynx.tasm.PageConfig;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.UIBody.UIBodyView;
import com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper;
import com.lynx.tasm.performance.TimingCollector;
import com.lynx.tasm.performance.longtasktiming.LynxLongTaskMonitor;
import com.lynx.tasm.utils.LynxConstants;
import java.lang.ref.WeakReference;
import java.util.HashMap;

public class UIBody extends UIGroup<UIBodyView> {
  @Nullable private UIBodyView mBodyView;

  private LynxAccessibilityWrapper mA11yWrapper;

  public UIBody(LynxContext context, final UIBodyView view) {
    super(context);
    mBodyView = view;
    initialize();
  }

  public UIBodyView getBodyView() {
    return mBodyView;
  }
  /**
   * when async render, we should attach LynxView
   * @param view
   */
  public void attachUIBodyView(UIBodyView view) {
    mBodyView = view;
    initialize();
  }

  @Override
  public void initialize() {
    super.initialize();
    initAccessibility();
  }

  protected void initAccessibility() {
    final UIBodyView view = getBodyView();
    if (view == null || view.isAccessibilityDisabled()) {
      return;
    }
    if (mA11yWrapper == null) {
      mA11yWrapper = new LynxAccessibilityWrapper(this);
    }
    mAccessibilityElementStatus = ACCESSIBILITY_ELEMENT_TRUE;
    view.setLynxAccessibilityWrapper(mA11yWrapper);
  }

  public void onPageConfigDecoded(PageConfig config) {
    if (mA11yWrapper != null) {
      mA11yWrapper.onPageConfigDecoded(config);
    }
  }

  @Override
  protected View getRealParentView() {
    return mBodyView;
  }

  @Override
  protected UIBodyView createView(final Context context) {
    return mBodyView;
  }

  @Override
  public void onLayoutUpdated() {
    super.onLayoutUpdated();
    if (mBodyView != null) {
      mBodyView.notifyMeaningfulLayout();
    }
  }

  @Override
  public boolean eventThrough() {
    // If <page event-through={true}/>, the res will be true.
    // Otherwise the res will be false, then check PageConfig enableEventThrough. If PageConfig
    // enableEventThrough == true, let the res be true. In other words, when
    // config.enableEventThrough == true or page.event-through == true, rootUI's eventThrough will
    // be true.
    boolean res = super.eventThrough();
    if (!res) {
      res |= mContext.enableEventThrough();
    }
    return res;
  }

  @Override
  public void removeAll() {
    super.removeAll();
    // removeAll function means reloading, we should clear some flags.
    if (mBodyView != null) {
      mBodyView.clearMeaningfulFlag();
    }
  }

  public boolean enableNewAccessibility() {
    return mA11yWrapper != null && mA11yWrapper.enableDelegate();
  }

  public LynxAccessibilityWrapper getLynxAccessibilityWrapper() {
    return mA11yWrapper;
  }

  public static class UIBodyView
      extends FrameLayout implements IDrawChildHook.IDrawChildHookBinding {
    private IDrawChildHook mDrawChildHook;
    private long mMeaningfulPaintTiming;
    private boolean mHasMeaningfulLayout;
    private boolean mHasMeaningfulPaint;

    private boolean mInterceptRequestLayout;
    private boolean mHasPendingRequestLayout;

    private WeakReference<TimingCollector> mTimingCollector = new WeakReference<>(null);

    private int mInstanceId = LynxContext.INSTANCE_ID_DEFAULT;

    protected LynxAccessibilityWrapper mA11yWrapper;

    public UIBodyView(Context context) {
      super(context);
    }

    public UIBodyView(Context context, AttributeSet attrs) {
      super(context, attrs);
    }

    @Override
    public void bindDrawChildHook(IDrawChildHook hook) {
      mDrawChildHook = hook;
    }

    public void setLynxAccessibilityWrapper(LynxAccessibilityWrapper wrapper) {
      mA11yWrapper = wrapper;
    }

    @Override
    public void requestLayout() {
      if (mInterceptRequestLayout) {
        mHasPendingRequestLayout = true;
        return;
      }
      mHasPendingRequestLayout = false;
      super.requestLayout();
    }

    @Override
    protected void dispatchDraw(final Canvas canvas) {
      String eventName = "LynxTemplateRender.Draw";
      boolean needLongTaskMonitor = false;
      TimingCollector timingCollector = mTimingCollector.get();
      LynxLongTaskMonitor.willProcessTask(eventName, mInstanceId);
      needLongTaskMonitor = true;
      markTraceIfNeed(eventName, false);
      if (mDrawChildHook != null) {
        mDrawChildHook.beforeDispatchDraw(canvas);
      }

      super.dispatchDraw(canvas);

      if (mDrawChildHook != null) {
        mDrawChildHook.afterDispatchDraw(canvas);
      }
      if (mHasMeaningfulLayout && !mHasMeaningfulPaint) {
        TraceEvent.instant(TraceEvent.CATEGORY_VITALS, "FirstMeaningfulPaint");
        mMeaningfulPaintTiming = System.currentTimeMillis();
        mHasMeaningfulPaint = true;
      }
      if (timingCollector != null) {
        timingCollector.markDrawEndTimingIfNeeded();
      }
      markTraceIfNeed(eventName, true);
      if (needLongTaskMonitor) {
        LynxLongTaskMonitor.didProcessTask();
      }
    }

    private void markTraceIfNeed(String event, boolean isEnd) {
      if (!TraceEvent.enableTrace()) {
        return;
      }
      HashMap map = new HashMap<String, String>();
      map.put("instance_id", mInstanceId + "");
      if (isEnd) {
        TraceEvent.endSection(TraceEvent.CATEGORY_VITALS, event);
      } else {
        TraceEvent.beginSection(TraceEvent.CATEGORY_VITALS, event, map);
      }
    }

    void notifyMeaningfulLayout() {
      mHasMeaningfulLayout = true;
    }

    public long getMeaningfulPaintTiming() {
      return mMeaningfulPaintTiming;
    }

    void clearMeaningfulFlag() {
      mHasMeaningfulLayout = false;
      mHasMeaningfulPaint = false;
      mMeaningfulPaintTiming = 0;
    }

    public void setTimingCollector(TimingCollector timingCollector) {
      mTimingCollector = new WeakReference<>(timingCollector);
    }

    public void setInstanceId(int instanceId) {
      mInstanceId = instanceId;
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

    /* intercept Hover event on UIBodyView, do not dispatch if consumed by
     * LynxAccessibilityNodeProvider */
    @Override
    public boolean dispatchHoverEvent(MotionEvent event) {
      if (isAccessibilityDisabled()) {
        return super.dispatchHoverEvent(event);
      }
      if (mA11yWrapper != null) {
        if (mA11yWrapper.enableHelper()) {
          return super.dispatchHoverEvent(event);
        } else if (mA11yWrapper.onHoverEvent(this, event)) {
          return true;
        }
      }
      return onHoverEvent(event);
    }

    /* WINDOW_CONTENT_CHANGED event come from scroll
     *  focused view will move when scroll if it is child of event source
     *  so set event source to UIBodyView
     * */
    @Override
    public boolean requestSendAccessibilityEvent(
        View child, android.view.accessibility.AccessibilityEvent event) {
      if (isAccessibilityDisabled()) {
        return super.requestSendAccessibilityEvent(child, event);
      }
      if (mA11yWrapper != null && !mA11yWrapper.enableHelper()
          && event.getEventType()
              == android.view.accessibility.AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED) {
        event.setSource(this);
      }
      return super.requestSendAccessibilityEvent(child, event);
    }

    @RestrictTo(RestrictTo.Scope.LIBRARY)
    public void SetShouldInterceptRequestLayout(boolean intercept) {
      mInterceptRequestLayout = intercept;
    }

    @RestrictTo(RestrictTo.Scope.LIBRARY)
    public boolean HasPendingRequestLayout() {
      return mHasPendingRequestLayout;
    }

    public boolean isAccessibilityDisabled() {
      return false;
    }
  }
}
