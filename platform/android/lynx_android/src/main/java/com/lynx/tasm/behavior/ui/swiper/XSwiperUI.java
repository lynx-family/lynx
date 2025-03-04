// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.swiper;

import static com.lynx.tasm.behavior.ui.swiper.ViewPager.INIT_ITEM_INDEX;
import static com.lynx.tasm.behavior.ui.swiper.ViewPager.SCROLL_DIRECTION_BEGIN;
import static com.lynx.tasm.behavior.ui.swiper.ViewPager.SCROLL_DIRECTION_END;
import static com.lynx.tasm.behavior.ui.swiper.ViewPager.SCROLL_STATE_DRAGGING;

import android.content.Context;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;
import android.view.View;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityEvent;
import androidx.core.view.AccessibilityDelegateCompat;
import androidx.core.view.ViewCompat;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.Dynamic;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.ReadableType;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxBehavior;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxGeneratorName;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.LynxUIMethod;
import com.lynx.tasm.behavior.LynxUIMethodConstants;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper;
import com.lynx.tasm.behavior.ui.list.UIList;
import com.lynx.tasm.behavior.ui.view.UISimpleView;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.event.LynxDetailEvent;
import com.lynx.tasm.utils.ColorUtils;
import com.lynx.tasm.utils.PixelUtils;
import com.lynx.tasm.utils.UnitUtils;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

@LynxGeneratorName(packageName = "com.lynx.tasm.behavior.ui.swiper")
@LynxBehavior(tagName = "swiper", isCreateAsync = true)
public class XSwiperUI extends UISimpleView<SwiperView> {
  public static final String TAG = "LynxSwiperUI";
  public static final boolean DEBUG = false;
  static final int SELECTED_COLOR = Color.argb(255, 255, 255, 255);
  static final int UNSELECTED_COLOR = Color.argb((int) (0.35f * 255.0f + 0.5f), 255, 255, 255);
  private static final String MODE_DEFAULT = "normal";
  private static final String MODE_CAROUSEL = "carousel";
  private static final String MODE_COVER_FLOW = "coverflow";
  private static final String MODE_FLAT_COVER_FLOW = "flat-coverflow";
  private static final String MODE_CARRY = "carry";
  private static final String BIND_CHANGE = "change";
  private static final String BIND_TRANSITION = "transition";
  private static final String BIND_SCROLL_START = "scrollstart";
  private static final String BIND_SCROLL_END = "scrollend";
  private static final String BIND_SCROLL_TO_BOUNCE = "scrolltobounce";
  private static final String BIND_CONTENT_SIZE_CHANGED = "contentsizechanged";
  private static final String METHOD_PARAMS_INDEX = "index";
  private static final String METHOD_PARAMS_SMOOTH = "smooth";
  private static final String METHOD_PARAMS_DIRECTION = "direction";
  private static final String METHOD_PARAMS_DIRECTION_BEGIN = "begin";
  private static final String METHOD_PARAMS_DIRECTION_END = "end";
  private String mMode = MODE_DEFAULT;
  private boolean mEnableChangeEvent = false;
  private boolean mEnableScrollStart = false;
  private boolean mEnableScrollEnd = false;
  private boolean mEnableTransitionEvent = false;
  private boolean mEnableScrollToBounce = false;
  private boolean mEnableContentSizeChanged = false;
  private int mContentWidth = -1;
  private int mContentHeight = -1;
  private int mPageMargin = -1;
  private int mNextMargin = -1;
  private int mPreviousMargin = -1;
  private boolean mIsVertical = false;
  private boolean mAutoPlay = false;
  protected boolean mCircular = false;
  protected boolean mSmoothScroll = true;
  protected int mInterval = 5000; // ms
  protected int mDuration = 500; // ms
  protected int mTransitionThrottle = 0; // ms
  private long mLastTransitionTime = 0; // ms
  protected boolean mAttachedToWindow = false;
  protected boolean mFinishReset = false;
  private boolean mLayoutPropsChanged = false;
  private boolean mCompatible = true;
  private boolean mScrollBeforeDetached = false;
  private boolean mTouchable = true;
  private ModeCoverFlowTransformer mCoverFlowTransformer = new ModeCoverFlowTransformer();
  private ModeCarryTransformer mCarryTransformer = new ModeCarryTransformer();
  protected final Handler mHandler = new Handler(Looper.getMainLooper());
  private final List<View> mChildrenList = new ArrayList<>();
  private Runnable mRunnable = new AutoScrollTask(this);
  private CustomSwiperAccessibilityDelegate mDelegate = null;

  public XSwiperUI(Context context) {
    super(context);
  }

  public XSwiperUI(LynxContext context) {
    super(context);
  }

  public XSwiperUI(LynxContext context, Object param) {
    super(context, param);
  }

  private static class AutoScrollTask implements Runnable {
    private WeakReference<XSwiperUI> mWeakUI;

