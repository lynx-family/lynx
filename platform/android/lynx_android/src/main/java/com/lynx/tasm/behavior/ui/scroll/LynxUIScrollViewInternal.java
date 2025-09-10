// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.scroll;

import android.content.Context;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIMethodConstants;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxFlattenUI;
import com.lynx.tasm.behavior.ui.scroll.base.LynxBaseScrollView;
import com.lynx.tasm.behavior.ui.scroll.base.LynxBaseScrollViewScrolling;
import com.lynx.tasm.behavior.ui.view.UISimpleView;
import com.lynx.tasm.event.LynxCustomEvent;
import com.lynx.tasm.utils.DeviceUtils;
import com.lynx.tasm.utils.UnitUtils;
import java.util.ArrayList;
import java.util.HashMap;

public class LynxUIScrollViewInternal
    extends UISimpleView<LynxBaseScrollView> implements LynxBaseScrollView.ScrollDelegate {
  private long mThrottle = 0;
  private long mLastUpdateTime = 0;
  private int[] mLastContentOffset = {0, 0};
  private int mUpperThreshold = 0;
  private int mLowerThreshold = 0;
  private boolean mAtUpper = false;
  private boolean mAtLower = false;

  @Override
  public void onScrollStateChanged(int from, int to) {
    switch (to) {
      case LynxBaseScrollView.SCROLL_STATE_IDLE:
        sendScrollEvent("scrollend", null);
        break;
      case LynxBaseScrollView.SCROLL_STATE_DRAGGING:
        sendScrollEvent("scrollstart", null);
        break;
      case LynxBaseScrollView.SCROLL_STATE_ANIMATING:
        if (from == LynxBaseScrollView.SCROLL_STATE_IDLE) {
          sendScrollEvent("scrollstart", null);
        }
        break;
      case LynxBaseScrollView.SCROLL_STATE_FLING:
        break;
      default:
        break;
    }

    HashMap<String, Object> params = new HashMap<>();
    params.put("previousState", from);
    params.put("currentState", to);
    sendScrollEvent("scrollstatechange", params);
  }

  @Override
  public void scrollViewDidScroll(LynxBaseScrollViewScrolling scrollView) {
    tryToSendScrollEvent();
    updateScrollPosition();
    updateSticky();
  }

  private void tryToSendScrollEvent() {
    if (mThrottle != 0) {
      long currentTime = System.currentTimeMillis();
      if (currentTime - mLastUpdateTime >= mThrottle) {
        int[] currentOffset = mView.getScrollOffset();
        HashMap<String, Object> params = new HashMap<>();
        params.put("deltaX", currentOffset[0] - mLastContentOffset[0]);
        params.put("deltaY", currentOffset[1] - mLastContentOffset[1]);
        sendScrollEvent("scroll", params);
        mLastContentOffset = currentOffset;
        mLastUpdateTime = currentTime;
      }
    } else {
      int[] currentOffset = mView.getScrollOffset();
      HashMap<String, Object> params = new HashMap<>();
      params.put("deltaX", currentOffset[0] - mLastContentOffset[0]);
      params.put("deltaY", currentOffset[1] - mLastContentOffset[1]);
      sendScrollEvent("scroll", params);
      mLastContentOffset = currentOffset;
    }
  }

  private void updateScrollPosition() {
    LynxBaseScrollView scrollView = mView;
    int scrollOffset = scrollView.isVertical() ? scrollView.getScrollOffsetVertically()
                                               : scrollView.getScrollOffsetHorizontally();
    int[] scrollRange = scrollView.isVertical() ? scrollView.getScrollRangeVertically()
                                                : scrollView.getScrollRangeHorizontally();

    boolean atUpper = scrollOffset <= scrollRange[0] + mUpperThreshold;
    boolean atLower = scrollOffset >= scrollRange[1] - mLowerThreshold;

    if (atUpper && !mAtUpper) {
      sendScrollEvent("scrolltoupper", null);
    }
    if (atLower && !mAtLower) {
      sendScrollEvent("scrolltolower", null);
    }

    mAtUpper = atUpper;
    mAtLower = atLower;
  }

  private void updateSticky() {
    int[] scrollOffset = getView().getScrollOffset();

    boolean stickyChanged = false;

    for (int index = 0; index < mChildren.size(); index++) {
      LynxBaseUI ui = mChildren.get(index);
      if (ui.checkStickyOnParentScroll(scrollOffset[0], scrollOffset[1])) {
        stickyChanged = true;
      }
    }
    if (stickyChanged) {
      invalidate();
    }
  }

  static int[] getPositionOf(LynxBaseUI target, LynxUIScrollViewInternal scroll, boolean isRTL) {
    int[] result = {0, 0};
    if (scroll != null && target != null) {
      if (isRTL) {
        int[] scrollRange = scroll.getView().getScrollRange();
        result[0] = scrollRange[1] - target.getLeft() - target.getWidth();
        result[1] = target.getTop();
      } else {
        result[0] = target.getLeft();
        result[1] = target.getTop();
      }
    }
    return result;
  }

  static void addOffset(int[] offset, int[] delta, boolean isRTL) {
    offset[0] += isRTL ? -delta[0] : delta[0];
    offset[1] += delta[1];
  }

  static void formatOffset(int[] offset, LynxUIScrollViewInternal scroll, boolean isRTL) {
    if (isRTL) {
      int[] scrollRange = scroll.getView().getScrollRange();
      offset[0] = scrollRange[1] - offset[0];
    }
  }

  private interface LynxUIScrollViewInternalNodeReadyBlock {
    void invoke(LynxUIScrollViewInternal ui);
  }

  private ArrayList<LynxUIScrollViewInternalNodeReadyBlock> mFirstRenderBlockArray =
      new ArrayList<>();

  public LynxUIScrollViewInternal(LynxContext context) {
    super(context);
  }

  @Override
  protected LynxBaseScrollView createView(Context context) {
    LynxBaseScrollView scrollView = new LynxBaseScrollView(context);
    scrollView.setScrollDelegate(this);
    return scrollView;
  }

  @Override
  public void destroy() {
    super.destroy();
    mView.setScrollDelegate(null);
  }

  public void setScrollOrientation(String value) {
    mView.setVertical("vertical".equals(value));
  }

  public void setEnableScroll(boolean value) {
    mView.enableScroll(value);
  }

  public void setBounces(boolean value) {
    mView.enableBounces(value);
  }

  public void setForwardsNestedScroll(int value) {
    mView.setForwardNestedScrollMode(value);
  }

  public void setBackwardsNestedScroll(int value) {
    mView.setBackwardNestedScrollMode(value);
  }

  public void setInitialScrollIndex(int value) {
    if (mFirstRenderBlockArray != null) {
      mFirstRenderBlockArray.add(ui -> {
        if (value >= 0 && value < ui.getChildCount()) {
          LynxBaseUI child = ui.getChildAt(value);
          ui.getView().scrollTo(getPositionOf(child, ui, ui.isRtl()));
        }
      });
    }
  }

  public void setInitialScrollOffset(String value) {
    if (mFirstRenderBlockArray != null) {
      mFirstRenderBlockArray.add(ui -> {
        int offset = (int) UnitUtils.toPxWithDisplayMetrics(value,
            ui.getView().isVertical() ? ui.getHeight() : ui.getWidth(), 0,
            getLynxContext().getScreenMetrics());
        int[] target = {offset, offset};
        formatOffset(target, ui, ui.isRtl());
        ui.getView().scrollTo(target);
      });
    }
  }

  public void setLowerThreshold(String value) {
    mLowerThreshold = (int) UnitUtils.toPxWithDisplayMetrics(value,
        mView.isVertical() ? getHeight() : getWidth(), 0, getLynxContext().getScreenMetrics());
  }
  public void setUpperThreshold(String value) {
    mUpperThreshold = (int) UnitUtils.toPxWithDisplayMetrics(value,
        mView.isVertical() ? getHeight() : getWidth(), 0, getLynxContext().getScreenMetrics());
  }

  public void setScrollEventThrottle(float value) {
    mThrottle = (long) value;
  }

  public void scrollTo(ReadableMap params, Callback callback) {
    int offset = (int) UnitUtils.toPxWithDisplayMetrics(params.getString("offset", "0px"),
        mView.isVertical() ? getHeight() : getWidth(), 0, getLynxContext().getScreenMetrics());
    boolean animated = params.getBoolean("animated", true);
    animated = params.getBoolean("smooth", animated);
    int index = params.getInt("index", -1);
    if (index >= 0 && index < getChildCount()) {
      LynxBaseUI child = getChildAt(index);
      int[] target = getPositionOf(child, this, this.isRtl());
      int[] delta = {offset, offset};
      addOffset(target, delta, this.isRtl());
      if (animated) {
        mView.animatedScrollTo(target, null);
      } else {
        mView.scrollTo(target);
      }
      callback.invoke(LynxUIMethodConstants.SUCCESS);
    } else {
      callback.invoke(LynxUIMethodConstants.PARAM_INVALID,
          "scrollTo index: " + index + " is out of range[0, " + getChildCount() + ").");
    }
  }

  public void scrollBy(ReadableMap params, Callback callback) {
    int offset = (int) UnitUtils.toPxWithDisplayMetrics(params.getString("offset", "0px"),
        mView.isVertical() ? getHeight() : getWidth(), 0, getLynxContext().getScreenMetrics());
    int[] delta = {offset, offset};
    mView.scrollBy(delta);
    callback.invoke(LynxUIMethodConstants.SUCCESS);
  }

  public void autoScroll(ReadableMap params, Callback callback) {
    boolean start = params.getBoolean("start", true);
    if (start) {
      int rate = (int) UnitUtils.toPxWithDisplayMetrics(params.getString("rate", "0px"),
          mView.isVertical() ? getHeight() : getWidth(), 0, getLynxContext().getScreenMetrics());
      int refreshRate = (int) DeviceUtils.getRefreshRate(mContext);
      if (refreshRate <= 0) {
        refreshRate = DeviceUtils.DEFAULT_DEVICE_REFRESH_RATE;
      }
      mView.autoScrollWithRate(
          rate > 0 ? Math.max(rate / refreshRate, 1) : Math.min(rate / refreshRate, -1), true,
          null);
    } else {
      mView.stopScrolling();
    }
    callback.invoke(LynxUIMethodConstants.SUCCESS);
  }

  @Override
  public void onNodeReady() {
    super.onNodeReady();
  }

  @Override
  public void onLayoutUpdated() {
    super.onLayoutUpdated();
    mView.mWidth = getWidth();
    mView.mHeight = getHeight();
  }

  @Override
  public boolean isScrollable() {
    return true;
  }

  @Override
  public boolean isScrollContainer() {
    return true;
  }

  @Override
  public int getScrollX() {
    return mView.getScrollX();
  }

  @Override
  public int getScrollY() {
    return mView.getScrollY();
  }

  @Override
  public void measure() {
    int contentHeight = 0;
    int contentWidth = 0;
    for (int i = 0; i < getChildCount(); ++i) {
      LynxBaseUI child = getChildAt(i);
      contentHeight = Math.max(contentHeight,
          child.getHeight() + child.getTop() + child.getMarginBottom() + mPaddingBottom);
      contentWidth = Math.max(contentWidth,
          child.getWidth() + child.getLeft() + child.getMarginRight() + mPaddingRight);
    }
    mView.setScrollContentSize(new int[] {contentWidth, contentHeight});
    flushFirstRenderOperations();
    updateScrollPosition();
    super.measure();
  }

  private void flushFirstRenderOperations() {
    if (mFirstRender) {
      for (LynxUIScrollViewInternalNodeReadyBlock block : mFirstRenderBlockArray) {
        block.invoke(this);
      }
      mFirstRenderBlockArray = null;
      mFirstRender = false;
    }
  }

  private void sendScrollEvent(String name, HashMap<String, Object> params) {
    HashMap<String, Object> scrollEventDetail = new HashMap<>();
    LynxBaseScrollView scrollView = mView;
    scrollEventDetail.put("scrollLeft", scrollView.getScrollOffsetHorizontally());
    scrollEventDetail.put("scrollTop", scrollView.getScrollOffsetVertically());
    scrollEventDetail.put("scrollHeight", scrollView.getScrollRangeVertically()[1]);
    scrollEventDetail.put("scrollWidth", scrollView.getScrollRangeHorizontally()[1]);
    scrollEventDetail.put(
        "isDragging", scrollView.currentScrollState() == LynxBaseScrollView.SCROLL_STATE_DRAGGING);
    scrollEventDetail.put("scrollState", scrollView.currentScrollState());

    if (params != null) {
      scrollEventDetail.putAll(params);
    }
    mContext.getEventEmitter().sendCustomEvent(
        new LynxCustomEvent(getSign(), name, scrollEventDetail));
  }

  public void scrollInto(LynxBaseUI node, boolean isSmooth, String type) {
    int[] scrollTarget = {0, 0};
    if ("center".equals(type)) {
      scrollTarget[1] -= (this.getView().getHeight() - node.getHeight()) / 2;
      scrollTarget[0] -= (this.getView().getWidth() - node.getWidth()) / 2;
    } else if ("end".equals(type)) {
      scrollTarget[1] -= (this.getView().getHeight() - node.getHeight());
      scrollTarget[0] -= (this.getView().getWidth() - node.getWidth());
    }

    while (node != this) {
      scrollTarget[1] += node.getTop();
      scrollTarget[0] += node.getLeft();
      node = (LynxBaseUI) node.getParentBaseUI();
      while (node instanceof LynxFlattenUI) {
        node = (LynxBaseUI) node.getParent();
      }
    }

    if (isSmooth) {
      mView.animatedScrollTo(scrollTarget, null);
    } else {
      mView.scrollTo(scrollTarget);
    }
  }
}
