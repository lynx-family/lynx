// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.frame;

import android.content.Context;
import android.graphics.Canvas;
import android.util.AttributeSet;
import androidx.annotation.RestrictTo;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.tasm.EmbeddedMode;
import com.lynx.tasm.LynxLoadMeta;
import com.lynx.tasm.LynxTemplateRender;
import com.lynx.tasm.LynxUpdateMeta;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.LynxViewBuilder;
import com.lynx.tasm.TemplateBundle;
import com.lynx.tasm.TemplateData;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.UIBody.UIBodyView;
import java.lang.ref.WeakReference;
import java.util.HashMap;

@RestrictTo(RestrictTo.Scope.LIBRARY)
public final class LynxFrameView extends UIBodyView {
  private static final String TAG = "LynxFrameView";
  private LynxTemplateRender mRender;
  private String mUrl;
  private WeakReference<LynxView> mRootView = null;
  private int mSign;
  private LynxContext mContext;
  private boolean mIsBundleLoaded = false;
  private boolean mIsIntrinsicSizeConsumed = true;
  private int mContentWidth = -1;
  private int mContentHeight = -1;
  private boolean mDestroyed = false;
  // Keep frame data/global-props as raw native handles until they are actually consumed by
  // loadBundle/onPropsUpdated. This avoids creating duplicate Java TemplateData wrappers for the
  // same native holder during async frame creation, while still letting FrameView recycle pending
  // handles if they are replaced or the view is destroyed before consumption.
  private long mInitDataPtr = 0;
  private long mGlobalPropsPtr = 0;
  private boolean mAutoWidth = false;
  private boolean mAutoHeight = false;
  private int mWidthMode = MeasureSpec.EXACTLY;
  private int mHeightMode = MeasureSpec.EXACTLY;
  private int mEmbeddedMode = EmbeddedMode.UNSET;
  private UIBodyView.attachLynxPageUICallback mAttachLynxPageUICallback;
  private int mPresetWidth = -1;
  private int mPresetHeight = -1;
  private Boolean mEnableMultiAsyncThread = null;

  public LynxFrameView(Context context) {
    super(context);
  }

  public LynxFrameView(Context context, AttributeSet attrs) {
    super(context, attrs);
  }

  void init(LynxContext context) {
    mContext = context;
    UIBodyView bodyView = mContext.getUIBodyView();
    if (bodyView != null) {
      if (bodyView instanceof LynxView) {
        mRootView = new WeakReference<>((LynxView) bodyView);
      } else if (bodyView instanceof LynxFrameView) {
        mRootView = new WeakReference<>(((LynxFrameView) bodyView).getRootView());
      }
    }
  }

  private boolean ensureRenderCreated() {
    if (mRender != null && mLynxUIRender != null) {
      return true;
    }

    UIBodyView bodyView = mContext.getUIBodyView();
    if (bodyView == null) {
      LLog.e(TAG, "ensureRenderCreated failed, bodyView is null");
      return false;
    }

    LynxViewBuilder builder = bodyView.getLynxViewBuilder();
    builder.setEnablePreUpdateData(true);
    builder.setEmbeddedMode(mEmbeddedMode);
    if (mEnableMultiAsyncThread != null) {
      builder.setEnableMultiAsyncThread(mEnableMultiAsyncThread);
    }
    if (mPresetWidth != -1 || mPresetHeight != -1) {
      builder.setPresetMeasuredSpec(
          MeasureSpec.makeMeasureSpec(mPresetWidth == -1 ? 0 : mPresetWidth, mWidthMode),
          MeasureSpec.makeMeasureSpec(mPresetHeight == -1 ? 0 : mPresetHeight, mHeightMode));
    }
    mLynxUIRender = builder.createLynxUIRenderer();
    mRender = new LynxTemplateRender(mContext, this, builder);

    if (mRender == null || mLynxUIRender == null) {
      LLog.e(TAG, "ensureRenderCreated failed, render or ui renderer creation returned null");
      return false;
    }

    if (mAttachLynxPageUICallback != null) {
      mRender.setAttachLynxPageUICallback(mAttachLynxPageUICallback);
      mAttachLynxPageUICallback = null;
    }
    return true;
  }

