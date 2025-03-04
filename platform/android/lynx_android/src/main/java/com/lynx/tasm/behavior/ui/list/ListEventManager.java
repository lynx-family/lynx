// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.list;

import static android.view.View.LAYOUT_DIRECTION_RTL;

import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.StaggeredGridLayoutManager;
import com.lynx.react.bridge.Dynamic;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableType;
import com.lynx.tasm.EventEmitter;
import com.lynx.tasm.LynxViewClient;
import com.lynx.tasm.behavior.ui.ScrollStateChangeListener;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.event.LynxDetailEvent;
import com.lynx.tasm.event.LynxListEvent;
import com.lynx.tasm.gesture.arena.GestureArenaManager;
import com.lynx.tasm.utils.DisplayMetricsHolder;
import com.lynx.tasm.utils.PixelUtils;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.Map;

/*
 *  listen RecyclerView state, send related events to front-end;
 * */
public class ListEventManager extends RecyclerView.OnScrollListener {
  /* event types of front-end */
  protected static final int SCROLL_EVENT_ON = 1 << 0;
  private static final int SCROLL_TO_UPPER_EVENT_ON = 1 << 1;
  private static final int SCROLL_TO_LOWER_EVENT_ON = 1 << 2;
  private static final int SCROLL_STATE_EVENT_ON = 1 << 3;
  private static final int LAYOUT_COMPONENT_ON = 1 << 4;

  private static final int SCROLL_EVENT_THROTTLE_DEFAULT = 200;
  private static final int UPPER_THRESHOLD_DEFAULT = 50;
  private static final int LOWER_THRESHOLD_DEFAULT = 50;

  private static final int SCROLL_DIRECTION_UP = -1;
  private static final int SCROLL_DIRECTION_DOWN = 1;

  // bit set 0x0 default; 0x1 upper; 0x2 lower; 0x4 ready to upper; 0x8 ready to lower
  private static final int BORDER_STATUS_DEFAULT = 0;
  private static final int BORDER_STATUS_UPPER = 1 << 0;
  private static final int BORDER_STATUS_LOWER = 1 << 1;
  private static final int BORDER_STATUS_READY_TO_UPPER = 1 << 2;
  private static final int BORDER_STATUS_READY_TO_LOWER = 1 << 3;

  private final EventEmitter mEventEmitter;
  private final RecyclerView mRecyclerView;
  private int mEventEnableBitMask = 0;
  private int mScrollEventThrottleMs = SCROLL_EVENT_THROTTLE_DEFAULT;
  private int mUpperThresholdPx = UPPER_THRESHOLD_DEFAULT;
  private int mLowerThresholdPx = LOWER_THRESHOLD_DEFAULT;
  private int mUpperThresholdItemCount = 0;
  private int mLowerThresholdItemCount = 0;
  private long mLastScrollEventTime = 0;

  private final UIList mUIList;
  // make border event trigger only once when user dragging
  private boolean mHasBorderWhenDragging = true;
  // the total scroll offset on y-axis
  int mScrollTop = 0;
  // the last time status of border, bit set 0x1 upper; 0x2 lower
  private int mLastBorderStatus = BORDER_STATUS_UPPER;

  /* package */ boolean mNeedsVisibleCells = false;
  public ListEventManager(EventEmitter eventEmitter, RecyclerView recyclerView, UIList list) {
    mEventEmitter = eventEmitter;
    mRecyclerView = recyclerView;
    mRecyclerView.addOnScrollListener(this);
    mUIList = list;
  }

  // get the event types that front-end need
  protected void setEvents(Map<String, EventsListener> events) {
    mEventEnableBitMask = 0;
    if (events == null) {
      return;
    }
    mEventEnableBitMask = events.containsKey(LynxListEvent.EVENT_SCROLL)
        ? (mEventEnableBitMask | SCROLL_EVENT_ON)
        : mEventEnableBitMask;
    mEventEnableBitMask = events.containsKey(LynxListEvent.EVENT_SCROLL_TOUPPER)
        ? (mEventEnableBitMask | SCROLL_TO_UPPER_EVENT_ON)
        : mEventEnableBitMask;
    mEventEnableBitMask = events.containsKey(LynxListEvent.EVENT_SCROLL_TOLOWER)
        ? (mEventEnableBitMask | SCROLL_TO_LOWER_EVENT_ON)
        : mEventEnableBitMask;
    mEventEnableBitMask = events.containsKey(LynxListEvent.EVENT_SCROLL_STATE_CHANGE)
        ? (mEventEnableBitMask | SCROLL_STATE_EVENT_ON)
        : mEventEnableBitMask;
    mEventEnableBitMask = events.containsKey(LynxListEvent.EVENT_LAYOUT_COMPLETE)
        ? (mEventEnableBitMask | LAYOUT_COMPONENT_ON)
        : mEventEnableBitMask;
  }

