// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtoolwrapper;

import android.graphics.Bitmap;
import com.lynx.react.bridge.ReadableMap;

public interface IDevToolDelegate {
  void onDispatchMessageEvent(ReadableMap map);

  // for devtool real-time screencast
  void takeScreenshot(ScreenshotBitmapHandler handler, String screenShotMode);

  // for devtool scrollIntoViewFromUI
  void scrollIntoViewFromUI(int nodeId);

  /**
   * for devtool inspector
   *
   * Retrieves the actual screenshot mode, which may not match the screenshot mode in DevToolStatus.
   * The reason is that not every rendering mode supports full-screen screenshots.
   * For iOS and Android native, the result of getActualScreenshotMode is consistent with the
   * screenshot mode stored in DevToolStatus.
   */
  String getActualScreenshotMode();

  // for devtool getNodeForLocation
  int getNodeForLocation(float x, float y, String mode);

  // will be called by devtool getBoxModel to obtain the true coordinates
  // of a transformed or non-transformed view in Android
  float[] getTransformValue(int id, float[] padBorderMarginLayout);

  // for devtool screen preview
  Bitmap getBitmapOfView();
}
