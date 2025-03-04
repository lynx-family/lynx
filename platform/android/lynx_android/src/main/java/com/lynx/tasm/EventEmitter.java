// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import android.os.Handler;
import android.os.Looper;
import androidx.annotation.NonNull;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxIntersectionObserverManager;
import com.lynx.tasm.common.LepusBuffer;
import com.lynx.tasm.core.LynxEngineProxy;
import com.lynx.tasm.event.LynxCustomEvent;
import com.lynx.tasm.event.LynxEvent;
import com.lynx.tasm.event.LynxEventDetail;
import com.lynx.tasm.event.LynxEventDetail.EVENT_TYPE;
import com.lynx.tasm.event.LynxInternalEvent;
import com.lynx.tasm.event.LynxTouchEvent;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.util.ArrayList;

public abstract class EventEmitter {
  private static final String TAG = "EventEmitter";

  public interface ITestTapTrack {
    void onTap();
  }

  public interface LynxEventObserver {
    void onLynxEvent(LynxEventType type, LynxEvent event);
  }

  public interface LynxEventReporter {
    boolean onLynxEvent(LynxEvent event);
    void onInternalEvent(@NonNull LynxInternalEvent event);
  }

  public enum LynxEventType {
    kLynxEventTypeTouchEvent,
    kLynxEventTypeCustomEvent,
    kLynxEventTypeLayoutEvent
  }

  public EventEmitter() {}

  public abstract void sendTouchEvent(LynxTouchEvent event);

  public abstract boolean onLynxEvent(LynxEvent event);

  public abstract void sendMultiTouchEvent(LynxTouchEvent event);

  public abstract void sendCustomEvent(LynxCustomEvent event);

  /**
   * Sends a custom gesture event with a specific gesture ID.
   *
   * @param gestureId The identifier of the specific gesture.
   * @param event The custom event to be sent.
   */
  public abstract void sendGestureEvent(int gestureId, LynxCustomEvent event);

  public abstract void onPseudoStatusChanged(int sign, int preStatus, int currentStatus);

  public abstract void setTestTapTracker(ITestTapTrack track);

  public abstract void sendLayoutEvent();

  public abstract void setInPreLoad(boolean preload);

  public abstract void addObserver(LynxEventObserver observer);

  public abstract void removeObserver(LynxEventObserver observer);

  public abstract void registerEventReporter(LynxEventReporter reporter);

  // TODO(songshourui.null): Remove this API later. First, remove the function implementation. If
  // subsequent verification confirms that API removal does not cause a build break, then delete the
  // API.
  public void onInternalEvent(LynxInternalEvent event) {}

  // TODO(hexionghui): This interface will be deleted later. Since UITTAdImage relies on this
  // interface, it cannot be deleted temporarily. First, remove the function implementation. If
  // subsequent verification confirms that API removal does not cause a build break, then delete the
  // API.
  public void sendInternalEvent(LynxInternalEvent event) {}
}
