// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.recorder;

import static java.net.HttpURLConnection.HTTP_OK;

import android.util.Base64;
import android.util.Base64InputStream;
import androidx.annotation.NonNull;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.provider.LynxResourceCallback;
import com.lynx.tasm.provider.LynxResourceProvider;
import com.lynx.tasm.provider.LynxResourceRequest;
import com.lynx.tasm.provider.LynxResourceResponse;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.zip.InflaterInputStream;
import org.json.JSONObject;

public class LynxRecorderSourceProvider extends LynxResourceProvider<Object, byte[]> {
  private static final String TAG = "LynxRecorderSourceProvider";

  private static final String ASSETS_SCHEME = "assets://";
  private static final int HTTP_TIME_OUT = 5000; // milliseconds
  private JSONObject mUrlRedirect = null;
  private JSONObject mOfflineScripts = null;

  public void setUrlRedirect(JSONObject urlRedirect) {
    mUrlRedirect = urlRedirect;
  }

  public void setOfflineScripts(JSONObject offlineScripts) {
    mOfflineScripts = offlineScripts;
  }

  @Override
  public void request(@NonNull final LynxResourceRequest<Object> request,
      @NonNull final LynxResourceCallback<byte[]> callback) {
    LLog.i(TAG, "LynxRecorderSourceProvider request " + request.getUrl());
    byte[] data = null;
    Throwable error = null;
    try {
      String requestUrl = request.getUrl();
      data = requestFromOfflineScripts(requestUrl);

      if (data != null) {
        // Recorded scripts take priority over redirected/network fetches.
      } else if (requestUrl.length() > ASSETS_SCHEME.length()
          && requestUrl.startsWith(ASSETS_SCHEME)) {
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
      LLog.i(TAG, "LynxRecorderSourceProvider request failed, error:" + error);
      callback.onResponse(LynxResourceResponse.failed(LynxResourceResponse.FAILED, error));
    } else {
      LLog.i(TAG, "LynxRecorderSourceProvider request successfully");
      callback.onResponse(LynxResourceResponse.success(data));
    }
  }

  private byte[] requestFromOfflineScripts(@NonNull String requestUrl) throws IOException {
    if (mOfflineScripts == null || !mOfflineScripts.has(requestUrl)) {
      return null;
    }
    String script = mOfflineScripts.optString(requestUrl, null);
    if (script == null) {
      return null;
    }
    return decodeRecordedScript(script);
  }

  private static byte[] decodeRecordedScript(@NonNull String script) throws IOException {
    byte[] encoded = script.getBytes(StandardCharsets.UTF_8);
    try (InputStream inputStream = new InflaterInputStream(
             new Base64InputStream(new ByteArrayInputStream(encoded), Base64.DEFAULT))) {
      return Utils.inputStreamToByteArray(inputStream);
    } catch (IOException inflateError) {
      try {
        return Base64.decode(script, Base64.DEFAULT);
      } catch (IllegalArgumentException decodeError) {
        return script.getBytes(StandardCharsets.UTF_8);
      }
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
