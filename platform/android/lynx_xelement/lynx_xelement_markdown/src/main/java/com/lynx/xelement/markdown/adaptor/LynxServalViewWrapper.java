// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.markdown.adaptor;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.os.Looper;
import androidx.annotation.NonNull;
import com.lynx.markdown.ServalMarkdownView;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import java.util.HashMap;
import java.util.Map;

public class LynxServalViewWrapper extends ServalMarkdownView {
  protected final ShadowNode mShadowNode;
  private final Handler mHandler = new Handler(Looper.getMainLooper());
  private final Map<Drawable, String> mAnimatedDrawables = new HashMap<>();

  public LynxServalViewWrapper(Context context, ShadowNode shadowNode) {
    super(context);
    mShadowNode = shadowNode;
  }

  public void registerAnimatedDrawable(Drawable drawable, String url) {
    mAnimatedDrawables.put(drawable, url);
  }

  public void unregisterAnimatedDrawable(Drawable drawable) {
    mAnimatedDrawables.remove(drawable);
  }

  @Override
  public void invalidateDrawable(@NonNull Drawable who) {
    String url = mAnimatedDrawables.get(who);
    if (url != null) {
      onImageLoaded(url);
    }
  }

  @Override
  public void scheduleDrawable(@NonNull Drawable who, @NonNull Runnable what, long when) {
    long delay = Math.max(0, when - android.os.SystemClock.uptimeMillis());
    mHandler.postDelayed(what, delay);
  }

  @Override
  public void unscheduleDrawable(@NonNull Drawable who, @NonNull Runnable what) {
    mHandler.removeCallbacks(what);
  }

  @Override
  public void requestMeasure() {
    super.requestMeasure();
    if (mShadowNode == null) {
      return;
    }
    mShadowNode.markDirty();
  }

  @Override
  public void requestAlign() {
    super.requestAlign();
    if (mShadowNode == null) {
      return;
    }
    mShadowNode.markDirty();
  }
}
