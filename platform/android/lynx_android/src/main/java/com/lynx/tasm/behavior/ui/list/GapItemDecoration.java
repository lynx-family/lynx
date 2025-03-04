// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.list;

import android.graphics.Rect;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.StaggeredGridLayoutManager;

public class GapItemDecoration extends RecyclerView.ItemDecoration {
  private static final int DEFAULT_COLUMN_COUNT = 1;
  private boolean mIsVertical = true;
  private boolean mIsRTL = false;
  private int mColumnCount = 0;
  private int mMainAxisGap = 0;
  private int mCrossAxisGap = 0;

  @Override
  public void getItemOffsets(@NonNull Rect outRect, @NonNull View view,
      @NonNull RecyclerView parent, @NonNull RecyclerView.State state) {
    super.getItemOffsets(outRect, view, parent, state);
    RecyclerView.Adapter adapter = parent.getAdapter();
    RecyclerView.ViewHolder holder = parent.getChildViewHolder(view);
    if (adapter instanceof UIListAdapter && holder instanceof ListViewHolder) {
      UIListAdapter uiListAdapter = (UIListAdapter) adapter;
      ListViewHolder listViewHolder = (ListViewHolder) holder;
      // apply main-axis-gap
      int position = listViewHolder.getAdapterPosition();
      boolean isFirstLineHolder = position == 0
          || (position < mColumnCount
              && uiListAdapter.getSectionHeaderForPosition(position) == RecyclerView.NO_POSITION);
      UpdateMainAxisOutRect(outRect, isFirstLineHolder);
      // apply cross-axis-gap
      if (mColumnCount > DEFAULT_COLUMN_COUNT && listViewHolder.itemView != null) {
        ViewGroup.LayoutParams layoutParams = listViewHolder.itemView.getLayoutParams();
        if (layoutParams instanceof GridLayoutManager.LayoutParams) {
          GridLayoutManager.LayoutParams gridLayoutParams =
              (GridLayoutManager.LayoutParams) layoutParams;
          UpdateCrossAxisOutRect(outRect, gridLayoutParams.getSpanSize() == mColumnCount,
              gridLayoutParams.getSpanIndex());
        } else if (layoutParams instanceof StaggeredGridLayoutManager.LayoutParams) {
          StaggeredGridLayoutManager.LayoutParams staggeredGridLayoutParams =
              (StaggeredGridLayoutManager.LayoutParams) layoutParams;
          UpdateCrossAxisOutRect(outRect, staggeredGridLayoutParams.isFullSpan(),
              staggeredGridLayoutParams.getSpanIndex());
        }
      }
    }
  }

  private void UpdateMainAxisOutRect(@NonNull Rect outRect, boolean isFirstLineHolder) {
    if (mIsVertical) {
      outRect.top = isFirstLineHolder ? 0 : mMainAxisGap;
    } else {
      if (mIsRTL) {
        outRect.right = isFirstLineHolder ? 0 : mMainAxisGap;
      } else {
        outRect.left = isFirstLineHolder ? 0 : mMainAxisGap;
      }
    }
  }

  private void UpdateCrossAxisOutRect(@NonNull Rect outRect, boolean isFullSpan, int spanIndex) {
    if (isFullSpan) {
      return;
    }
    int avgCrossAxisGap = mCrossAxisGap * (mColumnCount - 1) / mColumnCount;
    if (spanIndex == 0) {
      if (mIsVertical) {
        outRect.right = avgCrossAxisGap;
      } else {
        outRect.bottom = avgCrossAxisGap;
      }
    } else if (spanIndex == mColumnCount - 1) {
      if (mIsVertical) {
        outRect.left = avgCrossAxisGap;
      } else {
        outRect.top = avgCrossAxisGap;
      }
    } else {
      if (mIsVertical) {
        outRect.left = avgCrossAxisGap / 2;
        outRect.right = avgCrossAxisGap / 2;
      } else {
        outRect.top = avgCrossAxisGap / 2;
        outRect.bottom = avgCrossAxisGap / 2;
      }
    }
  }

  public void setIsRTL(boolean isRTL) {
    mIsRTL = isRTL;
  }

  public void setIsVertical(boolean isVertical) {
    mIsVertical = isVertical;
  }

  public void setColumnCount(int columnCount) {
    mColumnCount = columnCount;
  }

  public void setMainAxisGap(int mainAxisGap) {
    mMainAxisGap = mainAxisGap;
  }

  public void setCrossAxisGap(int crossAxisGap) {
    mCrossAxisGap = crossAxisGap;
  }
}