  public void setSign(int sign) {
    mSign = sign;
  }

  /**
   * Get root view of this LynxFrameView.
   * @return root view of this LynxFrameView.
   */
  public LynxView getRootView() {
    return mRootView == null ? null : mRootView.get();
  }

  boolean loadBundle(TemplateBundle bundle) {
    if (!ensureRenderCreated()) {
      LLog.e(TAG, "create render failed");
      return false;
    }

    if (hasInitializedSize(mContentWidth, mContentHeight)) {
      mRender.updateViewport(MeasureSpec.makeMeasureSpec(mContentWidth, mWidthMode),
          MeasureSpec.makeMeasureSpec(mContentHeight, mHeightMode));
    } else if (mPresetWidth != -1 || mPresetHeight != -1) {
      mRender.updateViewport(
          MeasureSpec.makeMeasureSpec(mPresetWidth == -1 ? 0 : mPresetWidth, mWidthMode),
          MeasureSpec.makeMeasureSpec(mPresetHeight == -1 ? 0 : mPresetHeight, mHeightMode));
    }

    LynxLoadMeta.Builder builder = new LynxLoadMeta.Builder();
    builder.setUrl(mUrl);
    builder.setTemplateBundle(bundle);
    TemplateData initData = consumePendingTemplateData(true);
    if (initData != null) {
      builder.setInitialData(initData);
    }
    TemplateData globalProps = consumePendingTemplateData(false);
    if (globalProps != null) {
      builder.setGlobalProps(globalProps);
    }
    mRender.loadTemplate(builder.build());
    mIsBundleLoaded = true;
    return true;
  }

  public void updateLayout(int width, int height) {
    if (TraceEvent.isTracingStarted()) {
      HashMap<String, String> traceProps = new HashMap<>();
      int widthMeasureSpec = MeasureSpec.makeMeasureSpec(width, mWidthMode);
      int heightMeasureSpec = MeasureSpec.makeMeasureSpec(height, mHeightMode);
      traceProps.put(TraceEventDef.WIDTH_MEASURE_SPEC, MeasureSpec.toString(widthMeasureSpec));
      traceProps.put(TraceEventDef.HEIGHT_MEASURE_SPEC, MeasureSpec.toString(heightMeasureSpec));
      TraceEvent.instant(
          TraceEvent.CATEGORY_DEFAULT, TraceEventDef.LYNX_FRAME_VIEW_UPDATE_LAYOUT, traceProps);
    }
    mContentWidth = width;
    mContentHeight = height;
  }

  void setAutoWidth(boolean value) {
    if (mAutoWidth == value) {
      return;
    }
    mAutoWidth = value;
    mWidthMode = value ? MeasureSpec.UNSPECIFIED : MeasureSpec.EXACTLY;
    if (mIsBundleLoaded && hasInitializedSize(mContentWidth, mContentHeight)) {
      mRender.updateViewport(MeasureSpec.makeMeasureSpec(mContentWidth, mWidthMode),
          MeasureSpec.makeMeasureSpec(mContentHeight, mHeightMode));
    }
  }

  void setAutoHeight(boolean value) {
    if (mAutoHeight == value) {
      return;
    }
    mAutoHeight = value;
    mHeightMode = value ? MeasureSpec.UNSPECIFIED : MeasureSpec.EXACTLY;
    if (mIsBundleLoaded && hasInitializedSize(mContentWidth, mContentHeight)) {
      mRender.updateViewport(MeasureSpec.makeMeasureSpec(mContentWidth, mWidthMode),
          MeasureSpec.makeMeasureSpec(mContentHeight, mHeightMode));
    }
  }

  void setPresetWidth(int width) {
    if (width < 0) {
      return;
    }
    mPresetWidth = width;
  }

  void setPresetHeight(int height) {
    if (height < 0) {
      return;
    }
    mPresetHeight = height;
  }

  void setEnableMultiAsyncThread(Boolean value) {
    mEnableMultiAsyncThread = value;
  }

