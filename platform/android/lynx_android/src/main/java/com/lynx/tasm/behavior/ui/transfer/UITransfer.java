// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.transfer;

import android.content.Context;
import android.text.TextUtils;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import com.lynx.tasm.EventEmitter;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.TouchEventDispatcher;
import com.lynx.tasm.behavior.shadow.MeasureMode;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.behavior.ui.UIGroup;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.event.LynxCustomEvent;
import com.lynx.tasm.performance.PerformanceController;
import com.lynx.tasm.utils.UnitUtils;
import java.util.HashMap;
import java.util.Map;

public class UITransfer extends UIGroup<TransferHostView> {
  static final String EVENT_TRANSFER_TIMING = "transfertiming";

  private TouchEventDispatcher mEventDispatcher;
  private TransferWrapperView mWrapperView;
  private boolean mHasHostConstraints;
  private String mTransferId;
  private String mAttachedTransferId;
  private boolean mNodeReady;
  private float mHostWidth;
  private MeasureMode mHostWidthMode = MeasureMode.UNDEFINED;
  private float mHostHeight;
  private MeasureMode mHostHeightMode = MeasureMode.UNDEFINED;
  private boolean firstDrawEndSent = false;

  public UITransfer(LynxContext context) {
    this(context, null);
  }

  public UITransfer(LynxContext context, Object params) {
    super(context, params);
  }

  @Override
  protected TransferHostView createView(Context context) {
    mWrapperView = new TransferWrapperView(context, this);
    getTouchEventDispatcher().setTouchEventSource(mWrapperView);
    return new TransferHostView(context, mWrapperView);
  }

  private void syncEventDispatcherConfig(@NonNull TouchEventDispatcher dispatcher) {
    if (mWrapperView != null) {
      dispatcher.setTouchEventSource(mWrapperView);
    }
    dispatcher.setHasTouchPseudo(mContext.getLynxUIOwner().getHasTouchPseudo());
    dispatcher.setHasTouchPseudo(mContext.getEnableFiberArch());
    dispatcher.setEnableMultiTouch(mContext.getEnableMultiTouch());
    dispatcher.setEnablePlatformGesture(mContext.isEnablePlatformGesture());

    String tapSlop = mContext.getTapSlop();
    if (tapSlop != null && !tapSlop.equals(TouchEventDispatcher.mTapSlopDefault)) {
      dispatcher.setTapSlop(
          UnitUtils.toPxWithDisplayMetrics(tapSlop, 0, 0, 0, 0, 0, 0, mContext.getScreenMetrics()));
    }
  }

  @Override
  public TouchEventDispatcher getTouchEventDispatcher() {
    if (mEventDispatcher == null) {
      mEventDispatcher = new TouchEventDispatcher(mContext.getLynxUIOwner());
    }
    syncEventDispatcherConfig(mEventDispatcher);
    return mEventDispatcher;
  }

  @LynxProp(name = "transfer-id")
  public void setTransferId(String transferId) {
    if (TextUtils.equals(mTransferId, transferId)) {
      return;
    }
    String previousTransferId = mTransferId;
    mTransferId = transferId;
    if (!mNodeReady) {
      return;
    }
    if (!TextUtils.isEmpty(previousTransferId)) {
      removeTransfer(previousTransferId);
    }
    dispatchTransferCreateIfNeeded();
  }

  double nowMs() {
    return ((double) PerformanceController.currentSystemTimeMicroseconds()) / 1000;
  }

  void sendTransferDrawEndEvent() {
    if (firstDrawEndSent) {
      return;
    }
    firstDrawEndSent = true;
    Map<String, EventsListener> events = mEvents;
    LynxContext lynxContext = getLynxContext();
    EventEmitter eventEmitter = lynxContext != null ? lynxContext.getEventEmitter() : null;

    if (events != null && events.containsKey(EVENT_TRANSFER_TIMING) && eventEmitter != null) {
      Map<String, Object> timing = new HashMap<>();
      timing.put("draw_end", nowMs());

      Map<String, Object> data = new HashMap<>();
      data.put("timing", timing);
      data.put("transfer_id", mTransferId);

      eventEmitter.sendCustomEvent(new LynxCustomEvent(getSign(), EVENT_TRANSFER_TIMING, data));
    }
  }

