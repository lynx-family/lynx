// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.image;

import android.content.Context;
import android.text.TextUtils;
import androidx.annotation.NonNull;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.behavior.ImageInterceptor;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.utils.ContextUtils;
import java.util.HashMap;
import java.util.Map;
import javax.xml.transform.Transformer;

public class ImageUrlRedirectUtils {
  private static final String TAG = "ImageUrlRedirectUtils";
  public static String redirectUrl(Context context, String originUrl) {
    return redirectUrl(context, originUrl, false);
  }

  public static String asyncRedirectUrl(LynxContext context, String originUrl) {
    return redirectUrl(context, originUrl, true);
  }

  private static String redirectUrl(Context context, String originUrl, boolean async) {
    if (context instanceof LynxContext && !TextUtils.isEmpty(originUrl)) {
      LynxContext lynxContext = ContextUtils.toLynxContext(context);
      if (lynxContext == null) {
        LLog.e(TAG, "redirecting url failed due to no context available");
        return originUrl;
      }
      ImageInterceptor interceptor;
      if (async) {
        interceptor = lynxContext.getAsyncImageInterceptor();
      } else {
        interceptor = lynxContext.imageInterceptor();
      }
      if (interceptor != null) {
        String templateUrl = lynxContext.getTemplateUrl();
        String redirectUrl = shouldRedirectImageUrl(interceptor, originUrl, templateUrl);
        return redirectUrl != null ? redirectResUrlIfNeed(lynxContext, redirectUrl) : originUrl;
      }
    }
    return originUrl;
  }

  public static void loadImage(@NonNull Context context, String cacheKey, @NonNull String src,
      float width, float height, final Transformer transformer,
      @NonNull final ImageInterceptor.CompletionHandler handler) {
    LynxContext lynxContext = ContextUtils.toLynxContext(context);
    if (lynxContext == null) {
      LLog.e(TAG, "load image failed due to no context available");
      return;
    }
    ImageInterceptor interceptor = lynxContext.imageInterceptor();
    if (interceptor != null) {
      interceptor.loadImage(lynxContext, cacheKey, src, width, height, transformer, handler);
    } else {
      handler.imageLoadCompletion(null, null);
    }
  }

  // Redirect res:///image_name.extension to res:///identifier
  private static String redirectResUrlIfNeed(Context context, String originResUrl) {
    if (originResUrl.startsWith("res:///")) {
      boolean isIdentifier = true;
      for (int i = 7; i < originResUrl.length(); ++i) {
        char c = originResUrl.charAt(i);
        if (!(c >= '0' && c <= '9')) {
          isIdentifier = false;
          break;
        }
      }
      if (!isIdentifier) {
        int indexOfDot = originResUrl.indexOf('.');
        indexOfDot = indexOfDot < 0 ? originResUrl.length() : indexOfDot;
        int id = context.getResources().getIdentifier(
            originResUrl.substring(7, indexOfDot), "drawable", context.getPackageName());
        return "res:///" + id;
      }
    }
    return originResUrl;
  }

  private static String shouldRedirectImageUrl(
      @NonNull ImageInterceptor interceptor, String origUrl, String templateUrl) {
    String traceEvent = "Interceptor.shouldRedirectImageUrl";
    if (TraceEvent.enableTrace()) {
      Map<String, String> props = new HashMap<>();
      props.put("url", origUrl);
      TraceEvent.beginSection(traceEvent, props);
    }

    String redirectUrl = interceptor.shouldRedirectImageUrl(origUrl);
    if (TraceEvent.enableTrace()) {
      TraceEvent.endSection(traceEvent);
    }
    if (redirectUrl != null) {
      return redirectUrl;
    }
    // only support Card resource dir res
    if (!TextUtils.isEmpty(origUrl) && origUrl.startsWith("./")
        && !TextUtils.isEmpty(templateUrl)) {
      int index = templateUrl.lastIndexOf("/");
      if (index > 0) {
        String tempUrl = templateUrl.substring(0, index);
        tempUrl += origUrl.substring(1);
        LLog.d(TAG, "shouldRedirectImageUrl use local image url:" + tempUrl);
        if (tempUrl.startsWith("http") || tempUrl.startsWith("file://")
            || tempUrl.startsWith("content://") || tempUrl.startsWith("res://")
            || tempUrl.startsWith("data:")) {
          return tempUrl;
        } else if (tempUrl.startsWith("assets:///")) {
          return tempUrl.replace("assets:///", "asset:///");
        } else if (tempUrl.startsWith("assets://")) {
          return tempUrl.replace("assets://", "asset:///");
        } else if (tempUrl.startsWith("asset:///")) {
          return tempUrl;
        } else {
          return "file://" + tempUrl;
        }
      }
    }
    return null;
  }
}
