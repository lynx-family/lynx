// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.logbox;

import android.app.Activity;
import android.content.Context;
import android.content.ContextWrapper;
import androidx.annotation.Keep;
import com.lynx.BuildConfig;
import com.lynx.devtool.LynxDevtoolEnv;
import com.lynx.devtoolwrapper.LogBoxLogLevel;
import com.lynx.devtoolwrapper.LynxBaseLogBoxProxy;
import com.lynx.devtoolwrapper.LynxDevtool;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.eventreport.LynxEventReporter;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.json.JSONException;
import org.json.JSONObject;

@Keep
public class LynxLogBoxProxy implements LynxBaseLogBoxProxy {
  private static final String TAG = "LynxLogBoxProxy";
  private WeakReference<LynxDevtool> mDevtool;
  private WeakReference<Context> mActivity;
  private Map<LogBoxLogLevel, List<String>> mLogs;

  public LynxLogBoxProxy(LynxDevtool devtool) {
    mDevtool = new WeakReference<>(devtool);
    mLogs = new HashMap<>();
    mLogs.put(LogBoxLogLevel.Error, new ArrayList<String>());
    mLogs.put(LogBoxLogLevel.Warn, new ArrayList<String>());
    mLogs.put(LogBoxLogLevel.Info, new ArrayList<String>());
    Context context = devtool.getLynxContext();
    attachContext(context);
  }

  public void attachContext(Context context) {
    Context activity = findActivityByContext(context);
    if (mActivity == null || mActivity.get() == null) {
      mActivity = new WeakReference<>(activity);
      showCacheLogMessage();
    } else {
      LLog.e(TAG, "LynxLogBoxProxy context has attached.");
    }
  }

  @Override
  public void showLogMessage(final LynxError error) {
    if (error == null) {
      return;
    }
    String message = error.getMsg();
    LogBoxLogLevel level =
        error.getLevel().equals(LynxError.LEVEL_WARN) ? LogBoxLogLevel.Warn : LogBoxLogLevel.Error;
    sendErrorEventToPerf(message, level);
    if (LynxDevtoolEnv.inst().isIgnoreErrorTypeEnabled(error.getErrorCode())) {
      return;
    }
    if (UIThreadUtils.isOnUiThread())
      onNewLog(message, level);
    else {
      UIThreadUtils.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          onNewLog(message, level);
        }
      });
    }
  }

  private void onNewLog(final String message, LogBoxLogLevel level) {
    LLog.i(TAG, "onNewLog with level " + level);
    List<String> logList = mLogs.get(level);
    if (logList == null) {
      return;
    }
    logList.add(message);
    if (mActivity == null || mActivity.get() == null) {
      // The log message will be cached until attachContext is called.
      return;
    }
    LynxLogBoxOwner.getInstance().dispatch(message, level, mActivity.get(), this);
  }

  private void showCacheLogMessage() {
    // Only warn and error level logs need to be displayed.
    showCacheLogMessageByLogLevel(LogBoxLogLevel.Warn);
    showCacheLogMessageByLogLevel(LogBoxLogLevel.Error);
  }

  private void showCacheLogMessageByLogLevel(LogBoxLogLevel level) {
    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
      @Override
      public void run() {
        List<String> logList = mLogs.get(level);
        if (logList == null || logList.isEmpty()) {
          return;
        }
        for (String message : logList) {
          LynxLogBoxOwner.getInstance().dispatch(
              message, level, mActivity.get(), LynxLogBoxProxy.this);
        }
      }
    });
  }

  public List<String> getLogMessages(LogBoxLogLevel level) {
    List<String> logs = mLogs.get(level);
    return logs == null ? new ArrayList<String>() : logs;
  }

  public String getLastLog(LogBoxLogLevel level) {
    List<String> logs = mLogs.get(level);
    if (logs == null) {
      return "";
    }
    return logs.isEmpty() ? "" : logs.get(logs.size() - 1);
  }

  public void clearLogsWithLevel(final LogBoxLogLevel level) {
    List<String> logs = mLogs.get(level);
    if (logs != null) {
      logs.clear();
    }
  }

  public void clearAllLogs() {
    for (List<String> logs : mLogs.values()) {
      logs.clear();
    }
  }

  public String getTemplateUrl() {
    LynxDevtool devtool = mDevtool.get();
    return devtool == null ? "" : devtool.getTemplateUrl();
  }

  public Map<String, Object> getAllJsSource() {
    LynxDevtool devtool = mDevtool.get();
    return devtool == null ? null : devtool.getAllJsSource();
  }

  public int getLogCount(final LogBoxLogLevel level) {
    List<String> logs = mLogs.get(level);
    return logs == null ? 0 : logs.size();
  }

  public void reloadView() {
    LynxDevtool devtool = mDevtool.get();
    if (devtool != null) {
      devtool.reloadView();
    }
  }

  public void onLoadTemplate() {
    LynxLogBoxOwner.getInstance().onLoadTemplate(mActivity.get(), this);
  }

  public void showConsole() {
    if (UIThreadUtils.isOnUiThread()) {
      showConsoleSync();
    } else {
      UIThreadUtils.runOnUiThread(new Runnable() {
        @Override
        public void run() {
          showConsoleSync();
        }
      });
    }
  }

  protected int getInstanceIdOfCurrentView() {
    LynxDevtool devtool = mDevtool.get();
    if (devtool != null) {
      LynxContext lynxContext = devtool.getLynxContext();
      if (lynxContext != null) {
        return lynxContext.getInstanceId();
      }
    }
    return LynxEventReporter.INSTANCE_ID_UNKNOWN;
  }

  private void showConsoleSync() {
    LynxLogBoxOwner.getInstance().showConsoleLog(mActivity.get(), this);
  }

  private Context findActivityByContext(Context context) {
    if (context == null) {
      return null;
    }
    while (context instanceof ContextWrapper) {
      if (context instanceof Activity) {
        return context;
      }
      context = ((ContextWrapper) context).getBaseContext();
    }
    return null;
  }

  private void sendErrorEventToPerf(final String message, final LogBoxLogLevel level) {
    if (level == LogBoxLogLevel.Info) {
      return;
    }
    LynxDevtool devtool = mDevtool.get();
    if (devtool == null) {
      return;
    }
    try {
      JSONObject eventData = new JSONObject();
      eventData.put("error", message);
      devtool.onPerfMetricsEvent("lynx_error_event", eventData);
    } catch (JSONException e) {
      LLog.e(TAG, e.toString());
    }
  }
}
