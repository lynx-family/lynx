// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.list;

import android.text.TextUtils;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import androidx.core.view.ViewCompat;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.StaggeredGridLayoutManager;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.ui.view.UIComponent;
import com.lynx.tasm.utils.PixelUtils;
import com.lynx.tasm.utils.UIThreadUtils;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

public class UIListAdapter extends RecyclerView.Adapter<ListViewHolder> {
  // Map the component name to RecyclerView's view type
  private final @NonNull HashMap<String, Integer> mTypesToInt;

  // Manage the pending updating view holder, produced by renderChild and updateChild
  // method call, consumed by the layoutDidUpdate method call.
  private final @NonNull HashMap<Long, ListViewHolder> mLayoutMap;

  private final @NonNull UIList mList;

  // new list arch enable
  boolean mNewArch = false;
  // one layout consume one diff result,  if multiable diff result come, using
  // notifyDataSetChanged()
  boolean mDiffResultConsumed = false;
  boolean mComponentInitMeasure = false;
  boolean isAsync = false;

  // Component names array.
  /* package */ JavaOnlyArray mViewNames;
  private JavaOnlyArray mFullSpans;
  private JavaOnlyArray mStickyTopItems;
  private JavaOnlyArray mStickyBottomItems;
  private JavaOnlyArray mComponentEstimatedHeight;
  private JavaOnlyArray mComponentEstimatedHeightPx;
  // the fiber's original data
  private JavaOnlyArray mFiberFullSpans;
  private JavaOnlyArray mFiberStickyTopItems;
  private JavaOnlyArray mFiberStickyBottomItems;

  private final OperationDispatcher mOperationDispatcher;
  private int mBaseOperationId = 0;

  private final @NonNull AppearEventCourier mCourier;

  JavaOnlyArray mItemKeys;
  HashMap<String, Integer> itemKeyMap;
  private boolean mAppearNotificationEnable = false;
  private boolean mDisAppearNotificationEnable = false;
  private boolean mReuseNotificationEnable = false;
  // when list‘s data has changes, the value will be ture, and we will init buffer cache
  boolean shouldInitCache = false;

  private final Runnable mDispatchOpRunnable = new Runnable() {
    @Override
    public void run() {
      // Checks if RecyclerView is in the middle of a layout or scroll and throws an
      // IllegalStateException if it is.
      try {
        mOperationDispatcher.dispatch();
      } catch (IllegalStateException e) {
        LLog.e(UIList.TAG,
            "operationDispatcher.dispatch() leads to the IllegalStateException : "
                + e.getMessage());
      }
    }
  };

  private final Runnable mNotifyDataSetChangedRunnable = new Runnable() {
    @Override
    public void run() {
      // Checks if RecyclerView is in the middle of a layout or scroll and throws an
      // IllegalStateException if it is.
      try {
        notifyDataSetChanged();
      } catch (IllegalStateException e) {
        LLog.e(UIList.TAG,
            "notifyDataSetChanged leads to the IllegalStateException : " + e.getMessage());
      }
    }
  };

  private ArrayList<HashMap<Integer, Integer>> mItemHeightInfo = new ArrayList<>();
  private HashMap<String, Integer> mAsyncItemSizeCache = new HashMap<>();

  public UIListAdapter(@NonNull UIList list, @NonNull AppearEventCourier eventManager) {
    mTypesToInt = new HashMap<>();
    mLayoutMap = new HashMap<>();
    mOperationDispatcher = new OperationDispatcher();
    mList = list;
    mCourier = eventManager;
  }

  /**
   * return true if it is the new list architecture
   */
  public final boolean getNewArch() {
    return mNewArch;
  }

  /**
   * full span info
   * such as <list-row></>、<header></>、<footer></>
   * @return
   */
  public final JavaOnlyArray getFullSpans() {
    return mFullSpans;
  }

  /**
   * sticky top info
   * @return
   */
  public final JavaOnlyArray getStickyTopItems() {
    return mStickyTopItems;
  }

  /**
   * sticky bottom info
   * @return
   */
  public final JavaOnlyArray getStickyBottomItems() {
    return mStickyBottomItems;
  }

  protected void initItemHeightData() {
    mItemHeightInfo.clear();
    for (int i = 0; i < mList.mColumnCount; i++) {
      HashMap<Integer, Integer> columnItemHeightInfo = new HashMap<>();
      mItemHeightInfo.add(columnItemHeightInfo);
    }
  }

  void updateChildrenInfo(JavaOnlyMap platformInfo) {
    boolean fullFlush = false;
    /* when insert new item before footer, need full flush, do not use diif
     *  because new item should render on screen
     * */
    if (mViewNames != null && mFullSpans != null && mViewNames.size() == mFullSpans.size()) {
      fullFlush = true;
    }

    if (platformInfo == null || platformInfo.isEmpty()) {
      return;
    }

    ReadableArray tempItemKeys = platformInfo.getArray("itemkeys");
    if (tempItemKeys instanceof JavaOnlyArray) {
      mItemKeys = ((JavaOnlyArray) tempItemKeys);
    }

    itemKeyMap = new HashMap<>();
    for (int i = 0; i < mItemKeys.size(); i++) {
      itemKeyMap.put(mItemKeys.getString(i), i);
    }

    ReadableArray tempFullSpans = platformInfo.getArray("fullspan");
    if (tempFullSpans instanceof JavaOnlyArray) {
      mFullSpans = ((JavaOnlyArray) tempFullSpans);
    }

    ReadableArray tempViewNames = platformInfo.getArray("viewTypes");
    if (tempViewNames instanceof JavaOnlyArray) {
      mViewNames = ((JavaOnlyArray) tempViewNames);
    }

    ReadableArray tempStickyTopItems = platformInfo.getArray("stickyTop");
    if (tempStickyTopItems instanceof JavaOnlyArray) {
      mStickyTopItems = ((JavaOnlyArray) tempStickyTopItems);
    }

    ReadableArray tempStickyBottomItems = platformInfo.getArray("stickyBottom");
    if (tempStickyBottomItems instanceof JavaOnlyArray) {
      mStickyBottomItems = ((JavaOnlyArray) tempStickyBottomItems);
    }

    ReadableArray tempComponentEstimatedHeight = platformInfo.getArray("estimatedHeight");
    if (tempComponentEstimatedHeight instanceof JavaOnlyArray) {
      mComponentEstimatedHeight = ((JavaOnlyArray) tempComponentEstimatedHeight);
    }

    ReadableArray tempComponentEstimatedHeightPx = platformInfo.getArray("estimatedHeightPx");
    if (tempComponentEstimatedHeightPx instanceof JavaOnlyArray) {
      mComponentEstimatedHeightPx = ((JavaOnlyArray) tempComponentEstimatedHeightPx);
    }

    boolean diffable = platformInfo.getBoolean("diffable");
    mNewArch = platformInfo.getBoolean("newarch");
    generateTypesToIntMap();
    // if front-end update data while list is background, list can not consume diff result and data.
    // when multiple diff result batch, android diff API is not correct, using
    // notifyDataSetChanged() instead

    if (!fullFlush && diffable && mDiffResultConsumed) {
      ReadableMap diffResult = platformInfo.getMap("diffResult");
      if (diffResult != null && diffResult.size() > 0) {
        shouldInitCache = true;
      }
      mOperationDispatcher.update(diffResult);
      dispatchOperationSafely();
    } else {
      shouldInitCache = true;
      // if reloading all list data, should clear the height's cache of items
      cleanAsyncItemSizeCache();
      notifyDataSetChangeSafely();
    }
  }