  @Override
  public boolean canHaveFlattenChild() {
    return false;
  }

  @Override
  public boolean isExternalExposureRoot() {
    return true;
  }

  @Override
  protected ViewGroup getChildViewGroup() {
    return mWrapperView;
  }

  @Override
  public void onNodeReady() {
    super.onNodeReady();
    mNodeReady = true;
    dispatchTransferCreateIfNeeded();
  }

  @Override
  public void onPropsUpdated() {
    super.onPropsUpdated();
    LynxView lynxView = mContext.getLynxView();
    if (lynxView != null && !TextUtils.isEmpty(mAttachedTransferId) && mWrapperView != null) {
      lynxView.dispatchTransferDatasetUpdate(mWrapperView, getDataset());
    }
  }

  @Override
  public void onNodeRemoved() {
    super.onNodeRemoved();
    cleanupTransfer();
  }

  @Override
  public void destroy() {
    cleanupTransfer();
    super.destroy();
  }

  private void cleanupTransfer() {
    if (!TextUtils.isEmpty(mAttachedTransferId)) {
      removeTransfer(mAttachedTransferId);
    }
    mAttachedTransferId = null;
  }

  private void removeTransfer(String transferId) {
    LynxView lynxView = mContext.getLynxView();
    if (lynxView != null && mWrapperView != null) {
      lynxView.dispatchTransferRemove(transferId, mWrapperView);
    } else if (mWrapperView != null) {
      removeFromParent(mWrapperView);
    }
    mAttachedTransferId = null;
  }

  private void dispatchTransferCreateIfNeeded() {
    if (!mNodeReady || TextUtils.isEmpty(mTransferId) || mWrapperView == null
        || TextUtils.equals(mAttachedTransferId, mTransferId)) {
      return;
    }
    LynxView lynxView = mContext.getLynxView();
    if (lynxView == null) {
      mAttachedTransferId = mTransferId;
      return;
    }
    firstDrawEndSent = false;
    lynxView.dispatchTransferCreate(mTransferId, this, mWrapperView);
    mAttachedTransferId = mTransferId;
  }

  private static void removeFromParent(@NonNull View view) {
    if (view.getParent() instanceof ViewGroup) {
      ((ViewGroup) view.getParent()).removeView(view);
    }
  }

  void updateHostConstraints(int widthMeasureSpec, int heightMeasureSpec) {
    MeasureMode widthMode = MeasureMode.fromInt(MeasureMode.fromMeasureSpec(widthMeasureSpec));
    MeasureMode heightMode = MeasureMode.fromInt(MeasureMode.fromMeasureSpec(heightMeasureSpec));
    float width =
        widthMode == MeasureMode.UNDEFINED ? 0.0f : View.MeasureSpec.getSize(widthMeasureSpec);
    float height =
        heightMode == MeasureMode.UNDEFINED ? 0.0f : View.MeasureSpec.getSize(heightMeasureSpec);
    if (mHasHostConstraints && Float.compare(mHostWidth, width) == 0 && mHostWidthMode == widthMode
        && Float.compare(mHostHeight, height) == 0 && mHostHeightMode == heightMode) {
      return;
    }
    mHasHostConstraints = true;
    mHostWidth = width;
    mHostWidthMode = widthMode;
    mHostHeight = height;
    mHostHeightMode = heightMode;
    mContext.findShadowNodeAndRunTask(getSign(), (ShadowNode node) -> {
      if (node instanceof TransferShadowNode) {
        ((TransferShadowNode) node)
            .updateHostConstraints(mHostWidth, mHostWidthMode, mHostHeight, mHostHeightMode);
      }
    });
  }
}