  public boolean isLayoutCompleteEnable() {
    return (mEventEnableBitMask & LAYOUT_COMPONENT_ON) != 0;
  }

  public void sendLayoutCompleteEvent(final JavaOnlyArray cells) {
    if (!isLayoutCompleteEnable()) {
      return;
    }
    LynxDetailEvent detailEvent =
        new LynxDetailEvent(mUIList.getSign(), LynxListEvent.EVENT_LAYOUT_COMPLETE);
    detailEvent.addDetail("timestamp", new Date().getTime());
    detailEvent.addDetail("cells", cells);
    mEventEmitter.sendCustomEvent(detailEvent);
  }

  void setUpperThreshold(Dynamic value) {
    mUpperThresholdPx = dynamicToInt(value, UPPER_THRESHOLD_DEFAULT);
    mUpperThresholdItemCount = 0;
  }

  void setLowerThreshold(Dynamic value) {
    mLowerThresholdPx = dynamicToInt(value, LOWER_THRESHOLD_DEFAULT);
    mLowerThresholdItemCount = 0;
  }

  void setUpperThresholdItemCount(Dynamic value) {
    mUpperThresholdItemCount = dynamicToInt(value, 0);
    mUpperThresholdPx = 0;
  }

  void setLowerThresholdItemCount(Dynamic value) {
    mLowerThresholdItemCount = dynamicToInt(value, 0);
    mLowerThresholdPx = 0;
  }

  void setScrollEventThrottle(Dynamic value) {
    mScrollEventThrottleMs = dynamicToInt(value, SCROLL_EVENT_THROTTLE_DEFAULT);
  }

  @Override
  public void onScrolled(RecyclerView recyclerView, int dx, int dy) {
    // onScrolled will be call when recyclerView loaded, even no scroll
    if (dx == 0 && dy == 0) {
      return;
    }
    int scrollDistance = mUIList.isVertical() ? dy : dx;

    // if is RTL, the scrollDistance < 0
    if (LAYOUT_DIRECTION_RTL == mRecyclerView.getLayoutDirection()) {
      scrollDistance = -scrollDistance;
    }

    mScrollTop += scrollDistance;
    // send scroll event
    if (System.currentTimeMillis() - mLastScrollEventTime > mScrollEventThrottleMs) {
      sendScrollEvent(LynxListEvent.EVENT_SCROLL, SCROLL_EVENT_ON, mScrollTop, mScrollTop, dx, dy);
      mLastScrollEventTime = System.currentTimeMillis();
    }

    int status = updateBorderStatus();
    boolean isUpper = isUpper(status) && !isUpper(mLastBorderStatus);
    boolean isReadyUpper = isReadyUpper(status) && !isReadyUpper(mLastBorderStatus);
    boolean isLower = isLower(status) && !isLower(mLastBorderStatus);
    boolean isReadyLower = isReadyLower(status) && !isReadyLower(mLastBorderStatus);

    if (scrollDistance < 0 && (isUpper || isReadyUpper)) {
      sendScrollEvent(LynxListEvent.EVENT_SCROLL_TOUPPER, SCROLL_TO_UPPER_EVENT_ON, mScrollTop,
          mScrollTop, 0, 0);
    } else if (scrollDistance > 0 && (isLower || isReadyLower)) {
      sendScrollEvent(LynxListEvent.EVENT_SCROLL_TOLOWER, SCROLL_TO_LOWER_EVENT_ON, mScrollTop,
          mScrollTop, 0, 0);
    }

    mLastBorderStatus = status;
  }

  /**
   * reset scroll border status of last scrolling
   */
  protected void resetScrollBorderStatus() {
    mLastBorderStatus = BORDER_STATUS_DEFAULT;
  }