    public AutoScrollTask(XSwiperUI ui) {
      mWeakUI = new WeakReference<>(ui);
    }

    @Override
    public void run() {
      final XSwiperUI swiperUI = mWeakUI.get();
      if (swiperUI != null) {
        if (swiperUI.mAttachedToWindow && swiperUI.mAutoPlay) {
          final ViewPager viewPager = swiperUI.getView().getViewPager();
          int nextIndex = viewPager.getCurrentIndex() + 1;
          int totalCount = viewPager.getTotalCount();
          boolean resetToFirst = false;
          if (nextIndex == totalCount && (swiperUI.mFinishReset || swiperUI.mCircular)) {
            nextIndex = 0;
            resetToFirst = true;
          }
          swiperUI.setIndex(viewPager, nextIndex, swiperUI.mSmoothScroll, resetToFirst);
          swiperUI.mHandler.postDelayed(this, swiperUI.mInterval);
        }
      }
    }
  }

  @Override
  protected SwiperView createView(Context context) {
    final SwiperView swiperView = new SwiperView(context);
    swiperView.getViewPager().addPageScrollListener(new ViewPager.OnPageScrollListener() {
      private boolean mNeedRestartAutoPlay = false;

      @Override
      public void onPageScrollStart(int position, boolean isDragged) {
        if (mEnableScrollStart) {
          LynxDetailEvent event = new LynxDetailEvent(getSign(), BIND_SCROLL_START);
          event.addDetail("current", position);
          event.addDetail("isDragged", isDragged);
          if (getLynxContext() != null) {
            getLynxContext().getEventEmitter().sendCustomEvent(event);
          }
        }
      }

      @Override
      public void onPageScrolling(int position, boolean isDragged, float offsetX, float offsetY) {
        recognizeGesturere();
        if (mEnableTransitionEvent) {
          long current = System.currentTimeMillis();
          long transitionDuration = current - mLastTransitionTime;
          if (mTransitionThrottle > 0 && transitionDuration <= mTransitionThrottle) {
            return;
          }
          mLastTransitionTime = current;
          LynxDetailEvent event = new LynxDetailEvent(getSign(), BIND_TRANSITION);
          event.addDetail("current", swiperView.getViewPager().getCurrentIndex());
          event.addDetail("isDragged", isDragged);
          event.addDetail("dx", PixelUtils.pxToDip(offsetX));
          event.addDetail("dy", PixelUtils.pxToDip(offsetY));
          if (getLynxContext() != null) {
            getLynxContext().getEventEmitter().sendCustomEvent(event);
          }
        }
      }

      @Override
      public void onPageScrollEnd(int position) {
        if (mEnableScrollEnd) {
          LynxDetailEvent event = new LynxDetailEvent(getSign(), BIND_SCROLL_END);
          event.addDetail("current", position);
          if (getLynxContext() != null) {
            getLynxContext().getEventEmitter().sendCustomEvent(event);
          }
        }
      }

      @Override
      public void onPageChange(int oldPosition, int newPosition, boolean isInit) {
        mView.setSelected(newPosition);
        if (mEnableChangeEvent && !isInit) {
          LynxDetailEvent event = new LynxDetailEvent(getSign(), BIND_CHANGE);
          event.addDetail("current", newPosition);
          if (getLynxContext() != null) {
            getLynxContext().getEventEmitter().sendCustomEvent(event);
          }
        }
      }

      @Override
      public void onPageScrollStateChanged(int oldState, int newState) {
        if (mAutoPlay) {
          if (newState == SCROLL_STATE_DRAGGING) {
            mNeedRestartAutoPlay = true;
            mHandler.removeCallbacks(mRunnable);
          } else if (mNeedRestartAutoPlay) {
            mNeedRestartAutoPlay = false;
            mHandler.removeCallbacks(mRunnable);
            mHandler.postDelayed(mRunnable, mInterval);
          }
        }
      }

      @Override
      public void onScrollToBounce(boolean toBegin, boolean toEnd) {
        if (mEnableScrollToBounce) {
          LynxDetailEvent event = new LynxDetailEvent(getSign(), BIND_SCROLL_TO_BOUNCE);
          event.addDetail("isToBegin", toBegin);
          event.addDetail("isToEnd", toEnd);
          if (getLynxContext() != null) {
            getLynxContext().getEventEmitter().sendCustomEvent(event);
          }
        }
      }
    });
    swiperView.addOnAttachStateChangeListener(new View.OnAttachStateChangeListener() {
      @Override
      public void onViewAttachedToWindow(View v) {
        mAttachedToWindow = true;
        if (mAutoPlay) {
          mHandler.removeCallbacks(mRunnable);
          mHandler.postDelayed(mRunnable, mInterval);
        }
      }

      @Override
      public void onViewDetachedFromWindow(View v) {
        if (mScrollBeforeDetached && mView.getViewPager().mTriggerEvent) {
          // When viewpager is scrolling and detached from window, let viewpager scroll to
          // target position directly.
          mView.getViewPager().scrollToFinalPositionDirectly();
        }
        mAttachedToWindow = false;
        mHandler.removeCallbacks(mRunnable);
      }
    });
    LLog.i(TAG, "create Android NewSwiperView");
    return swiperView;
  }

