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
import com.lynx.tasm.behavior.ui.LynxFlattenUI;
import com.lynx.tasm.behavior.ui.utils.BackgroundDrawable;
import com.lynx.tasm.event.EventsListener;
import java.util.Map;

public class FlattenUIImage extends LynxFlattenUI {
  LynxImageManager mLynxImageManager;

  public FlattenUIImage(LynxContext context) {
    super(context);
    mLynxImageManager = new LynxImageManager(getLynxContext());
    mLynxImageManager.setLynxBaseUI(this);
  }

  @LynxUIMethod
  public void pauseAnimation(ReadableMap params, com.lynx.react.bridge.Callback callback) {
    mLynxImageManager.pauseAnimation(params, callback);
  }

  @LynxUIMethod
  public void resumeAnimation(ReadableMap params, com.lynx.react.bridge.Callback callback) {
    mLynxImageManager.resumeAnimation(params, callback);
  }

  @LynxUIMethod
  public void stopAnimation(ReadableMap params, com.lynx.react.bridge.Callback callback) {
    mLynxImageManager.stopAnimation(params, callback);
  }

  @LynxUIMethod
  public void startAnimate(ReadableMap params, com.lynx.react.bridge.Callback callback) {
    mLynxImageManager.startAnimate(params, callback);
  }

  @Override
  public void updatePropertiesInterval(StylesDiffMap props) {
    super.updatePropertiesInterval(props);
    mLynxImageManager.updatePropertiesInterval(props);
  }

  @Override
  public void onPropsUpdated() {
    super.onPropsUpdated();
    mLynxImageManager.onPropsUpdated();
  }

  @Override
  public void setImageRendering(int imageRendering) {
    super.setImageRendering(imageRendering);
    mLynxImageManager.setIsPixelated(imageRendering == IMAGERENDERING_PIXELATED);
  }

  @Override
  public void onNodeReady() {
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
  public void onDraw(Canvas canvas) {
    super.onDraw(canvas);
    mLynxImageManager.onDraw(canvas);
  }

  @Override
  public void setEvents(Map<String, EventsListener> events) {
    super.setEvents(events);
    mLynxImageManager.setEvents(events);
  }

  @Override
  public void onLayoutUpdated() {
    super.onLayoutUpdated();
    mLynxImageManager.onLayoutUpdated(getWidth(), getHeight(), getPaddingLeft(), getPaddingRight(),
        getPaddingTop(), getPaddingBottom());
  }
}
