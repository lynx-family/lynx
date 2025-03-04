// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.base;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Build;
import android.os.Environment;
import android.os.Trace;
import android.text.TextUtils;
import android.util.JsonReader;
import android.util.Log;
import android.widget.Toast;
import com.lynx.tasm.base.LynxTraceEnv;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.trace.BuildConfig;
import com.lynx.trace.CalledByNative;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.lang.reflect.Field;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.TimeZone;
import java.util.Timer;
import java.util.TimerTask;

/**
 * Helper class for tracing Lynx App, use 'am broadcast' to start/stop tracing:
 *    adb shell am broadcast -a com.lynx.uiapp.LYNX_TRACE_START
 *    adb shell am broadcast -a com.lynx.uiapp.LYNX_TRACE_STOP
 */
@SuppressWarnings("JniMissingFunction")
public class TraceController {
  private static final String ACTION_START = "LYNX_TRACE_START";
  private static final String ACTION_STOP = "LYNX_TRACE_STOP";
  private static final String CATEGORIES_EXTRA = "categories";
  private static final String FILE_EXTRA = "file";
  private static final String BUFFER_SIZE_EXTRA = "buffer";
  private static final String NATIVE_ONLY_EXTRA = "nativeOnly";
  private static final String TAG = "Lynx startup trace";
  private static final int DEFAULT_BUFFER_SIZE = 40960; // kb

  private static final long ATRACE_TAG_ALL = ~((-1L) << 27);

  private Context mContext;
  private List<CompleteCallback> mCompleteCallbacks = new ArrayList<>();
  private TraceBroadcastReceiver mBroadcastReceiver;
  private boolean mTracingStarted = false;
  private long mNativeTraceController = 0;
  private int tracingSession = -1;
  // Control Whether Record Java Trace or Not
  private static boolean sNativeTracingOnly = false;
  private static boolean isTraceEnvInit = false;
  private String traceFilePath;

  private TraceController() {
    try {
      if (mNativeTraceController == 0 && isTraceEnvInited()) {
        mNativeTraceController = nativeCreateTraceController();
      }
    } catch (java.lang.UnsatisfiedLinkError e) {
      Log.w(TAG, "failed to create NativeTraceController", e);
    } catch (Exception e) {
      Log.w(TAG, "failed to create NativeTraceController", e);
    }
    if (mNativeTraceController == 0) {
      Log.w(TAG, "failed to create NativeTraceController");
      return;
    }
  }

  private static class TraceControllerLoader {
    private static final TraceController INSTANCE = new TraceController();
  }

  public interface CompleteCallback {
    void onComplete(String traceFile);
  }

  public static boolean isNativeTracingOnly() {
    return sNativeTracingOnly;
  }

  public static TraceController getInstance() {
    return TraceControllerLoader.INSTANCE;
  }

  public long getNativeTraceController() {
    return mNativeTraceController;
  }

  public String startTrace() {
    File file = getFile();
    String fileName = file.getPath();
    startTracing(DEFAULT_BUFFER_SIZE, null, null, fileName, false);
    String logMessage = "Trace started at: " + fileName;
    Toast.makeText(mContext, logMessage, Toast.LENGTH_SHORT).show();
    Log.i(TAG, logMessage);
    return file.getAbsolutePath();
  }

  public void stopTrace() {
    stopTracing();
    Toast.makeText(mContext, "Trace stopped", Toast.LENGTH_SHORT).show();
    Log.i(TAG, "Trace stopped");
  }

  public void startStartupTracingIfNeeded() {
    if (mNativeTraceController != 0) {
      nativeStartStartupTracingIfNeeded(mNativeTraceController);
    }
  }

  public void startTracing(CompleteCallback callback, String config) {
    mCompleteCallbacks.add(callback);
    String traceFile = generateTracingFileName();
    startTracing(DEFAULT_BUFFER_SIZE, null, null, traceFile, false);
  }

  public void startTracing(CompleteCallback callback, Map<String, String> config) {
    mCompleteCallbacks.add(callback);
    String traceFile = generateTracingFileName();
    Boolean enableSystrace = false;
    int bufferSize = DEFAULT_BUFFER_SIZE;
    if (config.containsKey("trace_file")) {
      traceFile = config.get("trace_file");
    }
    if (config.containsKey("buffer_size")) {
      bufferSize = Integer.parseInt(config.get("buffer_size"));
    }
    if (config.containsKey("enable_systrace")) {
      enableSystrace = Boolean.parseBoolean(config.get("enable_systrace"));
    }
    startTracing(bufferSize, null, null, traceFile, enableSystrace);
  }

  public void stopTracing() {
    if (mNativeTraceController == 0 || !mTracingStarted) {
      return;
    }
    mTracingStarted = false;
    nativeStopTracing(mNativeTraceController, tracingSession);
    if (!traceFilePath.isEmpty()) {
      onTracingComplete(traceFilePath);
      traceFilePath = "";
    }
  }

  public boolean isTracingStarted() {
    return mTracingStarted;
  }

  @Deprecated
  public void recordClockSyncMarker(String syncId) {}

  public void onTracingComplete(String traceFile) {
    // callback only work once
    for (CompleteCallback callback : mCompleteCallbacks) {
      callback.onComplete(traceFile);
    }
    mCompleteCallbacks.clear();
  }

