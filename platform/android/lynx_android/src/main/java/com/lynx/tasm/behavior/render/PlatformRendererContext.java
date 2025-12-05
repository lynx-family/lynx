// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import android.graphics.PointF;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.mapbuffer.ReadableCompactArrayBuffer;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.shadow.TextLayout;
import com.lynx.tasm.behavior.shadow.TextMeasurerProvider;
import com.lynx.tasm.behavior.shadow.text.TextMeasurer;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.image.LynxImageManager;
import com.lynx.tasm.behavior.ui.scroll.AndroidScrollView;
import com.lynx.tasm.behavior.ui.scroll.UIScrollView;
import com.lynx.tasm.behavior.ui.utils.LynxUIHelper;
import com.lynx.tasm.behavior.ui.view.AndroidView;
import java.lang.ref.WeakReference;
import java.util.HashMap;

public class PlatformRendererContext implements TextMeasurerProvider {
  private static String TAG = "PlatformRendererContext";

  public static final class PlatformRendererType {
    public static final int kUnknown = 0;
    public static final int kView = 1;
    public static final int kPage = 2;
    public static final int kScroll = 3;
    public static final int kText = 4;
    public static final int kImage = 5;
    public static final int kList = 6;
  }

  WeakReference<UIBody.UIBodyView> mRootView = null;

  HashMap<Integer, ViewGroup> mViewHolder = new HashMap<>();

  private LynxContext mContext = null;
  private long mNativePtr = 0;
  private TextLayout mTextLayout;
  private boolean mDestroyed = false;

  // TextMeasurer instance for text measurement functionality
  private TextMeasurer mTextMeasurer = null;

  public TextMeasurer getTextMeasurer() {
    return mTextMeasurer;
  }

  public PlatformRendererContext(@Nullable UIBody.UIBodyView rootView, LynxContext context) {
    if (rootView != null) {
      this.mRootView = new WeakReference<>(rootView);
    }
    this.mContext = context;
    this.mNativePtr = nativeCreateEmbeddedViewContext(this);

    // Initialize TextMeasurer if layout mode is enabled
    if (context != null && context.isLayoutInElementModeOn()) {
      this.mTextMeasurer = new TextMeasurer(context);
      mTextLayout = new TextLayout(this);
    }
  }

  public void setRootView(@NonNull UIBody.UIBodyView rootView) {
    this.mRootView = new WeakReference<>(rootView);
  }

  public long getNativePtr() {
    return mNativePtr;
  }

  PointF convertPointInViewToScreen(int sign, PointF point) {
    ViewGroup view = mViewHolder.get(sign);
    if (view == null) {
      LLog.e(TAG, "convertPointInViewToScreen failed since can not find target view.");
    }
    return LynxUIHelper.convertPointInViewToScreen(view, point);
  }

  public int getTargetWidth(int sign) {
    View view = mViewHolder.get(sign);
    if (view == null) {
      LLog.e(TAG, "getTargetWidth failed since can not find target view.");
      return 0;
    }

    return view.getWidth();
  }

  public int getTargetHeight(int sign) {
    View view = mViewHolder.get(sign);
    if (view == null) {
      LLog.e(TAG, "getTargetHeight failed since can not find target view.");
      return 0;
    }

    return view.getHeight();
  }