  void setInitData(long dataPtr) {
    TraceEvent.instant(TraceEvent.CATEGORY_DEFAULT, TraceEventDef.LYNX_FRAME_VIEW_SET_INIT_DATA);
    mInitDataPtr = replacePendingTemplateDataPtr(mInitDataPtr, dataPtr);
  }

  void setGlobalProps(long dataPtr) {
    TraceEvent.instant(TraceEvent.CATEGORY_DEFAULT, TraceEventDef.LYNX_FRAME_VIEW_SET_GLOBAL_PROPS);
    mGlobalPropsPtr = replacePendingTemplateDataPtr(mGlobalPropsPtr, dataPtr);
  }

  void onPropsUpdated() {
    if (!mIsBundleLoaded || mRender == null) {
      LLog.e(TAG, "onPropsUpdated failed, bundle not loaded or render is null");
      return;
    }

    if (mInitDataPtr == 0 && mGlobalPropsPtr == 0) {
      return;
    }

    TemplateData initData = consumePendingTemplateData(true);
    TemplateData globalProps = consumePendingTemplateData(false);

    LynxUpdateMeta meta =
        new LynxUpdateMeta.Builder()
            .setUpdatedData(initData == null ? TemplateData.empty() : initData)
            .setUpdatedGlobalProps(globalProps == null ? TemplateData.empty() : globalProps)
            .build();
    mRender.updateMetaData(meta);
  }

  public void setUrl(String url) {
    mUrl = url;
  }

  @Override
  public void runOnTasmThread(Runnable runnable) {
    if (mRender != null) {
      mRender.runOnTasmThread(runnable);
    }
  }

  public void setEmbeddedMode(@EmbeddedMode.Mode int mode) {
    if (mEmbeddedMode != EmbeddedMode.UNSET) {
      LLog.e(TAG, "setEmbeddedMode failed, embeddedMode is already set");
      return;
    }
    mEmbeddedMode = mode;
  }

  private boolean hasInitializedSize(int width, int height) {
    return width != -1 && height != -1;
  }

  private long replacePendingTemplateDataPtr(long current, long next) {
    if (current != 0 && current != next) {
      recyclePendingTemplateDataPtr(current);
    }
    return next;
  }

  private void recyclePendingTemplateData() {
    if (mInitDataPtr != 0) {
      recyclePendingTemplateDataPtr(mInitDataPtr);
      mInitDataPtr = 0;
    }
    if (mGlobalPropsPtr != 0) {
      recyclePendingTemplateDataPtr(mGlobalPropsPtr);
      mGlobalPropsPtr = 0;
    }
  }

  private TemplateData consumePendingTemplateData(boolean isInitData) {
    long nativeTemplateDataPtr = isInitData ? mInitDataPtr : mGlobalPropsPtr;
    if (isInitData) {
      mInitDataPtr = 0;
    } else {
      mGlobalPropsPtr = 0;
    }
    return nativeTemplateDataPtr == 0
        ? null
        : TemplateData.fromNativeTemplateDataPtr(nativeTemplateDataPtr);
  }

  private void recyclePendingTemplateDataPtr(final long nativeTemplateDataPtr) {
    if (nativeTemplateDataPtr == 0) {
      return;
    }
    if (mContext != null) {
      mContext.runOnTasmThread(new Runnable() {
        @Override
        public void run() {
          TemplateData.fromNativeTemplateDataPtr(nativeTemplateDataPtr).recycle();
        }
      });
      return;
    }
    TemplateData.fromNativeTemplateDataPtr(nativeTemplateDataPtr).recycle();
  }

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    if (TraceEvent.isTracingStarted()) {
      HashMap<String, String> traceProps = new HashMap<>();
      traceProps.put(TraceEventDef.WIDTH_MEASURE_SPEC, MeasureSpec.toString(widthMeasureSpec));
      traceProps.put(TraceEventDef.HEIGHT_MEASURE_SPEC, MeasureSpec.toString(heightMeasureSpec));
      TraceEvent.instant(
          TraceEvent.CATEGORY_DEFAULT, TraceEventDef.LYNX_FRAME_VIEW_ON_MEASURE, traceProps);
    }

    if (!mIsBundleLoaded || mRender == null) {
      super.onMeasure(widthMeasureSpec, heightMeasureSpec);
      return;
    }