  private void dispatchOperationSafely() {
    if (shouldPostWhenComputingLayout()) {
      UIThreadUtils.runOnUiThread(mDispatchOpRunnable);
    } else {
      mDispatchOpRunnable.run();
    }
  }

  private void notifyDataSetChangeSafely() {
    if (shouldPostWhenComputingLayout()) {
      UIThreadUtils.runOnUiThread(mNotifyDataSetChangedRunnable);
    } else {
      mNotifyDataSetChangedRunnable.run();
    }
  }

  private boolean shouldPostWhenComputingLayout() {
    return mNewArch && mList != null && mList.getView() != null
        && mList.getView().isComputingLayout();
  }

  // init platform data
  private void initPlatformData() {
    if (mItemKeys == null) {
      mItemKeys = new JavaOnlyArray();
    }
    if (mViewNames == null) {
      mViewNames = new JavaOnlyArray();
    }
    if (mFullSpans == null) {
      mFullSpans = new JavaOnlyArray();
    }
    if (mFiberFullSpans == null) {
      mFiberFullSpans = new JavaOnlyArray();
    }
    if (mFiberStickyTopItems == null) {
      mFiberStickyTopItems = new JavaOnlyArray();
    }
    if (mStickyTopItems == null) {
      mStickyTopItems = new JavaOnlyArray();
    }
    if (mFiberStickyBottomItems == null) {
      mFiberStickyBottomItems = new JavaOnlyArray();
    }
    if (mStickyBottomItems == null) {
      mStickyBottomItems = new JavaOnlyArray();
    }
    if (mComponentEstimatedHeightPx == null) {
      mComponentEstimatedHeightPx = new JavaOnlyArray();
    }
  }

  // update list Action info
  void updateListActionInfo(ReadableMap listInfo) {
    if (listInfo == null) {
      return;
    }
    mNewArch = true;
    ReadableArray updateActions = listInfo.getArray("updateAction");
    ReadableArray insertActions = listInfo.getArray("insertAction");
    ReadableArray removeActions = listInfo.getArray("removeAction");
    if (updateActions == null && insertActions == null && removeActions == null) {
      return;
    }
    // init data
    initPlatformData();

    // update different actions
    mOperationDispatcher.dispatchUpdateActions(removeActions, insertActions, updateActions);
    // transform data
    mOperationDispatcher.transformExtraData();
    // flush actions
    boolean reloadAll = listInfo.getBoolean("reloadAll", false);
    if (reloadAll) {
      notifyDataSetChangeSafely();
    } else {
      mOperationDispatcher.flushNODiffActions(removeActions, insertActions, updateActions);
    }
  }

  private void generateTypesToIntMap() {
    for (int i = 0; i < mViewNames.size(); i++) {
      String name = mViewNames.getString(i);
      if (!mTypesToInt.containsKey(name)) {
        mTypesToInt.put(name, mTypesToInt.size());
      }
    }
  }

  long generateOperationId() {
    return ((long) mList.getSign() << 32) + mBaseOperationId++;
  }

  boolean getReuseNotificationEnabled() {
    return mReuseNotificationEnable;
  }

  @NonNull
  @Override
  public ListViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
    if (UIList.DEBUG) {
      LLog.i(UIList.TAG, "Adapter onCreateViewHolder " + viewType);
    }
    ViewGroup.LayoutParams layoutParams = new ViewGroup.LayoutParams(
        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
    ListViewHolder.WrapView wrapView = new ListViewHolder.WrapView(parent.getContext());
    wrapView.setLayoutParams(layoutParams);
    wrapView.setComponentInitMeasure(mComponentInitMeasure);
    wrapView.setLayoutDirection(mList.getView().getLayoutDirection());
    return new ListViewHolder(wrapView);
  }

  // bind measure list
  private void bindMeasureListener(ListViewHolder holder) {
    if (holder != null && holder.mRootView != null
        && holder.mRootView.getMeasureListener() == null) {
      holder.mRootView.setMeasureListener(new ListViewHolder.MeasureListener() {
        @Override
        public void onMeasureCompleted(String itemKey, int width, int height) {
          if (TextUtils.isEmpty(itemKey) || itemKeyMap == null
              || !itemKeyMap.containsKey(itemKey)) {
            return;
          }
          for (int i = 0; i < mItemHeightInfo.size(); i++) {
            int itemPos = itemKeyMap.get(itemKey);
            if (mItemHeightInfo.get(i) != null && mItemHeightInfo.get(i).containsKey(itemPos)) {
              mItemHeightInfo.get(i).put(itemPos, height);
            }
          }
        }
      });
    }
  }

