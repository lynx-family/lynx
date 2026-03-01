// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.frame;

import android.content.Context;
import android.util.AttributeSet;
import androidx.annotation.RestrictTo;
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
  private int mContentWidth = 0;
  private int mContentHeight = 0;
  private boolean mDestroyed = false;
  private TemplateData mInitData = null;
  private TemplateData mGlobalProps = null;
  private int mWidthMode = MeasureSpec.UNSPECIFIED;
  private int mHeightMode = MeasureSpec.UNSPECIFIED;

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
      LynxViewBuilder builder = bodyView.getLynxViewBuilder();
      builder.setEnablePreUpdateData(true);
      mLynxUIRender = builder.createLynxUIRenderer();
      mRender = new LynxTemplateRender(context, this, builder);
    }
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

  void loadBundle(TemplateBundle bundle) {
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
  }

  public void updateLayout(int width, int height) {
    mContentWidth = width;
    mContentHeight = height;
  }

  void setInitData(TemplateData data) {
    mInitData = data;
  }

  void setGlobalProps(TemplateData data) {
    mGlobalProps = data;
  }

  void onPropsUpdated() {
    if (!mIsBundleLoaded) {
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
    mRender.runOnTasmThread(runnable);
  }

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    if (!mIsBundleLoaded) {
      int width = MeasureSpec.getSize(widthMeasureSpec);
      if (width > 0) {
        mWidthMode = MeasureSpec.EXACTLY;
      }
      int height = MeasureSpec.getSize(heightMeasureSpec);
      if (height > 0) {
        mHeightMode = MeasureSpec.EXACTLY;
      }
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

    mRender.onMeasure(MeasureSpec.makeMeasureSpec(targetWidth, mWidthMode),
        MeasureSpec.makeMeasureSpec(targetHeight, mHeightMode));
  }

  @Override
  protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
    mRender.onLayout(changed, left, top, right, bottom);
  }

  @Override
  public void setIntrinsicContentSize(int width, int height) {
    if (width == getIntrinsicWidth() && height == getIntrinsicHeight()) {
      return;
    }
    LLog.i(TAG, "LynxFrameView::setIntrinsicContentSize width:" + width + " height:" + height);

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
}
