// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.list;

import android.view.View;
import android.view.ViewGroup;
import com.lynx.react.bridge.Dynamic;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.ReadableType;
import com.lynx.tasm.ListNodeInfoFetcher;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.view.UIComponent;
import com.lynx.tasm.behavior.ui.view.UISimpleView;

public abstract class AbsLynxList<T extends ViewGroup> extends UISimpleView<T> {
  protected static final String LIST_TYPE_FLOW = "flow";
  protected static final String LIST_TYPE_WATERFALL = "waterfall";
  protected static final String LIST_TYPE_SINGLE = "single";
  protected static final String METHOD_PARAMS_POSITION = "position";
  protected static final String METHOD_PARAMS_INDEX = "index";
  protected static final String METHOD_PARAMS_SMOOTH = "smooth";
  protected static final String METHOD_PARAMS_ALIGN_TO = "alignTo";
  protected static final String METHOD_PARAMS_ITEM_HEIGHT = "itemHeight";
  protected static final String ALIGN_TO_NONE = "none";
  protected static final String ALIGN_TO_TOP = "top";
  protected static final String ALIGN_TO_BOTTOM = "bottom";
  protected static final String ALIGN_TO_MIDDLE = "middle";
  protected static final String METHOD_PARAMS_OFFSET = "offset";

  private int[] mCellViewLocation = new int[2];
  private int[] mRootViewLocation = new int[2];
  private ListNodeInfoFetcher mListNodeInfoFetcher;
  private LynxBaseUI mCurrentChild;

  public AbsLynxList(LynxContext context) {
    super(context);
    mListNodeInfoFetcher = context.getListNodeInfoFetcher();
    mCurrentChild = null;
  }

  /* read list data from lynx-core
   * @return  a map contains list new data
   * */
  public final JavaOnlyMap getPlatformInfo() {
    return mListNodeInfoFetcher.getPlatformInfo(getSign());
  }

  /* render child component
   * @params  index, the data index in list data array
   * @params  operationId, a unique id of this operation
   * */
  public final LynxUI renderChild(int index, long operationId) {
    mListNodeInfoFetcher.renderChild(getSign(), index, operationId);
    LynxUI node = (LynxUI) mCurrentChild;
    mCurrentChild = null;
    return node;
  }

  /* update child component
   * @params  child, the old reuse child
   * @params  index,  the data index in list data array
   * @params  operationId, a unique id of this operation
   * */
  public final void updateChild(LynxUI child, int index, long operationId) {
    mListNodeInfoFetcher.updateChild(getSign(), child.getSign(), index, operationId);
  }

  /* remove child component
   * @params child, the child LynxUI
   * */
  public final void removeChild(LynxUI child) {
    mListNodeInfoFetcher.removeChild(getSign(), child.getSign());
  }

  /* list new arch API, obtain child component
   * @params index, the child index in data array
   * */
  public final LynxUI obtainChild(int index, long operationId, boolean enableReuseNotification) {
    int childSign =
        mListNodeInfoFetcher.obtainChild(getSign(), index, operationId, enableReuseNotification);
    if (childSign > 0) {
      LynxBaseUI node = mContext.findLynxUIBySign(childSign);
      if (node != null && node instanceof UIComponent) {
        return (UIComponent) node;
      }
    }
    return null;
  }

  /**
   * list new arch API, obtain child component asynchronously
   * @param index  the child index in data array
   * @param operationId  the uniqueId for this operation
   */
  public final void obtainChildAsync(int index, long operationId) {
    mListNodeInfoFetcher.obtainChildAsync(getSign(), index, operationId);
  }

  /* list new arch API, recycler child component
   * @params child, the child LynxUI
   * */
  public final void recycleChild(LynxUI child) {
    mListNodeInfoFetcher.recycleChild(getSign(), child.getSign());
  }

  /**
   * list new arch API, recycler child component asynchronously
   * @param child  the child component
   */
  public final void recycleChildAsync(LynxUI child) {
    mListNodeInfoFetcher.recycleChildAsync(getSign(), child.getSign());
  }

  /* when a new child created
   * @params child, instance of child
   * @params index, index of child in children
   * */
  @Override
  public void onInsertChild(LynxBaseUI child, int index) {}

  @Override
  public final void insertChild(LynxBaseUI child, int index) {
    mCurrentChild = child;
    child.setParent(this);
    mChildren.add(mChildren.size(), child);
    onInsertChild(child, index);
  }

  @LynxProp(name = "scroll-y", customType = "true") public abstract void setScrollY(Dynamic enable);

  @LynxProp(name = "scroll-x", customType = "false")
  public abstract void setScrollX(Dynamic enable);

  @LynxProp(name = "needs-visible-cells", defaultBoolean = false)
  public abstract void setNeedVisibleCells(boolean needVisibleCells);

