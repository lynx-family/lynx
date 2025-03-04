// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.longtasktiming;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxEnvKey;

public class LynxLongTaskMonitor {
  public static final String PLATFORM_FUNC_TASK = "platform_func_task";
  private static boolean sEnable =
      LynxEnv.getBooleanFromExternalEnv(LynxEnvKey.ENABLE_LONG_TASK_TIMING, false);
  private static volatile boolean sIsNativeLibraryLoaded = false;
  // Called when a task starts. This method does the following:
  //  1. Captures the start timestamp and assigns it to LynxLongTaskTiming
  public static void willProcessTask(String name, int instanceId) {
    willProcessTask(PLATFORM_FUNC_TASK, name, null, instanceId);
  }
  // Called when a task starts. This method does the following:
  //  1. Creates a LynxLongTaskTiming object and sets the task name and type
  //  2. Captures the start timestamp and assigns it to LynxLongTaskTiming
  //  3. Gets the name of the current thread and assigns it to LynxLongTaskTiming
  public static void willProcessTask(String type, String name, String taskInfo, int instanceId) {
    if (canExecute()) {
      nativeWillProcessTask(type, name, taskInfo, instanceId);
    }
  }

  // Updates the LynxLongTaskTiming if necessary
  public static void updateLongTaskTimingIfNeed(String type, String name, String taskInfo) {
    if (canExecute()) {
      nativeUpdateLongTaskTimingIfNeed(type, name, taskInfo);
    }
  }

  // Called when a task ends
  public static void didProcessTask() {
    if (canExecute()) {
      nativeDidProcessTask();
    }
  }

  /**
   * Executes a handler if sEnable is true and the native library is loaded.
   *
   * @return true if sEnable is true and the native library is loaded, false otherwise.
   */
  private static boolean canExecute() {
    if (sEnable) {
      if (!sIsNativeLibraryLoaded) {
        sIsNativeLibraryLoaded = LynxEnv.inst().isNativeLibraryLoaded();
      }
      return sIsNativeLibraryLoaded;
    }
    return false;
  }

  private static native void nativeWillProcessTask(
      String type, String name, String taskInfo, int instanceId);
  private static native void nativeUpdateLongTaskTimingIfNeed(
      String type, String name, String taskInfo);
  private static native void nativeDidProcessTask();
}
