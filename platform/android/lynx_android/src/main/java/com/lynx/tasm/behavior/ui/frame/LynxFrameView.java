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
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.UIBody.UIBodyView;
import java.lang.ref.WeakReference;

@RestrictTo(RestrictTo.Scope.LIBRARY)
public final class LynxFrameView extends UIBodyView {
  private LynxTemplateRender mRender;
  private String mUrl;
  private WeakReference<LynxView> mRootView = null;

  public LynxFrameView(Context context) {
    super(context);
    init(context);
  }

  public LynxFrameView(Context context, AttributeSet attrs) {
    super(context, attrs);
    init(context);
  }

  private void init(Context context) {
    UIBodyView bodyView = ((LynxContext) context).getUIBodyView();
    if (bodyView != null) {
      if (bodyView instanceof LynxView) {
        mRootView = new WeakReference<>((LynxView) bodyView);
      } else if (bodyView instanceof LynxFrameView) {
        mRootView = new WeakReference<>(((LynxFrameView) bodyView).getRootView());
      }
      LynxViewBuilder builder = bodyView.getLynxViewBuilder();
      mLynxUIRender = builder.createLynxUIRenderer();
      mRender = new LynxTemplateRender(context, this, builder);
    }
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
    mRender.loadTemplate(builder.build());
  }

  public void updateViewport(int widthMeasureSpec, int heightMeasureSpec) {
    mRender.updateViewport(widthMeasureSpec, heightMeasureSpec);
  }

  public void updateMetaData(TemplateData initData, TemplateData globalProps) {
    if (initData == null && globalProps == null) {
      return;
    }
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
    mRender.runOnTasmThread(runnable);
  }

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    mRender.onMeasure(widthMeasureSpec, heightMeasureSpec);
  }

  @Override
  protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
    mRender.onLayout(changed, left, top, right, bottom);
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
    mRender.destroy();
  }
}
