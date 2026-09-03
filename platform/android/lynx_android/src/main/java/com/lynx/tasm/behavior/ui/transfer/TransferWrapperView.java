// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.transfer;

import android.content.Context;
import android.view.ViewTreeObserver;
import com.lynx.tasm.behavior.ui.UIExposure;
import com.lynx.tasm.behavior.ui.view.AndroidView;

final class TransferWrapperView extends AndroidView {
  private final UITransfer mTransferView;
  private final ViewTreeObserver.OnGlobalLayoutListener mGlobalLayoutListener;
  private final ViewTreeObserver.OnScrollChangedListener mScrollChangedListener;
  private final ViewTreeObserver.OnDrawListener mDrawListener;
  private ViewTreeObserver mExposureObserver;

  TransferWrapperView(Context context, UITransfer transferView) {
    super(context);
    mTransferView = transferView;
    mGlobalLayoutListener = this::requestExposureCheck;
    mScrollChangedListener = this::requestExposureCheck;
    mDrawListener = this::requestExposureCheck;
  }

  @Override
  protected void onAttachedToWindow() {
    super.onAttachedToWindow();
    registerExposureObservers();
    requestExposureCheck();
  }

  @Override
  protected void onDetachedFromWindow() {
    unregisterExposureObservers();
    super.onDetachedFromWindow();
    requestExposureCheck();
  }

  private void registerExposureObservers() {
    ViewTreeObserver observer = getViewTreeObserver();
    if (!observer.isAlive() || observer == mExposureObserver) {
      return;
    }
    unregisterExposureObservers();
    mExposureObserver = observer;
    observer.addOnGlobalLayoutListener(mGlobalLayoutListener);
    observer.addOnScrollChangedListener(mScrollChangedListener);
    observer.addOnDrawListener(mDrawListener);
  }

  private void unregisterExposureObservers() {
    if (mExposureObserver == null) {
      return;
    }
    if (mExposureObserver.isAlive()) {
      mExposureObserver.removeOnGlobalLayoutListener(mGlobalLayoutListener);
      mExposureObserver.removeOnScrollChangedListener(mScrollChangedListener);
      mExposureObserver.removeOnDrawListener(mDrawListener);
    }
    mExposureObserver = null;
  }

  private void requestExposureCheck() {
    UIExposure exposure = mTransferView.getLynxContext().getExposure();
    if (exposure != null) {
      exposure.requestCheckUI();
    }
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
}
