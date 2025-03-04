// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.helper;

import com.lynx.devtool.network.DownloadCallback;
import com.lynx.devtool.network.LynxDevToolDownloader;
import com.lynx.tasm.base.LLog;
import java.nio.charset.Charset;
import java.util.concurrent.atomic.AtomicBoolean;

public class LepusDebugInfoHelper {
  private static final String TAG = "LepusDebugInfoHelper";

  private AtomicBoolean mIsLoading;
  private String mDebugInfoUrl;
  private String mDebugInfo;

  public LepusDebugInfoHelper() {
    mIsLoading = new AtomicBoolean(false);
  }

  public void setDebugInfoUrl(String url) {
    mDebugInfoUrl = url;
  }

  public String getDebugInfoUrl() {
    return mDebugInfoUrl;
  }

  public String getDebugInfo(String url) {
    LLog.i(TAG, "lepus debug: debug info url: " + url);
    setDebugInfoUrl(url);
    mIsLoading.set(true);
    downloadDebugInfo();

    try {
      // Wait for downloading
      while (mIsLoading.get()) {
        Thread.sleep(10);
      }
    } catch (InterruptedException e) {
      LLog.e(TAG, e.toString());
    }

    return mDebugInfo;
  }

  private void downloadDebugInfo() {
    new LynxDevToolDownloader(mDebugInfoUrl, new DownloadCallback() {
      @Override
      public void onResponse(int status, int contentLength) {}
      @Override
      public void onData(byte[] bytes, int length) {
        mDebugInfo = new String(bytes, Charset.defaultCharset());
        mIsLoading.set(false);
      }
      @Override
      public void onFailure(String reason) {
        mDebugInfo = "";
        mIsLoading.set(false);
        LLog.e(TAG, "lepus debug: download debug info failed, the reason is: " + reason);
      }
    });
  }
}
