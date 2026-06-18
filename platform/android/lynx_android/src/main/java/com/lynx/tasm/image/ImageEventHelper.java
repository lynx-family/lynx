// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.image;

import android.text.TextUtils;
import androidx.annotation.AnyThread;
import androidx.annotation.RestrictTo;
import com.lynx.devtoolwrapper.MemoryListener;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.core.LynxThreadPool;
import com.lynx.tasm.eventreport.LynxEventReporter;
import com.lynx.tasm.service.ILynxMemoryMonitorService;
import com.lynx.tasm.service.ILynxMonitorService;
import com.lynx.tasm.service.LynxImageInfo;
import com.lynx.tasm.service.LynxMemoryInfo;
import com.lynx.tasm.service.LynxServiceCenter;
import java.util.HashMap;
import java.util.Map;
import org.json.JSONObject;

public class ImageEventHelper {
  /**
   * Report Image memory usage with new protocol, keep original method for AB testing
   * @param context LynxContext of current LynxView
   * @param sign sign of current LynxBaseUI, default value -1
   * @param url  resource schema of current image
   * @param isSuccess whether successfully fetch current image
   * @param hitCache  whether hit cache for current image
   * @param startTimeStamp timestamp when start fetching bitmap
   * @param getImageTimeStamp timestamp when finally get bitmap
   * @param memoryCost memory cost for current bitmap
   * @param info meta info of current bitmap
   */
  @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
  @AnyThread
  public static void monitorReporterV2(final LynxContext context, final int sign, final String url,
      final boolean isSuccess, final boolean hitCache, final long startTimeStamp,
      final long getImageTimeStamp, final int memoryCost, final JSONObject info) {
    if (TextUtils.isEmpty(url)) {
      return;
    }
    if (startTimeStamp <= 0 || getImageTimeStamp <= 0) {
      return;
    }
    final long fetchDuration = getImageTimeStamp - startTimeStamp;
    final long finishTimeStamp = System.currentTimeMillis();
    final long completeDuration = finishTimeStamp - startTimeStamp;
    LynxEventReporter.runOnReportThread(() -> {
      try {
        final LynxView lynxView = context != null ? context.getLynxView() : null;
        if (lynxView == null) {
          return;
        }

        ILynxMemoryMonitorService lynxMemoryMonitorService =
            LynxServiceCenter.inst().getService(ILynxMemoryMonitorService.class);
        if (lynxMemoryMonitorService == null) {
          return;
        }

        LynxMemoryInfo.Builder builder = new LynxMemoryInfo.Builder()
                                             .type(LynxMemoryInfo.TYPE_IMAGE)
                                             .resourceURL(url)
                                             .memoryCost((float) memoryCost)
                                             .finishTimeStamp(finishTimeStamp)
                                             .fetchDuration(fetchDuration)
                                             .completeDuration(completeDuration)
                                             .startTimeStamp(startTimeStamp)
                                             .isSuccess(isSuccess ? 1 : 0);
        builder.phase(lynxView.getRenderPhase());
        builder.templateURL(context.getTemplateUrl());

        if (info != null) {
          builder.viewHeight(info.optLong("viewHeight"))
              .viewWidth(info.optLong("viewWidth"))
              .width(info.optLong("width"))
              .height(info.optLong("height"))
              .config(info.optString("config"))
              .isFlattenAnim(info.optInt("isFlattenAnim"));
        }

        lynxMemoryMonitorService.reportMemoryUsage(builder.build());
      } catch (RuntimeException ignored) {
        // Ignore image report failures.
      }
    });
  }

  public static void monitorReporter(final LynxContext context, final String url,
      final boolean isSuccess, final boolean hitCache, final long startTimeStamp,
      final long getImageTimeStamp, final int memoryCost, final JSONObject info) {
    if (TextUtils.isEmpty(url)) {
      return;
    }
    if (startTimeStamp <= 0 || getImageTimeStamp <= 0) {
      return;
    }
    final long fetchDuration = getImageTimeStamp - startTimeStamp;
    final long finishTimeStamp = System.currentTimeMillis();
    final long completeDuration = finishTimeStamp - startTimeStamp;
    LynxThreadPool.getBriefIOExecutor().execute(() -> {
      try {
        // region timeMetrics
        JSONObject data = getReportData(context, url, isSuccess, hitCache, startTimeStamp,
            fetchDuration, completeDuration, finishTimeStamp, memoryCost, info);
        if (data != null) {
          // upload image memory info to devtool
          MemoryListener.getInstance().uploadImageInfo(data);

          ILynxMonitorService lynxMonitorService =
              LynxServiceCenter.inst().getService(ILynxMonitorService.class);
          if (lynxMonitorService != null) {
            lynxMonitorService.reportImageStatus("lynx_image_status", data);
          }
        }
      } catch (RuntimeException ignored) {
        // Ignore image report failures.
      }
    });
  }

