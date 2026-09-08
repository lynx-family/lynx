// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.sparkling;

import android.content.Context;
import android.util.Log;
import com.lynx.explorer.routes.LaunchDescriptor;
import com.tiktok.sparkling.Sparkling;
import com.tiktok.sparkling.SparklingContext;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.Map;

final class SparklingRouteLauncherImpl {
  private static final String TAG = "SparklingRouteLauncher";
  private SparklingRouteLauncherImpl() {}

  static boolean open(Context context, LaunchDescriptor descriptor) {
    try {
      SparklingContext sparklingContext = new SparklingContext();
      sparklingContext.scheme = descriptor.originalUrl;
      setIfPresent(sparklingContext, "url", descriptor.resourceUrl);
      setIfPresent(sparklingContext, "resourceUrl", descriptor.resourceUrl);
      setIfPresent(sparklingContext, "pageName", descriptor.pageName);
      Map<String, Object> launchProps = new HashMap<>(descriptor.globalProps);
      launchProps.put("explorerContainer", "Sparkling");
      launchProps.put("sparklingNavigation", true);
      launchProps.put("sparklingAvailable", true);
      launchProps.put("resourceUrl", descriptor.resourceUrl);
      setIfPresent(sparklingContext, "globalProps", launchProps);
      setIfPresent(sparklingContext, "initialData", descriptor.initialData);
      Sparkling.processSparklingContext(sparklingContext);
      boolean navigated = Sparkling.build(context.getApplicationContext(), sparklingContext).navigate();
      if (!navigated) Log.e(TAG, "Sparkling.navigate returned false for " + descriptor.originalUrl);
      return navigated;
    } catch (Throwable throwable) {
      Log.e(TAG, "Sparkling route failed for " + descriptor.originalUrl, throwable);
      return false;
    }
  }

  private static void setIfPresent(SparklingContext context, String property, Object value) {
    if (value == null) return;
    String suffix = property.substring(0, 1).toUpperCase() + property.substring(1);
    for (Method method : context.getClass().getMethods()) {
      if (method.getName().equals("set" + suffix) && method.getParameterTypes().length == 1) {
        try { method.invoke(context, value); } catch (Throwable ignored) {}
        return;
      }
    }
  }
}
