// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.recorder;
import static java.net.HttpURLConnection.HTTP_OK;

import android.util.Base64;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.component.DynamicComponentFetcher;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class LynxRecorderFetcher implements DynamicComponentFetcher {
  private static int count = 0;
  private static String ASSETS_SCHEMA = "assets://";
  private static final int HTTP_TIME_OUT = 5000; // milliseconds
  private static final int MIN_BASENAME_LENGTH = 5;

  private boolean disableSyncRequest = false;

  public void disableSyncRequest(boolean disable) {
    disableSyncRequest = disable;
  }

  private boolean shouldSendAsyncRequest() {
    if (disableSyncRequest) {
      return true;
    }
    return count++ % 2 == 0;
  }
  @Override
  public void loadDynamicComponent(String url, LoadedHandler handler) {
    try {
      JSONObject data = cachedParamsForURL(url);
      if (data != null) {
        byte[] res = Base64.decode(data.getString("source"), Base64.DEFAULT);
        if (data.getBoolean("sync_tag")) {
          handler.onComponentLoaded(res, null);
        } else {
          ThreadUtils.getThreadPool().execute(() -> { handler.onComponentLoaded(res, null); });
        }
      } else {
        if (shouldSendAsyncRequest()) {
          // async
          ThreadUtils.getThreadPool().execute(new Runnable() {
            @Override
            public void run() {
              try {
                handler.onComponentLoaded(loadDynamicComponentTemplate(url), null);
              } catch (Throwable e) {
                handler.onComponentLoaded(null, e);
              }
            }
          });
        } else {
          // sync
          byte[] res = null;
          Future<byte[]> future = ThreadUtils.getThreadPool().submit(new Callable() {
            @Override
            public byte[] call() {
              try {
                return loadDynamicComponentTemplate(url);
              } catch (Throwable e) {
                return null;
              }
            }
          });
          try {
            handler.onComponentLoaded(future.get(HTTP_TIME_OUT, TimeUnit.MILLISECONDS), null);
          } catch (InterruptedException | ExecutionException | TimeoutException e) {
            e.printStackTrace();
            handler.onComponentLoaded(null, e);
          }
        }
      }
    } catch (JSONException e) {
      e.printStackTrace();
      handler.onComponentLoaded(null, e);
    }
  }

  private HashMap<String, JSONObject> mDynamicMap;
  private HashMap<String, String> mUrlRedirectMap;
  public LynxRecorderFetcher() {
    // disable sync request dynamic component template to to ensure the stability of test result
    disableSyncRequest(true);
    mDynamicMap = new HashMap<>();
    mUrlRedirectMap = new HashMap<>();
  }

  public void setUrlRedirectMap(HashMap<String, String> redirectMap) {
    mUrlRedirectMap = redirectMap != null ? redirectMap : new HashMap<>();
  }

  public void parse(JSONArray actionList) {
    for (int i = 0; i < actionList.length(); ++i) {
      try {
        JSONObject action = actionList.getJSONObject(i);
        JSONObject params = action.getJSONObject("Params");
        if (action.getString("Function Name").equals("LoadComponentWithCallback")) {
          mDynamicMap.put(params.getString("url"), params);
        }
      } catch (JSONException e) {
        e.printStackTrace();
      }
    }
  }

  private JSONObject cachedParamsForURL(String url) {
    if (url == null || url.isEmpty()) {
      return null;
    }
    JSONObject cached = mDynamicMap.get(url);
    if (cached != null) {
      return cached;
    }
    String lookupUrl = url;
    if (mUrlRedirectMap.containsKey(url)) {
      String redirected = mUrlRedirectMap.get(url);
      if (redirected != null && !redirected.isEmpty()) {
        cached = mDynamicMap.get(redirected);
        if (cached != null) {
          return cached;
        }
        lookupUrl = redirected;
      }
    }
    String path = normalizedPath(lookupUrl);
    if (path != null && !path.isEmpty()) {
      cached = mDynamicMap.get(path);
      if (cached != null) {
        return cached;
      }
    }
    String basename = extractBasename(path != null ? path : lookupUrl);
    if (basename.length() >= MIN_BASENAME_LENGTH) {
      String bestKey = null;
      for (Map.Entry<String, JSONObject> entry : mDynamicMap.entrySet()) {
        String key = entry.getKey();
        if (key.endsWith(basename) || lookupUrl.endsWith(key)
            || (path != null && path.endsWith(key))) {
          if (bestKey == null || key.length() > bestKey.length()) {
            bestKey = key;
          }
        }
      }
      if (bestKey != null) {
        return mDynamicMap.get(bestKey);
      }
    }
    return null;
  }

  private static String normalizedPath(String url) {
    if (url == null || url.isEmpty()) {
      return null;
    }
    if (url.startsWith("/")) {
      return url;
    }
    try {
      URL parsed = new URL(url);
      String path = parsed.getPath();
      return (path == null || path.isEmpty()) ? null : path;
    } catch (Exception ignored) {
      return null;
    }
  }

  private static String extractBasename(String path) {
    if (path == null || path.isEmpty()) {
      return "";
    }
    int lastSlash = path.lastIndexOf('/');
    if (lastSlash >= 0 && lastSlash + 1 < path.length()) {
      return path.substring(lastSlash + 1);
    }
    return path;
  }

  private static byte[] loadDynamicComponentTemplate(String urlStr) throws Exception {
    if (urlStr.startsWith(ASSETS_SCHEMA)) {
      return loadDynamicComponentFromAssets(urlStr.substring(ASSETS_SCHEMA.length()));
    }
    URL url = new URL(urlStr);
    HttpURLConnection conn = (HttpURLConnection) url.openConnection();
    conn.setRequestMethod("GET");
    conn.setReadTimeout(HTTP_TIME_OUT);

    if (conn.getResponseCode() == HTTP_OK) {
      return Utils.inputStreamToByteArray(conn.getInputStream());
    }
    return null;
  }

  private static byte[] loadDynamicComponentFromAssets(String path) throws IOException {
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