  private void onContentSizeChanged(float width, float height) {
    if (mEnableContentSizeChanged && mContext.getScreenMetrics() != null) {
      LynxDetailEvent event = new LynxDetailEvent(getSign(), BIND_CONTENT_SIZE_CHANGED);
      event.addDetail("contentWidth", PixelUtils.pxToDip(width));
      event.addDetail("contentHeight", PixelUtils.pxToDip(height));
      if (getLynxContext() != null) {
        getLynxContext().getEventEmitter().sendCustomEvent(event);
      }
    }
  }

  @Override
  public void insertChild(LynxBaseUI child, int index) {
    if (child instanceof LynxUI) {
      mChildren.add(index, child);
      child.setParent(this);
      mChildrenList.add(index, ((LynxUI<?>) child).getView());
      setAdapter();
      getView().addIndicator();
      applyModeInternal(false, false, true);
    }
  }

  @Override
  public void removeChild(LynxBaseUI child) {
    if (child instanceof LynxUI) {
      mChildren.remove(child);
      mChildrenList.remove(((LynxUI<?>) child).getView());
      setAdapter();
      getView().removeIndicator();
      applyModeInternal(false, false, true);
    }
  }

  @Override
  public void setEvents(Map<String, EventsListener> events) {
    super.setEvents(events);
    if (events != null) {
      mEnableChangeEvent = events.containsKey(BIND_CHANGE);
      mEnableScrollStart = events.containsKey(BIND_SCROLL_START);
      mEnableScrollEnd = events.containsKey(BIND_SCROLL_END);
      mEnableTransitionEvent = events.containsKey(BIND_TRANSITION);
      mEnableScrollToBounce = events.containsKey(BIND_SCROLL_TO_BOUNCE);
      mEnableContentSizeChanged = events.containsKey(BIND_CONTENT_SIZE_CHANGED);
    }
  }

  // Merge all layout-related properties and flush to ViewPager
  @Override
  public void onPropsUpdated() {
    if (DEBUG) {
      LLog.i(TAG, "onPropsUpdated with mLayoutPropsChanged = " + mLayoutPropsChanged);
    }
    super.onPropsUpdated();
    if (mLayoutPropsChanged) {
      applyModeInternal(false, false, true);
      mLayoutPropsChanged = false;
    }
    createAccessibilityDelegateIfNeeded();
  }

  @Override
  public void onLayoutUpdated() {
    int contentWidth = getWidth();
    int contentHeight = getHeight();
    if (DEBUG) {
      LLog.i(TAG, "onLayoutUpdated: " + contentWidth + ", " + contentHeight);
    }
    // update padding / border
    super.onLayoutUpdated();
    int paddingTop = mPaddingTop + mBorderTopWidth;
    int paddingBottom = mPaddingBottom + mBorderBottomWidth;
    int paddingLeft = mPaddingLeft + mBorderLeftWidth;
    int paddingRight = mPaddingRight + mBorderRightWidth;
    mView.setPadding(paddingLeft, paddingTop, paddingRight, paddingBottom);
    // reset adapter
    mChildrenList.clear();
    for (LynxBaseUI mChild : mChildren) {
      mChildrenList.add(((LynxUI<?>) mChild).getView());
    }
    setAdapter();
    mView.setIsRtl(isRtl());
    if (getOverflow() != 0x00) {
      mView.setClipChildren(false);
    }
    // apply mode
    applyModeInternal(mContentWidth != contentWidth, mContentHeight != contentHeight, false);
    // trigger content size change
    if (mContentWidth != contentWidth || mContentHeight != contentHeight) {
      onContentSizeChanged(contentWidth, contentHeight);
      mContentWidth = contentWidth;
      mContentHeight = contentHeight;
    }
  }

  @Override
  public void onNodeReload() {
    super.onNodeReload();
    setCurrentIndexInner(0, false);
  }

  @Override
  public boolean isScrollContainer() {
    return true;
  }

  @Override
  public boolean needCustomLayout() {
    return true;
  }

  @Override
  public boolean isScrollable() {
    return true;
  }

  @Override
  public void setLynxDirection(int direction) {
    super.setLynxDirection(direction);
    if (direction == StyleConstants.DIRECTION_RTL
        || direction == StyleConstants.DIRECTION_LYNXRTL) {
      getView().setIsRtl(true);
    } else {
      getView().setIsRtl(false);
    }
    mLayoutPropsChanged = true;
  }

  /** ---------- Accessibility Section ---------- */

