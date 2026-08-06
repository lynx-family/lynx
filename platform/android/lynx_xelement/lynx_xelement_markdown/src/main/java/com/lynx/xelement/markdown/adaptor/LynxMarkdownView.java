// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.markdown.adaptor;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.view.Choreographer;
import com.lynx.markdown.MarkdownMeasurer;
import com.lynx.markdown.ServalMarkdownView;
import com.lynx.tasm.behavior.ui.view.AndroidView;
import com.lynx.xelement.markdown.LynxUIMarkdownShadowNode;

public class LynxMarkdownView extends AndroidView {
  private ServalMarkdownView mMarkdownView;
  private MarkdownMeasurer mMarkdownMeasurer;
  private LynxUIMarkdownShadowNode mShadowNode;
  private MarkdownResourceLoader mResourceLoader;
  private Choreographer.FrameCallback mFrameCallback = null;
  private int mMeasuredWidth;
  private int mMeasuredHeight;
  private int mContentLeftOffset;
  private int mContentTopOffset;

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
      addView(mMarkdownView);
      setDrawableCallbackOnLayoutThread(mMarkdownView);
    }
    mMeasuredWidth = bundle.mMeasuredWidth;
    mMeasuredHeight = bundle.mMeasuredHeight;
    layoutMarkdownView();
    return mMarkdownView;
  }

  public void setContentOffset(int left, int top) {
    if (mContentLeftOffset == left && mContentTopOffset == top) {
      return;
    }
    mContentLeftOffset = left;
    mContentTopOffset = top;
    layoutMarkdownView();
  }

  private void layoutMarkdownView() {
    if (mMarkdownView == null) {
      return;
    }
    mMarkdownView.layout(mContentLeftOffset, mContentTopOffset, mContentLeftOffset + mMeasuredWidth,
        mContentTopOffset + mMeasuredHeight);
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
    }
    Choreographer.getInstance().postFrameCallback(mFrameCallback);
  }

  public void destroy() {
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
    mMeasuredWidth = 0;
    mMeasuredHeight = 0;
  }

  @Override
  public void invalidate() {
    super.invalidate();
    if (mMarkdownView != null) {
      mMarkdownView.invalidate();
    }
  }
}
