// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.transfer;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Canvas;
import android.text.TextUtils;
import android.view.MotionEvent;
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
import com.lynx.tasm.behavior.ui.view.AndroidView;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.event.LynxCustomEvent;
import com.lynx.tasm.performance.PerformanceController;
import com.lynx.tasm.utils.UnitUtils;
import java.util.HashMap;
import java.util.Map;

public class UITransfer extends UIGroup<UITransfer.TransferHostView> {
  static final String EVENT_TRANSFER_TIMING = "transfertiming";

  private TouchEventDispatcher mEventDispatcher;
  private TransferWrapperView mWrapperView;
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
  public void onNodeReady() {
    super.onNodeReady();
    mNodeReady = true;
    dispatchTransferCreateIfNeeded();
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

  private void updateHostConstraints(int widthMeasureSpec, int heightMeasureSpec) {
    MeasureMode widthMode = MeasureMode.fromInt(MeasureMode.fromMeasureSpec(widthMeasureSpec));
    MeasureMode heightMode = MeasureMode.fromInt(MeasureMode.fromMeasureSpec(heightMeasureSpec));
    float width =
        widthMode == MeasureMode.UNDEFINED ? 0.0f : View.MeasureSpec.getSize(widthMeasureSpec);
    float height =
        heightMode == MeasureMode.UNDEFINED ? 0.0f : View.MeasureSpec.getSize(heightMeasureSpec);
    if (Float.compare(mHostWidth, width) == 0 && mHostWidthMode == widthMode
        && Float.compare(mHostHeight, height) == 0 && mHostHeightMode == heightMode) {
      return;
    }
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

  @SuppressLint({"", "ViewConstructor"})
  static final public class TransferHostView extends AndroidView {
    private final TransferWrapperView mWrapperView;

    TransferHostView(Context context, TransferWrapperView wrapperView) {
      super(context);
      mWrapperView = wrapperView;
    }

    @Override
    public void addView(View child, int index, ViewGroup.LayoutParams params) {
      mWrapperView.addView(child, index, params);
    }

    @Override
    public void removeView(View view) {
      mWrapperView.removeView(view);
    }

    @Override
    public void removeViewAt(int index) {
      mWrapperView.removeViewAt(index);
    }

    @Override
    public void removeAllViews() {
      mWrapperView.removeAllViews();
      super.removeAllViews();
    }
  }

  private static final class TransferWrapperView extends AndroidView {
    private final UITransfer mTransferView;

    TransferWrapperView(Context context, UITransfer transferView) {
      super(context);
      mTransferView = transferView;
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
      TouchEventDispatcher eventDispatcher = mTransferView.getTouchEventDispatcher();
      boolean consumed = eventDispatcher.onTouchEvent(ev, mTransferView);
      if (consumed && !eventDispatcher.consumeSlideEvent(ev)) {
        super.dispatchTouchEvent(ev);
      }
      return consumed;
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
      mTransferView.updateHostConstraints(widthMeasureSpec, heightMeasureSpec);

      mTransferView.measure();
      int measuredWidth = MeasureSpec.getSize(widthMeasureSpec);
      int measuredHeight = MeasureSpec.getSize(heightMeasureSpec);
      if (MeasureSpec.getMode(widthMeasureSpec) == MeasureSpec.UNSPECIFIED) {
        measuredWidth = mTransferView.getWidth();
      }
      if (MeasureSpec.getMode(heightMeasureSpec) == MeasureSpec.UNSPECIFIED) {
        measuredHeight = mTransferView.getHeight();
      }
      setMeasuredDimension(measuredWidth, measuredHeight);
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
      super.onLayout(changed, l, t, r, b);
      mTransferView.layout();
    }

    @Override
    protected void dispatchDraw(Canvas canvas) {
      super.dispatchDraw(canvas);
      mTransferView.sendTransferDrawEndEvent();
    }
  }
}
