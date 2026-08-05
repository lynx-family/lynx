// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import androidx.appcompat.app.AppCompatActivity;
import com.lynx.explorer.shell.TemplateDispatcher;

public class DebugBridgeActivity extends AppCompatActivity {
  private static final String TAG = "DebugBridgeActivity";

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    Uri data = getIntent().getData();
    if (data == null || !"lynx".equals(data.getScheme())) {
      finish();
      return;
    }

    String host = data.getHost();

    // Handle lynx://open?url= scheme for runtime page switching.
    if ("open".equals(host)) {
      String targetUrl = data.getQueryParameter("url");
      if (targetUrl != null && !targetUrl.isEmpty()) {
        Log.d(TAG, "Opening URL via lynx://open: " + targetUrl);
        TemplateDispatcher.dispatchUrl(
            this, targetUrl, Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
      }
      finish();
      return;
    }

    // Handle lynx://lynxview_page?bundle=<name> deeplinks.
    // Try Sparkling first; fall back to legacy TasmDispatcher.
    if ("lynxview_page".equals(host)) {
      String bundle = data.getQueryParameter("bundle");
      if (bundle != null && !bundle.isEmpty()) {
        String hybridUrl = data.toString().replaceFirst("^lynx://", "hybrid://");
        if (!trySparklingNavigate(hybridUrl)) {
          String legacyUrl = hybridToLegacy(bundle, data);
          Log.d(TAG, "Sparkling unavailable, falling back to: " + legacyUrl);
          TemplateDispatcher.dispatchUrl(
              this, legacyUrl, Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
        }
      }
      finish();
      return;
    }

    finish();
  }

  /** Convert lynxview_page deeplink to legacy file://lynx?local:// format. */
  private String hybridToLegacy(String bundle, Uri data) {
    StringBuilder legacyUrl = new StringBuilder("file://lynx?local://");
    legacyUrl.append(bundle);
    StringBuilder remaining = new StringBuilder();
    for (String key : data.getQueryParameterNames()) {
      if (!"bundle".equals(key)) {
        String value = data.getQueryParameter(key);
        if (remaining.length() > 0) {
          remaining.append("&");
        }
        remaining.append(key).append("=").append(value != null ? value : "");
      }
    }
    if (remaining.length() > 0) {
      legacyUrl.append("?").append(remaining);
    }
    return legacyUrl.toString();
  }

  /** Attempt to navigate via Sparkling SDK using reflection. */
  private boolean trySparklingNavigate(String hybridUrl) {
    try {
      Class<?> contextClass = Class.forName("com.tiktok.sparkling.SparklingContext");
      Object ctx = contextClass.getConstructor().newInstance();
      contextClass.getMethod("setScheme", String.class).invoke(ctx, hybridUrl);

      Class<?> sparklingClass = Class.forName("com.tiktok.sparkling.Sparkling");
      Object builder =
          sparklingClass.getMethod("build", Activity.class, contextClass).invoke(null, this, ctx);
      builder.getClass().getMethod("navigate").invoke(builder);
      return true;
    } catch (Exception e) {
      Log.d(TAG, "Sparkling navigation not available: " + e.getMessage());
      return false;
    }
  }
}
