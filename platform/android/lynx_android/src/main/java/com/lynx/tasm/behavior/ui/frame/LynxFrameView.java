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
  private TemplateData mInitData = null;
  private TemplateData mGlobalProps = null;
  private int mWidthMode = MeasureSpec.UNSPECIFIED;
  private int mHeightMode = MeasureSpec.UNSPECIFIED;
  private int mEmbeddedMode = EmbeddedMode.UNSET;

  public LynxFrameView(Context context) {
    super(context);
    init(context);
  }

  public LynxFrameView(Context context, AttributeSet attrs) {
    super(context, attrs);
    init(context);
  }

  private void init(Context context) {
    mContext = (LynxContext) context;
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
    if (bodyView != null) {
      LynxViewBuilder builder = bodyView.getLynxViewBuilder();
      builder.setEnablePreUpdateData(true);
      builder.setEmbeddedMode(mEmbeddedMode);
      mLynxUIRender = builder.createLynxUIRenderer();
      mRender = new LynxTemplateRender(mContext, this, builder);
      return true;
    }
    return false;
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

    mRender.updateViewport(MeasureSpec.makeMeasureSpec(mContentWidth, mWidthMode),
        MeasureSpec.makeMeasureSpec(mContentHeight, mHeightMode));
    LynxLoadMeta.Builder builder = new LynxLoadMeta.Builder();
    builder.setUrl(mUrl);
    builder.setTemplateBundle(bundle);
    if (mInitData != null) {
      builder.setInitialData(mInitData);
      mInitData = null;
    }
    if (mGlobalProps != null) {
      builder.setGlobalProps(mGlobalProps);
      mGlobalProps = null;
    }
    mRender.loadTemplate(builder.build());
    mIsBundleLoaded = true;
    return true;
  }

  public void updateLayout(int width, int height) {
    if (!mIsBundleLoaded || (mContentWidth == -1 && mContentHeight == -1)) {
      mWidthMode = width == 0 ? MeasureSpec.UNSPECIFIED : MeasureSpec.EXACTLY;
      mHeightMode = height == 0 ? MeasureSpec.UNSPECIFIED : MeasureSpec.EXACTLY;
    }

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

  void setInitData(TemplateData data) {
    TraceEvent.instant(TraceEvent.CATEGORY_DEFAULT, TraceEventDef.LYNX_FRAME_VIEW_SET_INIT_DATA);
    mInitData = data;
  }

  void setGlobalProps(TemplateData data) {
    TraceEvent.instant(TraceEvent.CATEGORY_DEFAULT, TraceEventDef.LYNX_FRAME_VIEW_SET_GLOBAL_PROPS);
    mGlobalProps = data;
  }

  void onPropsUpdated() {
    if (!mIsBundleLoaded || mRender == null) {
      LLog.e(TAG, "onPropsUpdated failed, bundle not loaded or render is null");
      return;
    }

    if (mInitData == null && mGlobalProps == null) {
      return;
    }

    LynxUpdateMeta meta =
        new LynxUpdateMeta.Builder()
            .setUpdatedData(mInitData == null ? TemplateData.empty() : mInitData)
            .setUpdatedGlobalProps(mGlobalProps == null ? TemplateData.empty() : mGlobalProps)
            .build();
    mRender.updateMetaData(meta);

    mInitData = null;
    mGlobalProps = null;
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

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    if (TraceEvent.isTracingStarted()) {
      HashMap<String, String> traceProps = new HashMap<>();
      traceProps.put(TraceEventDef.WIDTH_MEASURE_SPEC, MeasureSpec.toString(widthMeasureSpec));
      traceProps.put(TraceEventDef.HEIGHT_MEASURE_SPEC, MeasureSpec.toString(heightMeasureSpec));
      TraceEvent.instant(
          TraceEvent.CATEGORY_DEFAULT, TraceEventDef.LYNX_FRAME_VIEW_ON_MEASURE, traceProps);
    }

    if (!mIsBundleLoaded) {
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

    if (TraceEvent.isTracingStarted()) {
      HashMap<String, String> traceProps = new HashMap<>();
      int targetWidthMeasureSpec = MeasureSpec.makeMeasureSpec(targetWidth, mWidthMode);
      int targetHeightMeasureSpec = MeasureSpec.makeMeasureSpec(targetHeight, mHeightMode);
      traceProps.put(
          TraceEventDef.WIDTH_MEASURE_SPEC, MeasureSpec.toString(targetWidthMeasureSpec));
      traceProps.put(
          TraceEventDef.HEIGHT_MEASURE_SPEC, MeasureSpec.toString(targetHeightMeasureSpec));
      TraceEvent.instant(
          TraceEvent.CATEGORY_DEFAULT, TraceEventDef.LYNX_FRAME_VIEW_ON_MEASURE_TARGET, traceProps);
    }
    mRender.onMeasure(MeasureSpec.makeMeasureSpec(targetWidth, mWidthMode),
        MeasureSpec.makeMeasureSpec(targetHeight, mHeightMode));
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
