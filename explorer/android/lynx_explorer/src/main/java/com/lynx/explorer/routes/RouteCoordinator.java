// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.routes;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.util.Log;
import com.lynx.explorer.LynxViewShellActivity;
import com.lynx.explorer.shell.TemplateDispatcher;
import com.lynx.explorer.sparkling.SparklingRouteLauncher;

public final class RouteCoordinator {
  public static final String EXTRA_OPEN_WITH_SPARKLING = "lynx_open_with_sparkling";
  private static final String TAG = "RouteCoordinator";

  private RouteCoordinator() {}

  public static void open(Context context, String url) { open(context, url, false, Intent.FLAG_ACTIVITY_NEW_TASK); }

  public static void open(Context context, String url, boolean openWithSparkling, int flags) {
    if (url == null || url.trim().isEmpty()) {
      Log.e(TAG, "Cannot route empty URL");
      return;
    }
    String routedUrl = unwrapNestedSparkling(url);
    if (LaunchDescriptor.isSparklingCanonicalUrl(routedUrl)) {
      launchSparkling(context, LaunchDescriptor.fromSparklingCanonicalUrl(routedUrl).build(), flags);
      return;
    }
    LaunchDescriptor descriptor = LaunchDescriptor.fromLegacyUrl(routedUrl,
        openWithSparkling ? LaunchDescriptor.ContainerType.SPARKLING : LaunchDescriptor.ContainerType.LEGACY).build();
    if (openWithSparkling) {
      launchSparkling(context, descriptor, flags);
    } else {
      TemplateDispatcher.dispatchLegacyUrl(context, descriptor.toLegacyUrl(), flags);
    }
  }

  public static void openFromIntent(Context context, Intent intent) {
    String url = intent.getStringExtra("lynx_initial_url");
    if (url == null || url.isEmpty()) url = intent.getStringExtra(LynxViewShellActivity.URL_KEY);
    if ((url == null || url.isEmpty()) && intent.getData() != null) {
      url = intent.getData().getQueryParameter(LynxViewShellActivity.URL_KEY);
      if (url == null) url = intent.getDataString();
    }
    open(context, url, intent.getBooleanExtra(EXTRA_OPEN_WITH_SPARKLING, false), Intent.FLAG_ACTIVITY_NEW_TASK);
  }

  private static void launchSparkling(Context context, LaunchDescriptor descriptor, int flags) {
    if (!SparklingRouteLauncher.open(context, descriptor)) {
      Log.e(TAG, "Sparkling route failed; not falling back to Legacy: " + descriptor.originalUrl);
    }
  }

  private static String unwrapNestedSparkling(String url) {
    try {
      Uri uri = Uri.parse(url);
      for (String key : new String[] {"url", "bundle", "resource", "schema", "scheme"}) {
        String value = uri.getQueryParameter(key);
        if (LaunchDescriptor.isSparklingCanonicalUrl(value)) return value;
      }
    } catch (Throwable ignored) {}
    return url;
  }
}
