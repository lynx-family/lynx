// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.image;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.background.BackgroundLayerDrawable;
import com.lynx.tasm.event.LynxDetailEvent;
import com.lynx.tasm.image.ImageErrorCodeUtils;
import com.lynx.tasm.utils.ContextUtils;
import java.lang.ref.WeakReference;

public class BackgroundImageDrawable extends BackgroundLayerDrawable {
  public static final String EVENT_ERROR = "bgerror";
  public static final String EVENT_LOAD = "bgload";
  private final LynxImageManager mLynxImageManager;
  private int mImgWidth = 0;
  private int mImgHeight = 0;
  private final LynxContext mContext;
  private WeakReference<LynxBaseUI> mUI;
  private final String mUrl;
  private int mWidth;
  private int mHeight;

  private boolean mAttached = false;

  public BackgroundImageDrawable(Context context, String url) {
    this.mContext = ContextUtils.toLynxContext(context);
    this.mUrl = url;
    mLynxImageManager = new LynxImageManager(this.mContext) {
      @Override
      protected void onImageLoadSuccess(int width, int height) {
        mImgWidth = width;
        mImgHeight = height;
        mLynxImageManager.getSrcImageDrawable().setBounds(getBounds());
        if (context instanceof LynxContext) {
          if (mUI == null || mUI.get() == null) {
            return;
          }
          LynxBaseUI ui = mUI.get();
          if (ui.getEvents() != null
              && ui.getEvents().containsKey(BackgroundImageDrawable.EVENT_LOAD)) {
            LynxDetailEvent event =
                new LynxDetailEvent(ui.getSign(), BackgroundImageDrawable.EVENT_LOAD);
            event.addDetail("height", height);
            event.addDetail("width", width);
            event.addDetail("url", url);
            ((LynxContext) context).getEventEmitter().sendCustomEvent(event);
          }
        }
      }

      @Override
      protected void onImageLoadError(LynxError error, int categorizedCode, int imageErrorCode) {
        if (error == null || !(context instanceof LynxContext)) {
          return;
        }
        ((LynxContext) context).reportResourceError(url, "image", error);
        if (mUI == null || mUI.get() == null) {
          return;
        }
        LynxBaseUI ui = mUI.get();
        if (ui.getEvents() != null
            && ui.getEvents().containsKey(BackgroundImageDrawable.EVENT_ERROR)) {
          LynxDetailEvent event =
              new LynxDetailEvent(ui.getSign(), BackgroundImageDrawable.EVENT_ERROR);
          event.addDetail("errMsg", error.getSummaryMessage());
          event.addDetail("url", url);
          event.addDetail(ImageErrorCodeUtils.LYNX_IMAGE_CATEGORIZED_CODE_KEY, categorizedCode);
          event.addDetail(ImageErrorCodeUtils.LYNX_IMAGE_ERROR_CODE_KEY, imageErrorCode);
          ((LynxContext) context).getEventEmitter().sendCustomEvent(event);
        }
      }
    };
    mLynxImageManager.setDisableDefaultResize(true);
    //    mLynxImageManager.setMode();
  }

  @Override
  public boolean isReady() {
    return mLynxImageManager.getSrcImageDrawable() != null;
  }

  @Override
  public int getImageWidth() {
    return mImgWidth;
  }

  @Override
  public int getImageHeight() {
    return mImgHeight;
  }

  @Override
  public void setBitmapConfig(@Nullable Bitmap.Config config) {
    mLynxImageManager.setImageConfig(config);
  }

  @Override
  public void onAttach() {
    attachIfNeeded();
  }

  @Override
  public void onDetach() {
    mAttached = false;
    mLynxImageManager.destroy();
  }

  private void attachIfNeeded() {
    if (!mAttached) {
      mAttached = true;
      if (mWidth > 0 && mHeight > 0) {
        mLynxImageManager.updateNodeProps();
      }
    }
  }

  @Override
  public void onSizeChanged(int width, int height) {
    if (mWidth != width || mHeight != height) {
      mWidth = width;
      mHeight = height;
      mLynxImageManager.onLayoutUpdated(mWidth, mHeight, 0, 0, 0, 0);
      mLynxImageManager.updateNodeProps();
    }
  }

  private void updateImageDrawableBounds(Rect bounds) {
    if (mLynxImageManager.getSrcImageDrawable() != null) {
      mLynxImageManager.getSrcImageDrawable().setBounds(bounds);
    }
  }

  @Override
  protected void onBoundsChange(Rect bounds) {
    super.onBoundsChange(bounds);
    updateImageDrawableBounds(bounds);
  }

  @Override
  public void draw(@NonNull Canvas canvas) {
    mLynxImageManager.onDraw(canvas);
  }

  @Override
  public void setLynxUI(LynxBaseUI ui) {
    super.setLynxUI(ui);
    mUI = new WeakReference<>(ui);
    mLynxImageManager.setLocalCache(ui.getEnableLocalCache());
    mLynxImageManager.setLynxBaseUI(ui);
    mLynxImageManager.setSkipRedirection(ui.getSkipRedirection());
    mLynxImageManager.setSrc(mUrl);
    mLynxImageManager.updateRedirectCheckResult();
  }
}