  @Override
  public void onBindViewHolder(@NonNull ListViewHolder holder, int position, List<Object> payload) {
    bindMeasureListener(holder);
    if (mNewArch) {
      Integer toIndex = payload.isEmpty() ? position : (Integer) (payload.get(payload.size() - 1));
      bindViewHolderOnNewArch(holder, toIndex);
      return;
    }
    if (payload.isEmpty()) {
      onBindViewHolder(holder, position);
    } else {
      if (holder.getUIComponent() == null) {
        // when holder's component has been move to sticky, stickymanager will update it
        return;
      }
      long operationId = generateOperationId();
      Integer toIndex = (Integer) (payload.get(payload.size() - 1));
      mLayoutMap.put(operationId, holder);
      mCourier.onListNodeDetached(holder);
      holder.mRootView.mLayoutStatus = ListViewHolder.COMPONENT_LAYOUT_REUSE;
      mList.updateChild(holder.getUIComponent(), toIndex, operationId);
      mCourier.onListNodeAttached(holder);
      setComponentEstimatedSizeFromDataSource(holder, position);
    }
  }

  @Override
  public void onBindViewHolder(@NonNull ListViewHolder holder, int position) {
    bindMeasureListener(holder);
    if (UIList.DEBUG) {
      LLog.i(UIList.TAG, "Adapter onBindViewHolder " + position);
    }
    if (mNewArch) {
      bindViewHolderOnNewArch(holder, position);
      return;
    }
    // We produce an operation id by (list sign) << 23 + position. So we can
    // check the layoutDidFinish operation is produced by the list and specific position.
    long operationId = generateOperationId();
    mLayoutMap.put(operationId, holder);
    if (holder.getUIComponent() == null) {
      UIComponent child = (UIComponent) mList.renderChild(position, operationId);
      // renderChild should reuturn  non null child
      // while sometimes return null on android, the possible reason: lynxView is destroyed
      if (child != null) {
        holder.setUIComponent(child);
        mCourier.holderAttached(holder);
      } else {
        LLog.i(UIList.TAG, "Adapter onBindViewHolder " + position + "child null");
      }
    } else {
      holder.mRootView.mLayoutStatus = ListViewHolder.COMPONENT_LAYOUT_REUSE;
      mList.updateChild(holder.getUIComponent(), position, operationId);
    }

    if (mComponentEstimatedHeightPx != null && mComponentEstimatedHeightPx.size() > position) {
      int height = (int) PixelUtils.dipToPx(mComponentEstimatedHeightPx.getInt(position));
      holder.setEstimatedHeight(height);
    }
    if (mComponentEstimatedHeight != null && mComponentEstimatedHeight.size() > position) {
      holder.setEstimatedHeight(mComponentEstimatedHeight.getInt(position));
    }
  }

  @Override
  public void onViewAttachedToWindow(@NonNull ListViewHolder holder) {
    if (holder.getUIComponent() == null) {
      if (!isAsync || !mLayoutMap.containsValue(holder)) {
        if (mNewArch) {
          bindViewHolderOnNewArch(holder, holder.getAdapterPosition());
        } else {
          onBindViewHolder(holder, holder.getAdapterPosition());
        }
      }
    }
    if (!isAsync) {
      mCourier.onListNodeAttached(holder);
    }

    if (mNewArch && holder.getUIComponent() != null && mAppearNotificationEnable) {
      holder.getUIComponent().onListCellAppear(holder.getUIComponent().getItemKey(), mList);
    }
    // for non-first line item, add main-axis-gap between item
    int position = holder.getAdapterPosition();
    int remainItemCount = getItemCount() - holder.getAdapterPosition() - 1;
    boolean isLastLineHolder = remainItemCount == 0
        || (remainItemCount < mList.mColumnCount
            && getSectionFooterForPosition(position) == RecyclerView.NO_POSITION);
    if (!isLastLineHolder) {
      holder.mRootView.mMainAxisGap = mList.getMainAxisGap();
    } else {
      holder.mRootView.mMainAxisGap = 0;
    }

    // setFullSpan for header/footer when StaggeredGridLayoutManager
    RecyclerView.LayoutParams layoutParams =
        (RecyclerView.LayoutParams) holder.itemView.getLayoutParams();
    if (layoutParams instanceof StaggeredGridLayoutManager.LayoutParams) {
      StaggeredGridLayoutManager.LayoutParams staggerLayoutParams =
          (StaggeredGridLayoutManager.LayoutParams) layoutParams;
      staggerLayoutParams.setFullSpan(isFullSpan(position));
    }
    saveItemHeightInfo(holder, position);
  }

  @Override
  public void onViewDetachedFromWindow(@NonNull ListViewHolder holder) {
    mCourier.onListNodeDetached(holder);
    if (mNewArch) {
      if (holder.getUIComponent() != null && mDisAppearNotificationEnable) {
        boolean isExist =
            mItemKeys == null ? false : mItemKeys.contains(holder.getUIComponent().getItemKey());
        holder.getUIComponent().onListCellDisAppear(
            holder.getUIComponent().getItemKey(), mList, isExist);
      }
      if (!isAsync) {
        recycleHolderComponent(holder);
      }
    }
  }

  @Override
  public int getItemCount() {
    return mViewNames != null ? mViewNames.size() : 0;
  }

  @Override
  public int getItemViewType(int position) {
    Integer type = mTypesToInt.get(mViewNames.getString(position));
    return type != null ? type : 0;
  }

  @Override
  public long getItemId(int position) {
    if (hasStableIds()) {
      return position;
    }
    return RecyclerView.NO_ID;
  }