  @Override
  public void onScrollStateChanged(RecyclerView recyclerView, int newState) {
    mUIList.recognizeGesturere();
    switch (newState) {
      case RecyclerView.SCROLL_STATE_IDLE: { // scroll finish
        mUIList.getLynxContext().getFluencyTraceHelper().stop(mUIList.getSign());
        sendScrollStateChangeEvent(
            LynxListEvent.SCROLL_STATE_IDLE, LynxListEvent.EVENT_SCROLL_STATE_CHANGE);
        if (mUIList.isEnableScrollMonitor()) {
          mUIList.getLynxContext().getLynxViewClient().onScrollStop(new LynxViewClient.ScrollInfo(
              recyclerView, mUIList.getTagName(), mUIList.getScrollMonitorTag()));
        }
        if (mUIList.mEnableGapItemDecoration) {
          recyclerView.invalidateItemDecorations();
        }
        mUIList.notifyScrollStateChanged(ScrollStateChangeListener.SCROLL_STATE_IDLE);
        break;
      }
      case RecyclerView.SCROLL_STATE_DRAGGING: {
        mUIList.getLynxContext().getFluencyTraceHelper().start(
            mUIList.getSign(), "scroll", mUIList.getScrollMonitorTag());
        mHasBorderWhenDragging = false;
        sendScrollStateChangeEvent(
            LynxListEvent.SCROLL_STATE_DRAGGING, LynxListEvent.EVENT_SCROLL_STATE_CHANGE);
        if (mUIList.isEnableScrollMonitor()) {
          mUIList.getLynxContext().getLynxViewClient().onScrollStart(new LynxViewClient.ScrollInfo(
              recyclerView, mUIList.getTagName(), mUIList.getScrollMonitorTag()));
        }

        mUIList.notifyScrollStateChanged(ScrollStateChangeListener.SCROLL_STATE_DRAGGING);
        break;
      }
      case RecyclerView.SCROLL_STATE_SETTLING: {
        sendScrollStateChangeEvent(
            LynxListEvent.SCROLL_STATE_SETTLING, LynxListEvent.EVENT_SCROLL_STATE_CHANGE);
        if (mUIList.isEnableScrollMonitor()) {
          mUIList.getLynxContext().getLynxViewClient().onFling(new LynxViewClient.ScrollInfo(
              recyclerView, mUIList.getTagName(), mUIList.getScrollMonitorTag()));
        }
        mUIList.notifyScrollStateChanged(ScrollStateChangeListener.SCROLL_STATE_SETTLING);
        break;
      }
      default: {
        break;
      }
    }
  }

