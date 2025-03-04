// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.base;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Build;
import android.util.Log;
import java.io.File;

public class LynxNativeMemoryTracer {
  private static final String ACTION_START = "LYNX_MEMORY_TRACING_START";
  private static final String ACTION_STOP = "LYNX_MEMORY_TRACING_STOP";
  private static final String ACTION_REPORT = "LYNX_MEMORY_TRACING_REPORT";
  private static int sMinWatchedSize = 1;
  private static boolean sInstalled = false;
  private static boolean sStarted = false;
  private static int sNextReportIndex = 0;

  static class TracingBroadcastReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
      if (intent.getAction().endsWith(ACTION_START)) {
        int minWatchedSize = intent.getIntExtra("min_watched_size", -1);
        if (minWatchedSize > 0) {
          startTracing(minWatchedSize);
        } else {
          startTracing();
        }
      } else if (intent.getAction().endsWith(ACTION_STOP)) {
        stopTracing();
      } else if (intent.getAction().endsWith(ACTION_REPORT)) {
        String reportDir = intent.getStringExtra("report_dir");
        if (reportDir == null) {
          reportDir = context.getExternalFilesDir(null).getPath() + "/memory-reports";
        }
        writeMemoryRecordsToFile(reportDir);
      }
    }
  }

  public static void setup(Context context, int minWatchedSize) {
    if (minWatchedSize <= 0) {
      throw new IllegalArgumentException("min watched size should be greater than 0");
    }
    sMinWatchedSize = minWatchedSize;
    setup(context);
  }

  public static void setup(Context context) {
    IntentFilter filter = new IntentFilter();
    filter.addAction(context.getPackageName() + "." + ACTION_START);
    filter.addAction(context.getPackageName() + "." + ACTION_STOP);
    filter.addAction(context.getPackageName() + "." + ACTION_REPORT);
    // Android 14 (API level 34) or higher must specify a flag to indicate
    // whether or not the receiver should be exported to all other apps on the device
    // using context-registered
    // <p>
    // https://developer.android.com/about/versions/14/behavior-changes-14#runtime-receivers-exported
    // Todo(suguannan.906): replace 34 with Build.VERSION_CODES.UPSIDE_DOWN_CAKE
    //  after upgrading compileSdkVerion to 34 or higher
    BroadcastReceiver receiver = new TracingBroadcastReceiver();
    if (Build.VERSION.SDK_INT >= 34 && context.getApplicationInfo().targetSdkVersion >= 34) {
      // 0x4 means Context.RECEIVER_NOT_EXPORTED
      // <p>
      // https://developer.android.com/reference/android/content/Context.html?hl=en#RECEIVER_EXPORTED
      // Todo(suguannan.906): replace 0x4 with Context.RECEIVER_NOT_EXPORTED
      //  after upgrading compileSdkVerion to 34 or higher
      context.registerReceiver(receiver, filter, 0x4);
    } else {
      context.registerReceiver(receiver, filter);
    }
    sInstalled = true;
  }

  public static void writeMemoryRecordsToFile(String reportsDir) {
    if (!sStarted) {
      return;
    }
    if (reportsDir == null) {
      throw new NullPointerException("filePath is null");
    }
    File dir = new File(reportsDir);
    if (dir.exists() && !dir.isDirectory()) {
      throw new IllegalArgumentException(reportsDir + " is not a directory");
    }

    if (!dir.exists() && !dir.mkdirs()) {
      throw new IllegalArgumentException("can not create directory '" + reportsDir + "'");
    }
    String reportPath = dir.getPath() + "/lynx-native-memory-report-" + sNextReportIndex;
    sNextReportIndex++;
    nativeWriteRecordsToFile(reportPath);
  }

  public static void startTracing(int minWatchedSize) {
    if (!sInstalled) {
      return;
    }
    sNextReportIndex = 0;
    nativeStartTracing(minWatchedSize);
    sStarted = true;
  }

  public static void startTracing() {
    startTracing(sMinWatchedSize);
  }

  public static void stopTracing() {
    if (!sStarted) {
      return;
    }
    nativeStopTracing();
    sStarted = false;
  }

  private static native void nativeStartTracing(int min_watched_size);
  private static native void nativeStopTracing();
  private static native void nativeWriteRecordsToFile(String filePath);
}
