// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.basedevtool.logbox;

import android.app.Activity;
import android.content.Context;
import android.content.ContextWrapper;
import android.util.Log;
import androidx.annotation.Keep;
import com.lynx.basedevtool.utils.DevToolFileLoadCallback;
import com.lynx.basedevtool.utils.DevToolFileLoadUtils;
import com.lynx.basedevtool.utils.UIThreadUtils;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

@Keep
public class LogBoxProxy {
  private static final String TAG = "LogBoxProxy";

  private WeakReference<Context> mActivity;
  private Map<LogBoxLogLevel, List<String>> mLogs;
  private WeakReference<ILogBoxResourceProvider> mProvider;
  private String mErrNamespace;

  public LogBoxProxy(Context context, String errNamespace, ILogBoxResourceProvider provider) {
    mProvider = new WeakReference<>(provider);
    mActivity = new WeakReference<>(null);
    mLogs = new HashMap<>();
    mLogs.put(LogBoxLogLevel.Error, new ArrayList<String>());
    mLogs.put(LogBoxLogLevel.Warn, new ArrayList<String>());
    mErrNamespace = errNamespace != null ? errNamespace : "__unknown__";
    attachContext(context);
  }

  public void registerErrorParser(String path) {
    LogBoxEnv.inst().registerErrorParserLoader(
        mErrNamespace, new LogBoxEnv.ILogBoxErrorParserLoader() {
          @Override
          public void loadErrorParser(Context context, DevToolFileLoadCallback callback) {
            DevToolFileLoadUtils.loadFileFromLocal(context, path, callback);
          }
        });
  }

  public void attachContext(Context context) {
    if (mActivity == null || mActivity.get() == null) {
      Context activity = findActivityByContext(context);
      if (activity == null) {
        return;
      }
      mActivity = new WeakReference<>(activity);
      showCacheLogMessage();
    } else {
      Log.e(TAG, "LogBoxProxy context has attached.");
    }
  }

  public void showLogMessage(final String message, final LogBoxLogLevel level) {
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
    Log.i(TAG, "onNewLog with level " + level);
    List<String> logList = mLogs.get(level);
    if (logList == null) {
      return;
    }
    logList.add(message);
    if (mActivity == null || mActivity.get() == null) {
      // The log message will be cached until attachContext is called.
      return;
    }
    LogBoxOwner.getInstance().dispatch(message, level, mActivity.get(), this);
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
          LogBoxOwner.getInstance().dispatch(message, level, mActivity.get(), LogBoxProxy.this);
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

  public String getEntryUrlForLogSrc() {
    ILogBoxResourceProvider provider = mProvider.get();
    return provider == null ? "" : provider.getEntryUrlForLogSrc();
  }

  public Map<String, Object> getLogSources() {
    ILogBoxResourceProvider provider = mProvider.get();
    return provider == null ? null : provider.getLogSources();
  }

  public int getLogCount(final LogBoxLogLevel level) {
    List<String> logs = mLogs.get(level);
    return logs == null ? 0 : logs.size();
  }

  public void reset() {
    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
      @Override
      public void run() {
        LogBoxOwner.getInstance().onProxyReset(mActivity.get(), LogBoxProxy.this);
      }
    });
  }

  public String getErrorNamespace() {
    return mErrNamespace;
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
}