  /**
   * the caller is LayoutManager when try to scroll vertically / horizontally
   * layoutManager can not consume scroll mean border come
   * @param distance, target scroll offset
   * @param consumeDistance, scroll offset can consumed by layoutManager
   */
  void onScrollBy(int distance, int consumeDistance) {
    if (!mHasBorderWhenDragging && 0 == consumeDistance) {
      if (distance > 0) {
        // fix the problem if LowerThresholdItemCount/mLowerThresholdPx !=0, the scroll event should
        // cannot be called even if the list cannot scroll
        if (mUIList.getUpperLowerSwitch()
            && (mLowerThresholdItemCount != 0 || mLowerThresholdPx != 0)) {
          return;
        }
        sendScrollEvent(LynxListEvent.EVENT_SCROLL_TOLOWER, SCROLL_TO_LOWER_EVENT_ON, mScrollTop,
            mScrollTop, 0, 0);
        mHasBorderWhenDragging = true;
      } else if (distance < 0) {
        mScrollTop = 0;
        if (mUIList.getUpperLowerSwitch()
            && (mUpperThresholdItemCount != 0 || mUpperThresholdPx != 0)) {
          return;
        }
        sendScrollEvent(LynxListEvent.EVENT_SCROLL_TOUPPER, SCROLL_TO_UPPER_EVENT_ON, mScrollTop,
            mScrollTop, 0, 0);
        mHasBorderWhenDragging = true;
      }
    }
  }
  /**
   * check recyclerView come to border
   * @return border status, 0x1 : upper, 0x2 : lower, 0x4 : ready to upper, 0x8 : ready to lower
   */
  private int updateBorderStatus() {
    int status = 0;
    // check if current visible items reach the threshold-item-count
    RecyclerView.LayoutManager layoutManager = mRecyclerView.getLayoutManager();
    int childCount = mRecyclerView.getChildCount();
    int itemCount = layoutManager.getItemCount();
    boolean isVertical = mUIList.isVertical();
    int startOfVisibleArea =
        isVertical ? layoutManager.getPaddingTop() : layoutManager.getPaddingLeft();
    int endOfVisibleArea = isVertical ? layoutManager.getHeight() - layoutManager.getPaddingBottom()
                                      : layoutManager.getWidth() - layoutManager.getPaddingRight();

    if (mUpperThresholdItemCount > 0 || mLowerThresholdItemCount > 0) {
      int topBorderItemPosition = mUpperThresholdItemCount;
      int bottomBorderItemPosition = itemCount - mLowerThresholdItemCount - 1;
      int firstChildPosition = Integer.MAX_VALUE;
      int lastChildPosition = Integer.MIN_VALUE;

      for (int i = 0; i < childCount; ++i) {
        android.view.View child = mRecyclerView.getChildAt(i);
        int position = mRecyclerView.getChildLayoutPosition(child);
        int startBoundPx = isVertical ? layoutManager.getDecoratedTop(child)
                                      : layoutManager.getDecoratedLeft(child);
        int endBoundPx = isVertical ? layoutManager.getDecoratedBottom(child)
                                    : layoutManager.getDecoratedRight(child);
        if (endBoundPx > startOfVisibleArea) {
          firstChildPosition = Math.min(position, firstChildPosition);
        }
        if (startBoundPx < endOfVisibleArea) {
          lastChildPosition = Math.max(position, lastChildPosition);
        }
      }

      if (firstChildPosition < topBorderItemPosition) {
        status |= BORDER_STATUS_READY_TO_UPPER;
      }
      if (lastChildPosition > bottomBorderItemPosition) {
        status |= BORDER_STATUS_READY_TO_LOWER;
      }
    }

    // check if current visible area reach the threshold-px
    boolean firstItemVisible = mRecyclerView.findViewHolderForLayoutPosition(0) != null;
    boolean lastItemVisible = mRecyclerView.findViewHolderForLayoutPosition(itemCount - 1) != null;
    if (firstItemVisible || lastItemVisible) {
      // the min and max Y-offset of visible cells.
      int startBoundPx = Integer.MAX_VALUE;
      int endBoundPx = Integer.MIN_VALUE;
      for (int i = 0; i < childCount; ++i) {
        android.view.View child = mRecyclerView.getChildAt(i);
        startBoundPx = isVertical ? Math.min(layoutManager.getDecoratedTop(child), startBoundPx)
                                  : Math.min(layoutManager.getDecoratedLeft(child), startBoundPx);
        endBoundPx = isVertical ? Math.max(layoutManager.getDecoratedBottom(child), endBoundPx)
                                : Math.max(layoutManager.getDecoratedRight(child), endBoundPx);
      }
      if (firstItemVisible) {
        if (startBoundPx == startOfVisibleArea) {
          mScrollTop = 0;
        }
        // check if the top border threshold-px reached
        if (startBoundPx > startOfVisibleArea - mUpperThresholdPx) {
          status |= BORDER_STATUS_UPPER;
          if (mUpperThresholdItemCount > 0) {
            status &= (~BORDER_STATUS_READY_TO_UPPER);
          }
        }
      }

      if (lastItemVisible) {
        // check if the bottom border threshold-px reached
        if (endBoundPx < endOfVisibleArea + mLowerThresholdPx) {
          status |= BORDER_STATUS_LOWER;
          if (mLowerThresholdItemCount > 0) {
            status &= (~BORDER_STATUS_READY_TO_LOWER);
          }
        }
      }
    }
    return status;
  }

  /**
   * get scroll offset, when list is horizontal, return x offset, otherwise return y offset
   * @return
   */
  public int getScrollOffset() {
    return mScrollTop;
  }

  private boolean isUpper(int status) {
    return (status & BORDER_STATUS_UPPER) != 0;
  }

  private boolean isLower(int status) {
    return (status & BORDER_STATUS_LOWER) != 0;
  }

  private boolean isReadyUpper(int status) {
    return (status & BORDER_STATUS_READY_TO_UPPER) != 0;
  }

  private boolean isReadyLower(int status) {
    return (status & BORDER_STATUS_READY_TO_LOWER) != 0;
  }

  /* send scroll events to front-end */
  protected void sendScrollEvent(String type, int mask, int left, int top, int dx, int dy) {
    if ((mEventEnableBitMask & mask) != 0) {
      LynxListEvent event = LynxListEvent.createListEvent(mUIList.getSign(), type);
      JavaOnlyArray visibleCells = mNeedsVisibleCells ? getVisibleCellsInfo() : null;
      event.setScrollParams(left, top, dx, dy, visibleCells);
      mEventEmitter.sendCustomEvent(event);
    }
  }