  private String generateTracingFileName() {
    File file = getFile();
    return file.getPath();
  }

  @CalledByNative
  private String generateTracingFileDir() {
    return mContext.getExternalFilesDir(null).getPath();
  }

  private File getFile() {
    int pid = android.os.Process.myPid();
    SimpleDateFormat formatter = new SimpleDateFormat("yyyy-MM-dd-HHmmss", Locale.US);
    formatter.setTimeZone(TimeZone.getTimeZone("UTC"));
    File dir = mContext.getExternalFilesDir(null);
    return new File(dir, "lynx-profile-trace-" + pid + "-" + formatter.format(new Date()));
  }

  @CalledByNative
  private void refreshATraceTags() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
      try {
        Field field = Trace.class.getDeclaredField("sEnabledTags");
        field.setAccessible(true);
        field.setLong(null, ATRACE_TAG_ALL);
      } catch (Throwable e) {
        e.printStackTrace();
      }
    }
  }

  private static boolean isTraceEnvInited() {
    if (isTraceEnvInit) {
      return isTraceEnvInit;
    }
    isTraceEnvInit = LynxTraceEnv.inst().init();
    return isTraceEnvInit;
  }

  protected void startTracing(int bufferSize, String[] includeCategories,
      String[] excludeCategories, String traceFile, boolean enableSystrace) {
    if (mTracingStarted) {
      Toast.makeText(mContext, "Trace already started, please stop it first", Toast.LENGTH_SHORT)
          .show();
      return;
    }
    if (mNativeTraceController == 0) {
      Log.w(TAG, "tracing not enabled");
      return;
    }
    mTracingStarted = true;
    traceFilePath = traceFile.isEmpty() ? getFile().getPath() : traceFile;
    tracingSession = nativeStartTracing(mNativeTraceController, bufferSize, includeCategories,
        excludeCategories, traceFilePath, enableSystrace);
    Map<String, String> args = new HashMap<>();
    args.put("Version", BuildConfig.VERSION);
    TraceEvent.instant(TraceEvent.CATEGORY_VITALS, "Version", args);
  }

  private static class TraceIntentFilter extends IntentFilter {
    public TraceIntentFilter(Context context) {
      addAction(context.getPackageName() + "." + ACTION_START);
      addAction(context.getPackageName() + "." + ACTION_STOP);
    }
  }

  public void init(Context context) {
    mContext = context;
    mBroadcastReceiver = new TraceBroadcastReceiver();
    IntentFilter filter = new TraceIntentFilter(mContext);
    // Android 14 (API level 34) or higher must specify a flag to indicate
    // whether or not the receiver should be exported to all other apps on the device
    // using context-registered
    // <p>
    // https://developer.android.com/about/versions/14/behavior-changes-14#runtime-receivers-exported
    // Todo(suguannan.906): replace 34 with Build.VERSION_CODES.UPSIDE_DOWN_CAKE
    //  after upgrading compileSdkVerion to 34 or higher
    if (Build.VERSION.SDK_INT >= 34 && context.getApplicationInfo().targetSdkVersion >= 34) {
      // 0x4 means Context.RECEIVER_NOT_EXPORTED
      // <p>
      // https://developer.android.com/reference/android/content/Context.html?hl=en#RECEIVER_EXPORTED
      // Todo(suguannan.906): replace 0x4 to Context.RECEIVER_NOT_EXPORTED
      //  after upgrading compileSdkVerion to 34 or higher
      mContext.registerReceiver(mBroadcastReceiver, filter, 0x4);
    } else {
      mContext.registerReceiver(mBroadcastReceiver, filter);
    }
  }

  public void onTerminate() {
    mContext.unregisterReceiver(mBroadcastReceiver);
    mContext = null;
  }

  class TraceBroadcastReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
      if (intent.getAction().endsWith(ACTION_START)) {
        String categories = intent.getStringExtra(CATEGORIES_EXTRA);
        String filename = intent.getStringExtra(FILE_EXTRA);
        int bufferSize = intent.getIntExtra(BUFFER_SIZE_EXTRA, DEFAULT_BUFFER_SIZE);

        boolean isNativeOnly = intent.getBooleanExtra(NATIVE_ONLY_EXTRA, false);
        sNativeTracingOnly = isNativeOnly;

        if (filename == null) {
          filename = generateTracingFileName();
        }

        startTracing(
            bufferSize, categories != null ? categories.split(",") : null, null, filename, false);
        String logMessage = "Trace started at: " + filename;
        Toast.makeText(context, logMessage, Toast.LENGTH_SHORT).show();
        Log.i(TAG, logMessage);
      } else if (intent.getAction().endsWith(ACTION_STOP)) {
        sNativeTracingOnly = false;
        stopTracing();
        Toast.makeText(context, "Trace stopped", Toast.LENGTH_SHORT).show();
        Log.i(TAG, "Trace stopped");
      }
    }
  }

  private native long nativeCreateTraceController();
  private native int nativeStartTracing(long ptr, int bufferSize, String[] includeCategories,
      String[] excludeCategories, String traceFile, boolean enableSystrace);
  private native void nativeStopTracing(long ptr, int sessionId);
  private native void nativeStartStartupTracingIfNeeded(long ptr);
}
