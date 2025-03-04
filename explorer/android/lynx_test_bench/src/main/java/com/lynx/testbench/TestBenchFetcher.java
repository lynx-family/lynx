// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.testbench;
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
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class TestBenchFetcher implements DynamicComponentFetcher {
  private static int count = 0;
  private static String ASSETS_SCHEMA = "assets://";
  private static final int HTTP_TIME_OUT = 5000; // milliseconds

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
      if (mDynamicMap.containsKey(url)) {
        JSONObject data = mDynamicMap.get(url);
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
  public TestBenchFetcher() {
    // disable sync request dynamic component template to to ensure the stability of test result
    disableSyncRequest(true);
    mDynamicMap = new HashMap<>();
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
