// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.image;

import static com.lynx.tasm.behavior.AutoGenStyleConstants.IMAGERENDERING_PIXELATED;

import android.graphics.Canvas;
import android.graphics.Path;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIMethod;
import com.lynx.tasm.behavior.StylesDiffMap;
import com.lynx.tasm.behavior.ui.ViewInfo;
import com.lynx.tasm.behavior.ui.utils.BackgroundDrawable;
import com.lynx.tasm.behavior.ui.view.UIView;
import com.lynx.tasm.event.EventsListener;
import java.util.Map;

public class UIImage extends UIView {
  LynxImageManager mLynxImageManager;

  public UIImage(LynxContext context) {
    super(context);
    mLynxImageManager = new LynxImageManager(getLynxContext());
    mLynxImageManager.setLynxBaseUI(this);
    mView.setWillNotDraw(false);
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
      if (mLynxBackground.getBorderRadius() != null
          && mLynxBackground.getBorderRadius().updateSize(getWidth(), getHeight())) {
        mLynxImageManager.setBorderRadius(mLynxBackground.getBorderRadius().getArray());
      }
    }
    mLynxImageManager.onNodeReady();
  }

  @Override
  public void destroy() {
    super.destroy();
    mLynxImageManager.destroy();
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

    if (mLynxImageManager != null && !mLynxImageManager.getHasContent()) {
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
      mLynxImageManager.setViewInfo(parentViewInfo);
      // TODO(songshourui.null): We can nullify LynxImageManager upon src changes or layout updates
      // to optimize performance.
      mLynxImageManager = null;
    }
    super.detachWithViewInfo(parentViewInfo);
  }
}
