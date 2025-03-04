// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.network;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class LynxDevToolDownloader implements Runnable {
  private static final int CONNECTION_TIMEOUT = 1440; // 1.44s
  private static final int READ_TIMEOUT = 120000; // 120s
  private static final int NORMAL_RESPONSE_CODE = 299;
  private static final int BUFFER_SIZE = 8192;

  private static final ExecutorService sExecutorService = Executors.newCachedThreadPool();

  private final String mUrl;
  private final DownloadCallback mCallback;
  private final ByteArrayOutputStream mBuffer;

  public LynxDevToolDownloader(String url, DownloadCallback callback) {
    this.mUrl = url;
    this.mCallback = callback;
    this.mBuffer = new ByteArrayOutputStream();
    sExecutorService.execute(this);
  }

  @Override
  public void run() {
    try {
      URL u = new URL(mUrl);
      HttpURLConnection conn = (HttpURLConnection) u.openConnection();
      conn.setConnectTimeout(CONNECTION_TIMEOUT);
      conn.setReadTimeout(READ_TIMEOUT);
      conn.setDoOutput(false);
      conn.setDoInput(true);
      conn.connect();

      conn.setInstanceFollowRedirects(true);
      int res = conn.getResponseCode();
      if (res > NORMAL_RESPONSE_CODE) { // fail
        conn.disconnect();
        onFailure("Download connection failed and the response code is " + res);
        return;
      }
      int contentLength = conn.getContentLength();
      onResponse(res, contentLength);
      // Currently, the app will crash when the downloaded data is too large.
      // So we temporarily limit the maximum size of data that can be downloaded
      long maxDataSize = getSafeDataSize();
      if (contentLength > maxDataSize) {
        conn.disconnect();
        onFailure(
            "The size of the downloaded data has exceeded the maximum safe memory size, which may pose a risk of OOM. Currently, the content size is "
            + contentLength + " byte, and the safe memory available for use is " + maxDataSize);
        return;
      }

      InputStream in = conn.getInputStream();
      byte[] buf = new byte[BUFFER_SIZE];
      for (;;) {
        int readBytes = in.read(buf);
        if (readBytes == -1) {
          break;
        }
        mBuffer.write(buf, 0, readBytes);
      }
      onData(mBuffer.toByteArray(), mBuffer.size());
    } catch (Exception e) {
      onFailure("An exception occurred when download: " + e.getMessage());
    }
  }

  private void onFailure(String reason) {
    if (mCallback != null) {
      mCallback.onFailure(reason);
    }
  }

  private void onResponse(int status, int contentLength) {
    if (mCallback != null) {
      mCallback.onResponse(status, contentLength);
    }
  }

  private void onData(byte[] bytes, int length) {
    if (mCallback != null) {
      mCallback.onData(bytes, length);
    }
  }

  // In certain scenarios, once data downloading is completed, it can consume memory
  // up to 9 times the size of the original data, which easily lead to an OOM.
  // Therefore, it is necessary to check whether the size of the downloading data
  // exceeds the memory size that can be safely used.
  private long getSafeDataSize() {
    Runtime r = Runtime.getRuntime();
    return (r.maxMemory() - r.totalMemory() + r.freeMemory()) / 10;
  }
}
