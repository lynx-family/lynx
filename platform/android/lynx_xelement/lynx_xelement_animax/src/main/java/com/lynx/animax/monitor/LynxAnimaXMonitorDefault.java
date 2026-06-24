// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.animax.monitor;

import androidx.annotation.NonNull;
import com.lynx.animax.listener.AnimaXErrorParam;
import com.lynx.animax.service.IAnimaXMonitorService;
import com.lynx.animax.util.AnimaXLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.eventreport.LynxEventReporter;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Map;

public class LynxAnimaXMonitorDefault implements IAnimaXMonitorService {
  private static final String TAG = "LynxAnimaXMonitorDefault";
  private static final String INTEGRATION_TYPE_LYNX = "lynx";
  private String mCurrentUrl = AnimaXMonitorUtil.SRC_URL_UNKNOWN;

  @NonNull private final WeakReference<LynxContext> mContext;

  public LynxAnimaXMonitorDefault(LynxContext context) {
    mContext = new WeakReference<>(context);
  }

  @Override
  public void setCurrentUrl(String url) {
    mCurrentUrl = url;
  }

  @Override
  public void reportError(AnimaXErrorParam errorInfo) {
    LynxContext context = mContext.get();
    if (context == null || !context.enableEventReporter()) {
      return;
    }

    Map<String, Object> originParams = errorInfo.getOriginParams();
    Map<String, Object> paramsWithCommon = appendCommonParams(originParams);

    AnimaXLog.i(TAG, "reportError: " + paramsWithCommon);
    LynxEventReporter.onEvent(
        AnimaXMonitorUtil.ERROR_EVENT_NAME, paramsWithCommon, context.getInstanceId());
  }

  @Override
  public void reportPerformanceMetrics(MetricsAndEventStore perfMetrics) {
    LynxContext context = mContext.get();
    if (context == null || perfMetrics == null) {
      AnimaXLog.e(TAG,
          "report failed, metrics is null:" + (perfMetrics == null)
              + ", context is null:" + (context == null));
      return;
    }
    if (!context.enableEventReporter()) {
      return;
    }

    // merge the category map and the metrics map into one
    Map<String, Object> category = perfMetrics.getCategoryAsMap();
    Map<String, Object> metrics = perfMetrics.getMetricsAsMap();
    Map<String, Object> categoryAndMetricsCombined = new HashMap<>();
    categoryAndMetricsCombined.putAll(category);
    categoryAndMetricsCombined.putAll(metrics);

    // add common parameters
    Map<String, Object> paramsWithCommon = appendCommonParams(categoryAndMetricsCombined);

    AnimaXLog.i(TAG, "reportPerformanceMetrics: " + paramsWithCommon);
    LynxEventReporter.onEvent(
        AnimaXMonitorUtil.PERFORMANCE_EVENT_NAME, paramsWithCommon, context.getInstanceId());
  }

  /**
   * Append common parameters to the given parameters map
   */
  private Map<String, Object> appendCommonParams(Map<String, Object> originalParams) {
    LynxContext context = mContext.get();
    if (context == null) {
      return originalParams;
    }

    Map<String, Object> paramsWithCommon = new HashMap<>(originalParams);
    paramsWithCommon.put(
        AnimaXMonitorUtil.KEY_SRC_URL, AnimaXMonitorUtil.clearUrlQuery(mCurrentUrl));
    paramsWithCommon.put(
        AnimaXMonitorUtil.KEY_PAGE_URL, AnimaXMonitorUtil.clearUrlQuery(context.getTemplateUrl()));
    paramsWithCommon.put(AnimaXMonitorUtil.KEY_INTEGRATION_TYPE, INTEGRATION_TYPE_LYNX);

    return paramsWithCommon;
  }
}
