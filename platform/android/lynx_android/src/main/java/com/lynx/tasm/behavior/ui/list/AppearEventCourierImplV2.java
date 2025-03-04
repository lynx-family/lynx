// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.list;

import android.graphics.Rect;
import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.ViewTreeObserver;
import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import com.lynx.tasm.EventEmitter;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.ui.view.UIComponent;
import com.lynx.tasm.event.LynxListEvent;
import com.lynx.tasm.utils.DisplayMetricsHolder;
import java.util.ArrayList;
import java.util.HashSet;

public class AppearEventCourierImplV2 implements AppearEventCourierInterface {
  @NonNull final private EventEmitter mEventEmitter;
  final private Handler mHandler;
  final private CallBack mCallBack;
  final private RecyclerView mRecyclerView;
  final private HashSet<CellInfo> mLastVisibleCells = new HashSet<CellInfo>();
  final private Rect mScreenRect = new Rect();
  // do not create every time, use one instance
  final private int[] mViewLocationOnScreen = new int[2];
  final private Rect mViewRect = new Rect();
  final private static long INTERVAL_IN_MS = 200;

  private long mLastUpdateTimeStamp = 0;

  public AppearEventCourierImplV2(
      @NonNull EventEmitter emitter, @NonNull RecyclerView recyclerView) {
    DisplayMetrics metric =
        DisplayMetricsHolder.getRealScreenDisplayMetrics(recyclerView.getContext());
    mScreenRect.set(0, 0, metric.widthPixels, metric.heightPixels);
    mEventEmitter = emitter;
    mHandler = new Handler(Looper.getMainLooper());
    mCallBack = new CallBack();
    mRecyclerView = recyclerView;
    recyclerView.addOnScrollListener(new ListScrollListener());

    addTreeObserver();
  }

  @Override
  public void onListNodeAttached(@NonNull ListViewHolder holder) {}

  @Override
  public void onListNodeDetached(@NonNull ListViewHolder holder) {}

  @Override
  public void holderAttached(ListViewHolder holder) {}

  @Override
  public void onListLayout() {
    onListContentChange(false);
  }

  private void addTreeObserver() {
    ViewTreeObserver viewTreeObserver = mRecyclerView.getViewTreeObserver();
    viewTreeObserver.addOnDrawListener(new ViewTreeObserver.OnDrawListener() {
      @Override
      public void onDraw() {
        onListContentChange(false);
      }
    });
    viewTreeObserver.addOnScrollChangedListener(new ViewTreeObserver.OnScrollChangedListener() {
      @Override
      public void onScrollChanged() {
        onListContentChange(false);
      }
    });
  }

  private void sendNodeEvent(String type, CellInfo cellinfo) {
    if (UIList.DEBUG) {
      LLog.i(UIList.TAG, "sendNodeEvent " + type + "  " + cellinfo.mPosition + " " + cellinfo.mKey);
    }
    LynxListEvent event = LynxListEvent.createListEvent(cellinfo.mSign, type);
    event.addDetail("position", cellinfo.mPosition);
    event.addDetail("key", cellinfo.mKey);
    mEventEmitter.sendCustomEvent(event);
  }

  /* @param force true if must send node appear */
  private void onListContentChange(boolean force) {
    long now = System.currentTimeMillis();
    if (!force && now - mLastUpdateTimeStamp < INTERVAL_IN_MS) {
      return;
    }
    mLastUpdateTimeStamp = now;
    mHandler.removeCallbacks(mCallBack);
    long time_nano = System.nanoTime();
    HashSet<CellInfo> visibleCells = new HashSet<>(); // all visible cell on current frame
    ArrayList<CellInfo> appearCells = new ArrayList<>(); // new appear cell
    for (int i = 0; i < mRecyclerView.getChildCount(); ++i) {
      View child = mRecyclerView.getChildAt(i);
      ListViewHolder attachedHolder = (ListViewHolder) mRecyclerView.getChildViewHolder(child);

      UIComponent component = attachedHolder.getUIComponent();
      boolean bindEvent = (component != null && component.getEvents() != null)
          ? component.getEvents().containsKey(LynxListEvent.EVENT_NODE_APPEAR)
              | component.getEvents().containsKey(LynxListEvent.EVENT_NODE_DISAPPEAR)
          : false;
      if (bindEvent && checkViewAppearedOnScreen(child)) {
        CellInfo info = new CellInfo(attachedHolder.getUIComponent().getSign(),
            attachedHolder.getLayoutPosition(), attachedHolder.getUIComponent().getItemKey());
        visibleCells.add(info);
        boolean cellPresentOnLastFrame = mLastVisibleCells.contains(info);
        if (!cellPresentOnLastFrame) { // appear cells on this frame
          appearCells.add(info);
        }
      }
    }

    mLastVisibleCells.removeAll(visibleCells); // disappear cells on this frame
    for (CellInfo info : mLastVisibleCells) {
      sendNodeEvent(LynxListEvent.EVENT_NODE_DISAPPEAR, info);
    }
    mLastVisibleCells.clear();
    mLastVisibleCells.addAll(visibleCells);
    for (CellInfo info : appearCells) {
      sendNodeEvent(LynxListEvent.EVENT_NODE_APPEAR, info);
    }
    if (UIList.DEBUG) {
      LLog.i(
          UIList.TAG, "onListContentChange time " + (System.nanoTime() - time_nano) / 1000 + " us");
    }
    if (!force) {
      mHandler.postDelayed(mCallBack, INTERVAL_IN_MS);
    }
  }

  /* check if View is on screen according to Screen Rect and View Rect  */
  private boolean checkViewAppearedOnScreen(View view) {
    mViewRect.set(0, 0, view.getWidth(), view.getHeight());
    view.getLocationOnScreen(mViewLocationOnScreen);
    mViewRect.offset(mViewLocationOnScreen[0], mViewLocationOnScreen[1]);
    return mViewRect.intersect(mScreenRect);
  }

  private class ListScrollListener extends RecyclerView.OnScrollListener {
    @Override
    public void onScrolled(@NonNull RecyclerView recyclerView, int dx, int dy) {
      onListContentChange(false);
    }

    @Override
    public void onScrollStateChanged(@NonNull RecyclerView recyclerView, int newState) {
      if (RecyclerView.SCROLL_STATE_IDLE == newState) {
        onListContentChange(true);
      }
    }
  }

  private class CallBack implements Runnable {
    @Override
    public void run() {
      onListContentChange(true);
    }
  }

  private class CellInfo {
    int mSign;
    int mPosition;
    String mKey;

    public CellInfo(int sign, int pos, String key) {
      mSign = sign;
      mPosition = pos;
      mKey = key;
    }

    @Override
    public boolean equals(Object obj) {
      if (obj == null) {
        return false;
      }
      if (this == obj) {
        return true;
      }
      if (obj instanceof CellInfo) {
        CellInfo other = (CellInfo) obj;
        return (other.mSign == mSign) && (other.mPosition == mPosition)
            && TextUtils.equals(other.mKey, mKey);
      }
      return false;
    }

    @Override
    public int hashCode() {
      return mSign << 16 + mPosition;
    }
  }
}