  void onLayoutFinishAsync(@NonNull UIComponent component, long operationId) {
    ListViewHolder holder = mLayoutMap.get(operationId);
    updateItemSizeCacheIfNeeded(component);
    if (holder == null || holder.operationID != operationId) {
      if (UIList.DEBUG) {
        LLog.i(UIList.TAG,
            "the component is not valid. itemKey:" + component.getItemKey()
                + " hashCode:" + component.hashCode() + " operationId:" + operationId);
      }

      if (mList.mPreloadCache != null) {
        int first = mList.findFirstListItem();
        int last = mList.findLastListItem();
        if (itemKeyMap == null || !itemKeyMap.containsKey(component.getItemKey())) {
          recycleHolderComponent(component);
          return;
        }
        int pos = itemKeyMap.get(component.getItemKey());
        boolean upper = false, lower = false;
        if (first != -1) {
          upper = pos <= first;
        }
        if (!upper) {
          if (last != -1) {
            lower = pos >= last;
          }
        }
        if (pos >= first - mList.mPreloadBufferCount && pos <= last + mList.mPreloadBufferCount) {
          if (upper) {
            if (!mList.mPreloadCache.contains(component.getItemKey())) {
              mList.mPreloadCache.addComponent(component, true);
            }
          } else if (lower) {
            if (!mList.mPreloadCache.contains(component.getItemKey())) {
              mList.mPreloadCache.addComponent(component, false);
            }
          }
        }
      } else {
        // when scrolling, the original holder(A) is detached from the window. this holder(A) is
        // reused by other postion(B). so the original component can't be binded the
        // position(B).at the same time, we should recycle the component of the holder(A)
        recycleHolderComponent(component);
      }
      return;
    }
    mLayoutMap.remove(operationId);

    if (mList.mPreloadCache != null) {
      if (holder.getUIComponent() == component && component.getView() != null
          && holder.itemView == component.getView().getParent()) {
        if (UIList.DEBUG) {
          LLog.i(UIList.TAG,
              "the component is the same. itemKey:" + component.getItemKey()
                  + " hashCode:" + component.hashCode() + " operationId:" + operationId);
        }
        mCourier.onListNodeAttached(holder);
        holder.mRootView.mLayoutStatus = ListViewHolder.COMPONENT_LAYOUT_FINISH;
        holder.itemView.requestLayout();
        return;
      }
    } else {
      if (holder.getUIComponent() == component) {
        if (UIList.DEBUG) {
          LLog.i(UIList.TAG,
              "the component is the same. itemKey:" + component.getItemKey()
                  + " hashCode:" + component.hashCode() + " operationId:" + operationId);
        }
        mCourier.onListNodeAttached(holder);
        holder.mRootView.mLayoutStatus = ListViewHolder.COMPONENT_LAYOUT_FINISH;
        holder.itemView.requestLayout();
        return;
      }
    }

    if (holder.getUIComponent() != null) {
      recycleHolderComponent(holder);
    }

    if (component != null && component.getView().getParent() != null) {
      ViewGroup parent = (ViewGroup) component.getView().getParent();
      parent.removeView(component.getView());
    }
    if (UIList.DEBUG) {
      LLog.i(UIList.TAG,
          "onLayoutFinishAsync: setUIComponent:" + component.getItemKey() + "_"
              + component.hashCode() + "_" + operationId + "_" + holder.hashCode() + "_"
              + holder.getLayoutPosition());
    }
    holder.mRootView.mLayoutStatus = ListViewHolder.COMPONENT_LAYOUT_FINISH;
    holder.setUIComponent(component);
    mCourier.onListNodeAttached(holder);
    if (component != null) {
      component.setTop(0);
      component.setLeft(0);
      component.requestLayout();
      updateItemSizeCacheIfNeeded(component);
      holder.itemView.requestLayout();
    }
  }

  void onLayoutFinish(long operationId) {
    if (UIList.DEBUG) {
      LLog.i(UIList.TAG, "Adapter onLayoutFinish " + (operationId & 0xffff));
    }
    if (isAsync) {
      return;
    }

    ListViewHolder holder = mLayoutMap.remove(operationId);
    if (holder == null) {
      return;
    }
    UIComponent component = holder.getUIComponent();
    if (component != null) {
      component.setTop(0);
      component.setLeft(0);
      component.requestLayout();
      boolean widthChanged = component.getWidth() != holder.itemView.getWidth();
      boolean heightChanged = component.getHeight() != holder.itemView.getHeight();
      if (widthChanged || heightChanged) {
        holder.itemView.requestLayout();
      }
      if (UIList.DEBUG) {
        LLog.i(UIList.TAG,
            String.format("UIComponent layout finish, position %d (w %d, h %d)",
                holder.getAdapterPosition(), component.getWidth(), component.getHeight()));
      }

      // save item height
      if (mList.mEnableSizeCache && mList.isPartOnLayoutThreadStrategy()) {
        updateItemSizeCacheIfNeeded(component);
      }
    }

    holder.mRootView.mLayoutStatus = ListViewHolder.COMPONENT_LAYOUT_FINISH;
  }

  boolean isFullSpan(int position) {
    return mFullSpans.contains(position);
  }

  boolean isStickyTopItem(int position) {
    return mStickyTopItems.contains(position);
  }

  boolean isStickyBottomItem(int position) {
    return mStickyBottomItems.contains(position);
  }

  // get fullSpan pos before position
  int getSectionHeaderForPosition(int position) {
    for (int i = mFullSpans.size() - 1; i >= 0; --i) {
      Integer sectionPosition = ((Integer) mFullSpans.get(i));
      if (sectionPosition <= position) {
        return sectionPosition.intValue();
      }
    }
    return RecyclerView.NO_POSITION;
  }

  // get stickyHeader pos before position
  int getStickySectionHeaderForPosition(int position) {
    for (int i = mFullSpans.size() - 1; i >= 0; --i) {
      Integer sectionPosition = ((Integer) mFullSpans.get(i));
      if (sectionPosition <= position && isStickyTopItem(sectionPosition)) {
        return sectionPosition.intValue();
      }
    }
    return RecyclerView.NO_POSITION;
  }

  // get next stickyHeader pos after position
  int getNextStickySectionHeaderForPosition(int position) {
    if (mStickyTopItems != null) {
      for (int i = 0; i < mStickyTopItems.size(); i++) {
        int pos = mStickyTopItems.getInt(i);
        if (pos >= position) {
          return pos;
        }
      }
    }
    return RecyclerView.NO_POSITION;
  }