    int targetWidth = mContentWidth;
    int targetHeight = mContentHeight;
    if (!mIsIntrinsicSizeConsumed) {
      if (mWidthMode != MeasureSpec.EXACTLY) {
        targetWidth = getIntrinsicWidth();
      }
      if (mHeightMode != MeasureSpec.EXACTLY) {
        targetHeight = getIntrinsicHeight();
      }
      mIsIntrinsicSizeConsumed = true;
    }

    if (hasInitializedSize(targetWidth, targetHeight)) {
      if (TraceEvent.isTracingStarted()) {
        HashMap<String, String> traceProps = new HashMap<>();
        int targetWidthMeasureSpec = MeasureSpec.makeMeasureSpec(targetWidth, mWidthMode);
        int targetHeightMeasureSpec = MeasureSpec.makeMeasureSpec(targetHeight, mHeightMode);
        traceProps.put(
            TraceEventDef.WIDTH_MEASURE_SPEC, MeasureSpec.toString(targetWidthMeasureSpec));
        traceProps.put(
            TraceEventDef.HEIGHT_MEASURE_SPEC, MeasureSpec.toString(targetHeightMeasureSpec));
        TraceEvent.instant(TraceEvent.CATEGORY_DEFAULT,
            TraceEventDef.LYNX_FRAME_VIEW_ON_MEASURE_TARGET, traceProps);
      }
      mRender.onMeasure(MeasureSpec.makeMeasureSpec(targetWidth, mWidthMode),
          MeasureSpec.makeMeasureSpec(targetHeight, mHeightMode));
    } else {
      super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }
  }

  @Override
  protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
    if (mRender != null) {
      mRender.onLayout(changed, left, top, right, bottom);
    }
  }

  @Override
  public void setIntrinsicContentSize(int width, int height) {
    if (width == getIntrinsicWidth() && height == getIntrinsicHeight()) {
      return;
    }
    LLog.i(TAG, "LynxFrameView::setIntrinsicContentSize width:" + width + " height:" + height);

    if (TraceEvent.isTracingStarted()) {
      HashMap<String, String> traceProps = new HashMap<>();
      traceProps.put("width", String.valueOf(width));
      traceProps.put("height", String.valueOf(height));
      TraceEvent.instant(TraceEvent.CATEGORY_DEFAULT,
          TraceEventDef.LYNX_FRAME_VIEW_SET_INTRINSIC_CONTENT_SIZE, traceProps);
    }

    mContext.findShadowNodeAndRunTask(mSign, (node) -> {
      if (node instanceof FrameShadowNode) {
        ((FrameShadowNode) node).updateIntrinsicContentSize(width, height);
      }
    });
    mIsIntrinsicSizeConsumed = false;
    super.setIntrinsicContentSize(width, height);
  }

  @Override
  public void setAttachLynxPageUICallback(attachLynxPageUICallback callback) {
    if (mRender != null) {
      mRender.setAttachLynxPageUICallback(callback);
    } else {
      mAttachLynxPageUICallback = callback;
    }
  }

  @Override
  public LynxViewBuilder getLynxViewBuilder() {
    return null != mRender ? mRender.getLynxViewBuilder() : null;
  }

  void destroy() {
    if (mDestroyed) {
      return;
    }
    mDestroyed = true;

    LLog.i(TAG, "lynxframeview destroy " + this.toString());
    TraceEvent.beginSection(TraceEventDef.DESTORY_LYNXFRAMEVIEW);
    if (mRender != null) {
      mRender.onDetachedFromWindow();
      mRender.destroy();
      mRender = null;
    }
    recyclePendingTemplateData();
    TraceEvent.endSection(TraceEventDef.DESTORY_LYNXFRAMEVIEW);
  }

  @Override
  protected void dispatchDraw(Canvas canvas) {
    super.dispatchDraw(canvas);
    if (mRender != null) {
      mRender.onRootViewDraw(canvas);
    }
  }

  @Override
  public void sendGlobalEvent(String name, JavaOnlyArray params) {
    if (mRender != null) {
      mRender.sendGlobalEvent(name, params);
    }
  }
}
