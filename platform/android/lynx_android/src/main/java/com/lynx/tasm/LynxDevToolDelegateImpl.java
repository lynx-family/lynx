// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import android.graphics.Bitmap;
import com.lynx.devtoolwrapper.IDevToolDelegate;
import com.lynx.devtoolwrapper.ScreenshotBitmapHandler;
import com.lynx.devtoolwrapper.ScreenshotMode;
import com.lynx.react.bridge.ReadableMap;
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

  // for devtool real-time screencast
  @Override
  public void takeScreenshot(ScreenshotBitmapHandler handler, String screenShotMode) {
    LynxTemplateRender render = mRender.get();
    LynxView lynxView = render.mLynxView;
    if (lynxView != null) {
      if (lynxView.lynxUIRenderer() != null) {
        lynxView.lynxUIRenderer().takeScreenshot(handler, screenShotMode);
      }
    }
  }

  // for devtool scrollIntoViewFromUI
  @Override
  public void scrollIntoViewFromUI(int nodeId) {
    LynxTemplateRender render = mRender.get();
    LynxView lynxView = render.mLynxView;
    if (lynxView != null) {
      if (lynxView.lynxUIRenderer() != null) {
        lynxView.lynxUIRenderer().scrollIntoViewFromUI(nodeId);
      }
    }
  }

  /**
   * for devtool inspector
   *
   * Retrieves the actual screenshot mode, which may not match the screenshot mode in DevToolStatus.
   * The reason is that not every rendering mode supports full-screen screenshots.
   * For iOS and Android native, the result of getActualScreenshotMode is consistent with the
   * screenshot mode stored in DevToolStatus.
   */
  @Override
  public String getActualScreenshotMode() {
    LynxTemplateRender render = mRender.get();
    LynxView lynxView = render.mLynxView;
    if (lynxView != null) {
      if (lynxView.lynxUIRenderer() != null) {
        return lynxView.lynxUIRenderer().getActualScreenshotMode();
      }
    }
    // The default screenshot mode is full screen.
    return ScreenshotMode.SCREEN_SHOT_MODE_FULL_SCREEN;
  }

  // for devtool getNodeForLocation
  @Override
  public int getNodeForLocation(float x, float y, String mode) {
    LynxTemplateRender render = mRender.get();
    LynxView lynxView = render.mLynxView;
    if (lynxView != null) {
      if (lynxView.lynxUIRenderer() != null) {
        return lynxView.lynxUIRenderer().getNodeForLocation(x, y, mode);
      }
    }
    return 0;
  }

  // will be called by devtool getBoxModel to obtain the true coordinates
  // of a transformed or non-transformed view in Android
  @Override
  public float[] getTransformValue(int id, float[] padBorderMarginLayout) {
    LynxTemplateRender render = mRender.get();
    LynxView lynxView = render.mLynxView;
    if (lynxView != null) {
      if (lynxView.lynxUIRenderer() != null) {
        return lynxView.lynxUIRenderer().getTransformValue(id, padBorderMarginLayout);
      }
    }
    return new float[0];
  }

  @Override
  public Bitmap getBitmapOfView() {
    LynxTemplateRender render = mRender.get();
    LynxView lynxView = render.mLynxView;
    if (lynxView != null) {
      if (lynxView.lynxUIRenderer() != null) {
        return lynxView.lynxUIRenderer().getBitmapOfView();
      }
    }
    return null;
  }
}
