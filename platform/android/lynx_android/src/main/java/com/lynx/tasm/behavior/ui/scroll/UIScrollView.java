// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.scroll;

import static androidx.core.view.ViewCompat.LAYOUT_DIRECTION_LTR;
import static androidx.core.view.ViewCompat.LAYOUT_DIRECTION_RTL;
import static com.lynx.tasm.behavior.StyleConstants.DIRECTION_RTL;
import static com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityDelegate.DEBUG;
import static com.lynx.tasm.behavior.ui.scroll.AndroidScrollView.SCROLL_NESTED_SCROLL;
import static com.lynx.tasm.behavior.ui.scroll.AndroidScrollView.SCROLL_STATE_ANIMATION;
import static com.lynx.tasm.behavior.ui.scroll.AndroidScrollView.SCROLL_STATE_DRAGGING;
import static com.lynx.tasm.behavior.ui.scroll.AndroidScrollView.SCROLL_STATE_IDLE;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.View;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;
import android.widget.LinearLayout;
import androidx.annotation.MainThread;
import androidx.annotation.Nullable;
import androidx.core.math.MathUtils;
import androidx.core.view.AccessibilityDelegateCompat;
import androidx.core.view.ViewCompat;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.LynxViewClient;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.LynxUIMethod;
import com.lynx.tasm.behavior.LynxUIMethodConstants;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxFlattenUI;
import com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper;
import com.lynx.tasm.behavior.ui.list.UIList;
import com.lynx.tasm.behavior.ui.list.container.UIListContainer;
import com.lynx.tasm.behavior.ui.utils.LynxUIHelper;
import com.lynx.tasm.event.*;
import com.lynx.tasm.gesture.GestureArenaMember;
import com.lynx.tasm.gesture.LynxNewGestureDelegate;
import com.lynx.tasm.gesture.arena.GestureArenaManager;
import com.lynx.tasm.gesture.detector.GestureDetector;
import com.lynx.tasm.gesture.handler.BaseGestureHandler;
import com.lynx.tasm.utils.PixelUtils;
import com.lynx.tasm.utils.UIThreadUtils;
import com.lynx.tasm.utils.UnitUtils;
import java.util.HashMap;
import java.util.Map;

