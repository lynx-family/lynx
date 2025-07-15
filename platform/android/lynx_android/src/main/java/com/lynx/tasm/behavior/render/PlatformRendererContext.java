// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import android.view.ViewGroup;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.scroll.AndroidScrollView;
import com.lynx.tasm.behavior.ui.scroll.UIScrollView;
import com.lynx.tasm.behavior.ui.view.AndroidView;
import java.lang.ref.WeakReference;
import java.util.HashMap;

public class PlatformRendererContext {
  public static final class PlatformRendererType {
    public static final int kUnknown = 0;
    public static final int kView = 1;
    public static final int kPage = 2;
    public static final int kScroll = 3;
  }

  WeakReference<UIBody.UIBodyView> mRootView = null;

  HashMap<Integer, ViewGroup> mViewHolder = new HashMap();

  private LynxContext mContext = null;
  private long mNativePtr = 0;

  public PlatformRendererContext(@Nullable UIBody.UIBodyView rootView, LynxContext context) {
    if (rootView != null) {
      this.mRootView = new WeakReference<>(rootView);
    }
    this.mContext = context;
    this.mNativePtr = nativeCreateEmbeddedViewContext(this);
  }

  public void setRootView(@NonNull UIBody.UIBodyView rootView) {
    this.mRootView = new WeakReference<>(rootView);
  }

  public long getNativePtr() {
    return mNativePtr;
  }

  @CalledByNative
  public void createPlatformRenderer(int sign, int type) {
    switch (type) {
      case PlatformRendererType.kView: {
        AndroidView view = new AndroidView(mContext);
        mViewHolder.put(sign, view);
        break;
      }
      case PlatformRendererType.kPage: {
        UIBody.UIBodyView view = mRootView.get();
        if (view != null) {
          view.mSign = sign;
          mViewHolder.put(sign, view);
        }
        break;
      }
      case PlatformRendererType.kScroll: {
        AndroidScrollView scrollView = new AndroidScrollView(
            mContext, /*TODO: decoupling from UIScrollView*/ new UIScrollView(mContext));
        mViewHolder.put(sign, scrollView);
      }
      default:
        // TODO: support customized PlatformRendererHostView.
        break;
    }
  }

  @CalledByNative
  public void destroyPlatformRenderer(int sign) {
    mViewHolder.remove(sign);
  }

  @CalledByNative
  public void insertPlatformRenderer(int parent, int child, int index) {
    ViewGroup parentView = mViewHolder.get(parent);
    ViewGroup childView = mViewHolder.get(child);
    if (parentView == null || childView == null) {
      return;
    }
    int count = parentView.getChildCount();
    if (index == -1 || index >= count) {
      parentView.addView(childView);
    } else {
      parentView.addView(childView, index);
    }
  }

  @CalledByNative
  public void invalidatePlatformRenderer(int sign) {
    ViewGroup view = mViewHolder.get(sign);
    if (view != null) {
      view.invalidate();
    }
  }

  @CalledByNative
  public void removePlatformRendererFromParent(int sign) {
    ViewGroup view = mViewHolder.get(sign);
    if (view != null) {
      ViewGroup parent = (ViewGroup) view.getParent();
      if (parent != null) {
        parent.removeView(view);
      }
    }
  }

  public void getDrawingList(int id, DisplayList drawingList) {
    // TODO: Implement it
  }
  native long nativeCreateEmbeddedViewContext(PlatformRendererContext jThis);
}
