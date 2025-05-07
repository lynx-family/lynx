// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.logbox;

import android.content.Context;
import android.content.res.AssetManager;
import android.text.TextUtils;
import com.lynx.basedevtool.utils.DevToolDownloader;
import com.lynx.basedevtool.utils.DownloadCallback;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;

class LogBoxFileLoadUtils {
  public static void loadFileFromUrl(String url, DevToolLogBoxCallback callback) {
    if (callback == null) {
      return;
    }
    if (TextUtils.isEmpty(url) || !url.startsWith("http")) {
      callback.onFailure("url is invalid");
      return;
    }
    DevToolDownloader downloader = new DevToolDownloader(url, new DownloadCallback() {
      @Override
      public void onData(byte[] bytes, int length) {
        String content = new String(bytes, Charset.defaultCharset());
        callback.onSuccess(content);
      }

      @Override
      public void onFailure(String reason) {
        callback.onFailure(reason);
      }
    });
  }

  public static void loadFileFromLocal(
      Context context, String path, DevToolLogBoxCallback callback) {
    if (callback == null) {
      return;
    }
    if (context == null) {
      callback.onFailure("Context is null");
      return;
    }
    AssetManager am = context.getAssets();
    if (am == null) {
      callback.onFailure("AssetManager is null");
      return;
    }
    if (TextUtils.isEmpty(path)) {
      callback.onFailure("path is empty");
      return;
    }
    // todo(yanghuiwen): load data on background thread
    try (InputStream is = am.open(path); BufferedReader reader = new BufferedReader(
                                             new InputStreamReader(is, StandardCharsets.UTF_8))) {
      StringBuilder sb = new StringBuilder();
      char[] buffer = new char[8192];
      int len;
      while ((len = reader.read(buffer, 0, buffer.length)) > 0) {
        sb.append(buffer, 0, len);
      }
      callback.onSuccess(sb.toString());
    } catch (IOException e) {
      callback.onFailure(e.getMessage());
    }
  }
}