  int getSectionFooterForPosition(int position) {
    for (int i = 0; i < mFullSpans.size(); ++i) {
      Integer sectionPosition = ((Integer) mFullSpans.get(i));
      if (sectionPosition >= position) {
        return sectionPosition.intValue();
      }
    }
    return RecyclerView.NO_POSITION;
  }

  int getStickySectionFooterForPosition(int position) {
    for (int i = 0; i < mFullSpans.size(); ++i) {
      Integer sectionPosition = ((Integer) mFullSpans.get(i));
      if (sectionPosition >= position && isStickyBottomItem(sectionPosition)) {
        return sectionPosition.intValue();
      }
    }
    return RecyclerView.NO_POSITION;
  }

  final private class OperationDispatcher {
    private ReadableArray mInsertions;
    private ReadableArray mRemovals;
    private ReadableArray mUpdateFrom;
    private ReadableArray mUpdateTo;
    private ReadableArray mMoveFrom;
    private ReadableArray mMoveTo;

    void update(ReadableMap diffResult) {
      mInsertions = diffResult.getArray("insertions");
      mRemovals = diffResult.getArray("removals");
      mUpdateFrom = diffResult.getArray("updateFrom");
      mUpdateTo = diffResult.getArray("updateTo");
      mMoveFrom = diffResult.getArray("moveFrom");
      mMoveTo = diffResult.getArray("moveTo");
      if (isNotEmpty(mInsertions) || isNotEmpty(mRemovals) || isNotEmpty(mUpdateFrom)
          || isNotEmpty(mUpdateTo) || isNotEmpty(mMoveFrom) || isNotEmpty(mMoveTo)) {
        mDiffResultConsumed = false;
      }
    }

    private boolean isNotEmpty(ReadableArray result) {
      if (result != null && result.size() > 0) {
        return true;
      }
      return false;
    }

    void dispatch() {
      // change items
      if (mUpdateFrom != null) {
        for (int i = 0; i < mUpdateFrom.size(); ++i) {
          notifyItemChanged(mUpdateFrom.getInt(i), mUpdateTo.getInt(i));
        }
      }

      // move items
      if (mMoveFrom != null) {
        for (int i = 0; i < mMoveFrom.size(); ++i) {
          notifyItemMoved(mMoveFrom.getInt(i), mMoveTo.getInt(i));
        }
      }

      // remove items reversely to avoid other indices being changed
      if (mRemovals != null) {
        for (int i = mRemovals.size() - 1; i >= 0; --i) {
          notifyItemRemoved(mRemovals.getInt(i));
        }
      }

      // insert items
      if (mInsertions != null) {
        for (int i = 0; i < mInsertions.size(); ++i) {
          notifyItemInserted(mInsertions.getInt(i));
        }
      }
    }

    //  remove list component according to "remove" data
    //  insert list component according to "insertAction" data
    // update list component data according to "updateAction" data
    void dispatchUpdateActions(
        ReadableArray removeActions, ReadableArray insertActions, ReadableArray updateActions) {
      if (removeActions != null) {
        for (int i = removeActions.size() - 1; i >= 0; i--) {
          int position = removeActions.getInt(i);
          if (position < 0) {
            continue;
          }
          mItemKeys.remove(position);
          mViewNames.remove(position);
          mComponentEstimatedHeightPx.remove(position);
          mFiberFullSpans.remove(position);
          mFiberStickyTopItems.remove(position);
          mFiberStickyBottomItems.remove(position);
        }
      }

      if (insertActions != null) {
        for (int i = 0; i < insertActions.size(); i++) {
          ReadableMap itemInfo = insertActions.getMap(i);
          if (itemInfo == null) {
            continue;
          }
          int position = itemInfo.getInt("position");
          String itemKey = itemInfo.getString("item-key");
          String type = itemInfo.getString("type");
          boolean isFullSpan = itemInfo.getBoolean("full-span", false);
          boolean isStickyTop = itemInfo.getBoolean("sticky-top", false);
          boolean isStickyBottom = itemInfo.getBoolean("sticky-bottom", false);
          int componentEstimatedHeightPx = itemInfo.getInt("estimated-height-px", -1);
          mItemKeys.add(position, itemKey);
          mViewNames.add(position, type);

          if (!mTypesToInt.containsKey(type)) {
            mTypesToInt.put(type, mTypesToInt.size());
          }
          mFiberFullSpans.add(position, isFullSpan);
          mFiberStickyTopItems.add(position, isStickyTop);
          mFiberStickyBottomItems.add(position, isStickyBottom);
          mComponentEstimatedHeightPx.add(position, componentEstimatedHeightPx);
        }
      }
      if (updateActions != null) {
        for (int i = 0; i < updateActions.size(); i++) {
          ReadableMap itemInfo = updateActions.getMap(i);
          if (itemInfo == null) {
            continue;
          }
          int fromPos = itemInfo.getInt("from");
          int toPos = itemInfo.getInt("to");
          String itemKey = itemInfo.getString("item-key");
          String type = itemInfo.getString("type");
          boolean isFullSpan = itemInfo.getBoolean("full-span", false);
          boolean isStickyTop = itemInfo.getBoolean("sticky-top", false);
          boolean isStickyBottom = itemInfo.getBoolean("sticky-bottom", false);
          int componentEstimatedHeightPx = itemInfo.getInt("estimated-height-px", -1);
          mItemKeys.set(fromPos, itemKey);
          mViewNames.set(fromPos, type);
          if (!mTypesToInt.containsKey(type)) {
            mTypesToInt.put(type, mTypesToInt.size());
          }
          mFiberFullSpans.set(fromPos, isFullSpan);
          mFiberStickyTopItems.set(fromPos, isStickyTop);
          mFiberStickyBottomItems.set(fromPos, isStickyBottom);
          mComponentEstimatedHeightPx.set(fromPos, componentEstimatedHeightPx);
        }
      }
    }

