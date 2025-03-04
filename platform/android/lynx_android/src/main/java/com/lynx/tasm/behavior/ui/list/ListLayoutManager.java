// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.list;

import android.content.Context;
import android.view.View;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.RecyclerView.LayoutParams;
import androidx.recyclerview.widget.StaggeredGridLayoutManager;
import com.lynx.tasm.base.LLog;
/*
 *  Because the recyclerview padding make affect on all child include normal items/header/footer
 *  To clip padding of header/footer, re-implement layoutDecoratedWithMargins to modify
 *  wrapview layout of header/footer
 * */
public final class ListLayoutManager {
  public static final String TAG = "ListLayoutManager";
  public static interface ListLayoutInfo {
    /**
     *  consumed distance on vertical orientation
     * @return
     */
    public float getConsumedY();

    /**
     * consumed distance on horizontal orientation
     * @return
     */
    public float getConsumedX();
  }

  public static class ListLinearLayoutManager
      extends LinearLayoutManager implements ListLayoutInfo {
    private UIList mList;
    private float mConsumedY;
    private float mConsumedX;

    public ListLinearLayoutManager(Context context, UIList listRef) {
      super(context);
      mList = listRef;
    }

    // issue: #1978
    // override supportsPredictiveItemAnimations to make it return false to circumvent animation
    // crash when using diff
    @Override
    public boolean supportsPredictiveItemAnimations() {
      return false;
    }

    @Override
    public void layoutDecoratedWithMargins(View child, int left, int top, int right, int bottom) {
      if (LinearLayoutManager.VERTICAL == getOrientation()) {
        LayoutParams params = (LayoutParams) child.getLayoutParams();
        int position = params.getViewAdapterPosition();
        if (mList.getAdapter() != null
            && !mList.getAdapter().isFullSpan(position)) { // not header/footer
          super.layoutDecoratedWithMargins(child, left, top, right, bottom);
          return;
        }
        layoutFullSpan(this, child, top, bottom, mList.mEnableGapItemDecoration);
      } else {
        super.layoutDecoratedWithMargins(child, left, top, right, bottom);
      }
    }

    @Override
    public void onLayoutCompleted(RecyclerView.State state) {
      super.onLayoutCompleted(state);
      mList.onLayoutCompleted();
    }

    @Override
    public float getConsumedY() {
      return mConsumedY;
    }

    @Override
    public float getConsumedX() {
      return mConsumedX;
    }

    @Override
    public int scrollVerticallyBy(
        int dy, RecyclerView.Recycler recycler, RecyclerView.State state) {
      mConsumedY = super.scrollVerticallyBy(dy, recycler, state);
      mList.mListEventManager.onScrollBy(dy, (int) mConsumedY);
      return (int) mConsumedY;
    }

    @Override
    public int scrollHorizontallyBy(
        int dx, RecyclerView.Recycler recycler, RecyclerView.State state) {
      mConsumedX = super.scrollHorizontallyBy(dx, recycler, state);
      mList.mListEventManager.onScrollBy(dx, (int) mConsumedX);
      return (int) mConsumedX;
    }

    @Override
    public boolean canScrollVertically() {
      if (!mList.mEnableScroll) {
        return false;
      }
      return super.canScrollVertically();
    }

    @Override
    public boolean canScrollHorizontally() {
      if (!mList.mEnableScroll) {
        return false;
      }
      return super.canScrollHorizontally();
    }
  }

  public static class ListGridLayoutManager extends GridLayoutManager implements ListLayoutInfo {
    private UIList mList;
    private int mCrossAxisGap;
    private float mConsumedY;
    private float mConsumedX;

    public ListGridLayoutManager(Context context, int spanCount, int crossAxisGap, UIList listRef) {
      super(context, spanCount);
      mCrossAxisGap = crossAxisGap;
      mList = listRef;
    }

    @Override
    public boolean supportsPredictiveItemAnimations() {
      return false;
    }

    @Override
    public void layoutDecoratedWithMargins(View child, int left, int top, int right, int bottom) {
      if (LinearLayoutManager.VERTICAL == getOrientation()) {
        GridLayoutManager.LayoutParams layoutParam =
            (GridLayoutManager.LayoutParams) child.getLayoutParams();
        if (layoutParam.getSpanSize() != this.getSpanCount()) { // not header/footer
          if (!mList.mEnableGapItemDecoration) {
            left = adjustLeftWithGap(mList, getSpanCount(), mCrossAxisGap, left, right);
            right = left + child.getMeasuredWidth();
          }
          super.layoutDecoratedWithMargins(child, left, top, right, bottom);
          return;
        }
        layoutFullSpan(this, child, top, bottom, mList.mEnableGapItemDecoration);
      } else {
        super.layoutDecoratedWithMargins(child, left, top, right, bottom);
      }
    }

    @Override
    public void onLayoutCompleted(RecyclerView.State state) {
      super.onLayoutCompleted(state);
      mList.onLayoutCompleted();
    }

    @Override
    public float getConsumedY() {
      return mConsumedY;
    }

    @Override
    public float getConsumedX() {
      return mConsumedX;
    }

    @Override
    public int scrollVerticallyBy(
        int dy, RecyclerView.Recycler recycler, RecyclerView.State state) {
      mConsumedY = super.scrollVerticallyBy(dy, recycler, state);
      mList.mListEventManager.onScrollBy(dy, (int) mConsumedY);
      return (int) mConsumedY;
    }

    @Override
    public int scrollHorizontallyBy(
        int dx, RecyclerView.Recycler recycler, RecyclerView.State state) {
      mConsumedX = super.scrollHorizontallyBy(dx, recycler, state);
      mList.mListEventManager.onScrollBy(dx, (int) mConsumedX);
      return (int) mConsumedX;
    }

