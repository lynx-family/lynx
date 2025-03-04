// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.behavior.LynxUIMethodConstants;
import java.util.HashMap;
import java.util.Map;

public class LynxGetUIResult {
  private LynxGetUIResult(JavaOnlyArray uiArray, int errCode, String errMsg) {
    this.uiArray = uiArray;
    this.errCode = errCode;
    this.errMsg = errMsg;
  }

  public boolean succeed() {
    return errCode == LynxUIMethodConstants.SUCCESS;
  }

  public ReadableArray getUiArray() {
    return uiArray;
  }

  public int getErrCode() {
    return errCode;
  }

  public String getErrMsg() {
    return errMsg;
  }

  private final int errCode;
  private final JavaOnlyArray uiArray;
  private final String errMsg;

  @CalledByNative
  private static LynxGetUIResult create(JavaOnlyArray uiArray, int errCode, String errMsg) {
    return new LynxGetUIResult(uiArray, errCode, errMsg);
  }
}