  @Override
  public boolean isAccessibilityHostUI() {
    return true;
  }

  @Override
  public boolean isAccessibilityDirectionVertical() {
    return mIsVertical;
  }

  @Override
  public View getAccessibilityHostView() {
    return mView.getViewPager();
  }

  /**
   * Create and register accessibility delegate for viewpager if needed
   */
  private void createAccessibilityDelegateIfNeeded() {
    LynxAccessibilityWrapper wrapper = mContext.getLynxAccessibilityWrapper();
    if (wrapper != null && (wrapper.enableDelegate() || wrapper.enableHelper())) {
      if (mDelegate == null) {
        mDelegate = new CustomSwiperAccessibilityDelegate();
      }
      final ViewPager viewPager = mView.getViewPager();
      if (viewPager != null) {
        // Set delegate to the viewPager and accessibility important for viewPager and swiperView.
        ViewCompat.setAccessibilityDelegate(viewPager, mDelegate);
        ViewCompat.setImportantForAccessibility(
            viewPager, ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_YES);
        ViewCompat.setImportantForAccessibility(mView, ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_NO);
      }
    }
  }

  class CustomSwiperAccessibilityDelegate extends AccessibilityDelegateCompat {
    @Override
    public void onInitializeAccessibilityEvent(View host, AccessibilityEvent event) {
      super.onInitializeAccessibilityEvent(host, event);
      ViewPager viewPager = mView.getViewPager();
      if (viewPager != null) {
        event.setClassName(ViewPager.class.getName());
        final boolean canScroll = canScroll();
        // Sets the source is scrollable.
        event.setScrollable(canScroll);
        if (canScroll && event.getEventType() == AccessibilityEvent.TYPE_VIEW_SCROLLED) {
          // Sets the number of items that can be visited.
          event.setItemCount(mChildrenList.size());
        }
      }
    }

    @Override
    public void onInitializeAccessibilityNodeInfo(View host, AccessibilityNodeInfoCompat info) {
      super.onInitializeAccessibilityNodeInfo(host, info);
      info.setClassName(ViewPager.class.getName());
      final boolean canScroll = canScroll();
      info.setScrollable(canScroll);
      // Obtain and set the collectionInfo which means the current node is a collection. A
      // collection of items has rows and columns and may be hierarchical. For example, a horizontal
      // list is a collection with one column and many rows.
      final AccessibilityNodeInfoCompat.CollectionInfoCompat collectionInfo =
          AccessibilityNodeInfoCompat.CollectionInfoCompat.obtain(getRowCountForAccessibility(),
              getColumnCountForAccessibility(), false,
              AccessibilityNodeInfoCompat.CollectionInfoCompat.SELECTION_MODE_NONE);
      info.setCollectionInfo(collectionInfo);
      final ViewPager viewPager = mView.getViewPager();
      // Add actions that can be performed on the current node.
      if (canScroll && viewPager != null) {
        if (mIsVertical) {
          // Note: here we can not use if ... elseif ... because swiper can both scroll vertically
          // and horizontally.
          if (viewPager.canScrollVertically(1)) {
            info.addAction(AccessibilityNodeInfoCompat.ACTION_SCROLL_FORWARD);
          }
          if (viewPager.canScrollVertically(-1)) {
            info.addAction(AccessibilityNodeInfoCompat.ACTION_SCROLL_BACKWARD);
          }
        } else {
          // Note: here we can not use if ... elseif ... because swiper can both scroll vertically
          // and horizontally.
          if (viewPager.canScrollHorizontally(1)) {
            info.addAction(AccessibilityNodeInfoCompat.ACTION_SCROLL_FORWARD);
          }
          if (viewPager.canScrollHorizontally(-1)) {
            info.addAction(AccessibilityNodeInfoCompat.ACTION_SCROLL_BACKWARD);
          }
        }
      }
    }

    @Override
    public boolean performAccessibilityAction(View host, int action, Bundle args) {
      if (super.performAccessibilityAction(host, action, args)) {
        return true;
      }
      final ViewPager viewPager = mView.getViewPager();
      if (viewPager != null && viewPager.getCurrentIndex() != INIT_ITEM_INDEX) {
        final int currentIndex = viewPager.getCurrentIndex();
        switch (action) {
          case AccessibilityNodeInfoCompat.ACTION_SCROLL_FORWARD:
            if (canScrollViewPager(viewPager, 1)) {
              setCurrentIndex(currentIndex + 1);
              return true;
            }
            return false;
          case AccessibilityNodeInfoCompat.ACTION_SCROLL_BACKWARD:
            if (canScrollViewPager(viewPager, -1)) {
              setCurrentIndex(currentIndex - 1);
              return true;
            }
            return false;
        }
      }
      return false;
    }

    private int getRowCountForAccessibility() {
      final ViewPager viewPager = mView.getViewPager();
      if (viewPager == null || mChildrenList == null || mChildrenList.isEmpty()) {
        return 1;
      }
      return mIsVertical ? mChildrenList.size() : 1;
    }

