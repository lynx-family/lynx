// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.recorder;

import static java.net.HttpURLConnection.HTTP_OK;

import android.util.Base64;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
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
import java.util.HashMap;
import java.util.Map;
import java.util.zip.InflaterInputStream;
import org.json.JSONException;
import org.json.JSONObject;

public class LynxRecorderSourceProvider extends LynxResourceProvider<Object, byte[]> {
  private static final String TAG = "LynxRecorderSourceProvider";

  private static final String ASSETS_SCHEME = "assets://";
  private static final int HTTP_TIME_OUT = 5000; // milliseconds
  private static final int MIN_BASENAME_LENGTH = 5;
  private JSONObject mUrlRedirect = null;
  private JSONObject mOfflineScripts = null;
  private final Map<String, byte[]> mDecodedOfflineScripts = new HashMap<>();

  public void setUrlRedirect(JSONObject urlRedirect) {
    mUrlRedirect = urlRedirect;
  }

  public void setOfflineScripts(@Nullable JSONObject offlineScripts) {
    mOfflineScripts = offlineScripts;
    mDecodedOfflineScripts.clear();
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
        callback.onResponse(LynxResourceResponse.success(data));
        return;
      }

      if (requestUrl.length() > ASSETS_SCHEME.length() && requestUrl.startsWith(ASSETS_SCHEME)) {
        data = requestFromAssets(requestUrl.substring(ASSETS_SCHEME.length()));
      } else {
        if (mUrlRedirect != null && mUrlRedirect.has(requestUrl)) {
          requestUrl = mUrlRedirect.getString(requestUrl);
          data = requestFromOfflineScripts(requestUrl);
          if (data != null) {
            callback.onResponse(LynxResourceResponse.success(data));
            return;
          }
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

  private @Nullable byte[] requestFromOfflineScripts(@NonNull String requestUrl)
      throws JSONException, IOException {
    if (mOfflineScripts == null) {
      return null;
    }
    for (String candidateUrl : buildOfflineScriptCandidates(requestUrl)) {
      if (!mOfflineScripts.has(candidateUrl)) {
        continue;
      }
      if (mDecodedOfflineScripts.containsKey(candidateUrl)) {
        return mDecodedOfflineScripts.get(candidateUrl);
      }
      String encodedScript = mOfflineScripts.getString(candidateUrl);
      byte[] scriptBytes = decodeRecordedScript(encodedScript);
      mDecodedOfflineScripts.put(candidateUrl, scriptBytes);
      LLog.i(TAG,
          "LynxRecorderSourceProvider hit offline script cache " + candidateUrl + " for request "
              + requestUrl);
      return scriptBytes;
    }
    String normalizedPath = normalizedPath(requestUrl);
    if (normalizedPath != null && !normalizedPath.equals(requestUrl)) {
      for (String candidateUrl : buildOfflineScriptCandidates(normalizedPath)) {
        if (!mOfflineScripts.has(candidateUrl)) {
          continue;
        }
        if (mDecodedOfflineScripts.containsKey(candidateUrl)) {
          return mDecodedOfflineScripts.get(candidateUrl);
        }
        String encodedScript = mOfflineScripts.getString(candidateUrl);
        byte[] scriptBytes = decodeRecordedScript(encodedScript);
        mDecodedOfflineScripts.put(candidateUrl, scriptBytes);
        return scriptBytes;
      }
    }
    String basename = extractBasename(normalizedPath != null ? normalizedPath : requestUrl);
    if (basename.length() >= MIN_BASENAME_LENGTH) {
      String bestKey = null;
      for (java.util.Iterator<String> it = mOfflineScripts.keys(); it.hasNext();) {
        String key = it.next();
        if (key.endsWith(basename) || requestUrl.endsWith(key)
            || (normalizedPath != null && normalizedPath.endsWith(key))) {
          if (bestKey == null || key.length() > bestKey.length()) {
            bestKey = key;
          }
        }
      }
      if (bestKey != null) {
        if (mDecodedOfflineScripts.containsKey(bestKey)) {
          return mDecodedOfflineScripts.get(bestKey);
        }
        byte[] scriptBytes = decodeRecordedScript(mOfflineScripts.getString(bestKey));
        mDecodedOfflineScripts.put(bestKey, scriptBytes);
        return scriptBytes;
      }
    }
    return null;
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

  private static byte[] decodeRecordedScript(@NonNull String encodedScript) throws IOException {
    try {
      byte[] compressedData = Base64.decode(encodedScript, Base64.DEFAULT);
      try (InputStream in = new InflaterInputStream(new ByteArrayInputStream(compressedData));
           ByteArrayOutputStream out = new ByteArrayOutputStream()) {
        byte[] buffer = new byte[1024 * 4];
        int n;
        while ((n = in.read(buffer)) != -1) {
          out.write(buffer, 0, n);
        }
        return out.toByteArray();
      } catch (IOException inflateError) {
        return compressedData;
      }
    } catch (IllegalArgumentException decodeError) {
      return encodedScript.getBytes(StandardCharsets.UTF_8);
    }
  }

  private static String[] buildOfflineScriptCandidates(@NonNull String requestUrl) {
    try {
      URL url = new URL(requestUrl);
      String path = url.getPath();
      String query = url.getQuery();
      if (query == null || query.isEmpty()) {
        return new String[] {requestUrl, path};
      }
      return new String[] {requestUrl, path + "?" + query, path};
    } catch (Exception ignored) {
      return new String[] {requestUrl};
    }
  }

  private static @Nullable String normalizedPath(@NonNull String requestUrl) {
    if (requestUrl.isEmpty()) {
      return null;
    }
    if (requestUrl.startsWith("/")) {
      int queryPos = requestUrl.indexOf('?');
      return queryPos >= 0 ? requestUrl.substring(0, queryPos) : requestUrl;
    }
    try {
      URL url = new URL(requestUrl);
      String path = url.getPath();
      if (path == null || path.isEmpty()) {
        return null;
      }
      return path;
    } catch (Exception ignored) {
      return null;
    }
  }

  private static String extractBasename(@NonNull String path) {
    int lastSlash = path.lastIndexOf('/');
    if (lastSlash >= 0 && lastSlash + 1 < path.length()) {
      return path.substring(lastSlash + 1);
    }
    return path;
  }
}
