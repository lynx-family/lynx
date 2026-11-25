// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.transfer;

import android.graphics.Canvas;
import android.view.View;
import android.widget.FrameLayout;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.UiThread;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.trace.TraceEventDef;
import com.lynx.tasm.base.trace.TransferTrace;
import com.lynx.tasm.utils.UIThreadUtils;

/**
 * Experimental container for one component rendered by a shared {@link LynxView}.
 *
 * <p>Obtain instances from {@link LynxView#loadTransfer}. Removing this view from its parent
 * preserves component state. Call {@link #release()} when the component is no longer needed.
 * All instance operations must run on the UI thread.
 */
public final class LynxTransferredView extends FrameLayout {
  @Nullable private LynxView mOwner;
  private final String mTransferId;
  private final int mInstanceId;
  @Nullable private LynxTransferClient mClient;
  @Nullable private ReadableMap mPendingProps;
  private boolean mCreateDispatched;
  private boolean mLoaded;
  private boolean mFirstDrawn;

  LynxTransferredView(LynxView owner, String transferId, @Nullable LynxTransferClient client) {
    super(owner.getContext());
    mOwner = owner;
    mTransferId = transferId;
    mInstanceId = owner.getLynxContext().getInstanceId();
    mClient = client;
  }

  void notifyLoadStart() {
    if (mClient != null) {
      dispatchClientCallback(() -> mClient.onLoadStart(this));
    }
  }

  boolean isReleased() {
    return mOwner == null;
  }

  JavaOnlyMap prepareCreate(String componentName, ReadableMap initialProps) {
    JavaOnlyMap data = createEventData();
    data.putString("componentName", componentName);
    data.put("initialProps", mPendingProps != null ? mPendingProps : initialProps);
    mPendingProps = null;
    mCreateDispatched = true;
    return data;
  }

  JavaOnlyMap createEventData() {
    JavaOnlyMap data = new JavaOnlyMap();
    data.putString("transferId", mTransferId);
    return data;
  }

  /**
   * Replaces this component's props while keeping the frontend instance state. Unlike
   * {@link #reload}, the frontend updates the existing subtree in place.
   *
   * <p>Returning does not mean that rendering has completed. Page data and global props are not
   * changed. Updates after release or page destruction are logged and ignored.
   */
  @UiThread
  public void updateProps(@NonNull ReadableMap nextProps) {
    dispatchPropsCommand(LynxTransferManager.UPDATE_TRANSFER, TraceEventDef.TRANSFER_UPDATE,
        "updateProps", nextProps);
  }

  /**
   * Replaces this component's props and asks the frontend to discard its instance state before
   * rendering again. The managed container, its {@code transferId}, and its attached native content
   * are unchanged, so no create or destroy command is sent and the load callbacks are not repeated.
   *
   * <p>Returning does not mean that rendering has completed. Page data and global props are not
   * changed. Updates after release or page destruction are logged and ignored.
   */
  @UiThread
  public void reload(@NonNull ReadableMap nextProps) {
    dispatchPropsCommand(
        LynxTransferManager.RELOAD_TRANSFER, TraceEventDef.TRANSFER_RELOAD, "reload", nextProps);
  }

  private void dispatchPropsCommand(
      String command, String traceStage, String api, @NonNull ReadableMap nextProps) {
    UIThreadUtils.assertOnUiThread();
    if (mOwner == null) {
      LLog.w("LynxTransferredView", "Ignoring " + api + " for released transfer " + mTransferId);
      return;
    }
    if (!mCreateDispatched) {
      // onLoadStart is synchronous: fold its updates into the initial create command.
      mPendingProps = nextProps;
      return;
    }
    TransferTrace.begin(traceStage, mInstanceId, mTransferId, -1);
    JavaOnlyMap data = createEventData();
    data.put("props", nextProps);
    mOwner.dispatchTransferEvent(command, data);
    TransferTrace.end(traceStage);
  }

  /** Releases the component and its content. Repeated calls have no effect. */
  @UiThread
  public void release() {
    UIThreadUtils.assertOnUiThread();
    LynxView owner = mOwner;
    if (owner == null) {
      return;
    }
    owner.releaseTransferredView(mTransferId);
    try {
      dispose(TransferReleaseReason.EXPLICIT_RELEASE);
    } finally {
      if (mCreateDispatched) {
        owner.dispatchTransferEvent(LynxTransferManager.DESTROY_TRANSFER, createEventData());
      }
    }
  }

  void attachContent(View content) {
    if (mOwner == null) {
      return;
    }
    removeAllViews();
    addView(content,
        new FrameLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));
    if (!mLoaded) {
      mLoaded = true;
      if (mClient != null) {
        dispatchClientCallback(() -> mClient.onLoadSuccess(this));
      }
    }
  }

  void removeContent(View content) {
    removeView(content);
  }

  @Override
  protected void dispatchDraw(Canvas canvas) {
    boolean drawsContent = mOwner != null && mLoaded && getChildCount() > 0 && isAttachedToWindow()
        && getWidth() > 0 && getHeight() > 0;
    super.dispatchDraw(canvas);
    if (drawsContent && mOwner != null && !mFirstDrawn) {
      mFirstDrawn = true;
      if (mClient != null) {
        dispatchClientCallback(() -> mClient.onFirstScreen(this));
      }
    }
  }

  void dispose(TransferReleaseReason reason) {
    if (mOwner == null) {
      return;
    }
    TransferTrace.begin(TraceEventDef.TRANSFER_DISPOSE, mInstanceId, mTransferId, -1);
    mOwner = null;
    mPendingProps = null;
    LynxTransferClient client = mClient;
    mClient = null;
    removeAllViews();
    if (client != null) {
      dispatchClientCallback(() -> client.onReleased(this, reason));
    }
    TransferTrace.end(TraceEventDef.TRANSFER_DISPOSE);
  }

  private void dispatchClientCallback(Runnable callback) {
    try {
      callback.run();
    } catch (RuntimeException exception) {
      LLog.e("LynxTransferredView", "Transfer lifecycle callback failed: " + exception);
    }
  }
}