    @Override
    public boolean canScrollVertically() {
      if (!mList.mEnableScroll) {
        return false;
      }
      return super.canScrollVertically();
    }

    @Override
    public boolean canScrollHorizontally() {
      if (!mList.mEnableScroll) {
        return false;
      }
      return super.canScrollHorizontally();
    }

    void setCrossAxisGap(int crossAxisGap) {
      mCrossAxisGap = crossAxisGap;
    }
  }

  static class ListStaggeredGridLayoutManager
      extends StaggeredGridLayoutManager implements ListLayoutInfo {
    private UIList mList;
    private int mCrossAxisGap;
    private float mConsumedY;
    private float mConsumedX;
    public static final String TAG = "ListStaggeredGridLayoutManager";

    public ListStaggeredGridLayoutManager(int spanCount, int gap, int orientation, UIList listRef) {
      super(spanCount, orientation);
      mCrossAxisGap = gap;
      mList = listRef;
    }

    @Override
    public void onScrollStateChanged(int state) {
      try {
        super.onScrollStateChanged(state);
      } catch (Exception e) {
        LLog.e(ListStaggeredGridLayoutManager.TAG, e.getMessage());
      }
    }

    @Override
    public boolean supportsPredictiveItemAnimations() {
      return false;
    }

    @Override
    public void layoutDecoratedWithMargins(View child, int left, int top, int right, int bottom) {
      StaggeredGridLayoutManager.LayoutParams layoutParam =
          (StaggeredGridLayoutManager.LayoutParams) child.getLayoutParams();
      if (StaggeredGridLayoutManager.VERTICAL == getOrientation()) {
        if (!layoutParam.isFullSpan()) { // not header/footer
          if (!mList.mEnableGapItemDecoration) {
            left = adjustLeftWithGap(mList, getSpanCount(), mCrossAxisGap, left, right);
            right = left + child.getMeasuredWidth();
          }
          super.layoutDecoratedWithMargins(child, left, top, right, bottom);
        } else {
          layoutFullSpan(this, child, top, bottom, mList.mEnableGapItemDecoration);
        }
      } else {
        super.layoutDecoratedWithMargins(child, left, top, right, bottom);
      }
    }

    @Override
    public void onLayoutCompleted(RecyclerView.State state) {
      super.onLayoutCompleted(state);
      mList.onLayoutCompleted();
    }

    @Override
    public float getConsumedY() {
      return mConsumedY;
    }

    @Override
    public float getConsumedX() {
      return mConsumedX;
    }

    @Override
    public int scrollVerticallyBy(
        int dy, RecyclerView.Recycler recycler, RecyclerView.State state) {
      try {
        int consumeY = super.scrollVerticallyBy(dy, recycler, state);
        mList.mListEventManager.onScrollBy(dy, consumeY);
        return consumeY;
      } catch (NullPointerException e) {
        return 0;
      }
    }

    @Override
    public int scrollHorizontallyBy(
        int dx, RecyclerView.Recycler recycler, RecyclerView.State state) {
      try {
        int consumeX = super.scrollHorizontallyBy(dx, recycler, state);
        mList.mListEventManager.onScrollBy(dx, consumeX);
        return consumeX;
      } catch (NullPointerException e) {
        return 0;
      }
    }

    @Override
    public boolean canScrollVertically() {
      if (!mList.mEnableScroll) {
        return false;
      }
      return super.canScrollVertically();
    }

    @Override
    public boolean canScrollHorizontally() {
      if (!mList.mEnableScroll) {
        return false;
      }
      return super.canScrollHorizontally();
    }

    void setCrossAxisGap(int gap) {
      mCrossAxisGap = gap;
    }
  }

  private static void layoutFullSpan(RecyclerView.LayoutManager layoutManager, View view, int top,
      int bottom, boolean enableGapItemDecoration) {
    int left = calculateFullSpanOffset(layoutManager.getWidth(), view.getMeasuredWidth(),
        layoutManager.getPaddingLeft(), layoutManager.getPaddingRight());
    int right = left + view.getMeasuredWidth();
    if (enableGapItemDecoration) {
      layoutManager.layoutDecorated(view, left, top, right, bottom);
    } else {
      view.layout(left, top, right, bottom);
    }
  }

  /* get the x-offset of header/footer */
  private static int calculateFullSpanOffset(
      int totalWidth, int totalWidthOfView, int paddingLeft, int paddingRight) {
    int space = 0;
    space = totalWidth - totalWidthOfView;
    if (space <= 0) {
      // header width is larger than list width, full span, start form 0
      return 0;
    }

    space -= paddingLeft + paddingRight;
    if (space >= 0)
      return paddingLeft;
    double scale = ((double) paddingLeft) / (paddingLeft + paddingRight);
    return paddingLeft + (int) (space * scale);
  }

  private static int adjustLeftWithGap(UIList list, int spanCount, int gap, int left, int right) {
    if (gap > 0 && list != null && list.getRecyclerView() != null) {
      RecyclerView recyclerView = list.getRecyclerView();
      final int paddingLeft = recyclerView.getPaddingLeft();
      final int paddingRight = recyclerView.getPaddingRight();
      final int listWidth = recyclerView.getWidth();
      final int spanWidth = (listWidth - paddingLeft - paddingRight) / spanCount;
      // avoid java.lang.ArithmeticException: divide by zero
      if (spanWidth == 0) {
        LLog.i(TAG, "the width of list maybe 0 ");
        return left;
      }
      int spanIndex = (left - paddingLeft) / spanWidth;
      int viewWidth = right - left;
      return (gap + viewWidth) * spanIndex + paddingLeft;
    }
    return left;
  }
}
