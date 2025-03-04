// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.list;

import android.content.Context;
import android.graphics.Rect;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.FrameLayout.LayoutParams;
import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.behavior.ui.view.ComponentView;
import com.lynx.tasm.behavior.ui.view.UIComponent;

public class ListStickyManager
    extends RecyclerView.OnScrollListener implements View.OnAttachStateChangeListener {
  private UIList mUIList;
  private ContainerView mListContainer;

  // lastest sticky item after scroll
  private int mStickyTopPosition;
  private int mStickyOffset = 0;
  private int mStickyBottomPosition;
  // current sticky item info, contains position and component
  private StickyItemInfo mStickTopInfo;
  private StickyItemInfo mStickBottomInfo;
  private boolean mOldStickCategory = true;

  ListStickyManager(UIList list) {
    mUIList = list;
    mListContainer = new ListStickyManager.ContainerView(list.getLynxContext());

    mStickyTopPosition = RecyclerView.NO_POSITION;
    mStickyBottomPosition = RecyclerView.NO_POSITION;
    mStickTopInfo = new StickyItemInfo();
    mStickBottomInfo = new StickyItemInfo();

    // when RecyclerView scroll, refresh stick items
    list.getRecyclerView().addOnScrollListener(this);
    // when RecyclerView attached, means it added to parent, check parent container
    list.getRecyclerView().addOnAttachStateChangeListener(this);
  }

  @Override
  public void onViewAttachedToWindow(View recyclerView) {
    if (mListContainer.indexOfChild(recyclerView) >= 0) {
      return;
    }
    // when recyclerview addatched to UI ROOT, replace recyclerview's parent by our container
    mListContainer.addRecyclerView(recyclerView);
  }

  @Override
  public void onViewDetachedFromWindow(View v) {}

  @Override
  public void onScrolled(final RecyclerView recyclerView, int dx, int dy) {
    if (dy == 0) {
      // RecyclerView trigger onScrolled after first layout, with (dx dy) is 0.
      // do not updateSticky in Layout
      recyclerView.post(new Runnable() {
        @Override
        public void run() {
          updateSticky(recyclerView, 0);
        }
      });
    } else {
      updateSticky(recyclerView, dy);
    }
  }

  void setStickyOffset(int offset) {
    mStickyOffset = offset;
  }
  void setUseOldStickCategory(boolean oldStickCategory) {
    this.mOldStickCategory = oldStickCategory;
  }

  private void updateSticky(RecyclerView recyclerView, int dy) {
    if (dy > 0) {
      // scroll up,  restore bottom sticky item to it original holder
      restoreToHolder(mStickBottomInfo, false);
    } else if (dy < 0) {
      // scroll down, restore top sticky item to it original holder
      restoreToHolder(mStickTopInfo, true);
    }

    // find new items need show on stick
    updateTopAndBottomStickyPosition(recyclerView);
    // move sticky-top item to sticky-framelayout
    moveStickyIfNeed(mStickTopInfo, mStickyTopPosition, true);
    // move sticky-bottom item to sticky-framelayout
    moveStickyIfNeed(mStickBottomInfo, mStickyBottomPosition, false);
    // if two sticky item interaction, offset top one
    offsetStickyItemIfNeed();
  }

  /* when RecyclerView refresh LayoutManager, need clear all sticky item */
  void clear() {
    if (mStickTopInfo.mPosition != RecyclerView.NO_POSITION) {
      cleanOldStickyItem(mStickTopInfo);
    }
    if (mStickBottomInfo.mPosition != RecyclerView.NO_POSITION) {
      cleanOldStickyItem(mStickBottomInfo);
    }
  }

  ViewGroup getContainer() {
    return mListContainer;
  }

  // check if event should be handled by sticky-items
  EventTarget hitTest(int x, int y, boolean ignoreUserInteraction) {
    if (mStickTopInfo.mComponent != null) {
      Rect bounds = new Rect();
      mStickTopInfo.mComponent.getView().getHitRect(bounds);
      if (bounds.contains(x, y)) {
        return mStickTopInfo.mComponent.hitTest(x - mStickTopInfo.mComponent.getView().getLeft(),
            y - mStickTopInfo.mComponent.getView().getTop(), ignoreUserInteraction);
      }
    }
    if (mStickBottomInfo.mComponent != null) {
      Rect bounds = new Rect();
      mStickBottomInfo.mComponent.getView().getHitRect(bounds);
      if (bounds.contains(x, y)) {
        return mStickBottomInfo.mComponent.hitTest(
            x - mStickBottomInfo.mComponent.getView().getLeft(),
            y - mStickBottomInfo.mComponent.getView().getTop(), ignoreUserInteraction);
      }
    }

    return null;
  }

  void flushStickyComponent() {
    flushStickyComponent(mStickBottomInfo);
    flushStickyComponent(mStickTopInfo);
  }

  void flushStickyComponentAfterScrolling() {
    if (mOldStickCategory) {
      return;
    }
    // flush sticky footer
    UIListAdapter adapter = mUIList.getAdapter();
    int stickBottomPos = mStickBottomInfo.mPosition;
    if (stickBottomPos != RecyclerView.NO_POSITION) {
      if (adapter.isStickyBottomItem(stickBottomPos)) {
        flushStickyComponent(mStickBottomInfo);
      } else {
        forceRestoreToHolder(mStickBottomInfo, false, false);
        updateSticky(mUIList.getView(), 0);
      }
    }
    // flush sticky header
    int stickTopPos = mStickTopInfo.mPosition;
    if (stickTopPos != RecyclerView.NO_POSITION) {
      if (adapter.isStickyTopItem(stickTopPos)) {
        int firstVisible = Integer.MAX_VALUE;
        int lastVisible = Integer.MIN_VALUE;
        RecyclerView view = mUIList.getView();
        for (int i = 0; i < view.getChildCount(); ++i) {
          View child = view.getChildAt(i);
          if (child.getTop() <= mStickyOffset && child.getBottom() > mStickyOffset) {
            ListViewHolder holder = (ListViewHolder) view.getChildViewHolder(child);
            firstVisible = holder.getAdapterPosition();
          }
          if (child.getTop() < (view.getHeight() - mStickyOffset)
              && child.getBottom() >= (view.getHeight() - mStickyOffset)) {
            ListViewHolder holder = (ListViewHolder) view.getChildViewHolder(child);
            lastVisible = holder.getAdapterPosition();
          }
        }
        int sectionBegin = adapter.getStickySectionHeaderForPosition(firstVisible);
        int sectionEnd = adapter.getStickySectionFooterForPosition(lastVisible);
        if (sectionBegin != mStickTopInfo.mPosition) {
          forceRestoreToHolder(mStickTopInfo, true, false);
          updateSticky(mUIList.getView(), 0);
        }
      } else {
        forceRestoreToHolder(mStickTopInfo, true, false);
      }
    }
  }

  private void flushStickyComponent(StickyItemInfo stickInfo) {
    if (stickInfo.mPosition != RecyclerView.NO_POSITION && mUIList.getAdapter() != null
        && stickInfo.mPosition < mUIList.getAdapter().getItemCount()) {
      if (mUIList.getAdapter().mNewArch) {
        if (mUIList.isAsyncThreadStrategy()) {
          mUIList.obtainChildAsync(stickInfo.mPosition, mUIList.getAdapter().generateOperationId());
        } else {
          mUIList.obtainChild(stickInfo.mPosition, mUIList.getAdapter().generateOperationId(),
              mUIList.getAdapter().getReuseNotificationEnabled());
        }
      } else {
        mUIList.updateChild(
            stickInfo.mComponent, stickInfo.mPosition, mUIList.getAdapter().generateOperationId());
      }
    }
  }

  private static ViewGroup removeViewFromParent(final android.view.View view) {
    final android.view.ViewParent parent = view.getParent();
    if (parent instanceof ViewGroup) {
      ((ViewGroup) parent).removeView(view);
    }

    return (ViewGroup) parent;
  }

  private void updateTopAndBottomStickyPosition(RecyclerView recyclerView) {
    // find first and last visible item which intersect line with mStickyOffset
    int firstVisible = -1;
    int lastVisible = -1;
    for (int i = 0; i < recyclerView.getChildCount(); ++i) {
      View child = recyclerView.getChildAt(i);
      if (child.getTop() <= mStickyOffset && child.getBottom() > mStickyOffset) {
        ListViewHolder holder = (ListViewHolder) recyclerView.getChildViewHolder(child);
        firstVisible = holder.getAdapterPosition();
      }
      if (child.getTop() < (recyclerView.getHeight() - mStickyOffset)
          && child.getBottom() >= (recyclerView.getHeight() - mStickyOffset)) {
        ListViewHolder holder = (ListViewHolder) recyclerView.getChildViewHolder(child);
        lastVisible = holder.getAdapterPosition();
      }
    }
    UIListAdapter adapter = mUIList.getAdapter();
    // check adapter null pointer
    if (adapter == null) {
      return;
    }
    int sectionBegin = mOldStickCategory ? adapter.getSectionHeaderForPosition(firstVisible)
                                         : adapter.getStickySectionHeaderForPosition(firstVisible);
    int sectionEnd = mOldStickCategory ? adapter.getSectionFooterForPosition(lastVisible)
                                       : adapter.getStickySectionFooterForPosition(lastVisible);
    if (adapter.isStickyTopItem(sectionBegin)) {
      if (mStickyTopPosition != sectionBegin) {
        mStickyTopPosition = sectionBegin;
        if (UIList.DEBUG) {
          LLog.i(UIList.TAG, String.format("new sticky-top position %d", mStickyTopPosition));
        }
      }
    } else {
      mStickyTopPosition = RecyclerView.NO_POSITION;
    }
    if (adapter.isStickyBottomItem(sectionEnd)) {
      if (mStickyBottomPosition != sectionEnd) {
        mStickyBottomPosition = sectionEnd;
        if (UIList.DEBUG) {
          LLog.i(UIList.TAG, String.format("new sticky-bottom position %d", mStickyBottomPosition));
        }
      }
    } else {
      mStickyBottomPosition = RecyclerView.NO_POSITION;
    }

    // remove old sticky-top item when we have a new one
    if (mStickTopInfo.mPosition != RecyclerView.NO_POSITION
        && mStickyTopPosition != RecyclerView.NO_POSITION
        && mStickTopInfo.mPosition != mStickyTopPosition) {
      cleanOldStickyItem(mStickTopInfo);
    }
    // remove old sticky-bottom item when we have a new one
    if (mStickBottomInfo.mPosition != RecyclerView.NO_POSITION
        && mStickyBottomPosition != RecyclerView.NO_POSITION
        && mStickBottomInfo.mPosition != mStickyBottomPosition) {
      cleanOldStickyItem(mStickBottomInfo);
    }
  }

  private void moveStickyIfNeed(StickyItemInfo stickyItem, int newStickyItemPosition, boolean top) {
    if (newStickyItemPosition == RecyclerView.NO_POSITION
        || newStickyItemPosition == stickyItem.mPosition) {
      return;
    }

    // add new sticky item to ContainerView
    boolean needAddToSticky = false;
    RecyclerView recyclerView = mUIList.getRecyclerView();
    ListViewHolder holder =
        (ListViewHolder) recyclerView.findViewHolderForAdapterPosition(newStickyItemPosition);
    if (holder == null) {
      // create and bind sticky ComponentView ourself when the ComponentView does not exist
      holder = (ListViewHolder) recyclerView.getAdapter().createViewHolder(
          recyclerView, recyclerView.getAdapter().getItemViewType(newStickyItemPosition));
      if (mUIList.getAdapter().mNewArch) {
        mUIList.getAdapter().bindViewHolderOnNewArch(holder, newStickyItemPosition);
      } else {
        mUIList.getAdapter().bindViewHolder(holder, newStickyItemPosition);
      }
      needAddToSticky = true;
    } else {
      // holder have been created by RecyclerView,  check if it should sticky
      boolean isComponentViewOutOfTop = top && holder.mRootView.getTop() < mStickyOffset;
      boolean isComponentViewOutOfBottom =
          !top && holder.mRootView.getBottom() > mListContainer.getHeight() - mStickyOffset;
      needAddToSticky = isComponentViewOutOfTop || isComponentViewOutOfBottom;
    }

    if (needAddToSticky) {
      UIComponent component = holder.getUIComponent();
      if (component != null) {
        // remove view from WrapView and holder, this holder will be recycle and reuse sometime, so
        // remove component
        holder.removeUIComponent();
        LayoutParams layoutParams = new LayoutParams(LayoutParams.WRAP_CONTENT,
            LayoutParams.WRAP_CONTENT, top ? Gravity.TOP : Gravity.BOTTOM);
        layoutParams.topMargin = top ? mStickyOffset : 0;
        layoutParams.bottomMargin = top ? 0 : mStickyOffset;
        removeViewFromParent(component.getView());
        mListContainer.addView(component.getView(), layoutParams);
        stickyItem.mComponent = component;
        stickyItem.mPosition = newStickyItemPosition;
        if (UIList.DEBUG) {
          LLog.i(UIList.TAG, "finish moveSticky " + newStickyItemPosition);
        }
      }
    }
  }

  // force restore the view to holder. even if the hold is null, still rmove view from the sticky
  // container
  private void forceRestoreToHolder(
      StickyItemInfo stickyItem, boolean stickTop, boolean forceDetachedFromContainer) {
    if (stickyItem.mPosition == RecyclerView.NO_POSITION) {
      return;
    }
    ListViewHolder holder =
        (ListViewHolder) mUIList.getRecyclerView().findViewHolderForAdapterPosition(
            stickyItem.mPosition);
    if (holder == null) {
      // remove ComponentView from Container,  add to WrapView of ListViewHolder
      removeViewFromParent(stickyItem.mComponent.getView());
      stickyItem.mPosition = RecyclerView.NO_POSITION;
      stickyItem.mComponent = null;
      return;
    }
    restoreToHolderIfNeed(stickyItem, holder, stickTop, forceDetachedFromContainer);
  }

  // try restore stickView to the holder
  private void restoreToHolder(StickyItemInfo stickyItem, boolean stickTop) {
    if (stickyItem.mPosition == RecyclerView.NO_POSITION) {
      return;
    }
    ListViewHolder holder =
        (ListViewHolder) mUIList.getRecyclerView().findViewHolderForAdapterPosition(
            stickyItem.mPosition);
    if (holder == null) {
      return;
    }
    restoreToHolderIfNeed(stickyItem, holder, stickTop, false);
  }

  protected void resetStickyView() {
    forceRestoreToHolder(mStickTopInfo, true, true);
    forceRestoreToHolder(mStickBottomInfo, false, true);
  }

  // when the sticky item's position come to visible, restore sticky component to holder
  private void restoreToHolderIfNeed(StickyItemInfo stickyItem, @NonNull ListViewHolder holder,
      boolean stickTop, boolean forceDetachedFromContainer) {
    if (stickyItem.mPosition == RecyclerView.NO_POSITION) {
      return;
    }
    // when the sticky item's position come to visible, restore sticky component to holder
    int topOffset = holder.mRootView.getTop();
    View stickView = stickyItem.mComponent.getView();
    if (stickView == null) {
      return;
    }
    boolean needRestore = (stickTop && topOffset > stickView.getTop())
        || (!stickTop && topOffset < stickView.getTop());

    //  restore to the item viewHolder, or when the forceDetachedFromContainer is true, should force
    //  detach the sticky view from container
    if (needRestore || forceDetachedFromContainer) {
      if (UIList.DEBUG) {
        LLog.i(UIList.TAG, "restoreToHolderIfNeed stickyItem position" + stickyItem.mPosition);
      }
      // before restoring component, reset the component TranslationY
      resetComponentViewTranslationY(stickyItem);
      // remove ComponentView from Container,  add to WrapView of ListViewHolder
      removeViewFromParent(stickyItem.mComponent.getView());

      // if this holder have component, it is created by RecyclerView, remove it because we have
      // single instance
      if (holder.getUIComponent() != null) {
        if (mUIList.getAdapter().mNewArch) {
          mUIList.recycleChild(holder.getUIComponent());
        }
        holder.removeUIComponent();
      }
      holder.setUIComponent(stickyItem.mComponent);
      stickyItem.mPosition = RecyclerView.NO_POSITION;
      stickyItem.mComponent = null;
    }
  }

  // reset the component TranslationY
  private void resetComponentViewTranslationY(StickyItemInfo stickInfo) {
    if (stickInfo == null || stickInfo.mComponent == null
        || stickInfo.mComponent.getView() == null) {
      return;
    }
    stickInfo.mComponent.getView().setTranslationY(0.f);
  }

  private void cleanOldStickyItem(StickyItemInfo stickInfo) {
    if (UIList.DEBUG) {
      LLog.d(UIList.TAG, "cleanOldStickyItem position " + stickInfo.mPosition);
    }
    // before restoring component, reset the component TranslationY
    resetComponentViewTranslationY(stickInfo);
    // firstly remove view from container
    ListViewHolder stickyHolder =
        (ListViewHolder) mUIList.getRecyclerView().findViewHolderForAdapterPosition(
            stickInfo.mPosition);
    removeViewFromParent(stickInfo.mComponent.getView());
    // secondly restore view to ListViewHolder. if ListViewHolder is null, recycle the component
    if (stickyHolder != null) {
      stickyHolder.setUIComponent(stickInfo.mComponent);
      stickInfo.mPosition = RecyclerView.NO_POSITION;
      stickInfo.mComponent = null;
    } else {
      if (mUIList.getAdapter().mNewArch) {
        mUIList.recycleChild(stickInfo.mComponent);
      } else {
        mUIList.removeChild(stickInfo.mComponent);
      }
    }
  }

  private void offsetStickyItemIfNeed() {
    offsetStickyTopItemIfNeed();
    offsetStickyBottomInNeed();
  }

  private void offsetStickyTopItemIfNeed() {
    if (mStickTopInfo.mPosition == RecyclerView.NO_POSITION) {
      return;
    }

    RecyclerView view = mUIList.getView();
    if (view == null || view.getChildCount() <= 1) {
      return;
    }
    View firstView = view.getChildAt(0);
    View lastView = view.getChildAt(view.getChildCount() - 1);
    if (firstView == null || lastView == null) {
      return;
    }
    RecyclerView.ViewHolder firstViewHolder = view.getChildViewHolder(firstView);
    RecyclerView.ViewHolder lastViewHolder = view.getChildViewHolder(lastView);
    if (firstViewHolder == null || lastViewHolder == null) {
      return;
    }
    int first = firstViewHolder.getAdapterPosition();
    int last = lastViewHolder.getAdapterPosition();
    if (first < 0 || last < 0) {
      return;
    }
    // find next StickySectionHeader
    int nextSectionHeader =
        mUIList.getAdapter().getNextStickySectionHeaderForPosition(mStickTopInfo.mPosition + 1);
    // if the next StickySectionHeader is off-screen
    if (nextSectionHeader < first || nextSectionHeader > last) {
      return;
    }
    if (nextSectionHeader == RecyclerView.NO_POSITION) {
      return;
    }
    if (!mOldStickCategory && !mUIList.getAdapter().isStickyTopItem(nextSectionHeader)) {
      return;
    }
    int detaY = 0;
    RecyclerView recyclerView = mUIList.getRecyclerView();
    ListViewHolder holder =
        (ListViewHolder) recyclerView.findViewHolderForAdapterPosition(nextSectionHeader);
    if (holder != null) {
      detaY =
          Math.min(0, holder.itemView.getTop() - mStickTopInfo.mComponent.getView().getBottom());
    }
    mStickTopInfo.mComponent.getView().setTranslationY(detaY);
    if (mStickTopInfo.mComponent.getView().getBottom() + detaY < 0) {
      cleanOldStickyItem(mStickTopInfo);
    }
  }
  private void offsetStickyBottomInNeed() {
    if (mStickBottomInfo.mPosition == RecyclerView.NO_POSITION) {
      return;
    }
    int preSectionFooter =
        mUIList.getAdapter().getSectionHeaderForPosition(mStickBottomInfo.mPosition - 1);
    if (preSectionFooter == RecyclerView.NO_POSITION) {
      return;
    }
    if (!mOldStickCategory && !(mUIList.getAdapter().isStickyBottomItem(preSectionFooter))) {
      return;
    }
    int detaY = 0;
    RecyclerView recyclerView = mUIList.getRecyclerView();
    ListViewHolder holder =
        (ListViewHolder) recyclerView.findViewHolderForAdapterPosition(preSectionFooter);
    if (holder != null) {
      detaY =
          Math.max(0, holder.itemView.getBottom() - mStickBottomInfo.mComponent.getView().getTop());
    }
    mStickBottomInfo.mComponent.getView().setTranslationY(detaY);
    if (mStickBottomInfo.mComponent.getView().getTop() + detaY > recyclerView.getHeight()) {
      cleanOldStickyItem(mStickBottomInfo);
    }
  }
  class ContainerView extends FrameLayout {
    public ContainerView(@NonNull Context context) {
      super(context);
    }

    void addRecyclerView(View recyclerView) {
      // remove RecyclerView from parent
      ViewGroup parent = (ViewGroup) recyclerView.getParent();
      if (parent == null) {
        LLog.e("ListStickyManager", "addRecyclerView failed, parent is null.");
        return;
      }
      int indexOfRecyclerView = parent.indexOfChild(recyclerView);
      parent.removeViewAt(indexOfRecyclerView);
      // ViewContainer for RecyclerView and sticky footer/header
      LayoutParams rvLayoutParams =
          new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
      this.addView(recyclerView, rvLayoutParams);
      // add ViewContainer to parent
      parent.addView(this, indexOfRecyclerView);
    }

    protected void measureChildWithMargins(View child, int parentWidthMeasureSpec, int widthUsed,
        int parentHeightMeasureSpec, int heightUsed) {
      if (child instanceof ComponentView) {
        child.measure(child.getMeasuredWidth(), child.getMeasuredHeight());
        return;
      }
      super.measureChildWithMargins(
          child, parentWidthMeasureSpec, widthUsed, parentHeightMeasureSpec, heightUsed);
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
      // layout the stickyContainer, sometimes the child view may be removed from the parent,
      // the childCount is dirty,it will lead to the NPE.
      // getChildAt(i) is null
      try {
        super.onLayout(changed, left, top, right, bottom);
      } catch (NullPointerException e) {
        e.printStackTrace();
      }
      // for lynx async layout
      offsetStickyItemIfNeed();
    }
  }

  UIComponent getStickyTopComponent() {
    return mStickTopInfo.mComponent;
  }

  UIComponent getStickyBottomComponent() {
    return mStickBottomInfo.mComponent;
  }

  private static class StickyItemInfo {
    private UIComponent mComponent;
    private int mPosition;

    public StickyItemInfo() {
      mComponent = null;
      mPosition = RecyclerView.NO_POSITION;
    }
  }
}