    // transform extra data ,such as full-span、sticky-top、sticky-bottom
    void transformExtraData() {
      transformFullSpans();
      transformStickTopItems();
      transformStickBottomItems();
    }

    private void transformFullSpans() {
      mFullSpans.clear();
      for (int i = 0; i < mFiberFullSpans.size(); i++) {
        if (mFiberFullSpans.getBoolean(i)) {
          mFullSpans.add(i);
        }
      }
    }

    private void transformStickTopItems() {
      mStickyTopItems.clear();
      for (int i = 0; i < mFiberStickyTopItems.size(); i++) {
        if (mFiberStickyTopItems.getBoolean(i)) {
          mStickyTopItems.add(i);
        }
      }
    }

    private void transformStickBottomItems() {
      mStickyBottomItems.clear();
      for (int i = 0; i < mFiberStickyBottomItems.size(); i++) {
        if (mFiberStickyBottomItems.getBoolean(i)) {
          mStickyBottomItems.add(i);
        }
      }
    }

    void flushNODiffActions(
        ReadableArray removeActions, ReadableArray insertActions, ReadableArray updateActions) {
      if (removeActions != null) {
        for (int i = removeActions.size() - 1; i >= 0; i--) {
          int position = removeActions.getInt(i);
          if (position < 0) {
            continue;
          }
          notifyItemRemoved(position);
        }
      }

      if (insertActions != null) {
        for (int i = 0; i < insertActions.size(); i++) {
          ReadableMap itemInfo = insertActions.getMap(i);
          if (itemInfo == null) {
            continue;
          }
          int position = itemInfo.getInt("position");
          notifyItemInserted(position);
        }
      }

      if (updateActions != null) {
        for (int i = 0; i < updateActions.size(); i++) {
          ReadableMap itemInfo = updateActions.getMap(i);
          if (itemInfo == null) {
            continue;
          }
          int fromPos = itemInfo.getInt("from");
          int toPos = itemInfo.getInt("to");
          boolean isFlush = itemInfo.getBoolean("flush", false);
          if (isFlush) {
            notifyItemChanged(fromPos, toPos);
          }
        }
      }
    }
  }

  protected void bindViewHolderOnNewArch(ListViewHolder holder, int position) {
    if (UIList.DEBUG) {
      LLog.i(UIList.TAG,
          "bindViewHolderOnNewArch position=" + position + ", itemKey=" + mItemKeys.get(position)
              + ", viewHolder=" + holder.hashCode());
    }
    UIComponent child = null;
    if (mList.mPreloadCache != null) {
      child = mList.mPreloadCache.removeComponent(mItemKeys.getString(position));
    }

    if (child == null) {
      long operationId = generateOperationId();
      if (UIList.DEBUG) {
        LLog.i(UIList.TAG,
            "bindViewHolderOnNewArch  pos:" + position + " itemKey: " + mItemKeys.get(position)
                + " id:" + operationId);
      }
      mLayoutMap.put(operationId, holder);
      if (isAsync) {
        if (mList.mItemHolderType == UIList.ITEM_HOLDER_TYPE_CLEAR
            || (mList.mItemHolderType == UIList.ITEM_HOLDER_TYPE_DEFAULT
                && holder.getUIComponent() != null
                && !TextUtils.isEmpty(holder.getUIComponent().getItemKey())
                && !holder.getUIComponent().getItemKey().equals(mItemKeys.getString(position)))) {
          holder.removeUIComponent();
        }

        holder.operationID = operationId;
        mList.obtainChildAsync(position, operationId);
        ViewGroup.LayoutParams layoutParams = holder.itemView.getLayoutParams();
        if (layoutParams instanceof StaggeredGridLayoutManager.LayoutParams) {
          ((StaggeredGridLayoutManager.LayoutParams) layoutParams)
              .setFullSpan(isFullSpan(position));
        }
        String itemKey = (String) mItemKeys.get(position);
        // init wrapView layout status
        holder.mRootView.mLayoutStatus = ListViewHolder.COMPONENT_LAYOUT_INIT;

        if (mList.mEnableSizeCache && mAsyncItemSizeCache.containsKey(itemKey)) {
          setEstimatedSizeInternal(holder, mAsyncItemSizeCache.get(itemKey));
        } else {
          setComponentEstimatedSizeFromDataSource(holder, position);
        }
        return;
      } else {
        child =
            (UIComponent) mList.obtainChild(position, operationId, this.mReuseNotificationEnable);
      }

      if (child == null) {
        mLayoutMap.remove(operationId);
        return;
      }
      /**
       *  on this scene: 0->1,1->0. the diff result:update 0->1,remove 1, insert 0;
       *  when binding the viewHolder ,the view is still attached to the Window,
       *  leading  the ViewHolder has not the component View on 0 position. and the itemView is
       *white.
       **/
      if (!mList.mIgnoreAttachForBinding && ViewCompat.isAttachedToWindow(child.getView())) {
        mLayoutMap.remove(operationId);
        return;
      }
      if (child != holder.getUIComponent()) {
        if (child.getView().getParent() != null) {
          ViewGroup parent = (ViewGroup) child.getView().getParent();
          parent.removeView(child.getView());
        }

        recycleHolderComponent(holder);
        holder.setUIComponent(child);
        mCourier.holderAttached(holder);
      }

    } else {
      if (UIList.DEBUG) {
        LLog.i(UIList.TAG,
            " the child is cached. bindViewHolder " + position + " itemKey:" + child.getItemKey());
      }
      holder.mRootView.mLayoutStatus = ListViewHolder.COMPONENT_LAYOUT_FINISH;
      if (child != holder.getUIComponent()) {
        if (child.getView() != null && child.getView().getParent() != null) {
          ViewGroup parent = (ViewGroup) child.getView().getParent();
          parent.removeView(child.getView());
        }
        recycleHolderComponent(holder);
        // recycle the holder component, the layout status may be  COMPONENT_LAYOUT_INIT
        holder.mRootView.mLayoutStatus = ListViewHolder.COMPONENT_LAYOUT_FINISH;
        holder.setUIComponent(child);
        child.requestLayout();
        holder.itemView.forceLayout();
        mCourier.holderAttached(holder);
      } else {
        holder.itemView.requestLayout();
      }

      if (mList.mPreloadCache != null) {
        mList.mPreloadCache.preloadNextComponent(holder);
      }
    }
    if (mList.mEnableSizeCache && mList.isPartOnLayoutThreadStrategy()) {
      child.setOnUpdateListener(new UIComponent.OnUpdateListener() {
        @Override
        public void onLayoutUpdated(UIComponent ui) {
          // In PART_ON_LAYOUT, we should also save the component's height info in
          // UIComponent#onLayoutUpdated() because the component's height may be modified by
          // updateComponentDataByJS.
          if (ui != null && ui.getItemKey() != null) {
            updateItemSizeCacheIfNeeded(ui);
          }
        }
      });
    }
    bindViewHolderOnNewArchWithExtraInfo(holder, position);
  }

