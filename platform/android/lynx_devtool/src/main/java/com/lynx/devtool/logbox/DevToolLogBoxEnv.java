// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.logbox;

import android.content.Context;
import android.text.TextUtils;
import java.util.HashMap;
import java.util.Map;

class DevToolLogBoxEnv {
  private static final String TAG = "DevToolLogBoxEnv";
  private final Map<String, ILogBoxErrorParserLoader> mErrorParserLoaders;

  public interface ILogBoxErrorParserLoader {
    void loadErrorParser(Context context, DevToolLogBoxCallback callback);
  }

  public static DevToolLogBoxEnv inst() {
    return SingletonHolder.INSTANCE;
  }

  private static class SingletonHolder {
    private static final DevToolLogBoxEnv INSTANCE = new DevToolLogBoxEnv();
  }

  private DevToolLogBoxEnv() {
    mErrorParserLoaders = new HashMap<>();
  }

  public void registerErrorParserLoader(String namespace, ILogBoxErrorParserLoader loader) {
    if (TextUtils.isEmpty(namespace) || mErrorParserLoaders.get(namespace) != null
        || loader == null) {
      return;
    }
    synchronized (mErrorParserLoaders) {
      if (mErrorParserLoaders.get(namespace) == null) {
        mErrorParserLoaders.put(namespace, loader);
      }
    }
  }

  protected void loadErrorParser(
      Context context, String namespace, DevToolLogBoxCallback callback) {
    ILogBoxErrorParserLoader loader = mErrorParserLoaders.get(namespace);
    if (loader != null) {
      loader.loadErrorParser(context, callback);
    }
  }
}