    private int getColumnCountForAccessibility() {
      final ViewPager viewPager = mView.getViewPager();
      if (viewPager == null || mChildrenList == null || mChildrenList.isEmpty()) {
        return 1;
      }
      return mIsVertical ? 1 : mChildrenList.size();
    }

    private boolean canScroll() {
      return mTouchable && mView.getViewPager() != null && mChildrenList != null
          && mChildrenList.size() > 1;
    }

    private boolean canScrollViewPager(final ViewPager viewPager, final int direction) {
      return viewPager != null
          && ((mIsVertical && viewPager.canScrollVertically(direction))
              || ((!mIsVertical && viewPager.canScrollHorizontally(direction))));
    }
  }

  /** ---------- Accessibility Section End ---------- */

  private void setAdapter() {
    getView().getViewPager().setAdapter(new ViewPager.Adapter() {
      @Override
      public int getCount() {
        return mChildrenList.size();
      }

      @Override
      public View get(ViewGroup container, int position) {
        return mChildrenList.get(position);
      }

      @Override
      public void recycle(ViewGroup container, int position, View view) {}
    });
  }

  private void applyModeInternal(
      boolean hLayoutUpdate, boolean vLayoutUpdate, boolean propsUpdated) {
    final ViewPager viewPager = getView().getViewPager();
    final int contentSize = getContentSize(mIsVertical);
    if (contentSize == 0) {
      return;
    }
    if (DEBUG) {
      LLog.i(TAG,
          "applyModeInternal: contentSize = " + contentSize + ", hLayoutUpdated = " + hLayoutUpdate
              + ", vLayoutUpdated = " + vLayoutUpdate + ", propsUpdated = " + propsUpdated
              + ", mode = " + mMode);
    }
    viewPager.setHLayoutUpdated(hLayoutUpdate);
    viewPager.setVLayoutUpdated(vLayoutUpdate);
    viewPager.setPropsUpdated(propsUpdated);
    viewPager.setPageMargin(mPageMargin);
    // Note the setting order: setPageSize() takes precedence over settings.
    switch (mMode) {
      case MODE_DEFAULT:
        viewPager.setPageSize(contentSize);
        viewPager.setTransformer(null);
        viewPager.setOffset(0, false);
        break;
      case MODE_CAROUSEL:
        viewPager.setPageSize((int) Math.ceil(contentSize * 0.8f));
        viewPager.setTransformer(null);
        viewPager.setOffset(0, false);
        break;
      case MODE_COVER_FLOW:
        viewPager.setTransformer(mCoverFlowTransformer);
        if (setOffsetIfNeeded(viewPager, contentSize)) {
          viewPager.setPageSize((int) Math.ceil(contentSize * 0.6f));
          int offset = (int) (contentSize * 0.4f / 2);
          if (isRtl() && !mIsVertical) {
            viewPager.setOffset(-offset, false);
          } else {
            viewPager.setOffset(offset, false);
          }
        }
        break;
      case MODE_FLAT_COVER_FLOW:
        viewPager.setTransformer(null);
        if (setOffsetIfNeeded(viewPager, contentSize)) {
          viewPager.setPageSize((int) Math.ceil(contentSize * 0.6f));
          int offset = (int) (contentSize * 0.4f / 2);
          if (isRtl() && !mIsVertical) {
            viewPager.setOffset(-offset, false);
          } else {
            viewPager.setOffset(offset, false);
          }
        }
        break;
      case MODE_CARRY:
        viewPager.setTransformer(mCarryTransformer);
        if (setOffsetIfNeeded(viewPager, contentSize)) {
          viewPager.setPageSize(contentSize);
          viewPager.setOffset(0, false);
        }
        break;
    }
    viewPager.requestLayout();
  }

  private boolean setOffsetIfNeeded(ViewPager viewPager, int contentSize) {
    int remain = contentSize - mPreviousMargin - mNextMargin - mPageMargin - mPageMargin;
    if (!mCompatible) {
      // In refactor, we shouldn't use pageMargin to calculate page size.
      remain = contentSize - mPreviousMargin - mNextMargin;
    }
    if (mPreviousMargin >= 0 && mNextMargin >= 0 && remain > 0) {
      viewPager.setPageSize(remain);
      int offset = mPreviousMargin + mPageMargin;
      if (!mCompatible) {
        // In refactor, the pageMargin should not be considered in calculating first page offset
        offset = mPreviousMargin;
      }
      if (isRtl() && !mIsVertical) {
        viewPager.setOffset(-offset, false);
      } else {
        viewPager.setOffset(offset, false);
      }
      return false;
    }
    return true;
  }