public class UIScrollView extends AbsLynxUIScroll<AndroidScrollView>
    implements IScrollSticky, GestureArenaMember, LynxNewGestureDelegate {
  protected static final String TAG = "LynxUIScrollView";
  protected static final String LynxScrollViewInitialScrollOffset = "initialScrollOffset";
  protected static final String LynxScrollViewInitialScrollIndex = "initialScrollIndex";
  private static final String ACCESSIBILITY_TAG = "LynxAccessibilityScrollView";
  private static final String EVENT_CONTENT_SIZE_CHANGED = "contentsizechanged";
  static final int INVALID_INDEX = -1;
  private boolean mEnableScrollY;
  private boolean mEnableScrollToUpperEvent;
  boolean mEnableScrollToUpperEdgeEvent = false;
  private boolean mEnableScrollToLowerEvent;
  boolean mEnableScrollToLowerEdgeEvent = false;
  boolean mEnableScrollToNormalStateEvent = false;
  private boolean mEnableScrollEvent;
  private boolean mEnableScrollStartEvent;
  private boolean mEnableScrollEndEvent;
  private boolean mEnableScrollTap;
  private boolean mEnableContentSizeChangedEvent;
  private boolean mEnableSticky = false;

  // for new gesture
  private Map<Integer, BaseGestureHandler> mGestureHandlers;

  protected boolean mPreferenceConsumeGesture = false;
  // scroll-top/scroll-left is props set by FE before scroll-view layout do not take effect, should
  // pending and consume when layout
  private int mPendingScrollOffset = 0;
  private int mPendingInitialScrollOffset = 0;
  private int mLowerThreshold = 0;
  private int mUpperThreshold = 0;
  private int mPreviousStatus = BORDER_STATUS_UPPER;
  // bit set 0x1 upper; 0x2 lower
  private static final int BORDER_STATUS_UPPER = 0x1;
  private static final int BORDER_STATUS_LOWER = 0x2;
  int mPendingScrollToIndex = INVALID_INDEX;
  private int mPendingInitialScrollToIndex = INVALID_INDEX;

  // support bounceView
  private UIBounceView mBounceView;
  private UIBounceView mStartUIBounce;
  private UIBounceView mEndUIBounce;
  private boolean mEnableNewBounce = false;

  private CustomUIScrollViewAccessibilityDelegate mDelegate = null;

  private boolean mEnableScroll = true;

  private boolean mUsePagingTouchSlop = false;

  private int mFadingEdgeLength = 0;

  private Callback mScrollToCallback;

  private String mScrollToCallbackInfo;

  // For list native storage
  private int mListSign;
  private String mCurrentItemKey;
  // value may not be the latest as reused UI with same prop will not update mPropMap. Instead, it
  // will update through onNodeReady
  private HashMap<String, Integer> mPropMap = new HashMap<>();

  public UIScrollView(LynxContext context) {
    super(context);
  }

  @Override
  protected AndroidScrollView createView(Context context) {
    final AndroidScrollView view = new AndroidScrollView(context, this);
    view.setOnScrollListener(new AndroidScrollView.OnScrollListener() {
      @Override
      public void onFling(int velocity) {
        View view = mView;
        if (view == null) {
          return;
        }
        if (isEnableScrollMonitor()) {
          getLynxContext().getLynxViewClient().onFling(
              new LynxViewClient.ScrollInfo(view, getTagName(), getScrollMonitorTag()));
        }
      }

      @Override
      public void onScrollStart() {
        getLynxContext().getFluencyTraceHelper().start(getSign(), "scroll", getScrollMonitorTag());
        if (mEnableSticky) {
          onScrollSticky();
        }
        if (mEnableScrollStartEvent) {
          sendCustomEvent(getScrollX(), getScrollY(), getScrollX(), getScrollY(),
              LynxScrollEvent.EVENT_SCROLL_START);
        }
        if (isEnableScrollMonitor()) {
          getLynxContext().getLynxViewClient().onScrollStart(
              new LynxViewClient.ScrollInfo(view, getTagName(), getScrollMonitorTag()));
        }
      }

      @Override
      public void onScrollStop() {
        getLynxContext().getFluencyTraceHelper().stop(getSign());
        if (mEnableScrollEndEvent) {
          sendCustomEvent(getScrollX(), getScrollY(), getScrollX(), getScrollY(),
              LynxScrollEvent.EVENT_SCROLL_END);
        }
        if (isEnableScrollMonitor() && getLynxContext().getLynxViewClient() != null) {
          getLynxContext().getLynxViewClient().onScrollStop(
              new LynxViewClient.ScrollInfo(view, getTagName(), getScrollMonitorTag()));
        }
      }

      @Override
      public void onScrollChanged(int l, int t, int oldl, int oldt) {
        if (mEnableSticky) {
          onScrollSticky();
        }

        if (mEnableScrollEvent) {
          sendCustomEvent(l, t, oldl, oldt, LynxScrollEvent.EVENT_SCROLL);
        }

        if (mEnableScrollToLowerEvent || mEnableScrollToUpperEvent) {
          int status = updateBorderStatus(l, t);
          if (mEnableScrollToLowerEvent && isLower(status) && !isLower(mPreviousStatus)) {
            sendCustomEvent(getScrollX(), getScrollY(), getScrollX(), getScrollY(),
                LynxScrollEvent.EVENT_SCROLL_TOLOWER);
          } else if (mEnableScrollToUpperEvent && isUpper(status) && !isUpper(mPreviousStatus)) {
            sendCustomEvent(getScrollX(), getScrollY(), getScrollX(), getScrollY(),
                LynxScrollEvent.EVENT_SCROLL_TOUPPER);
          }
          mPreviousStatus = status;
        }
      }

      @Override
      public void onScrollStateChanged(int state) {
        LLog.i(TAG, "onScrollStateChanged: " + state);
        recognizeGestureInternal(state);
        if (mScrollToCallback != null && state == SCROLL_STATE_IDLE
            && mView.mLastScrollState == SCROLL_STATE_ANIMATION) {
          // Note: Here need temporarily store lastScrollToCallback and reset mScrollToCallback to
          // null to avoid invoking mScrollToCallback twice.
          Callback lastScrollToCallback = mScrollToCallback;
          String lastScrollToCallbackInfo = mScrollToCallbackInfo;
          mScrollToCallback = null;
          mScrollToCallbackInfo = "";
          lastScrollToCallback.invoke(LynxUIMethodConstants.SUCCESS, lastScrollToCallbackInfo);
        }
      }
    });
    return view;
  }

  protected void recognizeGestureInternal(int state) {
    if (!mEnableScrollTap) {
      // Default behavior align with iOS:
      // (1) If in fling or dragged status, the component in scroll-view will not respond to tap
      // event. (2) If invoke autoScroll or scrollTo method (in state animation), the component in
      // scroll-view will respond to tap event.
      if (state != SCROLL_STATE_IDLE && state != SCROLL_STATE_ANIMATION) {
        recognizeGesturere();
      }
    } else {
      if (state == SCROLL_STATE_DRAGGING || state == SCROLL_NESTED_SCROLL) {
        recognizeGesturere();
      }
    }
  }

  @Override
  protected View getRealParentView() {
    AndroidScrollView view = getView();
    if (!mEnableScrollY && view != null) {
      return view.getHScrollView();
    }
    return view;
  }

  @Override
  public boolean isScrollContainer() {
    return true;
  }

  @Override
  public void onLayoutUpdated() {
    super.onLayoutUpdated();
    int paddingLeft = mPaddingLeft + mBorderLeftWidth;
    int paddingRight = mPaddingRight + mBorderRightWidth;
    int paddingTop = mPaddingTop + mBorderTopWidth;
    int paddingBottom = mPaddingBottom + mBorderBottomWidth;
    mView.setPadding(paddingLeft, paddingTop, paddingRight, paddingBottom);
  }

  @Override
  public void onNodeReady() {
    super.onNodeReady();
    if (mFadingEdgeLength > 0) {
      mView.setFadingEdgeLength(mFadingEdgeLength);
      mView.setHorizontalFadingEdgeEnabled(!mEnableScrollY);
      mView.setVerticalFadingEdgeEnabled(mEnableScrollY);
    }
    if (mUsePagingTouchSlop) {
      if (!mEnableScrollY) {
        mView.getHScrollView().setPagingTouchSlopIfNeeded();
      } else {
        mView.setPagingTouchSlopIfNeeded();
      }
    }
    mFirstRender = false;
  }

  @Override
  public void insertChild(LynxBaseUI child, int index) {
    super.insertChild(child, index);
    if (child instanceof UIBounceView) {
      UIBounceView uiBounceView = (UIBounceView) child;
      if (mEnableNewBounce) {
        switch (uiBounceView.mDirection) {
          case UIBounceView.LEFT:
          case UIBounceView.TOP:
            mStartUIBounce = uiBounceView;
            break;
          case UIBounceView.RIGHT:
          case UIBounceView.BOTTOM:
            mEndUIBounce = uiBounceView;
            break;
        }
      } else {
        mBounceView = uiBounceView;
      }
    }
  }

  private void onScrollSticky() {
    final int l = getScrollX(), t = getScrollY();
    for (int index = 0; index < mChildren.size(); index++) {
      LynxBaseUI ui = mChildren.get(index);
      ui.checkStickyOnParentScroll(l, t);
    }
  }

  public void setLynxDirection(int direction) {
    mView.mDirectionChanged = direction != mLynxDirection;
    mLynxDirection = direction;
    if (direction == DIRECTION_RTL) {
      ViewCompat.setLayoutDirection(mView, LAYOUT_DIRECTION_RTL);
      ViewCompat.setLayoutDirection(mView.getHScrollView(), LAYOUT_DIRECTION_RTL);
      ViewCompat.setLayoutDirection(mView.getLinearLayout(), LAYOUT_DIRECTION_RTL);
    } else {
      ViewCompat.setLayoutDirection(mView, LAYOUT_DIRECTION_LTR);
      ViewCompat.setLayoutDirection(mView.getHScrollView(), LAYOUT_DIRECTION_LTR);
      ViewCompat.setLayoutDirection(mView.getLinearLayout(), LAYOUT_DIRECTION_LTR);
    }
  }

  @Override
  public void measure() {
    boolean isHorizontal = mView.getOrientation() == AndroidScrollView.HORIZONTAL;
    int contentWidth = getWidth();
    int contentHeight = getHeight();
    for (int i = 0; i < getChildCount(); ++i) {
      LynxBaseUI child = getChildAt(i);
      if (isHorizontal) {
        contentWidth = Math.max(contentWidth,
            child.getWidth() + child.getLeft() + child.getMarginRight() + mPaddingRight);
      } else {
        contentHeight = Math.max(contentHeight,
            child.getHeight() + child.getTop() + child.getMarginBottom() + mPaddingBottom);
      }
    }
    if (mView.getContentWidth() != contentWidth || mView.getContentHeight() != contentHeight) {
      onContentSizeChanged(contentWidth, contentHeight);
      mView.setMeasuredSize(contentWidth, contentHeight);
      mView.sendScrollToEdgeEvent(mView.getScrollX(), mView.getScrollY());
    }
    super.measure();
  }

  @Override
  public void layout() {
    if (getOverflow() != OVERFLOW_HIDDEN) {
      mView.setClipChildren(false);
      if (mView.getHScrollView() != null) {
        mView.getHScrollView().setClipChildren(false);
      }
      if (mView.getLinearLayout() != null) {
        mView.getLinearLayout().setClipToPadding(false);
      }
    }
    // Note: we should set scroll range and bounce scroll range before invoking super.layout().
    // In view#onLayout(), bounce scroll range will be consumed.
    if (mEnableNewBounce && !mEnableScrollY) {
      final int bounceScrollRange = mStartUIBounce != null ? mStartUIBounce.getWidth() : 0;
      mView.getHScrollView().setBounceScrollRange(getScrollRange(), bounceScrollRange);
    } else if (mEnableNewBounce && mEnableScrollY) {
      final int bounceScrollRange = mStartUIBounce != null ? mStartUIBounce.getHeight() : 0;
      mView.setBounceScrollRange(getScrollRange(), bounceScrollRange);
    }
    super.layout();
    // consume mPendingScrollOffset
    if (mPendingScrollOffset > 0) {
      if (mEnableScrollY && mPendingScrollOffset + getHeight() <= getView().getContentHeight()) {
        int originScrollX = getView().getRealScrollX();
        getView().setScrollTo(originScrollX, mPendingScrollOffset, false);
        mPendingScrollOffset = 0;
      } else if (!mEnableScrollY
          && mPendingScrollOffset + getWidth() <= getView().getContentWidth()) {
        int originScrollY = getView().getRealScrollY();
        getView().setScrollTo(mPendingScrollOffset, originScrollY, false);
        mPendingScrollOffset = 0;
      }
    }
  }

  @Override
  public void onNodeReload() {
    super.onNodeReload();
    if (mLynxDirection == DIRECTION_RTL) {
      getView().setScrollTo(getScrollRange(), 0, false);
    } else {
      getView().setScrollTo(0, 0, false);
    }
  }

  @Override
  public void invalidate() {
    mView.getLinearLayout().invalidate();
    mView.invalidate();
  }

  @Override
  public void destroy() {
    super.destroy();
    mScrollToCallback = null;
    mView.clearOnScrollListener();
    // remove arena member if destroy
    GestureArenaManager manager = getGestureArenaManager();
    if (manager != null) {
      manager.removeMember(this);
    }
    // clear gesture map if destroy
    if (mGestureHandlers != null) {
      mGestureHandlers.clear();
    }
  }

  @Override
  protected Rect getBoundRectForOverflow() {
    return super.getClipBounds();
  }

  @Override
  public void setScrollY(boolean enable) {
    mEnableScrollY = enable;
    handleScrollDirection();
  }

  @Override
  public void setScrollX(boolean enable) {
    mEnableScrollY = !enable;
    handleScrollDirection();
  }

  @Override
  public void setScrollBarEnable(boolean value) {
    try {
      mView.setScrollBarEnable(Boolean.valueOf(value));
    } catch (Exception e) {
      LLog.e("UIScrollView", e.getMessage());
    }
  }

  public void setEnableSticky() {
    mEnableSticky = true;
    onScrollSticky();
  }

  @Override
  public void setUpperThreshole(int value) {
    mUpperThreshold = value;
  }

  @Override
  public void setLowerThreshole(int value) {
    mLowerThreshold = value;
  }

  protected void setScrollTopInner(int value, boolean needConvertToPx, boolean isInitial) {
    int originScrollX = getView().getRealScrollX();
    int yOffset = needConvertToPx ? (int) PixelUtils.dipToPx(value) : value;
    if (!isInitial && yOffset + getHeight() <= getView().getContentHeight()) {
      getView().setScrollTo(originScrollX, yOffset, false);
      mPendingScrollOffset = 0;
    } else {
      if (isInitial) {
        setPendingInitialScrollOffset(yOffset);
      } else {
        mPendingScrollOffset = yOffset;
      }
    }
  }

  @Override
  public void setScrollTop(int value) {
    setScrollTopInner(value, true, false);
  }

  protected void setScrollLeftInner(int value, boolean needConvertToPx, boolean isInitial) {
    int originScrollY = getView().getRealScrollY();
    int xOffset = needConvertToPx ? (int) PixelUtils.dipToPx(value) : value;
    if (!isInitial && xOffset + getWidth() <= getView().getContentWidth()) {
      getView().setScrollTo(xOffset, originScrollY, false);
      mPendingScrollOffset = 0;
    } else {
      if (isInitial) {
        setPendingInitialScrollOffset(xOffset);
      } else {
        mPendingScrollOffset = xOffset;
      }
    }
  }

  @Override
  public void setScrollLeft(int value) {
    setScrollLeftInner(value, true, false);
  }

  protected void scrollToIndexInner(int index, boolean isInitial) {
    if (mView != null && mView.getLinearLayout() != null) {
      if (isInitial) {
        setPendingInitialScrollToIndex(INVALID_INDEX);
      } else {
        mPendingScrollToIndex = INVALID_INDEX;
      }
      if (index < 0) {
        LLog.e(TAG, "Invalid scroll-to-index with index < 0: " + index);
        return;
      }
      final int childrenCount = mChildren.size();
      // currently don't have children, pending to wait for children inserted
      if (childrenCount == 0) {
        if (isInitial) {
          setPendingInitialScrollToIndex(index);
        } else {
          mPendingScrollToIndex = index;
        }
      } else {
        if (index >= mChildren.size()) {
          LLog.e(TAG, "Invalid scroll-to-index with index out of boundary: " + index);
          return;
        }
        // valid scroll-to-index and children count
        if (!canInvokeScrollImmediately() || isInitial) {
          if (isInitial) {
            setPendingInitialScrollToIndex(index);
          } else {
            mPendingScrollToIndex = index;
          }
          return;
        }
        mView.setScrollToIndex(index);
      }
    }
  }

  private String constructListStateCacheKey() {
    return constructListStateCacheKey(getTagName(), mCurrentItemKey, getIdSelector());
  }

  private void setPendingInitialScrollToIndex(int index) {
    if (index == INVALID_INDEX) {
      mPendingInitialScrollToIndex = INVALID_INDEX;
    } else {
      if (getParentList() != null) {
        mPendingInitialScrollToIndex =
            getParentList().initialPropsFlushed(
                LynxScrollViewInitialScrollIndex, constructListStateCacheKey())
            ? INVALID_INDEX
            : index;
      } else {
        mPendingInitialScrollToIndex = index;
      }
    }
  }

  private void setPendingInitialScrollOffset(int offset) {
    if (offset == 0) {
      mPendingInitialScrollOffset = 0;
    } else {
      if (getParentList() != null) {
        mPendingInitialScrollOffset =
            getParentList().initialPropsFlushed(
                LynxScrollViewInitialScrollOffset, constructListStateCacheKey())
            ? 0
            : offset;
      } else {
        mPendingInitialScrollOffset = offset;
      }
    }
  }

  public void handleComputeScroll() {
    // consume pending initial-scroll-offset
    if (mPendingInitialScrollOffset > 0) {
      if (mFirstRender || getParentList() == null
          || !getParentList().initialPropsFlushed(
              LynxScrollViewInitialScrollOffset, constructListStateCacheKey())) {
        if (getParentList() != null) {
          getParentList().setInitialPropsHasFlushed(
              LynxScrollViewInitialScrollOffset, constructListStateCacheKey());
        }
        if (mEnableScrollY
            && mPendingInitialScrollOffset + getHeight() <= getView().getContentHeight()) {
          int originScrollX = getView().getRealScrollX();
          getView().setScrollTo(originScrollX, mPendingInitialScrollOffset, false);
          setPendingInitialScrollOffset(0);
        } else if (!mEnableScrollY
            && mPendingInitialScrollOffset + getWidth() <= getView().getContentWidth()) {
          int originScrollY = getView().getRealScrollY();
          getView().setScrollTo(mPendingInitialScrollOffset, originScrollY, false);
          setPendingInitialScrollOffset(0);
        }
      }
    }

    // consume pending initial-scroll-to-index
    if (mPendingInitialScrollToIndex != INVALID_INDEX) {
      if (mFirstRender || getParentList() == null
          || !getParentList().initialPropsFlushed(
              LynxScrollViewInitialScrollIndex, constructListStateCacheKey())) {
        if (getParentList() != null) {
          getParentList().setInitialPropsHasFlushed(
              LynxScrollViewInitialScrollIndex, constructListStateCacheKey());
        }
        mView.setScrollToIndex(mPendingInitialScrollToIndex);
      }
      mPendingInitialScrollToIndex = INVALID_INDEX;
    }
  }

  @Override
  public void scrollToIndex(int index) {
    scrollToIndexInner(index, false);
  }

  @LynxProp(name = "scroll-orientation", customType = "vertical")
  public void setScrollOrientation(String scrollOrientation) {
    if (TextUtils.equals(scrollOrientation, "vertical")) {
      mEnableScrollY = true;
    } else if (TextUtils.equals(scrollOrientation, "horizontal")) {
      mEnableScrollY = false;
    } else {
      mEnableScrollY = true;
    }
    handleScrollDirection();
  }

  @LynxProp(name = "enable-nested-scroll", defaultBoolean = false)
  public void setEnableNestedScroll(boolean nestedScroll) {
    if (mView == null) {
      return;
    }
    mView.setNestedScrollingEnabled(nestedScroll);
    if (mView.getHScrollView() != null) {
      mView.getHScrollView().setNestedScrollingEnabled(nestedScroll);
    }
  }

  @LynxProp(name = "initial-scroll-to-index")
  public void setInitialScrollToIndex(@Nullable Integer value) {
    if (value == null) {
      mPropMap.remove(LynxScrollViewInitialScrollIndex);
    } else {
      mPropMap.put(LynxScrollViewInitialScrollIndex, value);
      scrollToIndexInner(value, true);
    }
  }

  @LynxProp(name = "initial-scroll-offset")
  public void setInitialScrollOffset(@Nullable Integer value) {
    if (value == null) {
      mPropMap.remove(LynxScrollViewInitialScrollOffset);
    } else {
      mPropMap.put(LynxScrollViewInitialScrollOffset, (int) PixelUtils.dipToPx(value));
      if (mEnableScrollY) {
        setScrollTopInner(value, true, true);
      } else {
        setScrollLeftInner(value, true, true);
      }
    }
  }

  @LynxProp(name = "fading-edge-length")
  public void setFadingEdgeLength(String fadingEdgeLength) {
    mFadingEdgeLength = (int) UnitUtils.toPxWithDisplayMetrics(
        fadingEdgeLength, 0, 0.0f, mContext.getScreenMetrics());
    if (mFadingEdgeLength <= 0 && mView != null) {
      mView.setHorizontalFadingEdgeEnabled(false);
      mView.setVerticalFadingEdgeEnabled(false);
    }
  }

  @Override
  public void setScrollTap(boolean value) {
    mEnableScrollTap = value;
  }

  @Override
  public void setEnableScroll(boolean value) {
    if (mView != null) {
      mView.setEnableScroll(value);
    }
    mEnableScroll = value;
  }

  @LynxUIMethod
  public void autoScroll(ReadableMap params) {
    mView.autoScroll(params);
  }

  @Override
  public void sendCustomEvent(int l, int t, int oldl, int oldt, String type) {
    LynxScrollEvent event = LynxScrollEvent.createScrollEvent(getSign(), type);
    event.setScrollParams(
        l, t, mView.getContentHeight(), mView.getContentWidth(), l - oldl, t - oldt);
    if (getLynxContext() != null) {
      getLynxContext().getEventEmitter().sendCustomEvent(event);
    }
  }

  @Override
  public void scrollInto(LynxBaseUI node, boolean isSmooth, String block, String inline) {
    scrollInto(node, isSmooth, block, inline, 0);
  }

  @Override
  public void scrollInto(
      LynxBaseUI node, boolean isSmooth, String block, String inline, int bottomInset) {
    int scrollDistance = 0;
    if (mEnableScrollY) {
      if ("nearest".equals(block)) {
        return;
      }
      if ("center".equals(block)) {
        scrollDistance -= (this.getView().getHeight() - node.getHeight()) / 2;
      } else if ("end".equals(block)) {
        scrollDistance -= (this.getView().getHeight() - node.getHeight());
        scrollDistance += bottomInset;
      }
      while (node != this) {
        scrollDistance += node.getTop();
        node = (LynxBaseUI) node.getParentBaseUI();
        while (node instanceof LynxFlattenUI) {
          node = (LynxBaseUI) node.getParent();
        }
      }
      scrollDistance = Math.max(0,
          Math.min(scrollDistance, this.getView().getContentHeight() - this.getView().getHeight()));
      mView.setScrollTo(0, scrollDistance, isSmooth);
    } else {
      if ("nearest".equals(inline)) {
        return;
      }
      if ("center".equals(inline)) {
        scrollDistance -= (this.getView().getWidth() - node.getWidth()) / 2;
      } else if ("end".equals(inline)) {
        scrollDistance -= (this.getView().getWidth() - node.getWidth());
      }
      while (node != this) {
        scrollDistance += node.getLeft();
        node = (LynxBaseUI) node.getParentBaseUI();
        while (node instanceof LynxFlattenUI) {
          node = (LynxBaseUI) node.getParent();
        }
      }
      scrollDistance = Math.max(0,
          Math.min(scrollDistance, this.getView().getContentWidth() - this.getView().getWidth()));
      mView.setScrollTo(scrollDistance, 0, isSmooth);
    }
  }

  private void handleScrollDirection() {
    if (mEnableScrollY) {
      mView.setOrientation(AndroidScrollView.VERTICAL);
    } else {
      mView.setOrientation(AndroidScrollView.HORIZONTAL);
    }
  }

  @Override
  public void setEvents(Map<String, EventsListener> events) {
    super.setEvents(events);
    if (events == null) {
      return;
    }
    mEnableScrollToUpperEvent = false;
    mEnableScrollToLowerEvent = false;
    mEnableScrollEvent = false;
    mEnableScrollEndEvent = false;
    if (events.containsKey(LynxScrollEvent.EVENT_SCROLL_TOLOWER)) {
      mEnableScrollToLowerEvent = true;
    }
    if (events.containsKey(LynxScrollEvent.EVENT_SCROLL_TO_LOWER_EDGE)) {
      mEnableScrollToLowerEdgeEvent = true;
    }
    if (events.containsKey(LynxScrollEvent.EVENT_SCROLL_TOUPPER)) {
      mEnableScrollToUpperEvent = true;
    }
    if (events.containsKey(LynxScrollEvent.EVENT_SCROLL_TO_UPPER_EDGE)) {
      mEnableScrollToUpperEdgeEvent = true;
    }
    if (events.containsKey(LynxScrollEvent.EVENT_SCROLL_TO_NORMAL_STATE)) {
      mEnableScrollToNormalStateEvent = true;
    }
    if (events.containsKey(LynxScrollEvent.EVENT_SCROLL)) {
      mEnableScrollEvent = true;
    }

    if (events.containsKey(LynxScrollEvent.EVENT_SCROLL_START)) {
      mEnableScrollStartEvent = true;
    }
    if (events.containsKey(LynxScrollEvent.EVENT_SCROLL_END)) {
      mEnableScrollEndEvent = true;
    }
    if (events.containsKey(EVENT_CONTENT_SIZE_CHANGED)) {
      mEnableContentSizeChangedEvent = true;
    }
  }

  // TODO(yxping): change flatten bounds to make scrollview draw child properly
  @Override
  protected void drawChild(LynxFlattenUI child, Canvas canvas) {
    super.drawChild(child, canvas);
  }

  @Override
  public int getMemberScrollX() {
    return getScrollX();
  }

  @Override
  public int getMemberScrollY() {
    return getScrollY();
  }

  @Override
  public int getScrollX() {
    return mView.getHScrollView().getScrollX();
  }

  @Override
  public int getScrollY() {
    return mView.getScrollY();
  }

  @Override
  public void onInvalidate() {
    if (mView != null && isEnableNewGesture()) {
      ViewCompat.postInvalidateOnAnimation(mView);
    }
  }

  boolean canInvokeScrollImmediately() {
    final LinearLayout linearLayout = mView.getLinearLayout();
    int viewPortSize = mEnableScrollY ? mView.getHeight() : mView.getWidth();
    int contentSize = mEnableScrollY ? linearLayout.getHeight() : linearLayout.getWidth();
    return viewPortSize != 0 && contentSize != 0;
  }

  private void onContentSizeChanged(float w, float h) {
    if (mEnableContentSizeChangedEvent) {
      LynxDetailEvent event = new LynxDetailEvent(getSign(), EVENT_CONTENT_SIZE_CHANGED);
      event.addDetail("scrollWidth", PixelUtils.pxToDip(w));
      event.addDetail("scrollHeight", PixelUtils.pxToDip(h));
      if (getLynxContext() != null) {
        getLynxContext().getEventEmitter().sendCustomEvent(event);
      }
    }
  }

  /**
   * get scroll-view's scroll info
   * @param callback
   * @return scrollX / scrollY - content offset, scrollRange
   */
  @LynxUIMethod
  public void getScrollInfo(Callback callback) {
    int scrollX = getMemberScrollX();
    int scrollY = getMemberScrollY();
    JavaOnlyMap result = new JavaOnlyMap();
    result.putInt("scrollX", LynxUIHelper.px2dip(mContext, scrollX));
    result.putInt("scrollY", LynxUIHelper.px2dip(mContext, scrollY));
    result.putInt("scrollRange", LynxUIHelper.px2dip(mContext, getScrollRange()));
    callback.invoke(LynxUIMethodConstants.SUCCESS, result);
  }

  @LynxUIMethod
  public void scrollBy(ReadableMap params, Callback callback) {
    if (callback == null) {
      return;
    }
    if (params == null || !params.hasKey("offset")) {
      callback.invoke(
          LynxUIMethodConstants.PARAM_INVALID, "Invoke scrollBy failed due to param is null");
      return;
    }
    double offset = PixelUtils.dipToPx(params.getDouble("offset", 0));

    // need to post runnable in ui thread, otherwise may trigger npe in dispatchDraw
    UIThreadUtils.runOnUiThreadImmediately(() -> {
      float[] result = scrollBy((float) offset, (float) offset);
      JavaOnlyMap response = new JavaOnlyMap();
      response.putDouble("consumedX", (int) PixelUtils.pxToDip(result[0]));
      response.putDouble("consumedY", (int) PixelUtils.pxToDip(result[1]));
      response.putDouble("unconsumedX", (int) PixelUtils.pxToDip(result[2]));
      response.putDouble("unconsumedY", (int) PixelUtils.pxToDip(result[3]));

      callback.invoke(LynxUIMethodConstants.SUCCESS, response);
    });
  }

  @LynxUIMethod
  public void scrollTo(ReadableMap params, Callback callback) {
    if (mScrollToCallback != null) {
      // Note: Here need temporarily store lastScrollToCallback and reset mScrollToCallback to null
      // to avoid invoking mScrollToCallback twice.
      Callback lastScrollToCallback = mScrollToCallback;
      mScrollToCallback = null;
      lastScrollToCallback.invoke(LynxUIMethodConstants.SUCCESS,
          "Due to the start of a new scrollTo operation, the previous scrollTo has stopped.");
    }
    if (mChildren.isEmpty()) {
      callback.invoke(
          LynxUIMethodConstants.PARAM_INVALID, "Invoke scrollTo failed due to empty children.");
      return;
    }
    mScrollToCallbackInfo = "";
    double offset = params.getDouble("offset", 0);
    offset = PixelUtils.dipToPx(offset);
    boolean smooth = params.getBoolean("smooth", false);
    int index = INVALID_INDEX;
    if (params.hasKey("index")) {
      index = params.getInt("index");
      if (index < 0 || index >= mChildren.size()) {
        callback.invoke(LynxUIMethodConstants.PARAM_INVALID,
            "scrollTo index " + index + " is out of range [0, " + mChildren.size() + "]");
        return;
      }
    }
    boolean canScroll = true;
    if (mEnableScrollY) {
      if (index >= 0 && index < mChildren.size()) {
        offset += ((LynxBaseUI) mChildren.get(index)).getTop();
      }
      // In measure() method, we can make sure getScrollRange() >= 0, so here no need to check.
      if (offset < 0 || offset > getScrollRange()) {
        mScrollToCallbackInfo = "Target scroll position = " + offset + " is beyond threshold. ";
        offset = MathUtils.clamp(offset, 0, getScrollRange());
        mScrollToCallbackInfo += "Clamped to position = " + offset;
      }
      canScroll = offset != getScrollY();
      mView.setScrollTo(0, (int) offset, smooth);
    } else {
      if (index >= 0 && index < mChildren.size()) {
        if (mLynxDirection == DIRECTION_RTL) {
          int scrollDistance =
              mChildren.get(index).getLeft() + mChildren.get(index).getWidth() - getWidth();
          offset = Math.max(0, scrollDistance - offset);
        } else {
          offset += ((LynxBaseUI) mChildren.get(index)).getLeft();
        }
      }
      // In measure() method, we can make sure getScrollRange() >= 0, so here no need to check.
      if (offset < 0 || offset > getScrollRange()) {
        mScrollToCallbackInfo = "Target scroll position = " + offset + " is beyond threshold. ";
        offset = MathUtils.clamp(offset, 0, getScrollRange());
        mScrollToCallbackInfo += "Clamped to position = " + offset;
      }
      canScroll = offset != getScrollX();
      mView.setScrollTo((int) offset, 0, smooth);
    }
    if (smooth && canScroll) {
      mScrollToCallback = callback;
    } else {
      boolean success = mScrollToCallbackInfo.isEmpty();
      callback.invoke(success ? LynxUIMethodConstants.SUCCESS : LynxUIMethodConstants.PARAM_INVALID,
          mScrollToCallbackInfo);
      mScrollToCallback = null;
      mScrollToCallbackInfo = "";
    }
  }

  // call by androidscrollview
  void scrollToBounce(boolean smooth) {
    if (!mEnableNewBounce && mBounceView != null) {
      int offset = -1;
      if (mEnableScrollY) {
        if (mBounceView.mDirection == mBounceView.BOTTOM
            && mBounceView.getTop() < getScrollY() + getHeight()) {
          offset = mBounceView.getTop() - getHeight();
        } else if (mBounceView.mDirection == mBounceView.TOP
            && mBounceView.getHeight() > getScrollY()) {
          offset = mBounceView.getHeight();
        }
        if (offset > 0) {
          flingY(0);
          mView.setScrollTo(0, (int) offset, smooth);
        }
      } else {
        if (mBounceView.mDirection == mBounceView.RIGHT
            && mBounceView.getLeft() < getScrollX() + getWidth()) {
          offset = mBounceView.getLeft() - getWidth();
        } else if (mBounceView.mDirection == mBounceView.LEFT
            && mBounceView.getWidth() > getScrollX()) {
          offset = mBounceView.getWidth();
        }
        if (offset > 0) {
          flingX(0);
          mView.setScrollTo((int) offset, 0, smooth);
        }
      }
    }
  }

  private int updateBorderStatus(int l, int t) {
    return updateBorderStatus(l, t, mUpperThreshold, mLowerThreshold);
  }

  int updateBorderStatus(int l, int t, int upperThreadHold, int lowerThreadHold) {
    int status = 0;
    if (mEnableScrollY) { // scroll-y
      return computeStatus(
          t, upperThreadHold, lowerThreadHold, BORDER_STATUS_UPPER, BORDER_STATUS_LOWER);
    } else { // scroll-x
      if (mLynxDirection == DIRECTION_RTL) { // for RTL, left vs lower
        return computeStatus(
            l, lowerThreadHold, upperThreadHold, BORDER_STATUS_LOWER, BORDER_STATUS_UPPER);
      } else { // for LTR, left vs upper
        return computeStatus(
            l, upperThreadHold, lowerThreadHold, BORDER_STATUS_UPPER, BORDER_STATUS_LOWER);
      }
    }
  }

  private int computeStatus(int base, int threshold1, int threshold2, int status1, int status2) {
    int status = 0;
    if (base <= threshold1) {
      status |= status1;
    }
    if (mView.getHScrollView() != null) {
      View contentView = mView.getHScrollView().getChildAt(0);
      if (mEnableScrollY) {
        threshold2 = contentView.getMeasuredHeight() - mView.getMeasuredHeight() - threshold2;
      } else {
        threshold2 = contentView.getMeasuredWidth() - mView.getMeasuredWidth() - threshold2;
      }
      if (base >= threshold2) {
        status |= status2;
      }
    }
    return status;
  }

  static boolean isUpper(int status) {
    return (status & BORDER_STATUS_UPPER) != 0;
  }

  static boolean isLower(int status) {
    return (status & BORDER_STATUS_LOWER) != 0;
  }

  @Override
  public boolean canScroll(int direction) {
    if (mView == null) {
      return false;
    }
    switch (direction) {
      case SCROLL_UP:
        return mView.canScrollVertically(-1);
      case SCROLL_DOWN:
        return mView.canScrollVertically(1);
      case SCROLL_LEFT:
        return mView.getHScrollView().canScrollHorizontally(-1);
      case SCROLL_RIGHT:
        return mView.getHScrollView().canScrollHorizontally(1);
    }
    return false;
  }

  @Override
  public void scrollByX(double delta) {
    if (mView == null) {
      return;
    }
    int x = getScrollX();
    int y = getScrollY();
    mView.setScrollTo(x + (int) delta, y, false);
  }

  @Override
  public void scrollByY(double delta) {
    if (mView == null) {
      return;
    }
    int x = getScrollX();
    int y = getScrollY();
    mView.setScrollTo(x, y + (int) delta, false);
  }

  @Override
  public void flingX(double velocityX) {
    if (mView == null) {
      return;
    }
    mView.getHScrollView().fling((int) velocityX);
  }

  @Override
  public void flingY(double velocityY) {
    if (mView == null) {
      return;
    }
    mView.fling((int) velocityY);
  }

  @Override
  public void setForbidFlingFocusChange(boolean value) {
    if (mView != null) {
      mView.forbidFocusChangeAfterFling();
    }
  }

  public void setBlockDescendantFocusability(boolean value) {
    if (mView != null) {
      mView.setBlockDescendantFocusability(value);
    }
  }

  /** ---------- Accessibility Section ---------- */

  @Override
  public void onPropsUpdated() {
    super.onPropsUpdated();
    if (mEnableScrollY) {
      mView.setEnableNewBounce(mEnableNewBounce);
    } else {
      mView.getHScrollView().setEnableNewBounce(mEnableNewBounce);
    }
    if (mGestureHandlers != null) {
      GestureArenaManager manager = getGestureArenaManager();
      // Check if the current UIList instance is already a member of the gesture arena
      if (manager != null && !manager.isMemberExist(getGestureArenaMemberId())) {
        // If not a member, add the UIList instance as a new member to the gesture arena
        mGestureArenaMemberId = manager.addMember(UIScrollView.this);
      }
    }
    updateAccessibilityDelegate();
  }

  /**
   * Register delegate for ScrollView or HorizontalScrollView if enable new accessibility.
   */
  private void updateAccessibilityDelegate() {
    LynxAccessibilityWrapper wrapper = mContext.getLynxAccessibilityWrapper();
    if (wrapper != null && (wrapper.enableDelegate() || wrapper.enableHelper())) {
      if (mDelegate == null) {
        mDelegate = new CustomUIScrollViewAccessibilityDelegate();
      }
      if (mEnableScrollY) {
        ViewCompat.setAccessibilityDelegate(mView, mDelegate);
        ViewCompat.setAccessibilityDelegate(mView.getHScrollView(), null);
      } else {
        ViewCompat.setAccessibilityDelegate(mView, null);
        ViewCompat.setAccessibilityDelegate(mView.getHScrollView(), mDelegate);
      }
      if (wrapper.enableHelper()) {
        ViewCompat.setImportantForAccessibility(mView,
            mEnableScrollY ? ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_YES
                           : ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_NO);
        ViewCompat.setImportantForAccessibility(mView.getHScrollView(),
            mEnableScrollY ? ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_NO
                           : ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_YES);
        ViewCompat.setImportantForAccessibility(
            mView.getLinearLayout(), ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_NO);
      }
    }
  }

  @Override
  public boolean isAccessibilityHostUI() {
    return true;
  }

  @Override
  public boolean isAccessibilityDirectionVertical() {
    return mEnableScrollY;
  }

  @Override
  public View getAccessibilityHostView() {
    return getRealParentView();
  }

  /**
   * Request that ui can be visible on the screen and scrolling if necessary just enough.
   *
   * @param child the direct ui making the request.
   * @param rect the rectangle in the child's coordinates the child wishes to be on the screen.
   * @param smooth animated scrolling.
   * @return true if scrollview scrolled to handle the operation
   */
  @Override
  public boolean requestChildUIRectangleOnScreen(LynxBaseUI child, Rect rect, boolean smooth) {
    if (!mEnableScroll || child == null) {
      return false;
    }
    int scrollOffset = 0;
    rect.offset(child.getLeft() - child.getScrollX(), child.getTop() - child.getScrollY());
    if (mEnableScrollY) {
      scrollOffset = mView.computeScrollDeltaToGetChildRectOnScreen(rect);
      if (scrollOffset != 0) {
        mView.setScrollTo(mView.getRealScrollX(), mView.getRealScrollY() + scrollOffset, smooth);
      }
    } else {
      scrollOffset = mView.getHScrollView().computeScrollDeltaToGetChildRectOnScreen(rect);
      if (scrollOffset != 0) {
        mView.setScrollTo(mView.getRealScrollX() + scrollOffset, mView.getRealScrollY(), smooth);
      }
    }
    return scrollOffset != 0;
  }

  private int getViewportSize() {
    return mEnableScrollY ? getHeight() - mView.getPaddingBottom() - mView.getPaddingTop()
                          : getWidth() - mView.getPaddingLeft() - mView.getPaddingRight();
  }

  protected int getScrollRange() {
    return (mEnableScrollY ? mView.getContentHeight() : mView.getContentWidth())
        - getViewportSize();
  }

  @Override
  public void onGestureScrollBy(float x, float y) {
    if (!isEnableNewGesture()) {
      return;
    }
    // need to run in ui thread, otherwise may trigger npe in dispatchDraw
    UIThreadUtils.runOnUiThreadImmediately(() -> {
      if (mView == null) {
        return;
      }
      if (mView.isHorizontal && mView.getHScrollView() != null) {
        mView.getHScrollView().scrollBy((int) x, 0);
      } else {
        mView.scrollBy(0, (int) y);
      }
      // when scroll, not trigger basic events
      if (Math.abs(x) > Float.MIN_VALUE || Math.abs(y) > Float.MIN_VALUE) {
        recognizeGesturere();
      }
    });
  }

  /**
   * Retrieves gesture handlers if new gestures are enabled.
   * @return The map of gesture handlers, or null if new gestures are disabled.
   */
  @Nullable
  @Override
  public Map<Integer, BaseGestureHandler> getGestureHandlers() {
    if (!isEnableNewGesture()) {
      return null; // Return null if new gestures are not enabled
    }
    if (mGestureHandlers == null) {
      mGestureHandlers = BaseGestureHandler.convertToGestureHandler(
          getSign(), getLynxContext(), UIScrollView.this, getGestureDetectorMap());
    }
    return mGestureHandlers;
  }

  /**
   * Determines if the gesture can be consumed based on new gesture feature.
   * @param deltaX The horizontal distance of the gesture.
   * @param deltaY The vertical distance of the gesture.
   * @return True if the gesture can be consumed, false otherwise.
   */
  @Override
  public boolean canConsumeGesture(float deltaX, float deltaY) {
    if (!isEnableNewGesture() || mView == null) {
      return false; // Cannot consume gesture if new gestures are disabled or mView is null
    }
    if (mView.isHorizontal()) {
      return !((isAtBorder(true) && deltaX < 0) || (isAtBorder(false) && deltaX > 0));
    } else {
      return !((isAtBorder(true) && deltaY < 0) || (isAtBorder(false) && deltaY > 0));
    }
  }

  @Override
  public boolean isAtBorder(boolean isStart) {
    // Cannot consume gesture if new gestures are disabled or mView is null
    if (!isEnableNewGesture() || mView == null) {
      return false;
    }
    if (isStart) {
      if (mView.isHorizontal()) {
        return !canScroll(SCROLL_LEFT);
      } else {
        return !canScroll(SCROLL_UP);
      }
    } else {
      if (mView.isHorizontal()) {
        return !canScroll(SCROLL_RIGHT);
      } else {
        return !canScroll(SCROLL_DOWN);
      }
    }
  }

  /**
   * Sets gesture detectors and manages gesture arena based on new gesture feature.
   * @param gestureDetectors The map of gesture detectors.
   */
  @Override
  public void setGestureDetectors(Map<Integer, GestureDetector> gestureDetectors) {
    super.setGestureDetectors(gestureDetectors);
    if (gestureDetectors == null || gestureDetectors.isEmpty()) {
      return; // Return if new gestures are disabled or gestureDetectors is empty
    }
    GestureArenaManager manager = getGestureArenaManager();
    if (manager == null) {
      return; // Return if GestureArenaManager is null
    }
    if (manager.isMemberExist(getGestureArenaMemberId())) {
      // when update gesture handlers, need to reset it
      if (mGestureHandlers != null) {
        mGestureHandlers.clear();
        mGestureHandlers = null;
      }
    }
    if (mGestureHandlers == null && getSign() > 0) {
      mGestureHandlers = BaseGestureHandler.convertToGestureHandler(
          getSign(), getLynxContext(), UIScrollView.this, getGestureDetectorMap());
    }
  }

  @Override
  protected void consumeGesture(boolean consumeGesture) {
    mView.consumeGesture(consumeGesture);
  }

  /**
   * Scrolls the view by the specified width and height.
   * @param deltaX The deltaX to scroll by.
   * @param deltaY The deltaY to scroll by.
   * @return An array containing scroll information [consumedX, consumedY, unconsumedX,
   *     unconsumedY].
   */
  @MainThread
  @Override
  public float[] scrollBy(float deltaX, float deltaY) {
    float[] res = new float[4];
    if (mView == null) {
      return res; // Return if mView is null
    }
    int mLastScrollOffsetX = getScrollX();
    int mLastScrollOffsetY = getScrollY();
    if (mView.isHorizontal) {
      mView.getHScrollView().scrollBy((int) deltaX, 0);
    } else {
      mView.scrollBy(0, (int) deltaY);
    }
    // when scroll, not trigger basic events
    if (Math.abs(deltaX) > Float.MIN_VALUE || Math.abs(deltaY) > Float.MIN_VALUE) {
      recognizeGesturere();
    }

    if (getView().isHorizontal) {
      res[0] = getScrollX() - mLastScrollOffsetX;
      res[1] = 0;
      res[2] = deltaX - res[0];
      res[3] = deltaY;
    } else {
      res[0] = 0;
      res[1] = getScrollY() - mLastScrollOffsetY;
      res[2] = deltaX;
      res[3] = deltaY - res[1];
    }
    return res;
  }

  protected class CustomUIScrollViewAccessibilityDelegate extends AccessibilityDelegateCompat {
    /**
     * Initializes an event with information about the host View which is the event source.
     *
     * @param host The View hosting the delegate.
     * @param event The event to initialize.
     */
    @Override
    public void onInitializeAccessibilityEvent(View host, AccessibilityEvent event) {
      super.onInitializeAccessibilityEvent(host, event);
      int scrollRange = getScrollRange();
      event.setScrollable(mEnableScroll && scrollRange > 0);
      event.setScrollX(mView.getRealScrollX());
      event.setScrollY(mView.getRealScrollY());
      if (mEnableScrollY) {
        event.setMaxScrollX(mView.getRealScrollX());
        event.setMaxScrollY(scrollRange);
      } else {
        event.setMaxScrollX(scrollRange);
        event.setMaxScrollY(mView.getRealScrollY());
      }
      if (DEBUG) {
        LLog.i(ACCESSIBILITY_TAG,
            "onInitializeAccessibilityEvent: " + host + ", scrollRange: " + scrollRange
                + ", isScrollable: " + event.isScrollable());
      }
    }

    /**
     * Initializes an node with information about the host view.
     *
     * @param host The View hosting the delegate.
     * @param info The instance to initialize.
     */
    @Override
    public void onInitializeAccessibilityNodeInfo(View host, AccessibilityNodeInfoCompat info) {
      if (DEBUG) {
        LLog.i(ACCESSIBILITY_TAG, "onInitializeAccessibilityNodeInfo: " + host);
      }
      super.onInitializeAccessibilityNodeInfo(host, info);
      int scrollRange = getScrollRange();
      boolean scrollable = mEnableScroll && scrollRange > 0;
      info.setScrollable(scrollable);
      if (scrollable) {
        if (mEnableScrollY) {
          if (mView.getRealScrollY() > 0) {
            info.addAction(
                AccessibilityNodeInfoCompat.AccessibilityActionCompat.ACTION_SCROLL_BACKWARD);
          } else if (mView.getRealScrollY() < scrollRange) {
            info.addAction(
                AccessibilityNodeInfoCompat.AccessibilityActionCompat.ACTION_SCROLL_FORWARD);
          }
        } else {
          if (mView.getRealScrollX() > 0) {
            info.addAction(
                AccessibilityNodeInfoCompat.AccessibilityActionCompat.ACTION_SCROLL_BACKWARD);
          } else if (mView.getRealScrollX() < scrollRange) {
            info.addAction(
                AccessibilityNodeInfoCompat.AccessibilityActionCompat.ACTION_SCROLL_FORWARD);
          }
        }
      }
    }

    /**
     * Perform the accessibility action on the view. Here we only need to handle
     * ACTION_SCROLL_FORWARD and ACTION_SCROLL_BACKWARD.
     *
     * @param host the View hosting the delegate.
     * @param action the action to perform.
     * @param args optional action arguments.
     * @return Whether the action was performed.
     */
    @Override
    public boolean performAccessibilityAction(View host, int action, Bundle args) {
      if (DEBUG) {
        LLog.i(ACCESSIBILITY_TAG, "scrollview performAction action = " + action);
      }
      int viewportSize = getViewportSize();
      int scrollRange = getScrollRange();
      boolean scrollable = mEnableScroll && scrollRange > 0;
      if (!scrollable) {
        return false;
      }
      int targetScrollX = 0;
      int targetScrollY = 0;
      int currentScrollX = mView.getRealScrollX();
      int currentScrollY = mView.getRealScrollY();
      switch (action) {
        case AccessibilityNodeInfo.ACTION_SCROLL_FORWARD:
          if (mEnableScrollY) {
            targetScrollY = Math.min(currentScrollY + viewportSize / 2, scrollRange);
            if (targetScrollY != currentScrollY) {
              mView.setScrollTo(0, targetScrollY, true);
            }
          } else {
            targetScrollX = Math.min(currentScrollX + viewportSize / 2, scrollRange);
            if (targetScrollX != currentScrollX) {
              mView.setScrollTo(targetScrollX, 0, true);
            }
          }
          return true;
        case AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD:
          if (mEnableScrollY) {
            targetScrollY = Math.max(currentScrollY - viewportSize / 2, 0);
            if (targetScrollY != currentScrollY) {
              mView.setScrollTo(0, targetScrollY, true);
            }
          } else {
            targetScrollX = Math.max(currentScrollX - viewportSize / 2, 0);
            if (targetScrollX != currentScrollX) {
              mView.setScrollTo(targetScrollX, 0, true);
            }
          }
          return true;
      }
      return false;
    }
  }

  /** ---------- Accessibility Section end ---------- */

  @Override
  public void setEnableNewNested(boolean value) {
    if (mView == null) {
      return;
    }
    mView.setEnableNewNested(value);
    if (mView.getHScrollView() != null) {
      mView.getHScrollView().setEnableNewNested(value);
    }
  }

  /**
   * @name: android-preference-consume-gesture
   * @description: Request parent not intercept motion event for consuming.
   * @category: different
   * @standardAction: keep
   * @supportVersion: 2.9
   **/
  @LynxProp(name = "android-preference-consume-gesture", defaultBoolean = false)
  public void setPreferenceConsumeGesture(boolean value) {
    mPreferenceConsumeGesture = value;
  }

  /**
   * @name: android-enable-new-bounce
   * @description: Used to enable new scroll-view bounce feature which supports both-side
   *bounce-view and custom bounce event triggered distance.
   * @category: different
   * @standardAction: keep
   * @supportVersion: 2.9
   **/
  @LynxProp(name = "android-enable-new-bounce", defaultBoolean = false)
  public void setEnableNewBounce(boolean value) {
    mEnableNewBounce = value;
  }

  @Override
  public void onListCellAppear(String itemKey, LynxBaseUI list) {
    super.onListCellAppear(itemKey, list);
    if (TextUtils.isEmpty(itemKey)) {
      return;
    }
    mCurrentItemKey = itemKey;
    mListSign = list.getSign();
    String cacheKey = constructListStateCacheKey(getTagName(), itemKey, getIdSelector());
    Object value = list.getValueFromNativeStorage(cacheKey);
    if (value != null) {
      // no first render and last time this UI shows it has a changed offset
      int offset = (int) value;
      if (getView().isHorizontal) {
        setScrollLeftInner(offset, false, false);
      } else {
        setScrollTopInner(offset, false, false);
      }
    } else {
      // first render
      if (mPropMap.containsKey(LynxScrollViewInitialScrollIndex)
          && !list.initialPropsFlushed(LynxScrollViewInitialScrollIndex, cacheKey)) {
        setPendingInitialScrollToIndex(mPropMap.get(LynxScrollViewInitialScrollIndex));
      }
      if (mPropMap.containsKey(LynxScrollViewInitialScrollOffset)
          && !list.initialPropsFlushed(LynxScrollViewInitialScrollOffset, cacheKey)) {
        setPendingInitialScrollOffset(mPropMap.get(LynxScrollViewInitialScrollOffset));
      }
    }
  }

  private LynxBaseUI getParentList() {
    LynxBaseUI list = getLynxContext().getLynxUIOwner().findLynxUIBySign(mListSign);
    if ((list instanceof UIList || list instanceof UIListContainer)) {
      return list;
    }
    return null;
  }

  /**
   * when the component of ScrollView  being reused to other component,
   * the callback will be called
   *
   * @param itemKey the key of listItem
   * @param list    lis
   */
  @Override
  public void onListCellPrepareForReuse(String itemKey, LynxBaseUI list) {
    if (TextUtils.isEmpty(itemKey)) {
      return;
    }
    mListSign = list.getSign();
    mCurrentItemKey = itemKey;
    resetOffset();
  }

  public void resetOffset() {
    if (mEnableScrollY) {
      mView.setScrollTo(0, 0, false);
    } else {
      if (mLynxDirection == DIRECTION_RTL) {
        mView.setScrollTo(mView.getContentWidth() - mView.getWidth(), 0, false);
      } else {
        mView.setScrollTo(0, 0, false);
      }
    }
  }

  /**
   * if the component of scrollView isDetached to window, the callBack will be called
   *
   * @param itemKey the key of listItem
   * @param list    list
   * @param isExist the view is still exist
   */
  @Override
  public void onListCellDisAppear(String itemKey, LynxBaseUI list, boolean isExist) {
    super.onListCellDisAppear(itemKey, list, isExist);
    if (TextUtils.isEmpty(itemKey)) {
      return;
    }
    String cacheKey = constructListStateCacheKey(getTagName(), itemKey, getIdSelector());
    mListSign = list.getSign();
    mCurrentItemKey = itemKey;
    if (isExist) {
      list.storeKeyToNativeStorage(cacheKey, mView.getRealScrollX());
    } else {
      list.removeKeyFromNativeStorage(cacheKey);
    }
  }

  /**
   * @name: android-touch-slop
   * @description: Set up the Android scroll-view's scrolling motion threshold. This property can be
   *set to 'default' or 'paging'. If set to 'paging', we use mPagingTouchSlop which is the distance
   *in pixels a touch can wander before we think the user is attempting a paged scroll. With the
   *'default' value, we use mTouchSlop which is the distance in pixels a touch can wander before we
   *think the user is scrolling. Usually the mPagingTouchSlop is equal 2 * mTouchSlop.
   * @category: different
   * @standardAction: keep
   * @supportVersion: 2.12
   **/
  @LynxProp(name = "android-touch-slop")
  public void setTouchSlop(String value) {
    mUsePagingTouchSlop = TextUtils.equals(value, "paging");
  }

  /**
   * @name: force-can-scroll
   * @description: Force scroll-view to consume gesture and do not pass it to parent.
   * @category: different
   * @standardAction: keep
   * @supportVersion: 2.10.2
   **/
  @LynxProp(name = "force-can-scroll", defaultBoolean = false)
  public void setForceCanScroll(boolean value) {
    if (mView != null) {
      mView.setForceCanScroll(value);
    }
  }
}
