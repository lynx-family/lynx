// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.resourcelistener;

public class LynxResourceLoadFailedInfo {
  private String mErrorMsg;
  private int mErrorCode;
  private int mCategorizedCode;

  public LynxResourceLoadFailedInfo(String errMsg, int errCode, int categorizedCode) {
    mErrorMsg = errMsg;
    mErrorCode = errCode;
    mCategorizedCode = categorizedCode;
  }

  public String getErrorMsg() {
    return mErrorMsg;
  }

  public int getErrorCode() {
    return mErrorCode;
  }

  public int getCategorizedCode() {
    return mCategorizedCode;
  }
}
