// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import android.os.Handler;
import android.os.Looper;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxIntersectionObserverManager;
import com.lynx.tasm.common.LepusBuffer;
import com.lynx.tasm.core.LynxEngineProxy;
import com.lynx.tasm.event.LynxCustomEvent;
import com.lynx.tasm.event.LynxEvent;
import com.lynx.tasm.event.LynxTouchEvent;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.util.ArrayList;

public class LynxEventEmitter extends EventEmitter {
  private static final String TAG = "EventEmitter";

  // TODO(songshourui.null): LynxEngineProxyWrapper is used to be replaced by test-related classes
  // during testing. Currently, LynxEngineProxy is a final class and cannot be mocked during
  // testing, hence the need for a LynxEngineProxyWrapper. In the future, once LynxEngineProxy can
  // be mocked, this class will be deleted.
  static class LynxEngineProxyWrapper {
    private LynxEngineProxy mEngineProxy = null;

    public LynxEngineProxyWrapper(LynxEngineProxy engineProxy) {
      mEngineProxy = engineProxy;
    }

    void sendTouchEvent(LynxTouchEvent event) {
      if (mEngineProxy != null) {
        mEngineProxy.sendTouchEvent(event);
      }
    }

    void sendMultiTouchEvent(LynxTouchEvent event) {
      if (mEngineProxy != null) {
        mEngineProxy.sendMultiTouchEvent(event);
      }
    }

    void sendCustomEvent(LynxCustomEvent event) {
      if (mEngineProxy != null) {
        mEngineProxy.sendCustomEvent(event);
      }
    }

    void sendGestureEvent(final String name, final int tag, int gestureId, final ByteBuffer params,
        final int length) {
      if (mEngineProxy != null) {
        mEngineProxy.sendGestureEvent(name, tag, gestureId, params, length);
      }
    }

    void onPseudoStatusChanged(final int id, final int preStatus, final int currentStatus) {
      if (mEngineProxy != null) {
        mEngineProxy.onPseudoStatusChanged(id, preStatus, currentStatus);
      }
    }
  };

  private boolean mInPreLoad = false;

  private WeakReference<LynxEventReporter> mEventReporter;

  LynxEngineProxyWrapper mEngineProxy;

  private ITestTapTrack mTrack;

  final ArrayList<LynxEventObserver> mEventObservers = new ArrayList<>();

  final Handler mHandler = new Handler(Looper.getMainLooper());

  public LynxEventEmitter(LynxEngineProxy engineProxy) {
    super();
    mEngineProxy = new LynxEngineProxyWrapper(engineProxy);
    mEventReporter = new WeakReference<>(null);
  }

  @Override
  public void sendTouchEvent(LynxTouchEvent event) {
    String name = event.getName();
    if (mEngineProxy != null && !mInPreLoad) {
      if (onLynxEvent(event)) {
        return;
      }
      if (mTrack != null && "tap".equals(name)) {
        mTrack.onTap();
      }

      mEngineProxy.sendTouchEvent(event);
    } else {
      LLog.e(TAG,
          "sendTouchEvent event: " + name + " failed since mEngineProxy is null or in preload.");
    }
  }

  @Override
  // The return value indicates whether the client intercepted the event.
  public boolean onLynxEvent(LynxEvent event) {
    LynxEventReporter reporter = mEventReporter.get();
    if (reporter != null) {
      return reporter.onLynxEvent(event);
    } else {
      LLog.e(
          TAG, "onLynxEvent event: " + event.getName() + " failed since mEventReporter is null.");
    }
    return false;
  }

  @Override
  public void sendMultiTouchEvent(LynxTouchEvent event) {
    if (mEngineProxy != null && !mInPreLoad) {
      if (onLynxEvent(event)) {
        return;
      }

      mEngineProxy.sendMultiTouchEvent(event);
    } else {
      LLog.e(TAG,
          "sendMultiTouchEvent event: " + event.getName()
              + " failed since mEngineProxy is null or in preload.");
    }
  }

  @Override
  public void sendCustomEvent(LynxCustomEvent event) {
    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
      @Override
      public void run() {
        String name = event.getName();
        if (mEngineProxy != null && !mInPreLoad) {
          if (onLynxEvent(event)) {
            return;
          }
          event.addDetail("timestamp", event.getTimestamp());
          mEngineProxy.sendCustomEvent(event);
        } else {
          LLog.e(TAG,
              "sendCustomEvent event: " + event.getName()
                  + " failed since mEngineProxy is null or in preload.");
        }
        notifyEventObservers(LynxEventType.kLynxEventTypeCustomEvent, event);
      }
    });
  }

  /**
   * Sends a custom gesture event with a specific gesture ID.
   *
   * @param gestureId The identifier of the specific gesture.
   * @param event The custom event to be sent.
   */
  @Override
  public void sendGestureEvent(int gestureId, LynxCustomEvent event) {
    String name = event.getName();
    if (mEngineProxy != null && !mInPreLoad) {
      ByteBuffer buffer = LepusBuffer.INSTANCE.encodeMessage(event.eventParams());
      int length = buffer == null ? 0 : buffer.position();
      mEngineProxy.sendGestureEvent(name, event.getTag(), gestureId, buffer, length);
    } else {
      LLog.e(TAG,
          "sendGestureEvent event: " + name + " failed since mEngineProxy is null or in preload.");
    }
  }

  @Override
  public void onPseudoStatusChanged(int sign, int preStatus, int currentStatus) {
    if (preStatus == currentStatus) {
      return;
    }
    if (mEngineProxy != null) {
      mEngineProxy.onPseudoStatusChanged(sign, preStatus, currentStatus);
    } else {
      LLog.e(TAG, "onPseudoStatusChanged id: " + sign + " failed since mEngineProxy is null.");
    }
  }

  @Override
  public void setTestTapTracker(ITestTapTrack track) {
    mTrack = track;
  }

  @Override
  public void sendLayoutEvent() {
    this.notifyEventObservers(LynxEventType.kLynxEventTypeLayoutEvent, null);
  }

  @Override
  public void setInPreLoad(boolean preload) {
    mInPreLoad = preload;
  }

  @Override
  public void addObserver(LynxEventObserver observer) {
    if (mEventObservers.contains(observer))
      return;
    mEventObservers.add(observer);
  }

  @Override
  public void removeObserver(LynxEventObserver observer) {
    if (!mEventObservers.contains(observer))
      return;
    mEventObservers.remove(observer);
  }

  private void notifyEventObservers(final LynxEventType type, final LynxEvent event) {
    Runnable runnable = new Runnable() {
      @Override
      public void run() {
        for (LynxEventObserver observer : mEventObservers) {
          // if use new IntersectionObserver, don't notify observer here.
          if (observer instanceof LynxIntersectionObserverManager) {
            LynxContext context = ((LynxIntersectionObserverManager) observer).getContext();
            if (context != null && context.getEnableNewIntersectionObserver()) {
              continue;
            }
          }

          observer.onLynxEvent(type, event);
        }
      }
    };

    if (Looper.getMainLooper() == Looper.myLooper()) {
      // if on main thread, exec the runnable
      runnable.run();
    } else {
      // if not on main thread, post to main thread
      mHandler.post(runnable);
    }
  }

  @Override
  public void registerEventReporter(LynxEventReporter reporter) {
    mEventReporter = new WeakReference<>(reporter);
  }
}