  private void bindViewHolderOnNewArchWithExtraInfo(ListViewHolder holder, int position) {
    ViewGroup.LayoutParams layoutParams = holder.itemView.getLayoutParams();
    if (layoutParams instanceof StaggeredGridLayoutManager.LayoutParams) {
      ((StaggeredGridLayoutManager.LayoutParams) layoutParams).setFullSpan(isFullSpan(position));
    }
    // priority use a valid cache size.the scene is:
    // the component A reuse the other component B, and the height of component A is not equal to
    // the height component of B.
    if (mList.mEnableSizeCache && mList.isPartOnLayoutThreadStrategy()) {
      UIComponent uiComponent = holder.getUIComponent();
      if (uiComponent != null && mAsyncItemSizeCache.containsKey(uiComponent.getItemKey())) {
        int cacheSize = mAsyncItemSizeCache.get(uiComponent.getItemKey());
        if (mList.isVertical() && cacheSize != uiComponent.getHeight()) {
          uiComponent.setHeight(cacheSize);
        } else if (!mList.isVertical() && cacheSize != uiComponent.getWidth()) {
          uiComponent.setWidth(cacheSize);
        }
      }
    }
    setComponentEstimatedSizeFromDataSource(holder, position);
  }

  void recycleHolderComponent(ListViewHolder holder) {
    UIComponent old = holder.getUIComponent();
    if (old != null) {
      holder.removeUIComponent();
      if (mList.mPreloadCache != null
          && (isComponentUsedByViewHolder(old) || isComponentUsedByBuffer(old))) {
        LLog.i(UIList.TAG,
            "when recycling the child asynchronously,if the view of the component is used, you cannot recycle the component. the itemKey is :"
                + old.getItemKey() + " position:" + holder.getAdapterPosition());
        return;
      }
      if (isAsync) {
        mList.recycleChildAsync(old);
        holder.mRootView.mLayoutStatus = ListViewHolder.COMPONENT_LAYOUT_INIT;
      } else {
        mList.recycleChild(old);
      }
    }
  }

  void recycleHolderComponent(UIComponent component) {
    if (component != null) {
      if (mList.mPreloadCache != null
          && (isComponentUsedByViewHolder(component) || isComponentUsedByBuffer(component))) {
        LLog.i(UIList.TAG,
            "recycleHolderComponent the component is used. itemKey :" + component.getItemKey());
        return;
      }
      if (isAsync) {
        mList.recycleChildAsync(component);
      } else {
        mList.recycleChild(component);
      }
    }
  }

  void setInternalCellAppearNotification(boolean isNeedAppearNotification) {
    this.mAppearNotificationEnable = isNeedAppearNotification;
  }

  void setInternalCellDisappearNotification(boolean isNeedDisAppearNotification) {
    this.mDisAppearNotificationEnable = isNeedDisAppearNotification;
  }

  void setInternalCellPrepareForReuseNotification(boolean isNeedReuseNotification) {
    this.mReuseNotificationEnable = isNeedReuseNotification;
  }

  /**
   * save List item height info
   * @param holder
   * @param position
   */
  private void saveItemHeightInfo(ListViewHolder holder, int position) {
    if (mItemHeightInfo.isEmpty()) {
      return;
    }
    int itemHeight = holder.getUIComponent() == null ? 0 : holder.getUIComponent().getHeight();
    if (mList.getView().getLayoutManager() instanceof ListLayoutManager.ListLinearLayoutManager) {
      mItemHeightInfo.get(0).put(position, itemHeight);
    } else if (mList.getView().getLayoutManager()
                   instanceof ListLayoutManager.ListStaggeredGridLayoutManager) {
      StaggeredGridLayoutManager.LayoutParams staggerLayoutParams =
          (StaggeredGridLayoutManager.LayoutParams) holder.itemView.getLayoutParams();
      if (isFullSpan(position)) {
        for (int i = 0; i < mItemHeightInfo.size(); i++) {
          mItemHeightInfo.get(i).put(position, itemHeight);
        }
      } else {
        for (int i = 0; i < mItemHeightInfo.size(); i++) {
          if (staggerLayoutParams.getSpanIndex() == i) {
            mItemHeightInfo.get(i).put(position, itemHeight);
          } else {
            mItemHeightInfo.get(i).remove(position);
          }
        }
      }
    } else if (mList.getView().getLayoutManager() instanceof GridLayoutManager) {
      GridLayoutManager layoutManager = (GridLayoutManager) mList.getView().getLayoutManager();
      int spanIndex =
          layoutManager.getSpanSizeLookup().getSpanIndex(position, layoutManager.getSpanCount());
      if (isFullSpan(position)) {
        for (int i = 0; i < mItemHeightInfo.size(); i++) {
          mItemHeightInfo.get(i).put(position, itemHeight);
        }
      } else {
        for (int i = 0; i < mItemHeightInfo.size(); i++) {
          if (spanIndex == i) {
            mItemHeightInfo.get(i).put(position, itemHeight);
          } else {
            mItemHeightInfo.get(i).remove(position);
          }
        }
      }
    }
  }

