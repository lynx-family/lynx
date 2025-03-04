// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui;

import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.Nullable;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;

public class ViewGroupDrawingOrderHelper {
  private final ViewGroup mViewGroup;
  private int mNumberOfChildrenWithZIndex = 0;
  private @Nullable int[] mDrawingOrderIndices;

  public ViewGroupDrawingOrderHelper(ViewGroup viewGroup) {
    mViewGroup = viewGroup;
  }

  /**
   * This should be called every time a view is added to the ViewGroup in {@link ViewGroup#addView}.
   *
   * @param view The view that is being added
   */
  public void handleAddView(View view) {
    if (UIGroup.getViewZIndex(view) != null) {
      mNumberOfChildrenWithZIndex++;
    }

    mDrawingOrderIndices = null;
  }

  /**
   * This should be called every time a view is removed from the ViewGroup in {@link
   * ViewGroup#removeView} and {@link ViewGroup#removeViewAt}.
   *
   * @param view The view that is being removed.
   */
  public void handleRemoveView(View view) {
    if (UIGroup.getViewZIndex(view) != null) {
      mNumberOfChildrenWithZIndex--;
    }

    mDrawingOrderIndices = null;
  }

  /**
   * If the ViewGroup should enable drawing order. ViewGroups should call {@link
   * ViewGroup#setChildrenDrawingOrderEnabled} with the value returned from this method when a view
   * is added or removed.
   */
  public boolean shouldEnableCustomDrawingOrder() {
    return mNumberOfChildrenWithZIndex > 0;
  }

  /**
   * The index of the child view that should be drawn. This should be used in {@link
   * ViewGroup#getChildDrawingOrder}.
   */
  public int getChildDrawingOrder(int childCount, int index) {
    if (mDrawingOrderIndices == null) {
      ArrayList<View> viewsToSort = new ArrayList<>();
      for (int i = 0; i < childCount; i++) {
        viewsToSort.add(mViewGroup.getChildAt(i));
      }
      // Sort the views by zIndex
      Collections.sort(viewsToSort, new Comparator<View>() {
        @Override
        public int compare(View view1, View view2) {
          Integer view1ZIndex = UIGroup.getViewZIndex(view1);
          if (view1ZIndex == null) {
            view1ZIndex = 0;
          }

          Integer view2ZIndex = UIGroup.getViewZIndex(view2);
          if (view2ZIndex == null) {
            view2ZIndex = 0;
          }

          return view1ZIndex - view2ZIndex;
        }
      });

      mDrawingOrderIndices = new int[childCount];
      for (int i = 0; i < childCount; i++) {
        View child = viewsToSort.get(i);
        mDrawingOrderIndices[i] = mViewGroup.indexOfChild(child);
      }
    }
    return mDrawingOrderIndices[index];
  }

  /** Recheck all children for z-index changes. */
  public void update() {
    mNumberOfChildrenWithZIndex = 0;
    for (int i = 0; i < mViewGroup.getChildCount(); i++) {
      if (UIGroup.getViewZIndex(mViewGroup.getChildAt(i)) != null) {
        mNumberOfChildrenWithZIndex++;
      }
    }
    mDrawingOrderIndices = null;
  }
}
