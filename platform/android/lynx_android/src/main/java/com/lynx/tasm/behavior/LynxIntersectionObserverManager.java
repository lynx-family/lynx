// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.EventEmitter;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.core.JSProxy;
import com.lynx.tasm.event.LynxCustomEvent;
import com.lynx.tasm.event.LynxEvent;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;
import java.util.ArrayList;

public class LynxIntersectionObserverManager
    extends LynxObserverManager implements EventEmitter.LynxEventObserver {
  private final ArrayList<LynxIntersectionObserver> mObservers;
  private final WeakReference<LynxContext> mContext;
  private final WeakReference<JSProxy> mJSProxy;
  private boolean mEnableNewIntersectionObserver;

  final private String TAG = "Lynx.IntersectionObserver";

  public LynxIntersectionObserverManager(LynxContext context, JSProxy proxy) {
    super("Lynx.IntersectionObserver");
    TraceEvent.beginSection("LynxIntersectionObserverManager initialized");
    mContext = new WeakReference<>(context);
    mRootBodyRef = new WeakReference<>(context.getUIBody());
    mJSProxy = new WeakReference<>(proxy);
    mObservers = new ArrayList<>();
    mEnableNewIntersectionObserver = false;
    TraceEvent.endSection("LynxIntersectionObserverManager initialized");
  }

  public LynxContext getContext() {
    return mContext.get();
  }

  // When use by setting intersection-observer on element, some functions will be called on other
  // thread, which will cause multi-threaded access to mObservers. Thus, throw them to UIThread.
  public void sendIntersectionObserverEvent(final int componentSign, final JavaOnlyMap args) {
    Runnable runnable = new Runnable() {
      @Override
      public void run() {
        LynxCustomEvent event = new LynxCustomEvent(componentSign, "intersection", args);
        LynxContext lynxContext = mContext.get();
        if (lynxContext != null) {
          EventEmitter eventEmitter = lynxContext.getEventEmitter();
          if (eventEmitter != null) {
            eventEmitter.sendCustomEvent(event);
          }
        }
      }
    };
    UIThreadUtils.runOnUiThreadImmediately(runnable);
  }

  public void callIntersectionObserver(
      final int observerId, final int callbackId, final JavaOnlyMap args) {
    Runnable runnable = new Runnable() {
      @Override
      public void run() {
        JSProxy proxy = mJSProxy.get();
        if (proxy != null) {
          proxy.callIntersectionObserver(observerId, callbackId, args);
        }
      }
    };
    UIThreadUtils.runOnUiThreadImmediately(runnable);
  }

  public void addIntersectionObserver(final LynxIntersectionObserver observer) {
    Runnable runnable = new Runnable() {
      @Override
      public void run() {
        if (mObservers.contains(observer)) {
          return;
        }
        mObservers.add(observer);
        if (mObservers.size() == 1) {
          LynxContext context = observer.getContext();
          if (context != null) {
            mEnableNewIntersectionObserver = context.getEnableNewIntersectionObserver();
          }
          if (mEnableNewIntersectionObserver) {
            addToObserverTree();
          }
        }
      }
    };
    UIThreadUtils.runOnUiThreadImmediately(runnable);
  }

  public void removeIntersectionObserver(final int observerId) {
    Runnable runnable = new Runnable() {
      @Override
      public void run() {
        for (LynxIntersectionObserver observer : mObservers) {
          if (observer.getObserverId() == observerId) {
            mObservers.remove(observer);
            if (mEnableNewIntersectionObserver && mObservers.isEmpty()) {
              destroy();
            }
            return;
          }
        }
      }
    };
    UIThreadUtils.runOnUiThreadImmediately(runnable);
  }

  public void removeAttachedIntersectionObserver(final LynxBaseUI ui) {
    Runnable runnable = new Runnable() {
      @Override
      public void run() {
        for (LynxIntersectionObserver observer : mObservers) {
          if (observer.getAttachedUI() == ui) {
            mObservers.remove(observer);
            if (mEnableNewIntersectionObserver && mObservers.isEmpty()) {
              destroy();
            }
            return;
          }
        }
      }
    };
    UIThreadUtils.runOnUiThreadImmediately(runnable);
  }

  // Running on UIThread
  public LynxIntersectionObserver getObserverById(int observerId) {
    for (LynxIntersectionObserver observer : mObservers) {
      if (observer.getObserverId() == observerId) {
        return observer;
      }
    }
    return null;
  }

  // Running on UIThread
  @Override
  public void onLynxEvent(EventEmitter.LynxEventType type, LynxEvent event) {
    if (this.mObservers.size() == 0)
      return;

    boolean shouldHandle = false;
    if (type == EventEmitter.LynxEventType.kLynxEventTypeLayoutEvent) {
      shouldHandle = true;
    } else if (type == EventEmitter.LynxEventType.kLynxEventTypeCustomEvent) {
      String name = event.getName();
      if ("scroll".equals(name) || "scrolltoupper".equals(name) || "scrolltolower".equals(name)) {
        shouldHandle = true;
      }
    }

    if (!shouldHandle)
      return;

    notifyObservers();
  }

  public void notifyObservers() {
    Runnable runnable = new Runnable() {
      @Override
      public void run() {
        for (LynxIntersectionObserver observer : mObservers) {
          if (observer == null) {
            LLog.e(TAG,
                "LynxIntersectionObserverManager.notifyObservers failed, because observer is null");
            return;
          }
          observer.checkForIntersections();
        }
      }
    };
    UIThreadUtils.runOnUiThreadImmediately(runnable);
  }

  @Override
  protected void observerHandlerInner() {
    // like Exposure, issue: #6896.
    if (!mRootViewPainted) {
      LLog.e(TAG, "Lynx intersectionObserverHandler failed since rootView not draw");
      return;
    }

    // like Exposure, issue: #6800.
    UIBody.UIBodyView view = getRootView();
    if (view == null) {
      LLog.e(TAG, "Lynx intersectionObserverHandler failed since rootView is null");
      return;
    }
    notifyObservers();
  }

  public void clear() {
    Runnable runnable = new Runnable() {
      @Override
      public void run() {
        mObservers.clear();
        destroy();
      }
    };
    UIThreadUtils.runOnUiThreadImmediately(runnable);
  }
}