  // send scroll state change events to front-end
  private void sendScrollStateChangeEvent(int state, String type) {
    if ((mEventEnableBitMask & SCROLL_STATE_EVENT_ON) == 0) {
      return;
    }
    LynxListEvent event = LynxListEvent.createListEvent(mUIList.getSign(), type);
    JavaOnlyArray visibleCells = mNeedsVisibleCells ? getVisibleCellsInfo() : null;
    event.setListScrollStateChangeParams(state, visibleCells);
    mEventEmitter.sendCustomEvent(event);
  }

  JavaOnlyArray getVisibleCellsInfo() {
    JavaOnlyArray visibleCells = new JavaOnlyArray();
    ArrayList<Integer> visiblePositions = new ArrayList<>();
    RecyclerView.LayoutManager lm = mRecyclerView.getLayoutManager();
    if (lm instanceof LinearLayoutManager) {
      LinearLayoutManager llm = (LinearLayoutManager) lm;
      int firstVisible = llm.findFirstVisibleItemPosition();
      int lastVisible = llm.findLastVisibleItemPosition();
      for (int i = firstVisible; i <= lastVisible; i++) {
        visiblePositions.add(i);
      }
    } else if (lm instanceof StaggeredGridLayoutManager) {
      StaggeredGridLayoutManager glm = (StaggeredGridLayoutManager) lm;
      int[] first = glm.findFirstVisibleItemPositions(null);
      int[] last = glm.findLastVisibleItemPositions(null);
      int min = Integer.MAX_VALUE;
      int max = Integer.MIN_VALUE;
      for (int value : first) {
        visiblePositions.add(value);
        max = Math.max(max, value);
      }
      for (int value : last) {
        visiblePositions.add(value);
        min = Math.min(min, value);
      }
      for (int i = max + 1; i < min; i++) {
        visiblePositions.add(i);
      }
      Collections.sort(visiblePositions);
    }
    for (int i : visiblePositions) {
      ListViewHolder holder = (ListViewHolder) mRecyclerView.findViewHolderForLayoutPosition(i);
      if (holder == null || holder.getUIComponent() == null) {
        continue;
      }
      android.view.View view = holder.itemView;
      JavaOnlyMap map = new JavaOnlyMap();
      map.put("id", holder.getUIComponent().getIdSelector());
      map.put("position", i);
      map.put("index", i);
      map.put("itemKey", holder.getUIComponent().getItemKey());
      map.put("top", PixelUtils.pxToDip(view.getTop()));
      map.put("bottom", PixelUtils.pxToDip(view.getBottom()));
      map.put("left", PixelUtils.pxToDip(view.getLeft()));
      map.put("right", PixelUtils.pxToDip(view.getRight()));
      visibleCells.add(map);
    }
    return visibleCells;
  }

  public static int dynamicToInt(Dynamic value, int defaultVal) {
    int intValue = defaultVal;
    ReadableType type = value.getType();
    if (type == ReadableType.String) {
      try {
        intValue = Integer.parseInt(value.asString());
      } catch (NumberFormatException e) {
        e.printStackTrace();
      }
    } else if (type == ReadableType.Int || type == ReadableType.Number
        || type == ReadableType.Long) {
      intValue = value.asInt();
    }
    return intValue;
  }

  public static boolean dynamicToBoolean(Dynamic value, boolean defaultVal) {
    boolean boolVal = defaultVal;
    if (value == null) {
      return boolVal;
    }
    ReadableType type = value.getType();

    if (type == ReadableType.String) {
      boolVal = Boolean.parseBoolean(value.asString());
    } else if (type == ReadableType.Int || type == ReadableType.Number
        || type == ReadableType.Long) {
      boolVal = value.asInt() != 0;
    } else if (type == ReadableType.Boolean) {
      boolVal = value.asBoolean();
    }
    return boolVal;
  }

  // int, long, number, str to str
  public static String dynamicToString(Dynamic value, String defaultVal) {
    String strVal = defaultVal;
    if (value == null) {
      return strVal;
    }
    ReadableType type = value.getType();
    if (type == ReadableType.String) {
      strVal = value.asString();
    } else if (type == ReadableType.Int || type == ReadableType.Number
        || type == ReadableType.Long) {
      strVal = String.valueOf(value.asLong());
    }
    return strVal;
  }
}