  public static JSONObject getReportData(LynxContext context, String url, boolean isSuccess,
      boolean hitCache, long startTimeStamp, long fetchDuration, long completeDuration,
      long finishTimeStamp, int memoryCost, JSONObject info) {
    try {
      final JSONObject data = new JSONObject();
      final JSONObject timeMetrics = new JSONObject();
      timeMetrics.put("fetchTime", fetchDuration / (double) 1000);
      timeMetrics.put("completeTime", completeDuration / (double) 1000);
      timeMetrics.put("fetchTimeStamp", startTimeStamp);
      timeMetrics.put("finishTimeStamp", finishTimeStamp);
      data.put("timeMetrics", timeMetrics);
      // endregion

      // region templateMetric
      final LynxView lynxView = context != null ? context.getLynxView() : null;
      if (lynxView != null) {
        String templateUrl = lynxView.getTemplateUrl();
        if (TextUtils.isEmpty(templateUrl)) {
          templateUrl = "";
        }
        final JSONObject templateMetric = new JSONObject();
        templateMetric.put("url", templateUrl);
        if (info != null) {
          final long viewWidth = info.optLong("viewWidth");
          final long viewHeight = info.optLong("viewHeight");
          final long width = info.optLong("width");
          final long height = info.optLong("height");
          templateMetric.put("viewWidth", viewWidth > 0 ? viewWidth : -1);
          templateMetric.put("viewHeight", viewHeight > 0 ? viewHeight : -1);
          templateMetric.put("width", width > 0 ? width : -1);
          templateMetric.put("height", height > 0 ? height : -1);
          templateMetric.put("flattenAnim", info.optInt("isFlattenAnim"));
          templateMetric.put("config", info.optString("config"));
        }
        data.put("metric", templateMetric);
      }
      // endregion

      data.put("image_url", url);
      data.put("successRate", isSuccess ? 1 : 0);
      data.put("memoryCost", (float) memoryCost); // float casting report follow iOS type
      data.put("resourceFromMemoryCache", hitCache ? 1 : 0);
      return data;
    } catch (Exception ignored) {
      // Ignore image report failures.
    }
    return null;
  }

  public static void reportImageInfo(final LynxContext context, final String url,
      final boolean isSuccess, final boolean hitCache, final long startTimeStamp,
      final long finishTimeStamp, final int memoryCost, final int errorCode) {
    LynxThreadPool.getBriefIOExecutor().execute(() -> {
      // There is no need to check shouldUseNewImage or not. It is different from iOS logic.
      LynxView lynxView = context != null ? context.getLynxView() : null;
      if (lynxView == null) {
        return;
      }
      ILynxMonitorService lynxMonitorService =
          LynxServiceCenter.inst().getService(ILynxMonitorService.class);
      if (lynxMonitorService == null) {
        return;
      }
      final LynxImageInfo imageInfo = new LynxImageInfo.Builder()
                                          .startTimeStamp(startTimeStamp)
                                          .finishTimeStamp(finishTimeStamp)
                                          .isSuccess(isSuccess)
                                          .url(url)
                                          .memoryCost(memoryCost)
                                          .errorCode(errorCode)
                                          .lynxView(lynxView)
                                          .hitMemoryCache(hitCache)
                                          .build();
      lynxMonitorService.reportImageInfo(imageInfo);
    });
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
  @AnyThread
  public static void reportImageEvent(final LynxContext context, final String url,
      final int errorCode, final boolean hitMemoryCache, final int imageOrigin,
      final long startTimeStamp, final long finishTimeStamp, final boolean isFlattenUI,
      final int width, final int height, Map<String, String> customParams) {
    if (context == null) {
      return;
    }
    try {
      Map<String, Object> props = new HashMap<>();
      props.put("load_start", startTimeStamp);
      props.put("load_finish", finishTimeStamp);
      props.put("cost", (finishTimeStamp - startTimeStamp));
      props.put("src", url);
      props.put("memory", hitMemoryCache);
      props.put("origin", imageOrigin);
      props.put("error_code", errorCode);
      props.put("flatten", isFlattenUI);
      props.put("width", width);
      props.put("height", height);
      if (customParams != null) {
        props.putAll(customParams);
      }
      LynxEventReporter.onEvent("lynxsdk_image_event", props, context.getInstanceId());
    } catch (Exception ignored) {
      // Ignore image report failures.
    }
  }
}
