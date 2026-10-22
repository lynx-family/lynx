// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.markdown.adaptor;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.view.Choreographer;
import android.view.View;
import com.lynx.markdown.CustomDrawView;
import com.lynx.markdown.MarkdownMeasurer;
import com.lynx.markdown.MarkdownValuePack;
import com.lynx.markdown.ServalMarkdownView;
import com.lynx.tasm.behavior.ui.view.AndroidView;
import com.lynx.tasm.utils.UIThreadUtils;
import com.lynx.xelement.markdown.LynxUIMarkdownShadowNode;

public class LynxMarkdownView extends AndroidView {
  private ServalMarkdownView mMarkdownView;
  private MarkdownMeasurer mMarkdownMeasurer;
  private LynxUIMarkdownShadowNode mShadowNode;
  private MarkdownResourceLoader mResourceLoader;
  private Choreographer.FrameCallback mFrameCallback = null;
  private long mMeasuredSize;
  private int mContentLeftOffset;
  private int mContentTopOffset;
  private Runnable mExposureUpdater;
  private boolean mExposureUpdatePending;

  public void setExposureUpdater(Runnable updater) {
    mExposureUpdater = updater;
  }

  public LynxMarkdownView(Context context) {
    super(context);
    setWillNotDraw(true);
  }

  public ServalMarkdownView setBundle(LynxMarkdownBundle bundle) {
    if (bundle == null || bundle.mMarkdownMeasurer == null) {
      return mMarkdownView;
    }
    if (mFrameCallback == null) {
      mFrameCallback = this::onVSync;
      Choreographer.getInstance().postFrameCallback(mFrameCallback);
    }
    boolean viewChanged = bundle.mMarkdownMeasurer != mMarkdownMeasurer;
    if (viewChanged) {
      if (mMarkdownView != null) {
        removeView(mMarkdownView);
        mMarkdownView.destroy();
      }
      setDrawableCallbackOnLayoutThread(null);
      mMarkdownMeasurer = bundle.mMarkdownMeasurer;
      mShadowNode = bundle.mShadowNode;
      mResourceLoader = bundle.mResourceLoader;
      mMarkdownView = new ServalMarkdownView(getContext(), false);
      mMarkdownView.setMarkdownMeasurer(mMarkdownMeasurer);
      mMarkdownView.disableInternalVSync(true);
      mMarkdownView.setExternalMeasure(true);
      addView(mMarkdownView);
      setDrawableCallbackOnLayoutThread(this);
    }
    mExposureUpdatePending = true;
    mMeasuredSize = bundle.mMeasuredSize;
    mMarkdownView.setExternalMeasuredSize(mMeasuredSize);
    layoutMarkdownView();
    return mMarkdownView;
  }

  public void setContentOffset(int left, int top) {
    if (mContentLeftOffset == left && mContentTopOffset == top) {
      return;
    }
    mContentLeftOffset = left;
    mContentTopOffset = top;
    mExposureUpdatePending = true;
    layoutMarkdownView();
  }

  private void layoutMarkdownView() {
    if (mMarkdownView == null) {
      return;
    }
    int width = MarkdownValuePack.unpackMeasureResultWidth(mMeasuredSize);
    int height = MarkdownValuePack.unpackMeasureResultHeight(mMeasuredSize);
    mMarkdownView.measure(View.MeasureSpec.makeMeasureSpec(width, View.MeasureSpec.EXACTLY),
        View.MeasureSpec.makeMeasureSpec(height, View.MeasureSpec.EXACTLY));
    mMarkdownView.layout(mContentLeftOffset, mContentTopOffset, mContentLeftOffset + width,
        mContentTopOffset + height);
  }

  private void setDrawableCallbackOnLayoutThread(Drawable.Callback callback) {
    LynxUIMarkdownShadowNode shadowNode = mShadowNode;
    MarkdownResourceLoader resourceLoader = mResourceLoader;
    if (shadowNode != null && resourceLoader != null) {
      shadowNode.runOnLayoutThread(() -> resourceLoader.setDrawableCallback(callback));
    }
  }

  @Override
  protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
    super.onLayout(changed, left, top, right, bottom);
    layoutMarkdownView();
  }

  private void onVSync(long time) {
    if (mFrameCallback == null) {
      return;
    }
    if (mMarkdownView != null) {
      mMarkdownView.onRendererFrame(time);
      if (mExposureUpdatePending && mExposureUpdater != null) {
        mExposureUpdatePending = false;
        mExposureUpdater.run();
      }
    }
    Choreographer.getInstance().postFrameCallback(mFrameCallback);
  }

  public void destroy() {
    mExposureUpdater = null;
    if (mFrameCallback != null) {
      Choreographer.getInstance().removeFrameCallback(mFrameCallback);
    }
    mFrameCallback = null;
    setDrawableCallbackOnLayoutThread(null);
    mResourceLoader = null;
    mShadowNode = null;
    if (mMarkdownView != null) {
      mMarkdownView.destroy();
      mMarkdownView = null;
    }
    mMarkdownMeasurer = null;
    mMeasuredSize = 0;
  }

  @Override
  public void invalidate() {
    super.invalidate();
    if (mMarkdownView != null) {
      mMarkdownView.invalidate();
    }
  }

  @Override
  public void invalidateDrawable(Drawable who) {
    UIThreadUtils.runOnUiThreadImmediately(() -> {
      if (mMarkdownView != null) {
        for (int i = 0; i < mMarkdownView.getChildCount(); i++) {
          View child = mMarkdownView.getChildAt(i);
          if (child instanceof CustomDrawView) {
            child.invalidate();
          }
        }
      }
    });
  }

  @Override
  public void scheduleDrawable(Drawable who, Runnable what, long when) {
    UIThreadUtils.runOnUiThreadAtTime(what, who, when);
  }

  @Override
  public void unscheduleDrawable(Drawable who, Runnable what) {
    UIThreadUtils.removeCallbacks(what, who);
  }
}
