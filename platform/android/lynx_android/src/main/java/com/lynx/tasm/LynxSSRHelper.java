// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;
import com.lynx.react.bridge.JavaOnlyArray;

public class LynxSSRHelper {
  private enum SSRHydrateStatus { UNDEFINED, PENDING, BEGINNING, FAILED, SUCCESSFUL }
  private SSRHydrateStatus mHydrateStatus = SSRHydrateStatus.UNDEFINED;
  private static final String CACHE_IDENTIFY = "from_ssr_cache";

  public void onLoadSSRDataBegan(String url) {
    mHydrateStatus = SSRHydrateStatus.PENDING;
  }

  public void onHydrateBegan() {
    mHydrateStatus = SSRHydrateStatus.PENDING;
  }

  public void onHydrateFinished() {
    mHydrateStatus = SSRHydrateStatus.SUCCESSFUL;
  }

  public void onErrorOccurred(int type, LynxError lynxError) {
    int errorCode = lynxError.getErrorCode();
    if (errorCode == LynxErrorBehavior.EB_APP_BUNDLE_LOAD) {
      mHydrateStatus = SSRHydrateStatus.FAILED;
    }
  }

  public boolean isHydratePending() {
    return mHydrateStatus == SSRHydrateStatus.PENDING;
  }

  public JavaOnlyArray processEventParams(JavaOnlyArray params) {
    JavaOnlyArray finalParams;
    if (params != null) {
      finalParams = params;
    } else {
      finalParams = new JavaOnlyArray();
    }
    finalParams.pushString(CACHE_IDENTIFY);
    return finalParams;
  }

  public boolean shouldSendEventToSSR() {
    if (mHydrateStatus == SSRHydrateStatus.PENDING || mHydrateStatus == SSRHydrateStatus.BEGINNING
        || mHydrateStatus == SSRHydrateStatus.FAILED) {
      return true;
    }
    return false;
  }
}
