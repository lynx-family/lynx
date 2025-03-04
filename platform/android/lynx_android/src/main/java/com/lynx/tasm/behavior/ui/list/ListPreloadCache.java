// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.list;

import android.text.TextUtils;
import android.view.View;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.ui.view.UIComponent;
import java.util.ArrayList;
import java.util.HashMap;

public class ListPreloadCache {
  private UIList mUIList;
  private int mCacheCount = 0;
  public ArrayList<UIComponent> upperCache = new ArrayList<>();
  public ArrayList<UIComponent> lowerCache = new ArrayList<>();

  private RecyclerView.OnScrollListener onScrollListener = new RecyclerView.OnScrollListener() {
    @Override
    public void onScrollStateChanged(@NonNull RecyclerView recyclerView, int newState) {
      super.onScrollStateChanged(recyclerView, newState);
      if (recyclerView == null || mUIList == null || mUIList.getAdapter() == null) {
        return;
      }
      // check component
      if (newState == RecyclerView.SCROLL_STATE_IDLE) {
        for (int i = 0; i < recyclerView.getChildCount(); i++) {
          View viewGroup = recyclerView.getChildAt(i);
          if (viewGroup instanceof ListViewHolder.WrapView) {
            ListViewHolder.WrapView wrapView = (ListViewHolder.WrapView) viewGroup;
            UIComponent component = wrapView.getUIComponent();
            if (component == null) {
              LLog.i(UIList.TAG,
                  "the scroll state of recyclerView is idle, the component is null. position is :"
                      + wrapView.mAdapterPosition);
              mUIList.getAdapter().notifyItemChanged(wrapView.mAdapterPosition);
            }
          }
        }
      }
    }
  };

  public ListPreloadCache(UIList uiList, int cacheCount) {
    this.mUIList = uiList;
    this.mCacheCount = cacheCount;
    initScrollListener();
  }

  private void initScrollListener() {
    if (mUIList != null && mUIList.getView() != null) {
      mUIList.getView().addOnScrollListener(onScrollListener);
    }
  }

  public void setCacheCount(int cacheCount) {
    this.mCacheCount = cacheCount;
  }

  @MainThread
  public void addComponent(@NonNull UIComponent component, boolean upper) {
    if (mCacheCount <= 0 || component == null) {
      return;
    }
    if (upper) {
      if (upperCache.size() > mCacheCount) {
        UIComponent recycleData = upperCache.remove(0);
        if (recycleData != null) {
          mUIList.getAdapter().recycleHolderComponent(recycleData);
        }
      }
      upperCache.add(component);
    } else {
      if (lowerCache.size() > mCacheCount) {
        UIComponent recycleData = lowerCache.remove(0);
        if (recycleData != null) {
          mUIList.getAdapter().recycleHolderComponent(recycleData);
        }
      }
      lowerCache.add(component);
    }
  }

  @MainThread
  public UIComponent removeComponent(@NonNull String itemKey) {
    if (TextUtils.isEmpty(itemKey)) {
      return null;
    }
    for (int i = 0; i < upperCache.size(); i++) {
      UIComponent component = upperCache.get(i);
      if (component != null && itemKey.equals(component.getItemKey())) {
        upperCache.remove(component);
        return component;
      }
    }

    for (int i = 0; i < lowerCache.size(); i++) {
      UIComponent component = lowerCache.get(i);
      if (component != null && itemKey.equals(component.getItemKey())) {
        lowerCache.remove(component);
        return component;
      }
    }
    return null;
  }

  public boolean contains(@NonNull String itemKey) {
    if (TextUtils.isEmpty(itemKey)) {
      return false;
    }
    for (int i = 0; i < upperCache.size(); i++) {
      UIComponent component = upperCache.get(i);
      if (component != null && itemKey.equals(component.getItemKey())) {
        return true;
      }
    }
    for (int i = 0; i < lowerCache.size(); i++) {
      UIComponent component = lowerCache.get(i);
      if (component != null && itemKey.equals(component.getItemKey())) {
        return true;
      }
    }
    return false;
  }

  /**
   * when the cache component is used by the holder, should  preload nex component to fill the pool
   * @param holder
   */
  @MainThread
  void preloadNextComponent(ListViewHolder holder) {
    if (mUIList == null || mUIList.getAdapter() == null || mUIList.getAdapter().mItemKeys == null
        || holder == null) {
      return;
    }
    UIListAdapter adapter = mUIList.getAdapter();
    int firstVisiblePosition = mUIList.findFirstListItem();
    int endVisiblePosition = mUIList.findLastListItem();
    if (holder.getUIComponent() != null) {
      if (firstVisiblePosition != -1 && holder.getAdapterPosition() < firstVisiblePosition) {
        if (isValidIndex(firstVisiblePosition - mCacheCount)) {
          if (adapter.mItemKeys != null
              && !contains(adapter.mItemKeys.getString(firstVisiblePosition - mCacheCount))) {
            preloadComponent(firstVisiblePosition - mCacheCount);
          }
        }
      } else if (endVisiblePosition != -1 && holder.getAdapterPosition() >= endVisiblePosition) {
        if (isValidIndex(endVisiblePosition + mCacheCount)) {
          if (adapter.mItemKeys != null
              & !contains(adapter.mItemKeys.getString(endVisiblePosition + mCacheCount))) {
            preloadComponent(endVisiblePosition + mCacheCount);
          }
        }
      }
    }
  }