  // Get the page size according to the css layout result
  private int getContentSize(boolean isVertical) {
    if (isVertical) {
      return getHeight() - getPaddingTop() - getPaddingBottom() - getBorderTopWidth()
          - getBorderBottomWidth();
    } else {
      return getWidth() - getPaddingLeft() - getPaddingRight() - getBorderLeftWidth()
          - getBorderRightWidth();
    }
  }

  @LynxUIMethod
  public void scrollTo(ReadableMap params, Callback callback) {
    final ViewPager viewPager = getView().getViewPager();
    if (viewPager == null || viewPager.getAdapter() == null) {
      callback.invoke(LynxUIMethodConstants.UNKNOWN,
          "Check failed when invoking scrollTo method: viewPager == null || adapter == null");
      return;
    }
    int index = params.getInt(METHOD_PARAMS_INDEX, -1);
    boolean smooth = params.getBoolean(METHOD_PARAMS_SMOOTH, mSmoothScroll);
    String direction = params.getString(METHOD_PARAMS_DIRECTION, METHOD_PARAMS_DIRECTION_END);
    int directionFlag = direction.equals(METHOD_PARAMS_DIRECTION_BEGIN) ? SCROLL_DIRECTION_BEGIN
                                                                        : SCROLL_DIRECTION_END;
    if (viewPager.getChildCount() == 0) {
      callback.invoke(LynxUIMethodConstants.UNKNOWN,
          "Check failed when invoking scrollTo method: no swiper item added to viewpager");
      return;
    }
    if (index < 0 || index >= viewPager.getTotalCount()) {
      callback.invoke(LynxUIMethodConstants.PARAM_INVALID,
          "Check failed when invoking scrollTo method: index < 0 or index >= data count");
      return;
    }
    setIndexImpl(viewPager, index, smooth, directionFlag);
    callback.invoke(LynxUIMethodConstants.SUCCESS);
  }

  @LynxProp(name = "mode")
  public void setMode(String mode) {
    mMode = mode;
    mLayoutPropsChanged = true;
  }

  @LynxProp(name = "autoplay", defaultBoolean = false)
  public void setAutoPlay(boolean autoPlay) {
    mAutoPlay = autoPlay;
    mHandler.removeCallbacks(mRunnable);
    if (mAutoPlay) {
      mHandler.postDelayed(mRunnable, mInterval);
    }
  }

  @LynxProp(name = "finish-reset", defaultBoolean = false)
  public void setFinishReset(boolean isReset) {
    mFinishReset = isReset;
  }

  protected void setCurrentIndexInner(final int index, boolean smooth) {
    final ViewPager viewPager = getView().getViewPager();
    if (DEBUG) {
      LLog.i(TAG,
          "LynxProp setCurrentIndex: " + index
              + ", with showing child view count = " + viewPager.getChildCount()
              + ", and adapter's child count = " + mChildrenList.size());
    }
    // Note: Use viewPager.getChildCount() > 0 to determine whether current can be set directly.
    if (viewPager.getChildCount() > 0) {
      viewPager.setPendingCurrentIndex(index, smooth);
      setIndex(viewPager, index, smooth, false);
    } else {
      viewPager.setPendingCurrentIndex(index, false);
    }
  }

  @LynxProp(name = "current", defaultInt = 0)
  public void setCurrentIndex(final int index) {
    setCurrentIndexInner(index, mSmoothScroll);
  }

  @Override
  public void onListCellAppear(String itemKey, UIList list) {
    super.onListCellPrepareForReuse(itemKey, list);
    if (TextUtils.isEmpty(itemKey)) {
      return;
    }
    String cacheKey = constructListStateCacheKey(getTagName(), itemKey, getIdSelector());
    int index = list.nativeListStateCache.containsKey(cacheKey)
        ? (int) list.nativeListStateCache.get(cacheKey)
        : -1;
    setCurrentIndexInner(index, false);
  }

  @Override
  public void onListCellPrepareForReuse(String itemKey, LynxBaseUI list) {}

  @Override
  public void onListCellDisAppear(String itemKey, LynxBaseUI list, boolean isExist) {
    super.onListCellDisAppear(itemKey, list, isExist);
    if (TextUtils.isEmpty(itemKey)) {
      return;
    }
    String cacheKey = constructListStateCacheKey(getTagName(), itemKey, getIdSelector());
    if (isExist) {
      list.storeKeyToNativeStorage(cacheKey, mView.getViewPager().getCurrentIndex());
    } else {
      list.removeKeyFromNativeStorage(cacheKey);
    }
  }