  @CalledByNative
  public void createPlatformRenderer(int sign, int type) {
    switch (type) {
      case PlatformRendererType.kView:
      case PlatformRendererType.kText:
      case PlatformRendererType.kImage: {
        ContainerRenderer view = new ContainerRenderer(mContext, this, sign);
        mViewHolder.put(sign, view);
        view.invalidate();
        break;
      }
      case PlatformRendererType.kPage: {
        UIBody.UIBodyView view = mRootView.get();
        if (view != null) {
          view.mSign = sign;
          view.setPlatformRendererContext(this);
          view.setWillNotDraw(false);

          view.invalidate();
          mViewHolder.put(sign, view);
        }
        break;
      }
      case PlatformRendererType.kScroll: {
        AndroidScrollView scrollView = new AndroidScrollView(
            mContext, /*TODO: decoupling from UIScrollView*/ new UIScrollView(mContext));
        mViewHolder.put(sign, scrollView);
      } break;
      case PlatformRendererType.kList: {
        // TODO: support <list/> platform view here.
      } break;
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
  public void updatePlatformRendererFrame(int sign, int left, int top, int width, int height) {
    ViewGroup view = mViewHolder.get(sign);
    if (view instanceof UIBody.UIBodyView) {
      ((UIBody.UIBodyView) view).setLynxFrame(left, top, left + width, top + height);
      view.requestLayout();
    } else if (view instanceof ContainerRenderer) {
      ((ContainerRenderer) view).setLynxFrame(left, top, left + width, top + height);
      view.requestLayout();
    }
  }

  public LynxImageManager getImage(int sign) {
    UIBody.UIBodyView rootView = mRootView.get();
    if (rootView != null) {
      return rootView.peekImageAccordingToNodeIndex(sign);
    }
    return null;
  }

  @CalledByNative
  public void createImage(int sign, String src, int width, int height) {
    // Create Image managed by LynxImageManager and register to UIBodyView
    LynxImageManager imageManager = new LynxImageManager(mContext);
    imageManager.setSrc(src);
    imageManager.onLayoutUpdated(width, height, 0, 0, 0, 0);
    UIBody.UIBodyView rootView = mRootView.get();
    if (rootView != null) {
      rootView.registerImageAccordingToNodeIndex(sign, imageManager);
    }
    imageManager.onNodeReady();
  }

  @CalledByNative
  public void destroyImage(int sign) {
    // Remove and release the image source from UIBodyView
    UIBody.UIBodyView rootView = mRootView.get();
    if (rootView != null) {
      rootView.obtainImageAccordingToNodeIndex(sign);
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

  public void getDisplayList(int id, DisplayList displayList) {
    if (displayList == null || mDestroyed || mNativePtr == 0) {
      return;
    }

    // Get the display list lengths first
    int[] lengths = nativeGetDisplayListLengths(mNativePtr, id);
    if (lengths == null || lengths.length != 3) {
      return;
    }

    int opsLength = lengths[0];
    int iArgvLength = lengths[1];
    int fArgvLength = lengths[2];

    // Allocate arrays
    displayList.ops = new int[opsLength];
    displayList.iArgv = new int[iArgvLength];
    displayList.fArgv = new float[fArgvLength];

    // Fill the arrays with actual data
    nativeGetDisplayListData(mNativePtr, id, displayList.ops, displayList.iArgv, displayList.fArgv);
  }

  public TextLayout getTextLayout() {
    return mTextLayout;
  }

  /**
   * Implements TextMeasurerProvider.measureText to delegate to the TextMeasurer instance.
   * This allows PlatformRendererContext to provide text measurement functionality directly.
   */
  @Override
  public float[] measureText(int sign, float width, int widthMode, float height, int heightMode,
      float[] inlineViewLayoutResult) {
    if (mTextMeasurer != null) {
      return mTextMeasurer.measureText(
          sign, width, widthMode, height, heightMode, inlineViewLayoutResult);
    }
    // Return default measurement if TextMeasurer is not available
    return new float[] {0.0f, 0.0f};
  }

  /**
   * Implements TextMeasurerProvider.dispatchLayoutBefore to delegate to the TextMeasurer instance.
   * This allows PlatformRendererContext to handle layout dispatch functionality directly.
   */
  @Override
  public void dispatchLayoutBefore(int sign, ReadableCompactArrayBuffer buffer) {
    if (mTextMeasurer != null) {
      mTextMeasurer.dispatchLayoutBefore(sign, buffer);
    }
  }

  @Override
  public float[] align(int sign) {
    if (mTextMeasurer != null) {
      return mTextMeasurer.align(sign);
    }
    return new float[0];
  }

  native long nativeCreateEmbeddedViewContext(PlatformRendererContext jThis);

  native int[] nativeGetDisplayListLengths(long nativePtr, int id);

  /**
   * Fills the provided arrays with display list data.
   * The arrays must be pre-allocated with lengths obtained from nativeGetDisplayListLengths().
   */
  native void nativeGetDisplayListData(
      long nativePtr, int id, int[] ops, int[] iArgv, float[] fArgv);

  public void destroy() {
    mDestroyed = true;
    mNativePtr = 0; // Clear native pointer to indicate context is destroyed
    mViewHolder.clear();
    mTextMeasurer = null;
    mTextLayout = null;
    UIBody.UIBodyView root = mRootView.get();
    if (root != null) {
      root.clearNodeIndexImageMap();
    }
  }
}