  /**
   * preload the component
   * @param position
   */
  @MainThread
  void preloadComponent(int position) {
    if (mUIList == null || mUIList.getAdapter() == null || !isValidIndex(position)) {
      return;
    }
    long genID = mUIList.getAdapter().generateOperationId();
    mUIList.obtainChildAsync(position, genID);
  }

  /**
   * when component is recycled from the recyclerPool,should enqueue Component
   * @param holder
   */
  @MainThread
  void enqueueComponentFromRecyclerPool(ListViewHolder holder) {
    if (mUIList == null || mUIList.getAdapter() == null || mUIList.getAdapter().itemKeyMap == null
        || holder == null) {
      return;
    }
    UIListAdapter adapter = mUIList.getAdapter();
    if (adapter.itemKeyMap == null) {
      return;
    }
    if (mCacheCount > 0) {
      int firstVisiblePosition = mUIList.findFirstListItem();
      int endVisiblePosition = mUIList.findLastListItem();
      if (holder.getUIComponent() != null) {
        if (adapter.itemKeyMap.containsKey(holder.getUIComponent().getItemKey())
            && isValidRange(adapter.itemKeyMap.get(holder.getUIComponent().getItemKey()),
                firstVisiblePosition - mCacheCount, endVisiblePosition + mCacheCount)) {
          if (firstVisiblePosition != -1 && holder.getLayoutPosition() <= firstVisiblePosition) {
            if (!contains(holder.getUIComponent().getItemKey())) {
              addComponent(holder.getUIComponent(), true);
            }

          } else if (endVisiblePosition != -1 && holder.getLayoutPosition() >= endVisiblePosition) {
            if (!contains(holder.getUIComponent().getItemKey())) {
              addComponent(holder.getUIComponent(), false);
            }
          }
        } else {
          mUIList.getAdapter().recycleHolderComponent(holder.getUIComponent());
        }
      }
    }
  }

  /**
   * when the  data of the list changes  and list has diff results,should init cache.
   */
  void initCache() {
    if (mUIList == null || mUIList.getAdapter() == null) {
      return;
    }
    JavaOnlyArray mItemKeys = mUIList.getAdapter().mItemKeys;
    HashMap<String, Integer> itemKeyMap = mUIList.getAdapter().itemKeyMap;
    if (mItemKeys == null || itemKeyMap == null) {
      return;
    }

    int firstVisiblePosition = mUIList.findFirstListItem();
    int endVisiblePosition = mUIList.findLastListItem();
    lowerCache.clear();
    upperCache.clear();

    for (int i = 1; i <= mCacheCount; i++) {
      if (firstVisiblePosition != -1) {
        int pos = firstVisiblePosition - i;
        if (pos >= 0 && pos < mItemKeys.size()) {
          if (!contains(mItemKeys.getString(pos))) {
            long genID = mUIList.getAdapter().generateOperationId();
            if (isValidIndex(pos))
              mUIList.obtainChildAsync(pos, genID);
          }
        }
      }
      if (endVisiblePosition != -1) {
        int end = endVisiblePosition + i;
        if (end < mItemKeys.size() && end >= 0) {
          if (!contains(mItemKeys.getString(end))) {
            long genID = mUIList.getAdapter().generateOperationId();
            if (isValidIndex(end))
              mUIList.obtainChildAsync(end, genID);
          }
        }
      }
    }
  }

  /**
   * the position is valid, if position >= start && position <= end
   *
   * @param position
   * @param start
   * @param end
   * @return
   */
  boolean isValidRange(int position, int start, int end) {
    return position >= start && position <= end;
  }

  boolean isValidIndex(int position) {
    if (mUIList == null || mUIList.getAdapter() == null || mUIList.getAdapter().mItemKeys == null) {
      return false;
    }
    if (isValidRange(position, 0, mUIList.getAdapter().mItemKeys.size() - 1)) {
      return true;
    }
    return false;
  }

  /**
   * clear the component at caches
   */
  public void clear() {
    upperCache.clear();
    lowerCache.clear();
  }

  public void destroy() {
    clear();
    if (mUIList != null && mUIList.getView() != null) {
      mUIList.getView().removeOnScrollListener(onScrollListener);
    }
  }
}