  protected int getScrollY() {
    RecyclerView.LayoutManager manager = mList.getView().getLayoutManager();
    if (mItemHeightInfo.isEmpty()) {
      return 0;
    }
    if (manager instanceof ListLayoutManager.ListLinearLayoutManager) {
      ListLayoutManager.ListLinearLayoutManager layoutManager =
          (ListLayoutManager.ListLinearLayoutManager) manager;
      int firstVisiblePosition = layoutManager.findFirstVisibleItemPosition();
      View firstVisibleView = layoutManager.findViewByPosition(firstVisiblePosition);
      int offsetY = firstVisibleView != null ? -(int) (firstVisibleView.getY()) : 0;
      HashMap<Integer, Integer> itemHeightInfo = mItemHeightInfo.get(0);
      for (int i = 0; i < firstVisiblePosition; i++) {
        offsetY += itemHeightInfo.get(i) == null ? 0 : itemHeightInfo.get(i);
      }
      return offsetY;
    } else if (manager instanceof ListLayoutManager.ListGridLayoutManager) {
      try {
        ListLayoutManager.ListGridLayoutManager layoutManager =
            (ListLayoutManager.ListGridLayoutManager) manager;
        int maxScrollResult = 0;
        for (int i = 0; i < mItemHeightInfo.size(); i++) {
          View child = layoutManager.getChildAt(i);
          if (child == null) {
            continue;
          }
          int childPosition = layoutManager.getPosition(child);
          int offsetY = -(int) (child.getY());
          HashMap<Integer, Integer> itemHeightMap = mItemHeightInfo.get(i);
          for (int j = 0; j < childPosition; j++) {
            offsetY += itemHeightMap.get(j) == null ? 0 : itemHeightMap.get(j);
          }
          maxScrollResult = Math.max(maxScrollResult, offsetY);
        }

        return maxScrollResult;
      } catch (Exception e) {
        e.printStackTrace();
        return 0;
      }
    } else {
      ListLayoutManager.ListStaggeredGridLayoutManager layoutManager =
          (ListLayoutManager.ListStaggeredGridLayoutManager) manager;
      int[] spanIndexInfo = new int[mList.mColumnCount];
      View[] childViews = new View[mList.mColumnCount];
      layoutManager.findFirstVisibleItemPositions(spanIndexInfo);
      for (int i = 0; i < spanIndexInfo.length; i++) {
        for (int j = 0; j < mList.getView().getChildCount(); j++) {
          View child = mList.getView().getChildAt(j);
          if (child == null) {
            continue;
          }
          ListViewHolder holder = (ListViewHolder) mList.getView().getChildViewHolder(child);
          if (holder == null) {
            continue;
          }
          if (spanIndexInfo[i] == holder.getAdapterPosition()) {
            childViews[i] = child;
            break;
          }
        }
      }
      int scrollResult = 0;
      for (int i = 0; i < spanIndexInfo.length && i < mItemHeightInfo.size(); i++) {
        if (childViews[i] == null) {
          continue;
        }
        int offsetY = -(int) (childViews[i].getY());
        HashMap<Integer, Integer> itemHeightMap = mItemHeightInfo.get(i);
        for (int j = 0; j < spanIndexInfo[i]; j++) {
          offsetY += itemHeightMap.get(j) == null ? 0 : itemHeightMap.get(j);
        }
        scrollResult = Math.max(scrollResult, offsetY);
      }
      return scrollResult;
    }
  }

  private void setComponentEstimatedSizeFromDataSource(ListViewHolder holder, int position) {
    if (mComponentEstimatedHeightPx != null && mComponentEstimatedHeightPx.size() > position) {
      int size = (int) PixelUtils.dipToPx(mComponentEstimatedHeightPx.getInt(position));
      setEstimatedSizeInternal(holder, size);
    }
    if (mComponentEstimatedHeight != null && mComponentEstimatedHeight.size() > position) {
      setEstimatedSizeInternal(holder, mComponentEstimatedHeight.getInt(position));
    }
  }

  boolean isComponentUsedByViewHolder(@NonNull UIComponent uiComponent) {
    if (mList != null) {
      RecyclerView recyclerView = mList.getRecyclerView();
      if (recyclerView != null) {
        for (int i = 0; i < recyclerView.getChildCount(); i++) {
          View viewGroup = recyclerView.getChildAt(i);
          if (viewGroup instanceof ListViewHolder.WrapView) {
            ListViewHolder.WrapView wrapView = (ListViewHolder.WrapView) viewGroup;
            UIComponent component = wrapView.getUIComponent();
            if (component != null && component.getItemKey() != null
                && (component.getItemKey().equals(uiComponent.getItemKey()))) {
              return true;
            }
          }
        }
      }
    }
    return false;
  }

  boolean isComponentUsedByBuffer(@NonNull UIComponent uiComponent) {
    if (mList.mPreloadCache != null) {
      if (mList.mPreloadCache.contains(uiComponent.getItemKey())) {
        return true;
      }
    }
    return false;
  }

  private void updateItemSizeCacheIfNeeded(UIComponent component) {
    if (!mList.mEnableSizeCache || component == null) {
      return;
    }
    String itemKey = component.getItemKey();
    if (!TextUtils.isEmpty(itemKey)) {
      int itemSize = mList.isVertical() ? component.getHeight() : component.getWidth();
      if (UIList.DEBUG) {
        LLog.i(UIList.TAG,
            "updateItemSizeCacheIfNeeded: itemKey = " + itemKey + ", itemSize = " + itemSize);
      }
      mAsyncItemSizeCache.put(component.getItemKey(), itemSize);
    }
  }

  private void setEstimatedSizeInternal(ListViewHolder holder, int size) {
    if (holder == null) {
      return;
    }
    if (mList.isVertical()) {
      holder.setEstimatedHeight(size);
    } else {
      holder.setEstimatedWidth(size);
    }
  }

  private void cleanAsyncItemSizeCache() {
    mAsyncItemSizeCache.clear();
  }
}