  @LynxProp(name = "upper-threshold", defaultInt = 50)
  public abstract void setUpperThreshold(Dynamic value);

  @LynxProp(name = "lower-threshold", defaultInt = 50)
  public abstract void setLowerThreshold(Dynamic value);

  @LynxProp(name = "scroll-event-throttle", customType = "200")
  public abstract void setScrollEventThrottle(Dynamic value);

  @LynxProp(name = "upper-threshold-item-count", defaultInt = 0)
  public void setUpperThresholdItemCount(Dynamic value) {}

  @LynxProp(name = "lower-threshold-item-count", defaultInt = 0)
  public void setLowerThresholdItemCount(Dynamic value) {}

  @LynxProp(name = "scroll-state-change-event-throttle", customType = "10")
  public abstract void setScrollStateChangeEventThrottle(String value);

  @LynxProp(name = "cache-queue-ratio", customType = "1")
  public abstract void setCacheQueueRatio(Dynamic value);

  @LynxProp(name = "column-count", defaultInt = 1)
  public abstract void setColumnCount(int columnCount);

  @LynxProp(name = "list-main-axis-gap", customType = "0")
  public abstract void setMainAxisGap(float gap);

  @LynxProp(name = "list-cross-axis-gap", customType = "0")
  public abstract void setCrossAxisGap(float gap);

  @LynxProp(name = "list-type", customType = "single")
  public abstract void setListType(String listType);

  @LynxProp(name = "update-animation", customType = "none")
  public abstract void setUpdateAnimation(String animationType);

  @LynxProp(name = "paging-enabled", customType = "false")
  public abstract void setEnablePagerSnap(Dynamic enable);

  @LynxProp(name = "item-snap") public abstract void setPagingAlignment(ReadableMap map);

  @LynxProp(name = "internal-cell-appear-notification", defaultBoolean = false)
  public void setInternalCellAppearNotification(boolean isNeedAppearNotification) {}

  @LynxProp(name = "internal-cell-disappear-notification", defaultBoolean = false)
  public void setInternalCellDisappearNotification(boolean isNeedDisAppearNotification) {}

  @LynxProp(name = "internal-cell-prepare-for-reuse-notification", defaultBoolean = false)
  public void setInternalCellPrepareForReuseNotification(boolean isNeedReuseNotification) {}

  @LynxProp(name = "should-request-state-restore", defaultBoolean = false)
  public void setShouldRequestStateRestore(boolean shouldRequestStateRestore) {}

  @LynxProp(name = "over-scroll", customType = "false")
  public void setOverScroll(Dynamic enable) {
    ReadableType type = enable.getType();
    boolean alwaysOverscroll = true;
    if (type == ReadableType.String) {
      alwaysOverscroll = "true".equals(enable.asString());
    } else if (type == ReadableType.Boolean) {
      alwaysOverscroll = enable.asBoolean();
    }
    if (alwaysOverscroll) {
      mView.setOverScrollMode(View.OVER_SCROLL_ALWAYS);
    } else {
      mView.setOverScrollMode(View.OVER_SCROLL_NEVER);
    }
  }

  @LynxProp(name = "sticky") public abstract void setEnableSticky(Dynamic enable);

  @LynxProp(name = "sticky-offset", defaultInt = 0)
  public abstract void setStickyOffset(Dynamic value);

  // event api
  public abstract void sendCustomEvent(int left, int top, int dx, int dy, String type);

  public double getCellOffsetByIndex(int index) {
    double res = 0;
    for (LynxBaseUI child : mChildren) {
      if (child.getSign() == index && child instanceof LynxUI) {
        getLynxContext().getUIBody().getView().getLocationOnScreen(mRootViewLocation);
        ((LynxUI) child).getView().getLocationOnScreen(mCellViewLocation);
        res = mCellViewLocation[1] - mRootViewLocation[1];
        break;
      }
    }
    return res;
  }

  @LynxProp(name = "enable-scroll", customType = "true")
  public abstract void setScrollEnable(Dynamic value);

  @LynxProp(name = "touch-scroll", customType = "true")
  public abstract void setTouchScroll(Dynamic value);

  @LynxProp(name = "initial-scroll-index", customType = "0")
  public abstract void setInitialScrollIndex(Dynamic value);

  @Override
  public boolean isScrollable() {
    return true;
  }

  @LynxProp(name = "no-invalidate", defaultBoolean = false)
  public abstract void setNoInvalidate(boolean noInvalidate);

  /**
   * When the component is first rendered, trigger mUIComponent#measureChildren() in layout pass.
   * Detail can be seen issue: #7563.
   */
  @LynxProp(name = "component-init-measure", defaultBoolean = false)
  public abstract void setComponentInitMeasure(boolean value);
}