  private void setIndex(
      final ViewPager viewPager, int index, boolean isSmoothScroll, boolean resetToFirst) {
    int totalCount = viewPager.getTotalCount();
    int mCurrentIndex = viewPager.getCurrentIndex();
    if (mCircular && index == 0 && mCurrentIndex == totalCount - 1) {
      setIndexImpl(viewPager, index, isSmoothScroll,
          (totalCount > 2 || resetToFirst) ? SCROLL_DIRECTION_END
                                           : SCROLL_DIRECTION_BEGIN); // totalCount - 1 -> 0
    } else if (mCircular && index == totalCount - 1 && mCurrentIndex == 0) {
      setIndexImpl(viewPager, index, isSmoothScroll,
          totalCount > 2 ? SCROLL_DIRECTION_BEGIN : SCROLL_DIRECTION_END); // 0 -> totalCount - 1
    } else {
      setIndexImpl(viewPager, index, isSmoothScroll,
          index < mCurrentIndex ? SCROLL_DIRECTION_BEGIN : SCROLL_DIRECTION_END);
    }
  }

  private void setIndexImpl(
      final ViewPager viewPager, int index, boolean isSmoothScroll, int direction) {
    int totalCount = viewPager.getTotalCount();
    if (index >= 0 && index < totalCount) {
      viewPager.setCurrentIndex(index, isSmoothScroll, direction);
    }
  }

  @LynxProp(name = "page-margin")
  public void setPageMargin(Dynamic pageMargin) {
    if (pageMargin.getType() == ReadableType.String) {
      String pageMarginStrValue = pageMargin.asString();
      if (pageMarginStrValue.endsWith("px") || pageMarginStrValue.endsWith("rpx")) {
        int margin = (int) UnitUtils.toPxWithDisplayMetrics(
            pageMarginStrValue, 0, 0, 0, 0, 0.f, getLynxContext().getScreenMetrics());
        mPageMargin = margin > 0 ? margin : 0;
        mLayoutPropsChanged = true;
      }
    }
  }

  @LynxProp(name = "previous-margin")
  public void setPreviousMargin(Dynamic previousMarginValue) {
    ReadableType type = previousMarginValue.getType();
    if (type != ReadableType.String) {
      return;
    }
    String previousMarginStrValue = previousMarginValue.asString();
    if (previousMarginStrValue.endsWith("px") || previousMarginStrValue.endsWith("rpx")) {
      int value = (int) UnitUtils.toPxWithDisplayMetrics(
          previousMarginStrValue, 0, 0, 0, 0, -1.0f, getLynxContext().getScreenMetrics());
      mPreviousMargin = value >= 0 ? value : -1;
      mLayoutPropsChanged = true;
    }
  }

  @LynxProp(name = "next-margin")
  public void setNextMargin(Dynamic nextMarginValue) {
    ReadableType type = nextMarginValue.getType();
    if (type != ReadableType.String) {
      return;
    }
    String nextMarginStrValue = nextMarginValue.asString();
    if (nextMarginStrValue.endsWith("px") || nextMarginStrValue.endsWith("rpx")) {
      int value = (int) UnitUtils.toPxWithDisplayMetrics(
          nextMarginStrValue, 0, 0, 0, 0, -1.0f, getLynxContext().getScreenMetrics());
      mNextMargin = value >= 0 ? value : -1;
      mLayoutPropsChanged = true;
    }
  }

  @Deprecated
  @LynxProp(name = "orientation")
  public void setOrientation(String orientation) {
    if ("vertical".equals(orientation)) {
      mIsVertical = true;
      getView().setOrientation(SwiperView.ORIENTATION_VERTICAL);
    } else if ("horizontal".equals(orientation)) {
      mIsVertical = false;
      getView().setOrientation(SwiperView.ORIENTATION_HORIZONTAL);
    }
    mLayoutPropsChanged = true;
  }

  @LynxProp(name = "vertical", defaultBoolean = false)
  public void setVertical(boolean isVertical) {
    if (isVertical) {
      mView.setOrientation(SwiperView.ORIENTATION_VERTICAL);
    } else {
      mView.setOrientation(SwiperView.ORIENTATION_HORIZONTAL);
    }
    mIsVertical = isVertical;
    mLayoutPropsChanged = true;
  }

  @LynxProp(name = "norm-translation-factor", defaultFloat = 0.f)
  public void setNormalTranslationFactor(double factor) {
    if (factor <= 1. && factor >= -1.) {
      mCarryTransformer.setNormTranslationFactor((float) factor);
      mLayoutPropsChanged = true;
    }
  }

  @LynxProp(name = "interval", defaultInt = 5000)
  public void setInterval(int interval) {
    mInterval = interval;
  }

  @LynxProp(name = "duration", defaultInt = 500)
  public void setDuration(int duration) {
    mDuration = duration;
    if (mSmoothScroll) {
      getView().getViewPager().setAnimDuration(duration);
    } else {
      getView().getViewPager().setAnimDuration(0);
    }
  }

  @LynxProp(name = "circular", defaultBoolean = false)
  public void setCircular(boolean circular) {
    mCircular = circular;
    mView.getViewPager().setLoop(circular);
  }

  @LynxProp(name = "touchable", defaultBoolean = false)
  public void setTouchable(boolean touchable) {
    mTouchable = touchable;
    getView().getViewPager().setTouchable(touchable);
  }

