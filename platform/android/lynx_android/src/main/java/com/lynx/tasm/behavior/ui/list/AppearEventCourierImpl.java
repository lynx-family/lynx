// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.list;

import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;
import androidx.annotation.NonNull;
import com.lynx.tasm.EventEmitter;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.ui.view.UIComponent;
import com.lynx.tasm.event.LynxListEvent;
import java.lang.ref.WeakReference;
import java.util.Arrays;
import java.util.LinkedList;

public class AppearEventCourierImpl implements AppearEventCourierInterface {
  @NonNull private final EventEmitter mEventEmitter;
  private final Handler mHandler;
  private Callback mCallback;
  private LinkedList<AppearEvent> mFlushingQueue;
  private LinkedList<AppearEvent> mPendingQueue;
  private boolean mEnableDisappear = false;

  public AppearEventCourierImpl(@NonNull EventEmitter emitter) {
    mEventEmitter = emitter;
    mHandler = new Handler(Looper.getMainLooper());
    mPendingQueue = new LinkedList<>();
    mFlushingQueue = new LinkedList<>();
    mCallback = new Callback(this);
    int firstDelay = 500;
    mHandler.postDelayed(mCallback, firstDelay);
  }

  void setDisappear(boolean enableDisappear) {
    mEnableDisappear = enableDisappear;
  }

  @Override
  public void onListNodeAttached(@NonNull ListViewHolder holder) {
    if (UIList.DEBUG) {
      LLog.i(UIList.TAG, "onNodeAppear " + holder.getLayoutPosition());
    }
    mPendingQueue.push(new AppearEvent(holder, LynxListEvent.EVENT_NODE_APPEAR));
    startTimerIfNeeded();
  }

  @Override
  public void onListNodeDetached(@NonNull ListViewHolder holder) {
    if (UIList.DEBUG) {
      LLog.i(UIList.TAG, "onNodeDisappear " + holder.getLayoutPosition());
    }
    mPendingQueue.push(new AppearEvent(holder, LynxListEvent.EVENT_NODE_DISAPPEAR));
    startTimerIfNeeded();
  }

  @Override
  public void holderAttached(ListViewHolder holder) {
    for (AppearEvent event : mFlushingQueue) {
      if (event.holder == holder && holder.getUIComponent() != null) {
        event.key = holder.getUIComponent().getItemKey();
      }
    }
    for (AppearEvent event : mPendingQueue) {
      if (event.holder == holder) {
        event.key = holder.getUIComponent().getItemKey();
      }
    }
  }

  @Override
  public void onListLayout() {}

  private boolean isDuplicatedEvent(AppearEvent event1, AppearEvent event2) {
    if (mEnableDisappear) {
      if (!TextUtils.isEmpty(event1.type) && !event1.type.equals(event2.type)) {
        return false;
      }
    }
    if (event1.key != null || event2.key != null) {
      return TextUtils.equals(event1.key, event2.key);
    }
    return event1.position == event2.position;
  }

  private void flush() {
    mCallback = null;
    if (UIList.DEBUG) {
      LLog.i(UIList.TAG,
          "Courier flush pending " + mPendingQueue.size() + " "
              + Arrays.toString(mPendingQueue.toArray()) + " flushing " + mFlushingQueue.size()
              + " " + Arrays.toString(mFlushingQueue.toArray()));
    }
    while (mFlushingQueue.size() > 0) {
      AppearEvent event = mFlushingQueue.removeFirst();
      if (!valid(event)) {
        continue;
      }
      boolean foundDuplicate = false;
      for (AppearEvent pendingEvent : mFlushingQueue) {
        if (isDuplicatedEvent(event, pendingEvent)) {
          foundDuplicate = true;
          mFlushingQueue.remove(pendingEvent);
          break;
        }
      }
      if (foundDuplicate) {
        continue;
      }
      for (AppearEvent pendingEvent : mPendingQueue) {
        if (isDuplicatedEvent(event, pendingEvent)) {
          foundDuplicate = true;
          mPendingQueue.remove(pendingEvent);
          break;
        }
      }
      if (!foundDuplicate && shouldSend(event)) {
        sendNodeEvent(event);
      }
    }
    mFlushingQueue = mPendingQueue;
    mPendingQueue = new LinkedList<>();
    if (mFlushingQueue.size() > 0) {
      startTimerIfNeeded();
    }
  }

