// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.image;

import static com.lynx.tasm.behavior.AutoGenStyleConstants.IMAGERENDERING_PIXELATED;

import android.graphics.Canvas;
import android.view.View;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIMethod;
import com.lynx.tasm.behavior.StylesDiffMap;
import com.lynx.tasm.behavior.ui.MeaningfulPaintingArea;
import com.lynx.tasm.behavior.ui.UIParams;
import com.lynx.tasm.behavior.ui.ViewInfo;
import com.lynx.tasm.behavior.ui.view.UIView;
import com.lynx.tasm.event.EventsListener;
import java.util.Map;

public class UIImage extends UIView {
  LynxImageManager mLynxImageManager;

  public UIImage(LynxContext context) {
    this(context, null);
  }

  public UIImage(LynxContext context, Object params) {
    super(context, params);
    if (mContext != null && mContext.isFallbackProcess() && mContext.getUIBodyView() != null
        && params instanceof UIParams) {
      setNodeIndex(((UIParams) params).mNodeIndex);
      mLynxImageManager = context.getUIBodyView().obtainImageAccordingToNodeIndex(mNodeIndex);
    }
    if (mLynxImageManager == null) {
      mLynxImageManager = new LynxImageManager(getLynxContext());
    }
    mLynxImageManager.setLynxBaseUI(this);
    mView.setWillNotDraw(false);
  }

  @Override
  protected boolean needGenerateMeaningfulPaintingArea() {
    return true;
  }

  @Override
  protected MeaningfulPaintingArea convertToMeaningfulPaintingArea(int offsetX, int offsetY) {
    if (mLynxImageManager != null) {
      mLynxImageManager.tryHandleResult();
    }

    MeaningfulPaintingArea area =
        new MeaningfulPaintingArea(offsetX + getOriginLeft(), offsetY + getOriginTop(), getWidth(),
            getHeight(), mLynxImageManager != null ? mLynxImageManager.getHasContent() : false);
    area.setAlpha(mView != null ? mView.getAlpha() : getAlpha());
    area.setScaleX(mView != null ? mView.getScaleX() : getScaleX());
    area.setScaleY(mView != null ? mView.getScaleY() : getScaleY());
    area.setVisibleStatus(
        mView != null ? mView.getVisibility() : (getVisibility() ? View.VISIBLE : View.INVISIBLE));

    return area;
  }

  @Override
  public MeaningfulContentStatus getMeaningfulContentStatus() {
    if (mLynxImageManager != null && mLynxImageManager.getHasContent()) {
      return MeaningfulContentStatus.PRESENTED;
    }
    return MeaningfulContentStatus.PENDING;
  }

  private void ensureLynxImageManager() {
    if (mLynxImageManager != null) {
      return;
    }
    mLynxImageManager = new LynxImageManager(getLynxContext());
    mLynxImageManager.setLynxBaseUI(this);
    mLynxImageManager.setEvents(mEvents);
    mLynxImageManager.updatePropertiesInterval(mProps);
    mLynxImageManager.onLayoutUpdated(getWidth(), getHeight(), getPaddingLeft(), getPaddingRight(),
        getPaddingTop(), getPaddingBottom());
    mLynxImageManager.onNodeReady();
  }

  // region UIMethod
  @LynxUIMethod
  public void pauseAnimation(ReadableMap params, com.lynx.react.bridge.Callback callback) {
    ensureLynxImageManager();
    mLynxImageManager.pauseAnimation(params, callback);
  }

  @LynxUIMethod
  public void resumeAnimation(ReadableMap params, com.lynx.react.bridge.Callback callback) {
    ensureLynxImageManager();
    mLynxImageManager.resumeAnimation(params, callback);
  }

  @LynxUIMethod
  public void stopAnimation(ReadableMap params, com.lynx.react.bridge.Callback callback) {
    ensureLynxImageManager();
    mLynxImageManager.stopAnimation(params, callback);
  }

  @LynxUIMethod
  public void startAnimate(ReadableMap params, com.lynx.react.bridge.Callback callback) {
    ensureLynxImageManager();
    mLynxImageManager.startAnimate(params, callback);
  }

  @Override
  public void setImageRendering(int imageRendering) {
    ensureLynxImageManager();
    super.setImageRendering(imageRendering);
    mLynxImageManager.setIsPixelated(imageRendering == IMAGERENDERING_PIXELATED);
  }

  // endregion

  @Override
  public void updatePropertiesInterval(StylesDiffMap props) {
    ensureLynxImageManager();
    super.updatePropertiesInterval(props);
    mLynxImageManager.updatePropertiesInterval(props.mBackingMap);
  }

  @Override
  public int getInitialOverflowType() {
    return OVERFLOW_HIDDEN;
  }

  @Override
  public void onPropsUpdated() {
    ensureLynxImageManager();
    super.onPropsUpdated();
    mLynxImageManager.onPropsUpdated();
  }

  @Override
  public void onNodeReady() {
    ensureLynxImageManager();
    super.onNodeReady();
    if (mLynxBackground.getDrawable() != null) {
      mLynxImageManager.setBorderWidth(
          mLynxBackground.getDrawable().getDirectionAwareBorderInsets());
      if (mLynxBackground.getBorderRadius() != null) {
        boolean radiusSizeChanged =
            mLynxBackground.getBorderRadius().updateSize(getWidth(), getHeight());
        mLynxImageManager.setBorderRadius(
            mLynxBackground.getBorderRadius().getArray(), radiusSizeChanged);
      }
    }
    mLynxImageManager.onNodeReady();
  }

  @Override
  public void destroy() {
    super.destroy();
    if (mLynxImageManager != null) {
      // mLynxImageManager will be set to null when detached from the LynxImageManager.
      // And we will ensure it exists before draw and we can ignore the destroy call if it's null.
      mLynxImageManager.destroy();
    }
  }

  @Override
  public void beforeDraw(Canvas canvas) {
    ensureLynxImageManager();
    super.beforeDraw(canvas);
    mLynxImageManager.onDraw(canvas);
  }

  @Override
  public void beforeProcessViewInfo(ViewInfo info) {
    ensureLynxImageManager();

    if (mLynxImageManager != null) {
      mViewInfo.setImageManagerUsedInBeforeDraw(mLynxImageManager);
    }

    super.beforeProcessViewInfo(info);
  }

  @Override
  public void onAttach() {
    super.onAttach();
  }

  @Override
  public void onDetach() {
    super.onDetach();
  }

  @Override
  public void setEvents(Map<String, EventsListener> events) {
    ensureLynxImageManager();
    super.setEvents(events);
    mLynxImageManager.setEvents(events);
  }

  @Override
  public void onLayoutUpdated() {
    ensureLynxImageManager();
    super.onLayoutUpdated();
    mLynxImageManager.onLayoutUpdated(getWidth(), getHeight(), getPaddingLeft(), getPaddingRight(),
        getPaddingTop(), getPaddingBottom());
  }

  @Override
  protected void detachWithViewInfo(ViewInfo parentViewInfo) {
    if (mLynxImageManager != null) {
      mLynxImageManager.setLynxBaseUI(null);
      mLynxImageManager.setViewInfo(mViewInfo != null ? mViewInfo : parentViewInfo);
      getLynxContext().getUIBodyView().registerImageAccordingToNodeIndex(
          mNodeIndex, mLynxImageManager);
      // TODO(songshourui.null): We can nullify LynxImageManager upon src changes or layout updates
      // to optimize performance.
      mLynxImageManager = null;
    }

    super.detachWithViewInfo(parentViewInfo);
  }
}
