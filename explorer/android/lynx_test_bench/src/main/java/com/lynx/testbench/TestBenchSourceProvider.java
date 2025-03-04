// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.testbench;

import static java.net.HttpURLConnection.HTTP_OK;

import androidx.annotation.NonNull;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.provider.LynxResourceCallback;
import com.lynx.tasm.provider.LynxResourceProvider;
import com.lynx.tasm.provider.LynxResourceRequest;
import com.lynx.tasm.provider.LynxResourceResponse;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import org.json.JSONObject;

public class TestBenchSourceProvider extends LynxResourceProvider<Object, byte[]> {
  private static final String TAG = "TestBenchSourceProvider";

  private static final String ASSETS_SCHEME = "assets://";
  private static final int HTTP_TIME_OUT = 5000; // milliseconds
  private JSONObject mUrlRedirect = null;

  public void setUrlRedirect(JSONObject urlRedirect) {
    mUrlRedirect = urlRedirect;
  }
  @Override
  public void request(@NonNull final LynxResourceRequest<Object> request,
      @NonNull final LynxResourceCallback<byte[]> callback) {
    LLog.i(TAG, "TestBenchSourceProvider request " + request.getUrl());
    byte[] data = null;
    Throwable error = null;
    try {
      String requestUrl = request.getUrl();

      if (requestUrl.length() > ASSETS_SCHEME.length() && requestUrl.startsWith(ASSETS_SCHEME)) {
        data = requestFromAssets(requestUrl.substring(ASSETS_SCHEME.length()));
      } else {
        if (mUrlRedirect != null && mUrlRedirect.has(requestUrl)) {
          requestUrl = mUrlRedirect.getString(requestUrl);
        }
        data = requestFromURL(requestUrl);
      }
    } catch (Throwable e) {
      error = e;
    }

    if (error != null) {
      LLog.i(TAG, "TestBenchSourceProvider request failed, error:" + error);
      callback.onResponse(LynxResourceResponse.failed(LynxResourceResponse.FAILED, error));
    } else {
      LLog.i(TAG, "TestBenchSourceProvider request successfully");
      callback.onResponse(LynxResourceResponse.success(data));
    }
  }

  private byte[] requestFromURL(@NonNull String requestUrl) throws IOException {
    URL url = new URL(requestUrl);
    HttpURLConnection conn = (HttpURLConnection) url.openConnection();
    conn.setRequestMethod("GET");
    conn.setReadTimeout(HTTP_TIME_OUT);
    if (conn.getResponseCode() == HTTP_OK) {
      return Utils.inputStreamToByteArray(conn.getInputStream());
    } else {
      throw new IOException(conn.getResponseMessage());
    }
  }

  private byte[] requestFromAssets(@NonNull String path) throws IOException {
    try (InputStream in = LynxEnv.inst().getAppContext().getAssets().open(path);
         ByteArrayOutputStream out = new ByteArrayOutputStream()) {
      byte[] buffer = new byte[1024 * 4];
      int n;
      while ((n = in.read(buffer)) != -1) {
        out.write(buffer, 0, n);
      }
      return out.toByteArray();
    }
  }
}
