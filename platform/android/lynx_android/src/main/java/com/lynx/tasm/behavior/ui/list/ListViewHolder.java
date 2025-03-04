// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.list;

import android.content.Context;
import android.graphics.Rect;
import android.util.LayoutDirection;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import androidx.annotation.NonNull;
import androidx.core.view.ViewCompat;
import androidx.recyclerview.widget.RecyclerView;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.ui.view.UIComponent;

public class ListViewHolder extends RecyclerView.ViewHolder {
  // onCreateViewHolder: new Holder/WrapView/component have zero width and height
  static final int COMPONENT_LAYOUT_INIT = 0;
  // onBindViewHolder: reuse holder/component/WrapView, so does the width and height
  static final int COMPONENT_LAYOUT_REUSE = 1;
  // onLayoutFinish:  lynx layout result come to component
  static final int COMPONENT_LAYOUT_FINISH = 2;
  // WrapView onMeasure: component's width and hegith  have been set to WrapView
  static final int COMPONENT_LAYOUT_EFFECT = 3;
  /* package */ final WrapView mRootView;

  protected long operationID = 0;

  public ListViewHolder(WrapView itemView) {
    super(itemView);
    mRootView = itemView;
  }

  public void setEstimatedHeight(int height) {
    if (height > 0) {
      mRootView.mEstimatedHeight = height;
    }
  }

  public void setEstimatedWidth(int width) {
    if (width > 0) {
      mRootView.mEstimatedWidth = width;
    }
  }

  void setUIComponent(UIComponent component) {
    mRootView.mUIComponent = component;
    FrameLayout.LayoutParams layoutParams = new FrameLayout.LayoutParams(
        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
    mRootView.addView(component.getView(), layoutParams);
    mRootView.mAdapterPosition = this.getAdapterPosition();
  }

  void removeUIComponent() {
    if (mRootView.mUIComponent != null) {
      mRootView.removeAllViews();
      mRootView.mUIComponent = null;
    }
  }

  public UIComponent getUIComponent() {
    return mRootView.mUIComponent;
  }

  static class WrapView extends FrameLayout {
    /* package */ int mLayoutStatus = COMPONENT_LAYOUT_INIT;
    private UIComponent mUIComponent;
    protected int mAdapterPosition;
    private int mEstimatedHeight = 400;
    private int mEstimatedWidth = -1;
    private boolean mComponentInitMeasure = false;
    int mMainAxisGap = 0;
    private MeasureListener mMeasureListener;
    private int mDirection = LayoutDirection.LTR;
    public WrapView(@NonNull Context context) {
      super(context);
    }

    protected void setMeasureListener(MeasureListener measureListener) {
      this.mMeasureListener = measureListener;
    }

    public void setLayoutDirection(int direction) {
      mDirection = direction;
    }

    protected MeasureListener getMeasureListener() {
      return this.mMeasureListener;
    }

    UIComponent getUIComponent() {
      return mUIComponent;
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
      int measuredWidth;
      int measuredHeight;
      if (COMPONENT_LAYOUT_INIT == mLayoutStatus) {
        measuredWidth =
            mEstimatedWidth > 0 ? mEstimatedWidth : MeasureSpec.getSize(widthMeasureSpec);
        measuredHeight = mEstimatedHeight;
        if (mComponentInitMeasure && mUIComponent != null && mUIComponent.getView() != null) {
          if (mUIComponent.getView().isLayoutRequested()) {
            mUIComponent.measureChildren();
          }
        }
      } else if (mUIComponent != null) {
        mUIComponent.measure();
        if (COMPONENT_LAYOUT_FINISH == mLayoutStatus) {
          mLayoutStatus = COMPONENT_LAYOUT_EFFECT;
        }
        measuredWidth =
            mUIComponent.getWidth() + mUIComponent.getMarginLeft() + mUIComponent.getMarginRight();
        measuredHeight =
            mUIComponent.getHeight() + mUIComponent.getMarginTop() + mUIComponent.getMarginBottom();
        measuredHeight += mMainAxisGap;
        if (mMeasureListener != null) {
          mMeasureListener.onMeasureCompleted(
              mUIComponent.getItemKey(), measuredWidth, measuredHeight);
        }
      } else {
        measuredWidth = getMeasuredWidth();
        measuredHeight = getMeasuredHeight();
      }

      setMeasuredDimension(measuredWidth, measuredHeight);

      if (UIList.DEBUG) {
        LLog.i(UIList.TAG,
            String.format(
                "WrapView.onMeasure: position=%d, itemKey=%s, measuredSize=(w %d, h %d), mLayoutStatus=%d",
                mAdapterPosition, (mUIComponent != null ? mUIComponent.getItemKey() : ""),
                getMeasuredWidth(), getMeasuredHeight(), mLayoutStatus));
      }
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
      if (UIList.DEBUG) {
        LLog.i(UIList.TAG,
            String.format(
                "WrapView.onLayout: position=%d, itemKey=%s, layout=(l %d, t %d, r %d, b %d), mLayoutStatus=%d",
                mAdapterPosition, (mUIComponent != null ? mUIComponent.getItemKey() : ""), left,
                top, right, bottom, mLayoutStatus));
      }
      if (mUIComponent != null) {
        mUIComponent.setTop(0);
        mUIComponent.setLeft(0);
        mUIComponent.layout();

        int childLeft = mUIComponent.getMarginLeft();
        int childTop = mUIComponent.getMarginTop();
        int childWidth = mUIComponent.getWidth();
        int childHeight = mUIComponent.getHeight();

        int gapSize = 0;
        if (mDirection == LayoutDirection.RTL) {
          gapSize = right - left - childWidth;
        }
        /**
         * if list-type is "flow",the wrap container view size may be larger than the component
         * view's size. when the lynx-driection is RTL, should add the gapSize.
         *
         */
        mUIComponent.getView().layout(childLeft + gapSize, childTop,
            childLeft + childWidth + gapSize, childTop + childHeight);

        /*
         *  bug: css overflow not correct
         *  Android view hierarchy: RecyclerView -> WrapView -> ComponentView
         *  LynxUI hierarchy:  UIList -> UIComponent
         *  In LynxUI layout() will set setClipChildren(false) on WrapView. So does RecyclerView
         * here
         * */
        if (mUIComponent.getOverflow() != 0x00 && getParent() != null) {
          ((ViewGroup) this.getParent()).setClipChildren(false);
        }
        Rect bounds = ViewCompat.getClipBounds(mUIComponent.getView());
        if (bounds != null) {
          bounds.set(Math.min(bounds.left, left), Math.min(bounds.top, top),
              Math.max(bounds.right, right), Math.max(bounds.bottom, bottom));
          ViewCompat.setClipBounds(this, bounds);
        }
      }
    }

    public void setComponentInitMeasure(boolean value) {
      this.mComponentInitMeasure = value;
    }
  }

  interface MeasureListener {
    void onMeasureCompleted(String itemKey, int width, int height);
  }
}
