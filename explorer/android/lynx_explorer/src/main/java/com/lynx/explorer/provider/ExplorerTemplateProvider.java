// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.provider;

import android.content.Context;
import android.content.res.AssetManager;
import android.os.Handler;
import android.os.Looper;
import com.lynx.tasm.provider.AbsTemplateProvider;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

/** Loads Explorer-packaged templates from assets and delegates absolute remote URLs. */
public final class ExplorerTemplateProvider extends AbsTemplateProvider {
  private static final long LOCAL_DELIVERY_DELAY_MS = 250L;
  private static final String SPARKLING_ASSET_ROOT = "extensions/sparkling-go/";

  private final AssetManager assets;
  private final DemoTemplateProvider remoteProvider = new DemoTemplateProvider();
  private final Handler mainHandler = new Handler(Looper.getMainLooper());

  public ExplorerTemplateProvider(Context context) {
    assets = context.getApplicationContext().getAssets();
  }

  @Override
  public void loadTemplate(String url, Callback callback) {
    if (url.startsWith("http://") || url.startsWith("https://")) {
      remoteProvider.loadTemplate(url, callback);
      return;
    }

    String assetPath = normalizeAssetPath(url);
    try (InputStream input = openBundledTemplate(assetPath)) {
      byte[] template = readAllBytes(input);
      // Sparkling creates its LynxView and starts loading before its asynchronous runtime is
      // ready. Network providers naturally bridge that window; local assets need the same turn.
      mainHandler.postDelayed(() -> callback.onSuccess(template), LOCAL_DELIVERY_DELAY_MS);
    } catch (IOException error) {
      callback.onFailed(
          "Unable to load bundled Explorer template '" + assetPath + "': " + error.getMessage());
    }
  }

  private InputStream openBundledTemplate(String assetPath) throws IOException {
    try {
      return assets.open(assetPath);
    } catch (IOException rootError) {
      // sparkling-navigation resolves sibling pages by bundle filename. Android assets have no
      // implicit base URL, so retry bare bundle names beside the packaged Sparkling Go entry.
      if (!assetPath.contains("/") && assetPath.endsWith(".lynx.bundle")) {
        return assets.open(SPARKLING_ASSET_ROOT + assetPath);
      }
      throw rootError;
    }
  }

  private static String normalizeAssetPath(String url) {
    String path = url;
    String[] prefixes = {"file://lynx?local://", "local://", "./Resource/", "/"};
    for (String prefix : prefixes) {
      if (path.startsWith(prefix)) {
        path = path.substring(prefix.length());
      }
    }
    return path;
  }

  private static byte[] readAllBytes(InputStream input) throws IOException {
    ByteArrayOutputStream output = new ByteArrayOutputStream();
    byte[] buffer = new byte[16 * 1024];
    int count;
    while ((count = input.read(buffer)) != -1) {
      output.write(buffer, 0, count);
    }
    return output.toByteArray();
  }
}
