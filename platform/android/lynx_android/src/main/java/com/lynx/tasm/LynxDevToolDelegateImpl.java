// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import android.graphics.Bitmap;
import com.lynx.devtoolwrapper.IDevToolDelegate;
import com.lynx.devtoolwrapper.ScreenshotBitmapHandler;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.behavior.ILynxUIRenderer;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;

public class LynxDevToolDelegateImpl implements IDevToolDelegate {
  private WeakReference<LynxTemplateRender> mRender = null;

  public LynxDevToolDelegateImpl(LynxTemplateRender render) {
    mRender = new WeakReference<>(render);
  }

  @Override
  public void onDispatchMessageEvent(ReadableMap map) {
    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
      @Override
      public void run() {
        UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
          @Override
          public void run() {
            LynxTemplateRender render = mRender.get();
            if (render == null) {
              return;
            }
            render.dispatchMessageEvent(map);
          }
        });
      }
    });
  }

  private ILynxUIRenderer getLynxUIRenderer() {
    LynxTemplateRender render = mRender.get();
    return render != null ? render.lynxUIRenderer() : null;
  }

  // for devtool real-time screencast
  @Override
  public void takeScreenshot(ScreenshotBitmapHandler handler, String screenShotMode) {
    ILynxUIRenderer render = getLynxUIRenderer();
    if (render != null) {
      render.takeScreenshot(handler, screenShotMode);
    }
  }

  // for devtool scrollIntoViewFromUI
  @Override
  public void scrollIntoViewFromUI(int nodeId) {
    ILynxUIRenderer render = getLynxUIRenderer();
    if (render != null) {
      render.scrollIntoViewFromUI(nodeId);
    }
  }

  @Override
  public boolean supportsFullscreenScreenshot() {
    ILynxUIRenderer render = getLynxUIRenderer();
    return render != null && render.supportsFullscreenScreenshot();
  }

  // for devtool getNodeForLocation
  @Override
  public int getNodeForLocation(float x, float y, String mode) {
    ILynxUIRenderer render = getLynxUIRenderer();
    if (render != null) {
      return render.getNodeForLocation(x, y, mode);
    }
    return 0;
  }

  // will be called by devtool getBoxModel to obtain the true coordinates
  // of a transformed or non-transformed view in Android
  @Override
  public float[] getTransformValue(int id, float[] padBorderMarginLayout) {
    ILynxUIRenderer render = getLynxUIRenderer();
    if (render != null) {
      return render.getTransformValue(id, padBorderMarginLayout);
    }
    return new float[0];
  }

  @Override
  public Bitmap getBitmapOfView() {
    ILynxUIRenderer render = getLynxUIRenderer();
    if (render != null) {
      return render.getBitmapOfView();
    }
    return null;
  }

  @Override
  public boolean isClayRenderer() {
    ILynxUIRenderer renderer = getLynxUIRenderer();
    return renderer != null && renderer.getRenderType() == ILynxUIRenderer.RenderType.CLAY;
  }

  @Override
  public String getLynxUITree() {
    LynxTemplateRender render = mRender.get();
    return render != null ? render.getLynxUITree() : "";
  }

  @Override
  public String getUINodeInfo(int id) {
    LynxTemplateRender render = mRender.get();
    return render != null ? render.getUINodeInfo(id) : "";
  }

  @Override
  public int setUIStyle(int id, String name, String content) {
    LynxTemplateRender render = mRender.get();
    return render != null ? render.setUIStyle(id, name, content) : -1;
  }
}