  @LynxProp(name = "smooth-scroll", defaultBoolean = true)
  public void setSmoothScroll(boolean smoothScroll) {
    this.mSmoothScroll = smoothScroll;
    if (mSmoothScroll) {
      getView().getViewPager().setAnimDuration(mDuration);
    } else {
      getView().getViewPager().setAnimDuration(0);
    }
  }

  @LynxProp(name = "indicator-dots", defaultBoolean = false)
  public void setIndicator(boolean enable) {
    getView().enableIndicator(enable);
  }

  @LynxProp(name = "indicator-color")
  public void setIndicatorColor(String color) {
    int value;
    try {
      value = ColorUtils.parse(color);
    } catch (Exception e) {
      value = UNSELECTED_COLOR;
    }
    getView().setUnSelectedColor(value);
  }

  @LynxProp(name = "indicator-active-color")
  public void setIndicatorActiveColor(String color) {
    int value;
    try {
      value = ColorUtils.parse(color);
    } catch (Exception e) {
      value = SELECTED_COLOR;
    }
    getView().setSelectedColor(value);
  }

  @LynxProp(name = "keep-item-view", defaultBoolean = false)
  public void setKeepItemView(boolean value) {
    mView.getViewPager().setKeepItemView(value);
  }

  @LynxProp(name = "force-can-scroll", defaultBoolean = false)
  public void setForceCanScroll(boolean canScroll) {
    final ViewPager viewPager = mView.getViewPager();
    viewPager.setForceCanScroll(canScroll);
  }

  @LynxProp(name = "compatible", defaultBoolean = true)
  public void setCompatible(boolean value) {
    mCompatible = value;
    mLayoutPropsChanged = true;
  }

  @LynxProp(name = "enable-vice-loop", defaultBoolean = true)
  public void setEnableViceLoop(boolean value) {
    final ViewPager viewPager = getView().getViewPager();
    viewPager.setEnableViceLoop(value);
  }

  @LynxProp(name = "enable-bounce", defaultBoolean = false)
  public void setEnableBounce(final boolean value) {
    final ViewPager viewPager = getView().getViewPager();
    viewPager.setEnableBounce(value);
  }

  @LynxProp(name = "bounce-begin-threshold", defaultFloat = 0.3f)
  public void setBounceBeginThreshold(final float value) {
    final ViewPager viewPager = getView().getViewPager();
    viewPager.setBounceBeginThreshold(value);
  }

  @LynxProp(name = "bounce-end-threshold", defaultFloat = 0.3f)
  public void setBounceEndThreshold(final float value) {
    final ViewPager viewPager = getView().getViewPager();
    viewPager.setBounceEndThreshold(value);
  }

  @LynxProp(name = "bounce-duration", defaultInt = 500)
  public void setBounceDuration(int bounceDuration) {
    getView().getViewPager().setBounceDuration(bounceDuration);
  }

  @LynxProp(name = "ignore-layout-update", defaultBoolean = false)
  public void setIgnoreLayoutUpdate(boolean value) {
    getView().getViewPager().setIgnoreLayoutUpdate(value);
  }

  @LynxProp(name = "scroll-before-detached", defaultBoolean = false)
  public void setScrollBeforeDetached(boolean value) {
    mScrollBeforeDetached = value;
  }

  @LynxProp(name = "max-x-scale")
  public void setMaxXScale(double scale) {
    mCarryTransformer.setMaxScaleX((float) scale);
  }

  @LynxProp(name = "min-x-scale")
  public void setMinXScale(double scale) {
    mCarryTransformer.setMinScaleX((float) scale);
  }

  @LynxProp(name = "max-y-scale")
  public void setMaxYScale(double scale) {
    mCarryTransformer.setMaxScaleY((float) scale);
  }

  @LynxProp(name = "min-y-scale")
  public void setMinYScale(double scale) {
    mCarryTransformer.setMinScaleY((float) scale);
  }

  @LynxProp(name = "transition-throttle", defaultInt = 0)
  public void setTransitionThrottle(int value) {
    mTransitionThrottle = value;
  }

  @LynxProp(name = "handle-gesture", defaultBoolean = true)
  public void setHandleGesture(boolean value) {
    getView().getViewPager().setHandleGesture(value);
  }

  @Override
  public boolean enableAutoClipRadius() {
    return true;
  }

  /**
   * @name: enable-nested-child
   * @description: Check scrollable within child views. If any child can scroll, viewpager will not
   *               intercept ACTION_MOVE to let child consume.
   * @category: different
   * @standardAction: keep
   * @supportVersion: 2.10
   **/
  @LynxProp(name = "enable-nested-child", defaultBoolean = false)
  public void setEnableNestedChild(boolean value) {
    final ViewPager viewPager = mView.getViewPager();
    if (viewPager != null) {
      viewPager.setEnableNestedChild(value);
    }
  }
}