  private void startTimerIfNeeded() {
    if (mCallback != null) {
      return;
    }
    mCallback = new Callback(this);
    int scrollDelay = 50;
    mHandler.postDelayed(mCallback, scrollDelay);
  }

  protected void sendNodeEvent(AppearEvent appearEvent) {
    if (!valid(appearEvent)) {
      return;
    }

    if (UIList.DEBUG) {
      LLog.i(UIList.TAG,
          "sendNodeEvent " + appearEvent.type + "  " + appearEvent.position + " "
              + appearEvent.key);
    }
    LynxListEvent event = LynxListEvent.createListEvent(appearEvent.sign, appearEvent.type);

    event.addDetail("position", appearEvent.position);
    event.addDetail("key", appearEvent.key);
    mEventEmitter.sendCustomEvent(event);
  }

  private static class Callback implements Runnable {
    WeakReference<AppearEventCourierImpl> mCourier;
    public Callback(AppearEventCourierImpl courier) {
      mCourier = new WeakReference<>(courier);
    }

    @Override
    public void run() {
      AppearEventCourierImpl courier = mCourier.get();
      if (courier == null) {
        return;
      }
      courier.flush();
    }
  }

  /**
   * mEnableDisappear is false:should invoke custom event;
   * mEnableDisappear is true:the component is null,the nodeDisappear event should be invoked,
   * but the nodeAppear event should not be invoked;
   * @param event
   * @return
   */
  boolean shouldSend(AppearEvent event) {
    if (!valid(event)) {
      return false;
    }

    ListViewHolder holder = event.holder;
    UIComponent component = holder.getUIComponent();
    if (component == null || component.getEvents() == null) {
      return mEnableDisappear ? isDisAppearEvent(event) : false;
    }
    return component.getEvents().containsKey(event.type);
  }

  /**
   * if the type of the event is EVENT_NODE_APPEAR, return true;
   * @param event
   * @return
   */
  boolean isAppearEvent(@NonNull AppearEvent event) {
    return LynxListEvent.EVENT_NODE_APPEAR.equals(event.type);
  }

  /**
   *  if the type of the event is EVENT_NODE_DISAPPEAR, return true;
   * @param event
   * @return
   */
  boolean isDisAppearEvent(@NonNull AppearEvent event) {
    return LynxListEvent.EVENT_NODE_DISAPPEAR.equals(event.type);
  }

  /**
   *
   * if event is null,the event is  not valid.
   * when mEnableDisappear is false:
   * if  the component is null, the event is  not valid;
   * when mEnableDisappear is true:
   * if the component is null, the disappear event is valid,the appear event is not valid
   * @param event
   * @return
   */
  boolean valid(AppearEvent event) {
    if (event == null) {
      return false;
    }
    ListViewHolder holder = event.holder;
    if (holder.getUIComponent() == null) {
      return mEnableDisappear ? isDisAppearEvent(event) : false;
    }
    return event.type != null;
  }

  protected static class AppearEvent {
    public AppearEvent(ListViewHolder holder, String type) {
      this.holder = holder;
      this.type = type;
      this.position = holder.getLayoutPosition();
      if (holder.getUIComponent() != null) {
        this.key = holder.getUIComponent().getItemKey();
        this.sign = holder.getUIComponent().getSign();
      }
    }
    ListViewHolder holder;
    public String type;
    public int position;
    public String key;
    public int sign;

    @NonNull
    @Override
    public String toString() {
      return "{type='" + type + "', position=" + position + ", key='" + key + "'}";
    }
  }
}
